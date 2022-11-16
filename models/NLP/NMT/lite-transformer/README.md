# Lite Transformer

本工程用于展示如何在RKNPU平台上部署lite transformer模型。

训练及导出onnx请参考：https://github.com/airockchip/lite-transformer/blob/master/README_RK.md



## 支持情况

<details>
<summary>Detail</summary>
<table>
    <tr>
        <td></td>
        <td>Lite_Transformer</td>
    </tr>
    <tr>
        <td>Toolkit1 - python demo</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>&#160&#160&#160&#160 &#160&#160&#160&#160 &#160&#160&#160&#160 &#160- C demo</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>Toolkit2 - python demo</td>
        <td>&#10004;</td>
    </tr>
    <tr>
        <td>&#160&#160&#160&#160 &#160&#160&#160&#160 &#160&#160&#160&#160 &#160- C demo</td>
        <td>&#10004;</td>
    </tr>
</table>
</details>




## rknn模型转换

请参考[[模型转换](./RKNN_model_convert/README.md)]

- 请注意！目前 RKNN-Toolkit1.7.3 转换此模型时可能会出现问题，请使用 RKNN-Toolkit 1.7.4.dev.1115d8fe 版本解决此问题。获取链接: [网盘(密码rknn)](https://eyun.baidu.com/s/3bqgIr0N)
- 预训练权重与RKNN模型获取链接: [网盘(密码:rknn)](https://eyun.baidu.com/s/3humTUNq) 
  - 预训练权重路径为 models/NLP/NMT/lite-transformer/checkpoints/{dataset_name}
  - RKNN模型路径为 models/NLP/NMT/lite-transformer/deploy_models/{dataset_name}/model_cvt/{platform}
- 由于ncv15数据集较小，预训练权重效果较差，后续会基于较大数据集训练提供新的预训练权重。



## rknn板端部署

请参考[[Toolkit1板端部署](./RKNN_C_demo/RKNPU/README.md)]

请参考[[Toolkit2板端部署](./RKNN_C_demo/RKNPU2/README.md)]







