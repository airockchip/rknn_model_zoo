
# Guidelines for exporting mms_tts onnx models

## Table of contents
- [Export mms\_tts onnx model](#export-mms_tts-onnx-model)
- [Special Notes](#special-notes)


## Export mms_tts onnx model

1.Install environment

```sh
pip install torch==1.10.0+cpu torchvision==0.11.0+cpu torchaudio==0.10.0 -f https://download.pytorch.org/whl/torch_stable.html

pip install transformers==4.39.3
```

2.Modify source code<br>

Copy all the code in [modeling_vits_for_export_onnx.py](./python/modeling_vits_for_export_onnx.py) to the `transformers/models/vits/modeling_vits.py` installation package path. For example, copy to `~/python3.8/site-packages/transformers/models/vits/modeling_vits.py`

***Differences from the original source code***

- To handle dynamic shapes caused by `Range` operators, the model is split into `encoder/decoder/middle_process`.
    ```py
    # before
    @add_start_docstrings_to_model_forward(VITS_INPUTS_DOCSTRING)
    @replace_return_docstrings(output_type=VitsModelOutput, config_class=_CONFIG_FOR_DOC)
    def forward(
        self,
        input_ids: Optional[torch.Tensor] = None,
        attention_mask: Optional[torch.Tensor] = None,
        speaker_id: Optional[int] = None,
        output_attentions: Optional[bool] = None,
        output_hidden_states: Optional[bool] = None,
        return_dict: Optional[bool] = None,
        labels: Optional[torch.FloatTensor] = None,
    ) -> Union[Tuple[Any], VitsModelOutput]:
        output_attentions = output_attentions if output_attentions is not None else self.config.output_attentions
        output_hidden_states = (
            output_hidden_states if output_hidden_states is not None else self.config.output_hidden_states
        )
        return_dict = return_dict if return_dict is not None else self.config.use_return_dict

        if attention_mask is not None:
            input_padding_mask = attention_mask.unsqueeze(-1).float()
        else:
            input_padding_mask = torch.ones_like(input_ids).unsqueeze(-1).float()

        if self.config.num_speakers > 1 and speaker_id is not None:
            if not 0 <= speaker_id < self.config.num_speakers:
                raise ValueError(f"Set `speaker_id` in the range 0-{self.config.num_speakers - 1}.")
            if isinstance(speaker_id, int):
                speaker_id = torch.full(size=(1,), fill_value=speaker_id, device=self.device)
            speaker_embeddings = self.embed_speaker(speaker_id).unsqueeze(-1)
        else:
            speaker_embeddings = None

        if labels is not None:
            raise NotImplementedError("Training of VITS is not supported yet.")

        text_encoder_output = self.text_encoder(
            input_ids=input_ids,
            padding_mask=input_padding_mask,
            attention_mask=attention_mask,
            output_attentions=output_attentions,
            output_hidden_states=output_hidden_states,
            return_dict=return_dict,
        )
        hidden_states = text_encoder_output[0] if not return_dict else text_encoder_output.last_hidden_state
        hidden_states = hidden_states.transpose(1, 2)
        input_padding_mask = input_padding_mask.transpose(1, 2)
        prior_means = text_encoder_output[1] if not return_dict else text_encoder_output.prior_means
        prior_log_variances = text_encoder_output[2] if not return_dict else text_encoder_output.prior_log_variances

        if self.config.use_stochastic_duration_prediction:
            log_duration = self.duration_predictor(
                hidden_states,
                input_padding_mask,
                speaker_embeddings,
                reverse=True,
                noise_scale=self.noise_scale_duration,
            )
        else:
            log_duration = self.duration_predictor(hidden_states, input_padding_mask, speaker_embeddings)

        length_scale = 1.0 / self.speaking_rate
        duration = torch.ceil(torch.exp(log_duration) * input_padding_mask * length_scale)
        predicted_lengths = torch.clamp_min(torch.sum(duration, [1, 2]), 1).long()

        # Create a padding mask for the output lengths of shape (batch, 1, max_output_length)
        indices = torch.arange(predicted_lengths.max(), dtype=predicted_lengths.dtype, device=predicted_lengths.device)
        output_padding_mask = indices.unsqueeze(0) < predicted_lengths.unsqueeze(1)
        output_padding_mask = output_padding_mask.unsqueeze(1).to(input_padding_mask.dtype)

        # Reconstruct an attention tensor of shape (batch, 1, out_length, in_length)
        attn_mask = torch.unsqueeze(input_padding_mask, 2) * torch.unsqueeze(output_padding_mask, -1)
        batch_size, _, output_length, input_length = attn_mask.shape
        cum_duration = torch.cumsum(duration, -1).view(batch_size * input_length, 1)
        indices = torch.arange(output_length, dtype=duration.dtype, device=duration.device)
        valid_indices = indices.unsqueeze(0) < cum_duration
        valid_indices = valid_indices.to(attn_mask.dtype).view(batch_size, input_length, output_length)
        padded_indices = valid_indices - nn.functional.pad(valid_indices, [0, 0, 1, 0, 0, 0])[:, :-1]
        attn = padded_indices.unsqueeze(1).transpose(2, 3) * attn_mask

        # Expand prior distribution
        prior_means = torch.matmul(attn.squeeze(1), prior_means).transpose(1, 2)
        prior_log_variances = torch.matmul(attn.squeeze(1), prior_log_variances).transpose(1, 2)

        prior_latents = prior_means + torch.randn_like(prior_means) * torch.exp(prior_log_variances) * self.noise_scale
        latents = self.flow(prior_latents, output_padding_mask, speaker_embeddings, reverse=True)

        spectrogram = latents * output_padding_mask
        waveform = self.decoder(spectrogram, speaker_embeddings)
        waveform = waveform.squeeze(1)
        sequence_lengths = predicted_lengths * np.prod(self.config.upsample_rates)

        if not return_dict:
            outputs = (waveform, sequence_lengths, spectrogram) + text_encoder_output[3:]
            return outputs

        return VitsModelOutput(
            waveform=waveform,
            sequence_lengths=sequence_lengths,
            spectrogram=spectrogram,
            hidden_states=text_encoder_output.hidden_states,
            attentions=text_encoder_output.attentions,
        )

    ---------------------------------------------

    # after
    def forward_encoder(
        self,
        input_ids: Optional[torch.Tensor] = None,
        attention_mask: Optional[torch.Tensor] = None,
        speaker_id: Optional[int] = None,
        output_attentions: Optional[bool] = None,
        output_hidden_states: Optional[bool] = None,
        return_dict: Optional[bool] = None,
        labels: Optional[torch.FloatTensor] = None,
    ) -> Union[Tuple[Any], VitsModelOutput]:

        output_attentions = output_attentions if output_attentions is not None else self.config.output_attentions
        output_hidden_states = (
            output_hidden_states if output_hidden_states is not None else self.config.output_hidden_states
        )
        return_dict = return_dict if return_dict is not None else self.config.use_return_dict

        if attention_mask is not None:
            input_padding_mask = attention_mask.unsqueeze(-1).float()
        else:
            input_padding_mask = torch.ones_like(input_ids).unsqueeze(-1).float()

        if self.config.num_speakers > 1 and speaker_id is not None:
            # if not 0 <= speaker_id < self.config.num_speakers:
            #     raise ValueError(f"Set `speaker_id` in the range 0-{self.config.num_speakers - 1}.")
            if isinstance(speaker_id, int):
                speaker_id = torch.full(size=(1,), fill_value=speaker_id, device=self.device)
            speaker_embeddings = self.embed_speaker(speaker_id).unsqueeze(-1)
        else:
            speaker_embeddings = None
        speaker_embeddings = None

        if labels is not None:
            raise NotImplementedError("Training of VITS is not supported yet.")

        text_encoder_output = self.text_encoder(
            input_ids=input_ids,
            padding_mask=input_padding_mask,
            attention_mask=attention_mask,
            output_attentions=output_attentions,
            output_hidden_states=output_hidden_states,
            return_dict=return_dict,
        )
        hidden_states = text_encoder_output[0] if not return_dict else text_encoder_output.last_hidden_state
        hidden_states = hidden_states.transpose(1, 2)
        input_padding_mask = input_padding_mask.transpose(1, 2)
        prior_means = text_encoder_output[1] if not return_dict else text_encoder_output.prior_means
        prior_log_variances = text_encoder_output[2] if not return_dict else text_encoder_output.prior_log_variances

        if self.config.use_stochastic_duration_prediction:
            log_duration = self.duration_predictor(
                hidden_states,
                input_padding_mask,
                speaker_embeddings,
                reverse=True,
                noise_scale=self.noise_scale_duration,
            )
        else:
            log_duration = self.duration_predictor(hidden_states, input_padding_mask, speaker_embeddings)
    
        return log_duration, input_padding_mask, prior_means, prior_log_variances

    def forward_decoder(
        self,
        attn: Optional[torch.Tensor] = None,
        output_padding_mask: Optional[torch.Tensor] = None,
        prior_means: Optional[torch.Tensor] = None,
        prior_log_variances: Optional[torch.Tensor] = None,
    ) -> Union[Tuple[Any], VitsModelOutput]:

        # Expand prior distribution
        speaker_embeddings = None
        prior_means = torch.matmul(attn.squeeze(1), prior_means).transpose(1, 2)
        prior_log_variances = torch.matmul(attn.squeeze(1), prior_log_variances).transpose(1, 2)

        prior_latents = prior_means + torch.randn(prior_means.shape) * torch.exp(prior_log_variances) * self.noise_scale
        latents = self.flow(prior_latents, output_padding_mask, speaker_embeddings, reverse=True)

        spectrogram = latents * output_padding_mask
        
        waveform = self.decoder(spectrogram, speaker_embeddings)
        waveform = waveform.squeeze(1)
        return waveform

    def forward(self, *args, **kwargs) -> Union[Tuple[Any], VitsModelOutput]:
        if len(args) == 2:
            return self.forward_encoder(*args, **kwargs)
        else:
            return self.forward_decoder(*args, **kwargs)
    ```
- To handle the randomness of the results, the input of random values ​​is fixed, so `randn_like_latents.npy` is generated in `export_onnx.py` before exporting the model.
    ```py
    # before
    latents = (
                torch.randn(inputs.size(0), 2, inputs.size(2)).to(device=inputs.device, dtype=inputs.dtype)
                * noise_scale
                )

    ---------------------------------------------

    # after
    randn_like_latents = np.load(randn_like_latents_path)
    randn_like_latents = torch.from_numpy(randn_like_latents).to(device=inputs.device, dtype=inputs.dtype)
    latents = (
                randn_like_latents
                * noise_scale
                )
    ```

- Replace `GreaterOrEqual/LessOrEqual` to avoid precision loss issues.
    ```py
    # before
    inside_interval_mask = (inputs >= -tail_bound).float().int() * (inputs <= tail_bound).float().int()

    ---------------------------------------------

    # after
    inside_interval_mask = (-inputs < tail_bound).float().int() * (-inputs > -tail_bound).float().int()
    ```
- Replace `Cumsum` to avoid failure in converting onnx model to rknn model.
    ```py
    # before
    cumwidths = torch.cumsum(widths, dim=-1)

    ---------------------------------------------

    # after
    cumwidths = torch.zeros_like(widths)
    for i in range(widths.size(-1)):
        if i == 0:
            cumwidths[..., i] = widths[..., i]
        else:
            cumwidths[..., i] = cumwidths[..., i - 1] + widths[..., i]

    ```

- Replace the operation of obtaining element values similar to `a[indices]` to avoid generating dynamic shape.
    ```py
    # before
    outputs[outside_interval_mask] = inputs[outside_interval_mask]

    ---------------------------------------------

    # after
    outputs = torch.zeros_like(inputs)
    outputs = torch.add(outputs, outside_interval_mask.float().int() * inputs)
    ```

3.Export onnx model
```sh
cd python
python export_onnx.py --max_length <MAX_LENGTH>

# such as:  
python export_onnx.py --max_length 200
```

*Description:*
- <MAX_LENGTH>: Specify the maximum length of the encoder model input. Such as `100`, `200`, `300`. Default is `200`. 


## Special Notes
1.About Python Demo
- The value of `MAX_LENGTH` in `mms_tts.py` should be modified according to the input length of the encoder model.

2.About CPP Demo
- The value of `MAX_LENGTH` in `process.h` should be modified according to the input length of the encoder model.