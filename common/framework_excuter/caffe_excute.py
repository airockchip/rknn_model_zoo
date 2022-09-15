import cv2
import numpy as np

class Caffe_model_container:
    def __init__(self, prototxt, caffemodel, output_nodes, mean_values, std_values) -> None:
        self.net = cv2.dnn.readNetFromCaffe(prototxt, caffemodel)
        self.output_nodes = output_nodes
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
        outputs = self.net.forward(self.output_nodes)

        return outputs