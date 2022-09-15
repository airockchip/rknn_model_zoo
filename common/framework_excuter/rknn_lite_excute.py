from rknnlite.api import RKNNLite


class RKNN_model_container():
    def __init__(self, model_path, target=None, device_id=None) -> None:
        rknn_lite = RKNNLite()

        # load RKNN model
        print('--> Load RKNN model')
        ret = rknn_lite.load_rknn(model_path)
        if ret != 0:
            print('Load RKNN model failed')
            exit(ret)
        print('done')
        print('--> Init runtime environment')
        if target==None:
            ret = rknn_lite.init_runtime()
        else:
            ret = rknn_lite.init_runtime(target=target, core_mask=RKNNLite.NPU_CORE_AUTO)
        if ret != 0:
            print('Init runtime environment failed')
            exit(ret)
        print('done')
        
        self.rknn = rknn_lite 

    def run(self, inputs):
        if isinstance(inputs, list) or isinstance(inputs, tuple):
            pass
        else:
            inputs = [inputs]

        result = self.rknn.inference(inputs=inputs)
    
        return result

    # def eval_perf(self):
    #     return self.rknn.eval_perf()

    # def eval_memory(self):
    #     return self.rknn.eval_memory()

    def release(self):
        self.rknn.release()