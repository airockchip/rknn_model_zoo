import torch

class Torch_model_container:
    def __init__(self, model_path, quantize_type=None) -> None:
        self.pt_model = torch.jit.load(model_path)

        if quantize_type == 'qnnpack':
            torch.backends.quantized.engine = 'qnnpack'


    def run(self, input_datas):
        assert isinstance(input_datas, list), "input_datas should be a list, like [np.ndarray, np.ndarray]"

        input_datas_torch_type = []
        for _data in input_datas:
            input_datas_torch_type.append(torch.tensor(_data))

        result = self.pt_model(*input_datas_torch_type)

        if not (isinstance(result, list) or isinstance(result, tuple)):
            result = [result]

        for i in range(len(result)):
            result[i] = torch.dequantize(result[i])

        for i in range(len(result)):
            # TODO support quantized_output
            result[i] = result[i].cpu().detach().numpy()

        return result