import numpy as np

name = [
        'model_file/decoder_fake_input/encoder_output.npy',
        'model_file/decoder_fake_input/prev_key0.npy',
        'model_file/decoder_fake_input/prev_value0.npy',
        'model_file/decoder_fake_input/prev_key1.npy',
        'model_file/decoder_fake_input/prev_value1.npy',
        'model_file/decoder_fake_input/prev_key2.npy',
        'model_file/decoder_fake_input/prev_value2.npy']


for _f in name:
    _d = np.load(_f)
    if len(_d.shape)<4:
        _d = _d.reshape(1, *_d.shape)
    _d = _d.transpose(0,2,3,1)
    np.save(_f.replace('.npy', '_nhwc.npy'), _d)