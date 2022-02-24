from tensorflow.keras.models import load_model

class Keras_model_container:
    def __init__(self, model_path) -> None:
        self.net = load_model(model_path)

    def run(self, input_datas):
        outputs = self.net.predict(input_datas)

        return outputs