import torch
torch.backends.quantized.engine = 'qnnpack'

def multi_list_unfold(tl):
    def unfold(_inl, target):
        if not isinstance(_inl, list) and not isinstance(_inl, tuple):
            target.append(_inl)
        else:
            unfold(_inl)

def flatten_list(in_list):
    flatten = lambda x: [subitem for item in x for subitem in flatten(item)] if type(x) is list else [x]
    return flatten(in_list)

class Torch_model_container:
    def __init__(self, model_path, qnnpack=False) -> None:
        if qnnpack is True:
            torch.backends.quantized.engine = 'qnnpack'

        #! Backends must be set before load model.
        self.pt_model = torch.jit.load(model_path)
        self.pt_model.eval()
        holdon = 1

    def run(self, input_datas):
        assert isinstance(input_datas, list), "input_datas should be a list, like [np.ndarray, np.ndarray]"

        input_datas_torch_type = []
        for _data in input_datas:
            input_datas_torch_type.append(torch.tensor(_data))

        for i,val in enumerate(input_datas_torch_type):
            if val.dtype == torch.float64:
                input_datas_torch_type[i] = input_datas_torch_type[i].float()

        result = self.pt_model(*input_datas_torch_type)

        if isinstance(result, tuple):
            result = list(result)
        if not isinstance(result, list):
            result = [result]
        
        result = flatten_list(result)

        for i in range(len(result)):
            result[i] = torch.dequantize(result[i])

        for i in range(len(result)):
            # TODO support quantized_output
            result[i] = result[i].cpu().detach().numpy()

        return result