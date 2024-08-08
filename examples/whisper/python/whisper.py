import numpy as np
from rknn.api import RKNN
import argparse
import soundfile as sf
import onnxruntime
import torch
import torch.nn.functional as F

SAMPLE_RATE = 16000
N_FFT = 400
HOP_LENGTH = 160
CHUNK_LENGTH = 20
N_SAMPLES = CHUNK_LENGTH * SAMPLE_RATE

with open('../model/vocab.txt', 'r') as f:
    vocab = {}
    for line in f:
        if len(line.strip().split(' ')) < 2:
            key = line.strip().split(' ')[0]
            value = ""
        else:
            key, value = line.strip().split(' ')
        vocab[key] = value

def mel_filters(n_mels):
    assert n_mels in {80}, f"Unsupported n_mels: {n_mels}"
    filters_path =  "../model/mel_80_filters.txt"
    mels_data = np.loadtxt(filters_path, dtype=np.float32).reshape((80, 201))
    return torch.from_numpy(mels_data)

def pad_or_trim(array, length=N_SAMPLES, axis=-1):
    if array.shape[axis] > length:
        array = array.take(indices=range(length), axis=axis)

    if array.shape[axis] < length:
        pad_widths = [(0, 0)] * array.ndim
        pad_widths[axis] = (0, length - array.shape[axis])
        array = np.pad(array, pad_widths)

    return array

def log_mel_spectrogram(audio, n_mels=80, padding=0):
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

def decode(out_encoder, decoder_model, args):
    # tokenizer = whisper.decoding.get_tokenizer( True, #model.is_multilingual
    #                                             task="transcribe", 
    #                                             language="en",
    #                                             )
    
    end_token = 50257 # tokenizer.eot
    tokens = [50258, 50259, 50359, 50363] # tokenizer.sot_sequence_including_notimestamps
    timestamp_begin = 50364 # tokenizer.timestamp_begin

    max_tokens = 12
    tokens_str = ''
    pop_id = max_tokens

    tokens = tokens * int(max_tokens/4)
    next_token = 50258 # tokenizer.sot

    while next_token != end_token:
        if args.encoder_model_path.endswith(".rknn"):
            out_decoder = decoder_model.inference([np.asarray([tokens], dtype="int64"), out_encoder])[0]
        elif args.encoder_model_path.endswith(".onnx"):
            out_decoder = decoder_model.run(["out"], {"tokens": np.asarray([tokens], dtype="int64"), "audio": out_encoder})[0]
        
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

    result = tokens_str.replace('\u0120', ' ').replace('<|endoftext|>', '')
    return result

def init_rknn_model(model_path, target, device_id):
    # Create RKNN object
    rknn = RKNN()

    # Load RKNN model
    print('--> Loading model')
    ret = rknn.load_rknn(model_path)
    if ret != 0:
        print('Load RKNN model \"{}\" failed!'.format(model_path))
        exit(ret)
    print('done')

    # init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime(target=target, device_id=device_id)
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    return rknn

def init_onnx_model(model_path):
    model = onnxruntime.InferenceSession(model_path,  providers=['AzureExecutionProvider', 'CPUExecutionProvider'])
    return model

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Whisper Python Demo', add_help=True)
    # basic params
    parser.add_argument('--encoder_model_path', type=str, required=True,
                        help='model path, could be .rknn or .onnx file')
    parser.add_argument('--decoder_model_path', type=str, required=True,
                        help='model path, could be .rknn or .onnx file')
    parser.add_argument('--target', type=str,
                        default='rk3566', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str,
                        default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    audio_data, sample_rate = sf.read("../model/test.wav")
    audio_array = np.array(audio_data, dtype=np.float32)
    audio = pad_or_trim(audio_array.flatten())
    x_mel = log_mel_spectrogram(audio).numpy()

    # Init model
    if args.encoder_model_path.endswith(".rknn") and args.decoder_model_path.endswith(".rknn"):
        encoder_model = init_rknn_model(args.encoder_model_path, args.target, args.device_id)
        decoder_model = init_rknn_model(args.decoder_model_path, args.target, args.device_id)
    elif args.encoder_model_path.endswith(".onnx") and args.decoder_model_path.endswith(".onnx"):
        encoder_model = init_onnx_model(args.encoder_model_path)
        decoder_model = init_onnx_model(args.decoder_model_path)

    # Encode
    if args.encoder_model_path.endswith(".rknn") and args.decoder_model_path.endswith(".rknn"):
        out_encoder = encoder_model.inference(inputs=[x_mel])[0]
    elif args.encoder_model_path.endswith(".onnx") and args.decoder_model_path.endswith(".onnx"):
        out_encoder = encoder_model.run(None, {"x": [x_mel]})[0]
        
    # Decode
    result = decode(out_encoder, decoder_model, args)
    print("Whisper output:", result)