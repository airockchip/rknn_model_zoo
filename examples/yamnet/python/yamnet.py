import numpy as np
from rknn.api import RKNN
import argparse
import soundfile as sf
import onnxruntime
import scipy

CHUNK_LENGTH = 3  # 3 seconds
MAX_N_SAMPLES = CHUNK_LENGTH * 16000


def ensure_sample_rate(waveform, original_sample_rate, desired_sample_rate=16000):
    if original_sample_rate != desired_sample_rate:
        print("resample_audio: {} HZ -> {} HZ".format(original_sample_rate, desired_sample_rate))
        desired_length = int(round(float(len(waveform)) / original_sample_rate * desired_sample_rate))
        waveform = scipy.signal.resample(waveform, desired_length)
    return waveform, desired_sample_rate

def ensure_channels(waveform, original_channels, desired_channels=1):
    if original_channels != desired_channels:
        print("convert_channels: {} -> {}".format(original_channels, desired_channels))
        waveform = np.mean(waveform, axis=1)
    return waveform, desired_channels

def init_model(model_path, target=None, device_id=None):
    if model_path.endswith(".rknn"):
        # Create RKNN object
        model = RKNN()

        # Load RKNN model
        print('--> Loading model')
        ret = model.load_rknn(model_path)
        if ret != 0:
            print('Load RKNN model \"{}\" failed!'.format(model_path))
            exit(ret)
        print('done')

        # init runtime environment
        print('--> Init runtime environment')
        ret = model.init_runtime(target=target, device_id=device_id)
        if ret != 0:
            print('Init runtime environment failed')
            exit(ret)
        print('done')

    elif model_path.endswith(".onnx"):
        model = onnxruntime.InferenceSession(model_path,  providers=['CPUExecutionProvider'])

    return model

def run_model(model, audio):
    if 'rknn' in str(type(model)):
        outputs  = model.inference(inputs=audio)
    elif 'onnx' in str(type(model)):
        outputs  = model.run(None, {model.get_inputs()[0].name: audio})

    return outputs

def release_model(model):
    if 'rknn' in str(type(model)):
        model.release()
    elif 'onnx' in str(type(model)):
        del model
    model = None

def post_process(outputs):
    scores = outputs[2]
    top_class_index = scores.mean(axis=0).argmax()
    return top_class_index

def pad_or_trim(array, length, axis=-1):
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
            value = ' '.join(line[1:])
            data_dict[key] = value
    return data_dict

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Yamnet Python Demo', add_help=True)
    # basic params
    parser.add_argument('--model_path', type=str, required=True, help='model path, could be .rknn/.onnx file')
    parser.add_argument('--target', type=str, default='rk3588', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    audio_data, sample_rate = sf.read("../model/test.wav")
    channels = audio_data.ndim
    audio_data, channels = ensure_channels(audio_data, channels)
    audio_data, sample_rate = ensure_sample_rate(audio_data, sample_rate)
    audio_array = np.array(audio_data, dtype=np.float32)
    audio = pad_or_trim(audio_array.flatten(), MAX_N_SAMPLES)
    audio = np.expand_dims(audio, 0)
    label = read_txt_to_dict("../model/yamnet_class_map.txt")

    # Init model
    model = init_model(args.model_path, args.target, args.device_id)

    # Run model
    outputs = run_model(model, audio)

    # Post process
    top_class_index = post_process(outputs)
    print("\nThe main sound is: ", label[str(top_class_index)])

    # Release model
    release_model(model)