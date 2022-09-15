import torch


class torch_runner:
    def __init__(self, model_path, backend=None) -> None:
        self.model = torch.load(model_path)

        if backend == 'qnnpack':
            torch.backends.quantized.engine = backend

    def run(self, inputs):
        if isinstance(inputs, list) or isinstance(inputs, tuple):
            inputs = [torch.tensor(_input) for _input in inputs]
        else:
            inputs = [torch.tensor(inputs)]

        result = self.model(*inputs)

        if isinstance(result, list) or isinstance(result, tuple):
            result = [_result.cpu().detach().numpy() for _result in result]
        else:
            result = [result.cpu().detach().numpy()]

        return result
