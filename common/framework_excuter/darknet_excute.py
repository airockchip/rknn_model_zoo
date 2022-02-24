import cv2
import numpy as np

class Darknet_model_container:
    def __init__(self, model_cfg, model_weights, mean_values, std_values) -> None:
        self.net = cv2.dnn.readNetFromDarknet(
            cfgFile=model_cfg,
            darknetModel=model_weights
        )
        self.mean_values = mean_values
        self.std_values = std_values


    def run(self, input_datas):
        #! now only support single input, input also should be image
        #! std values should have same values for each
        blob = cv2.dnn.blobFromImage(input_datas[0], 
                                     1.0/self.std_values[0], 
                                     (input_datas[0].shape[1], input_datas[0].shape[0]), 
                                     tuple(self.mean_values))
        self.net.setInput(blob)

        output_names = self.net.getUnconnectedOutLayersNames()
        new_output_names = []
        for oup in output_names:
            out_idx = self.net.getLayerId(oup)
            layer = self.net.getLayer(out_idx)
            # 若为yolo层则取yolo层之前的作为输出
            if layer.type == 'Region':
                new_output_names.append(self.net.getLayer(out_idx - 2).name)
            else:
                new_output_names.append(layer.name)

        outputs = self.net.forward(new_output_names)
        return outputs