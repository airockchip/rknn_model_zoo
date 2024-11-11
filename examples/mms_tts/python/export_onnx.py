
import os
randn_like_latents_path = '../model/randn_like_latents.npy'
absolute_randn_like_latents_path = os.path.abspath(randn_like_latents_path)
os.system(f'export randn_like_latents_path={absolute_randn_like_latents_path}')
os.environ['randn_like_latents_path'] = absolute_randn_like_latents_path

from transformers import VitsModel, AutoTokenizer
import torch
import torch.nn as nn
import numpy as np
import argparse
import warnings
warnings.filterwarnings("ignore", category=UserWarning)
warnings.filterwarnings("ignore", category=torch.jit.TracerWarning)

def setup_model(model_name):
    model = VitsModel.from_pretrained(model_name)
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    model.requires_grad_(False)
    model.eval()
    return model, tokenizer

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Export mms_tts onnx model', add_help=True)
    parser.add_argument('--max_length', type=int, required=True, default= 200,
                        help='input length of encoder model, default is 200')
    args = parser.parse_args()

    model, tokenizer = setup_model("facebook/mms-tts-eng")

    text = "some example text in the English language"
    inputs = tokenizer(text, return_tensors="pt", padding="max_length", max_length=args.max_length, truncation=True)
    input_ids = inputs['input_ids']
    attention_mask = inputs['attention_mask']
    
    randn_like_latents = torch.randn(input_ids.size(0), 2, input_ids.size(1))
    np.save(randn_like_latents_path, randn_like_latents)
    
    log_duration, input_padding_mask, prior_means, prior_log_variances = model(input_ids, attention_mask)

    speaking_rate = 1.0
    length_scale = 1.0 / speaking_rate
    duration = torch.ceil(torch.exp(log_duration) * input_padding_mask * length_scale)
    predicted_lengths = torch.clamp_min(torch.sum(duration, [1, 2]), 1).long()
    # predicted_lengths_max = predicted_lengths.max()
    predicted_lengths_max = args.max_length * 2
    indices = torch.arange(predicted_lengths_max, dtype=predicted_lengths.dtype, device=predicted_lengths.device)
    output_padding_mask = indices.unsqueeze(0) < predicted_lengths.unsqueeze(1)
    output_padding_mask = output_padding_mask.unsqueeze(1).to(input_padding_mask.dtype)
    attn_mask = torch.unsqueeze(input_padding_mask, 2) * torch.unsqueeze(output_padding_mask, -1)
    batch_size, _, output_length, input_length = attn_mask.shape
    cum_duration = torch.cumsum(duration, -1).view(batch_size * input_length, 1)
    indices = torch.arange(output_length, dtype=duration.dtype, device=duration.device)
    valid_indices = indices.unsqueeze(0) < cum_duration
    valid_indices = valid_indices.to(attn_mask.dtype).view(batch_size, input_length, output_length)
    padded_indices = valid_indices - nn.functional.pad(valid_indices, [0, 0, 1, 0, 0, 0])[:, :-1]
    attn = padded_indices.unsqueeze(1).transpose(2, 3) * attn_mask
    
    save_encoder_model_path = "../model/mms_tts_eng_encoder_{}.onnx".format(args.max_length)
    save_decoder_model_path = "../model/mms_tts_eng_decoder_{}.onnx".format(args.max_length)
    torch.onnx.export( 
        model,
        (input_ids, attention_mask),
        save_encoder_model_path,
        do_constant_folding=True,
        export_params=True,
        input_names=['input_ids', 'attention_mask'],
        output_names=['log_duration', 'input_padding_mask', 'prior_means', 'prior_log_variances'],
        opset_version=12)
    print("\nThe encoder model is saved in:", save_encoder_model_path)

    torch.onnx.export(
        model,
        (attn, output_padding_mask, prior_means, prior_log_variances),
        save_decoder_model_path,
        do_constant_folding=True,
        export_params=True,
        input_names=['attn', 'output_padding_mask', 'prior_means', 'prior_log_variances'],
        output_names=['waveform'],
        opset_version=12)

    print("The decoder model is saved in:", save_decoder_model_path)
