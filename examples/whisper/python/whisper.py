import numpy as np
from rknn.api import RKNN
import argparse
import soundfile as sf
import onnxruntime
import torch
import torch.nn.functional as F
import scipy

SAMPLE_RATE = 16000
N_FFT = 400
HOP_LENGTH = 160
CHUNK_LENGTH = 20
N_SAMPLES = CHUNK_LENGTH * SAMPLE_RATE
MAX_LENGTH = CHUNK_LENGTH * 100
N_MELS = 80


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

def get_char_index(c):
    if 'A' <= c <= 'Z':
        return ord(c) - ord('A')
    elif 'a' <= c <= 'z':
        return ord(c) - ord('a') + (ord('Z') - ord('A') + 1)
    elif '0' <= c <= '9':
        return ord(c) - ord('0') + (ord('Z') - ord('A')) + (ord('z') - ord('a')) + 2
    elif c == '+':
        return 62
    elif c == '/':
        return 63
    else:
        print(f"Unknown character {ord(c)}, {c}")
        exit(-1)

def base64_decode(encoded_string):
    if not encoded_string:
        print("Empty string!")
        exit(-1)

    output_length = len(encoded_string) // 4 * 3
    decoded_string = bytearray(output_length)

    index = 0
    output_index = 0
    while index < len(encoded_string):
        if encoded_string[index] == '=':
            return " "

        first_byte = (get_char_index(encoded_string[index]) << 2) + ((get_char_index(encoded_string[index + 1]) & 0x30) >> 4)
        decoded_string[output_index] = first_byte

        if index + 2 < len(encoded_string) and encoded_string[index + 2] != '=':
            second_byte = ((get_char_index(encoded_string[index + 1]) & 0x0f) << 4) + ((get_char_index(encoded_string[index + 2]) & 0x3c) >> 2)
            decoded_string[output_index + 1] = second_byte

            if index + 3 < len(encoded_string) and encoded_string[index + 3] != '=':
                third_byte = ((get_char_index(encoded_string[index + 2]) & 0x03) << 6) + get_char_index(encoded_string[index + 3])
                decoded_string[output_index + 2] = third_byte
                output_index += 3
            else:
                output_index += 2
        else:
            output_index += 1

        index += 4
            
    return decoded_string.decode('utf-8', errors='replace')

def read_vocab(vocab_path):
    with open(vocab_path, 'r') as f:
        vocab = {}
        for line in f:
            if len(line.strip().split(' ')) < 2:
                key = line.strip().split(' ')[0]
                value = ""
            else:
                key, value = line.strip().split(' ')
            vocab[key] = value
    return vocab

def pad_or_trim(audio_array):
    x_mel = np.zeros((N_MELS, MAX_LENGTH), dtype=np.float32)
    real_length = audio_array.shape[1] if audio_array.shape[1] <= MAX_LENGTH else MAX_LENGTH
    x_mel[:, :real_length] = audio_array[:, :real_length]
    return x_mel

def mel_filters(n_mels):
    assert n_mels in {80}, f"Unsupported n_mels: {n_mels}"
    filters_path =  "../model/mel_80_filters.txt"
    mels_data = np.loadtxt(filters_path, dtype=np.float32).reshape((80, 201))
    return torch.from_numpy(mels_data)

def log_mel_spectrogram(audio, n_mels, padding=0):
    if not torch.is_tensor(audio):
        audio = torch.from_numpy(audio)

    if padding > 0:
        audio = F.pad(audio, (0, padding))
    window = torch.hann_window(N_FFT)

    stft = torch.stft(audio, N_FFT, HOP_LENGTH, window=window, return_complex=True)
    magnitudes = stft[..., :-1].abs() ** 2

    filters = mel_filters(n_mels)
    mel_spec = filters @ magnitudes

    log_spec = torch.clamp(mel_spec, min=1e-10).log10()
    log_spec = torch.maximum(log_spec, log_spec.max() - 8.0)
    log_spec = (log_spec + 4.0) / 4.0
    return log_spec

def run_encoder(encoder_model, in_encoder):
    if 'rknn' in str(type(encoder_model)):
        out_encoder = encoder_model.inference(inputs=in_encoder)[0]
    elif 'onnx' in str(type(encoder_model)):
        out_encoder = encoder_model.run(None, {"x": in_encoder})[0]

    return out_encoder

def _decode(decoder_model, tokens, out_encoder):
    if 'rknn' in str(type(decoder_model)):
        out_decoder = decoder_model.inference([np.asarray([tokens], dtype="int64"), out_encoder])[0]
    elif 'onnx' in str(type(decoder_model)):
        out_decoder = decoder_model.run(None, {"tokens": np.asarray([tokens], dtype="int64"), "audio": out_encoder})[0]

    return out_decoder

def run_decoder(decoder_model, out_encoder, vocab, task_code):
    # tokenizer = whisper.decoding.get_tokenizer( True, #model.is_multilingual
    #                                             task="transcribe", 
    #                                             language="en",
    #                                             )
    
    end_token = 50257 # tokenizer.eot
    tokens = [50258, task_code, 50359, 50363] # tokenizer.sot_sequence_including_notimestamps
    timestamp_begin = 50364 # tokenizer.timestamp_begin

    max_tokens = 12
    tokens_str = ''
    pop_id = max_tokens

    tokens = tokens * int(max_tokens/4)
    next_token = 50258 # tokenizer.sot

    while next_token != end_token:
        out_decoder = _decode(decoder_model, tokens, out_encoder)
        next_token = out_decoder[0, -1].argmax()
        next_token_str = vocab[str(next_token)]
        tokens.append(next_token)

        if next_token == end_token:
            tokens.pop(-1)
            next_token = tokens[-1]
            break
        if next_token > timestamp_begin:
            continue
        if pop_id >4:
            pop_id -= 1

        tokens.pop(pop_id)
        tokens_str += next_token_str

    result = tokens_str.replace('\u0120', ' ').replace('<|endoftext|>', '').replace('\n', '')
    if task_code == 50260: # TASK_FOR_ZH
        result = base64_decode(result)
    return result

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

def release_model(model):
    if 'rknn' in str(type(model)):
        model.release()
    elif 'onnx' in str(type(model)):
        del model
    model = None

def load_array_from_file(filename):
    with open(filename, 'r') as file:
        data = file.readlines()

    array = []
    for line in data:
        row = [float(num) for num in line.split()]
        array.extend(row)

    return np.array(array).reshape((80, 2000))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Whisper Python Demo', add_help=True)
    # basic params
    parser.add_argument('--encoder_model_path', type=str, required=True, help='model path, could be .rknn or .onnx file')
    parser.add_argument('--decoder_model_path', type=str, required=True, help='model path, could be .rknn or .onnx file')
    parser.add_argument('--task', type=str, required=True, help='recognition task, could be en or zh')
    parser.add_argument('--audio_path', type=str, required=True, help='audio path')
    parser.add_argument('--target', type=str, default='rk3588', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str, default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    if args.task == "en":
        vocab_path = "../model/vocab_en.txt"
        task_code = 50259
    elif args.task == "zh":
        vocab_path = "../model/vocab_zh.txt"
        task_code = 50260
    else:
        print("\n\033[1;33mCurrently only English or Chinese recognition tasks are supported. Please specify --task as en or zh\033[0m")
        exit(1)
    vocab = read_vocab(vocab_path)
    audio_data, sample_rate = sf.read(args.audio_path)
    channels = audio_data.ndim
    audio_data, channels = ensure_channels(audio_data, channels)
    audio_data, sample_rate = ensure_sample_rate(audio_data, sample_rate)
    audio_array = np.array(audio_data, dtype=np.float32)
    audio_array= log_mel_spectrogram(audio_array, N_MELS).numpy()
    x_mel = pad_or_trim(audio_array)
    x_mel = np.expand_dims(x_mel, 0)

    # Init/Encode/Decode
    encoder_model = init_model(args.encoder_model_path, args.target, args.device_id)
    decoder_model = init_model(args.decoder_model_path, args.target, args.device_id)
    out_encoder = run_encoder(encoder_model, x_mel)
    result = run_decoder(decoder_model, out_encoder, vocab, task_code)
    print("\nWhisper output:", result)

    # Release
    release_model(encoder_model)
    release_model(decoder_model)