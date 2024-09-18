import torch
from transformers import Wav2Vec2ForCTC, Wav2Vec2Processor
import numpy as np
import onnx
from onnxsim import simplify
import argparse
import warnings
warnings.filterwarnings("ignore", category=UserWarning)
warnings.filterwarnings("ignore", category=torch.jit.TracerWarning)


def setup_model(model_name):
    processor = Wav2Vec2Processor.from_pretrained(model_name)
    model = Wav2Vec2ForCTC.from_pretrained(model_name)
    return processor, model

def setup_data(processor, chunk_length):
    sample_rate = 16000
    max_length = chunk_length * sample_rate
    audio_array = np.random.randn(1, max_length).astype(np.float32)
    input_values = processor(audio_array, return_tensors="pt", padding="max_length", max_length=max_length, truncation=True).input_values
    return input_values

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Export wav2vec2 onnx model", add_help=True)
    parser.add_argument("--chunk_length", type=int, required=True, default= 20,
                        help="audio length, default is 20 seconds")
    args = parser.parse_args()

    processor, model = setup_model("facebook/wav2vec2-base-960h")
    input_values = setup_data(processor, args.chunk_length)

    save_model_path = "../model/wav2vec2_base_960h_{}s.onnx".format(args.chunk_length)
    torch.onnx.export(
        model,
        input_values,
        save_model_path,
        do_constant_folding=True,
        export_params=True,
        input_names=["input"],
        output_names=["output"],
        opset_version=12)

    # simplify
    original_model = onnx.load(save_model_path)
    simplified_model, check = simplify(original_model)
    onnx.save(simplified_model, save_model_path)

    print("\nThe model is saved in:", save_model_path)
