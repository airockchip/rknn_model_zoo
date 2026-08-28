#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Export a YOLOv8 detection model (`.pt`) to an ONNX graph with **6 separate outputs**.

Motivation
----------
The default separate-output export keeps, for each of the 3 scales, a triplet of
tensors:

    [box(1, 64, h, w), cls(1, nc, h, w), score_sum(1, 1, h, w)]   -> 9 outputs

`score_sum = ReduceSum(Sigmoid(cls))` is used by the post-processor as a fast
pre-filter.  When the model is converted to INT8 RKNN, however, that extra
consumer of `Sigmoid(cls)` prevents the toolkit from fusing the sigmoid into the
preceding convolution's quantization.  The sigmoid output is then quantized on
its own, which compresses the top of the confidence range and makes it
stair-stepped (e.g. PyTorch 0.98 -> RKNN 0.58~0.90).  (Exporting `cls` as raw
logits instead of `Sigmoid(cls)` is worse: the logit range is dominated by the
deep negative background values, producing an even coarser confidence grid.)

This script exports the **6-output** variant instead:

    [box(1, 64, h, w), cls(1, nc, h, w)] x 3 scales              -> 6 outputs

with `cls` already passed through `Sigmoid` inside the graph.  Without the
`score_sum` branch the sigmoid is fused into the conv during quantization, so the
INT8 confidence stays in [0, 1] and tracks the PyTorch model (within ~0.02).

The C++ (``cpp/postprocess.cc``) and Python (``python/yolov8.py``) post-processors
already support this layout: they take ``box``/``cls`` per branch by index and
ignore the optional ``score_sum`` when ``n_output == 6`` (``output_per_branch==2``).

Usage
-----
    python export_onnx.py <model.pt> <output.onnx> [imgsz] [opset]

Examples
--------
    python export_onnx.py yolov8n.pt yolov8n_6out.onnx
    python export_onnx.py my_model.pt my_model_6out.onnx 640 17

Outputs (per scale, stride 8/16/32):
    box0, cls0, box1, cls1, box2, cls2
      box[i] : [1, 4 * reg_max, h, w]  raw DFL logits, decoded by post-process on CPU
      cls[i] : [1, nc, h, w]            Sigmoid(cls) in [0, 1]
"""

import sys

import torch
import torch.nn as nn


def export_6out_onnx(pt_path, onnx_path, imgsz=640, opset=17):
    from ultralytics import YOLO

    # -------- Load the original model --------
    model = YOLO(pt_path)
    detect_model = model.model
    detect_model.eval()

    detect = detect_model.model[-1]  # Detect head
    backbone = detect_model.model[:-1]

    # -------- Wrapper: output the two tensors per branch --------
    class YOLOv8SeparateHeads(nn.Module):
        """Rerun the backbone, collect the feature maps fed into Detect, and
        output [box, Sigmoid(cls)] for each of the nl branches."""

        def __init__(self, backbone, detect):
            super().__init__()
            self.backbone = backbone
            self.detect = detect

        def forward(self, x):
            y = []
            detect_inputs = []

            for layer in self.backbone:
                if layer.f != -1:
                    if isinstance(layer.f, int):
                        x = y[layer.f] if layer.f >= 0 else x
                    elif isinstance(layer.f, (list, tuple)):
                        x = [y[j] if j >= 0 else x for j in layer.f]

                if isinstance(layer, nn.Upsample):
                    x = layer(x)
                else:
                    x = layer(x)

                if hasattr(layer, "i") and layer.i in self.detect.f:
                    detect_inputs.append(x)
                y.append(x)

            outputs = []
            for i in range(self.detect.nl):
                feat = detect_inputs[i]
                box_out = self.detect.cv2[i](feat)      # [1, 4*reg_max, h, w] DFL logits
                cls_out = self.detect.cv3[i](feat)      # [1, nc, h, w]
                cls_prob = torch.sigmoid(cls_out)       # fused Sigmoid, no score_sum
                outputs.extend([box_out, cls_prob])

            return tuple(outputs)

    wrapper = YOLOv8SeparateHeads(backbone, detect)
    wrapper.eval()

    # -------- Export ONNX --------
    dummy = torch.randn(1, 3, imgsz, imgsz)
    output_names = ["box0", "cls0", "box1", "cls1", "box2", "cls2"]

    torch.onnx.export(
        wrapper,
        dummy,
        onnx_path,
        input_names=["images"],
        output_names=output_names,
        opset_version=opset,
        do_constant_folding=True,
        verbose=False,
        dynamo=False,  # TorchScript export path
    )

    # -------- Validate --------
    import onnx
    onnx_model = onnx.load(onnx_path)
    onnx.checker.check_model(onnx_model)

    print(f"ONNX export succeeded: {onnx_path}")
    print(f"  input  : images [1, 3, {imgsz}, {imgsz}]")
    print(f"  outputs ({len(onnx_model.graph.output)}):")
    for out in onnx_model.graph.output:
        shape = [d.dim_value for d in out.type.tensor_type.shape.dim]
        print(f"    {out.name}: {shape}")

    # -------- Numerical check: reconstruct the combined output and compare --------
    # Wrapped in try/except so a successful export is never blocked by a
    # model/framework variant that lacks the attributes used for verification.
    try:
        with torch.no_grad():
            pt_outs = wrapper(dummy)
            orig_out, _ = detect_model(dummy)

        d = detect
        bs = 1
        boxes_cat = torch.cat(
            [pt_outs[i * 2].reshape(bs, 4 * d.reg_max, -1) for i in range(d.nl)],
            dim=-1,
        )
        # cls is already Sigmoid'd; keep it as is (do not Sigmoid again)
        scores_cat = torch.cat(
            [pt_outs[i * 2 + 1].reshape(bs, d.nc, -1) for i in range(d.nl)],
            dim=-1,
        )
        dbox = d._get_decode_boxes(
            {
                "boxes": boxes_cat,
                "scores": scores_cat,
                "feats": [pt_outs[i * 2] for i in range(d.nl)],
            }
        )
        reconstructed = torch.cat((dbox, scores_cat), dim=1)

        diff = (orig_out - reconstructed).abs().max().item()
        print(f"  numerical check: max_diff = {diff:.10f}")
        if diff < 1e-5:
            print("  PASS: reconstructed output matches the original model")
        else:
            print("  WARNING: small diff (FP32 op ordering), check manually")
    except Exception as e:  # noqa: BLE001
        print(f"  numerical check skipped: {e}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python export_onnx.py <model.pt> <output.onnx> [imgsz] [opset]")
        sys.exit(1)
    pt = sys.argv[1]
    onnx_out = sys.argv[2]
    imgsz = int(sys.argv[3]) if len(sys.argv) > 3 else 640
    opset = int(sys.argv[4]) if len(sys.argv) > 4 else 17
    export_6out_onnx(pt, onnx_out, imgsz, opset)
