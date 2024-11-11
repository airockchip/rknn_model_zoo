import numpy as np
from rknn.api import RKNN
import argparse
import onnxruntime
import soundfile as sf
import torch
import torch.nn as nn
onnxruntime.set_default_logger_severity(3)

MAX_LENGTH = 200

vocab = {' ': 19, "'": 1, '-': 14, '0': 23, '1': 15, '2': 28, '3': 11, '4': 27, '5': 35, '6': 36, '_': 30, 
        'a': 26, 'b': 24, 'c': 12, 'd': 5, 'e': 7, 'f': 20, 'g': 37, 'h': 6, 'i': 18, 'j': 16, 'k': 0, 'l': 21, 'm': 17, 
        'n': 29, 'o': 22, 'p': 13, 'q': 34, 'r': 25, 's': 8, 't': 33, 'u': 4, 'v': 32, 'w': 9, 'x': 31, 'y': 3, 'z': 2, '–': 10}

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

def run_encoder(encoder_model, input_ids_array, attention_mask_array):
    if 'rknn' in str(type(encoder_model)):
        log_duration, input_padding_mask, prior_means, prior_log_variances = encoder_model.inference(inputs=[input_ids_array, attention_mask_array])
    elif 'onnx' in str(type(encoder_model)):
        log_duration, input_padding_mask, prior_means, prior_log_variances = encoder_model.run(None, {"input_ids": input_ids_array, "attention_mask": attention_mask_array})

    return log_duration, input_padding_mask, prior_means, prior_log_variances

def run_decoder(decoder_model, attn, output_padding_mask, prior_means, prior_log_variances):
    if 'rknn' in str(type(decoder_model)):
        waveform  = decoder_model.inference(inputs=[attn, output_padding_mask, prior_means, prior_log_variances])[0]
    elif 'onnx' in str(type(decoder_model)):
        waveform  = decoder_model.run(None, {"attn": attn, "output_padding_mask": output_padding_mask, "prior_means": prior_means, "prior_log_variances": prior_log_variances})[0]

    return waveform

def pad_or_trim(token_id, attention_mask, max_length):
    pad_len = max_length - len(token_id)
    if pad_len <= 0:
        token_id = token_id[:max_length]
        attention_mask = attention_mask[:max_length]

    if pad_len > 0:
        token_id = token_id + [0] * pad_len
        attention_mask = attention_mask + [0] * pad_len

    return token_id, attention_mask

def preprocess_input(text, vocab, max_length):
    text = list(text.lower())
    input_id = []
    for token in text:
        if token not in vocab:
            continue
        input_id.append(0)
        input_id.append(int(vocab[token]))
    input_id.append(0)
    attention_mask = [1] * len(input_id)

    input_id, attention_mask = pad_or_trim(input_id, attention_mask, max_length)

    input_ids_array = np.array(input_id)[None,...]
    attention_mask_array = np.array(attention_mask)[None,...]

    return input_ids_array, attention_mask_array

def middle_process(log_duration, input_padding_mask, max_length):
    log_duration = torch.tensor(log_duration)
    input_padding_mask = torch.tensor(input_padding_mask)

    speaking_rate = 1
    length_scale = 1.0 / speaking_rate
    duration = torch.ceil(torch.exp(log_duration) * input_padding_mask * length_scale)
    predicted_lengths = torch.clamp_min(torch.sum(duration, [1, 2]), 1).long()

    # Create a padding mask for the output lengths of shape (batch, 1, max_output_length)
    predicted_lengths_max_real = predicted_lengths.max()
    predicted_lengths_max = max_length * 2

    indices = torch.arange(predicted_lengths_max, dtype=predicted_lengths.dtype)
    output_padding_mask = indices.unsqueeze(0) < predicted_lengths.unsqueeze(1)
    output_padding_mask = output_padding_mask.unsqueeze(1).to(input_padding_mask.dtype)

    # Reconstruct an attention tensor of shape (batch, 1, out_length, in_length)
    attn_mask = torch.unsqueeze(input_padding_mask, 2) * torch.unsqueeze(output_padding_mask, -1)
    batch_size, _, output_length, input_length = attn_mask.shape
    cum_duration = torch.cumsum(duration, -1).view(batch_size * input_length, 1)
    indices = torch.arange(output_length, dtype=duration.dtype)
    valid_indices = indices.unsqueeze(0) < cum_duration
    valid_indices = valid_indices.to(attn_mask.dtype).view(batch_size, input_length, output_length)
    padded_indices = valid_indices - nn.functional.pad(valid_indices, [0, 0, 1, 0, 0, 0])[:, :-1]
    attn = padded_indices.unsqueeze(1).transpose(2, 3) * attn_mask

    attn = attn.numpy()
    output_padding_mask = output_padding_mask.numpy()
    
    return attn, output_padding_mask, predicted_lengths_max_real

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='MMS_TTS Python Demo', add_help=True)
    # basic params
    parser.add_argument('--encoder_model_path', type=str, required=True,
                        help='model path, could be .rknn or .onnx file')
    parser.add_argument('--decoder_model_path', type=str, required=True,
                        help='model path, could be .rknn or .onnx file')
    parser.add_argument('--target', type=str,
                        default='rk3588', help='target RKNPU platform')
    parser.add_argument('--device_id', type=str,
                        default=None, help='device id')
    args = parser.parse_args()

    # Set inputs
    text = "Mister quilter is the apostle of the middle classes and we are glad to welcome his gospel."
    input_ids_array, attention_mask_array = preprocess_input(text, vocab, max_length=MAX_LENGTH)

    # Init model
    encoder_model = init_model(args.encoder_model_path, args.target, args.device_id)
    decoder_model = init_model(args.decoder_model_path, args.target, args.device_id)

    # Encode
    log_duration, input_padding_mask, prior_means, prior_log_variances = run_encoder(encoder_model, input_ids_array, attention_mask_array)

    # Middle process
    attn, output_padding_mask, predicted_lengths_max_real = middle_process(log_duration, input_padding_mask, MAX_LENGTH)

    # Decode
    waveform = run_decoder(decoder_model, attn, output_padding_mask, prior_means, prior_log_variances)

    # Post process
    audio_save_path = "../output.wav"
    sf.write(file=audio_save_path, data=np.array(waveform[0][:predicted_lengths_max_real * 256]), samplerate=16000)
    print('\nThe output wav file is saved:', audio_save_path)

    # Release
    release_model(encoder_model)
    release_model(decoder_model)
    

