import tensorflow as tf

class Tflite_model_container:
    def __init__(self, model_path) -> None:
        self.interpreter = tf.lite.Interpreter(model_path=model_path)
        self.interpreter.allocate_tensors()

        # Get input and output tensors.
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()
        # print(input_details)

    def run(self, input_datas):
        if len(input_datas) < len(self.input_details):
            assert False,'inputs_datas number not match onnx model{} input'.format(model_path)
        elif len(input_datas) > len(self.input_details):
            print('WARNING: input datas number large than onnx input node')

        for i in range(len(input_datas)):
            input_datas[i] = input_datas[i].astype(self.input_details[i]['dtype'])
            self.interpreter.set_tensor(self.input_details[i]['index'], input_datas[i])
        self.interpreter.invoke()

        outputs = []
        for i in range(0, len(self.output_details)):
            output_data = self.interpreter.get_tensor(self.output_details[i]['index'])
            outputs.append(output_data)

        return outputs