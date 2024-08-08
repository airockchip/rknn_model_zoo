import numpy as np
from rknn.api import RKNN
import argparse
import soundfile as sf
import onnxruntime

MAX_N_SAMPLES = 3 * 16000  # 3 seconds


def pad_or_trim(array, length=MAX_N_SAMPLES, axis=-1):
    if array.shape[axis] > length:
        array = array.take(indices=range(length), axis=axis)

    if array.shape[axis] < length:
        pad_widths = [(0, 0)] * array.ndim
        pad_widths[axis] = (0, length - array.shape[axis])
        array = np.pad(array, pad_widths)

    return array


def read_txt_to_dict(filename):
    data_dict = {}
    with open(filename, 'r') as txtfile:
        for line in txtfile:
            line = line.strip().split(' ')
            key = line[0]
            value = ''.join(line[1:])
            data_dict[key] = value
    return data_dict


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Yamnet Python Demo', add_help=True)
    # basic params
    parser.add_argument('--model_path', type=str, required=True,
                        help='model path, could be .rknn/.onnx file')
    parser.add_argument('--target', type=str,
                        default='rk3566', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str,
                        default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    audio_data, sample_rate = sf.read("../model/test.wav")
    audio_array = np.array(audio_data, dtype=np.float32)
    audio = pad_or_trim(audio_array.flatten())
    audio = np.expand_dims(audio, 0)
    label = read_txt_to_dict("../model/yamnet_class_map.txt")

    # Set model
    if args.model_path.endswith(".rknn"):
        # Create RKNN object
        rknn = RKNN()

        # Load RKNN model
        print('--> Loading model')
        ret = rknn.load_rknn(args.model_path)
        if ret != 0:
            print('Load RKNN model \"{}\" failed!'.format(args.model_path))
            exit(ret)
        print('done')

        # init runtime environment
        print('--> Init runtime environment')
        ret = rknn.init_runtime(target=args.target, device_id=args.device_id)
        if ret != 0:
            print('Init runtime environment failed')
            exit(ret)
        print('done')

        # Inference
        print('--> Running model')
        outputs = rknn.inference(inputs=audio)
        print('--> done')

        rknn.release()

    elif args.model_path.endswith(".onnx"):
        model = onnxruntime.InferenceSession(args.model_path,  providers=['CPUExecutionProvider'])
        inputs = {model.get_inputs()[0].name: audio}
        outputs = model.run(None, inputs)

    # Post process
    scores = outputs[2]
    top_class_index = scores.mean(axis=0).argmax()
    print("\nThe main sound is: ", label[str(top_class_index)])
