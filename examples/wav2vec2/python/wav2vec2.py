import numpy as np
from rknn.api import RKNN
import argparse
import soundfile as sf
import onnxruntime
import scipy

CHUNK_LENGTH = 20  # 20 seconds
MAX_N_SAMPLES = CHUNK_LENGTH * 16000

tokenizer_dict = {0: "<pad>", 1: "<s>", 2: "</s>", 3: "<unk>", 4: "|", 5: "E", 6: "T", 7: "A", 8: "O", 9: "N", 10: "I", 
                    11: "H", 12: "S", 13: "R", 14: "D", 15: "L", 16: "U", 17: "M", 18: "W", 19: "C", 20: "F", 21: "G", 
                    22: "Y", 23: "P", 24: "B", 25: "V", 26: "K", 27: "'", 28: "X", 29: "J", 30: "Q", 31: "Z"}

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

def run_model(model, audio_array):
    if 'rknn' in str(type(model)):
        outputs  = model.inference(inputs=audio_array)[0]
    elif 'onnx' in str(type(model)):
        outputs  = model.run(None, {model.get_inputs()[0].name: audio_array})[0]

    return outputs

def release_model(model):
    if 'rknn' in str(type(model)):
        model.release()
    elif 'onnx' in str(type(model)):
        del model
    model = None

def pre_process(audio_array, max_length, pad_value=0):
    array_length = len(audio_array)
    
    if array_length < max_length:
        pad_length = max_length - array_length
        audio_array = np.pad(audio_array, (0, pad_length), mode='constant', constant_values=pad_value)
    elif array_length > max_length:
        audio_array = audio_array[:max_length]

    return audio_array

def compress_sequence(sequence):
    compressed_sequence = [sequence[0]]

    for i in range(1, len(sequence)):
        if sequence[i] != sequence[i - 1]:
            compressed_sequence.append(sequence[i])
    
    return compressed_sequence

def decode(token_ids):
    token_ids = compress_sequence(token_ids)
    transcriptions = []

    for token_id in token_ids:
        if token_id <= 4:
            if token_id == 4:
                transcriptions.append(' ')
            continue
        token = tokenizer_dict[token_id]
        transcriptions.append(token)

    transcription = "".join(transcriptions)
    return transcription

def post_process(output):
    predicted_ids = np.argmax(output, axis=-1)
    transcription = decode(predicted_ids[0])
    return transcription

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Wav2vec2 Python Demo', add_help=True)
    # basic params
    parser.add_argument('--model_path', type=str, required=True, help='model path, could be .rknn file')
    parser.add_argument('--target', type=str, default='rk3588', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    audio_data, sample_rate = sf.read("../model/test.wav")
    channels = audio_data.ndim
    audio_data, channels = ensure_channels(audio_data, channels)
    audio_data, sample_rate = ensure_sample_rate(audio_data, sample_rate)
    audio_array = np.array(audio_data, dtype=np.float32)
    audio_array = pre_process(audio_array, MAX_N_SAMPLES)
    audio_array = np.expand_dims(audio_array, axis=0)

    # Init model
    model = init_model(args.model_path, args.target, args.device_id)

    # Run model
    outputs = run_model(model, audio_array)

    # Post process
    transcription = post_process(outputs)
    print('\nWav2vec2 output:', transcription)

    # Release model
    release_model(model)