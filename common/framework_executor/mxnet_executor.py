import mxnet as mx
from mxnet import gluon

class Mxnet_model_container:
    def __init__(self, symbol, params, input_nodes) -> None:
        if input_nodes is not None:
            net = gluon.nn.SymbolBlock.imports(symbol_file=symbol, input_names=input_nodes, param_file=params, ctx=mx.cpu())
        else:
            net = gluon.nn.SymbolBlock.imports(symbol_file=symbol, input_names=['data'], param_file=params, ctx=mx.cpu())

        self.net = net

    def run(self, input_datas):
        #! now only support single input, input also should be image
        mx_input = mx.nd.array(input_datas[0])
        outputs = self.net(mx_input).asnumpy()
        return outputs
