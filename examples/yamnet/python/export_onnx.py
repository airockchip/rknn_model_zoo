import tensorflow as tf
import tensorflow_hub as hub
import onnx
from onnxsim import simplify
import numpy
from onnx import numpy_helper
from onnx import helper
import os
import argparse
import warnings
warnings.filterwarnings("ignore", category=UserWarning)

def export_saved_model(saved_model_path):
    model = hub.load('https://www.kaggle.com/models/google/yamnet/TensorFlow2/yamnet/1')

    @tf.function(input_signature=[tf.TensorSpec(shape=[None], dtype=tf.float32)])
    def yamnet_inference(waveform):
        scores, embeddings, log_mel_spectrogram = model(waveform)
        return {'scores': scores, 'embeddings': embeddings, 'log_mel_spectrogram': log_mel_spectrogram}

    concrete_func = yamnet_inference.get_concrete_function(tf.TensorSpec(shape=[None], dtype=tf.float32))
    tf.saved_model.save(model, saved_model_path, signatures=concrete_func)

def modify_input_shape(onnx_model_path, max_length):
    model = onnx.load(onnx_model_path)
    original_input = model.graph.input[0]

    new_input_name = "new_input"
    new_input_shape = [1, max_length]
    new_input_dtype = onnx.TensorProto.FLOAT
    new_input = helper.make_tensor_value_info(new_input_name, new_input_dtype, new_input_shape)

    output_shape_tensor_name = "reshape_output_shape_tensor"
    output_shape_tensor = numpy_helper.from_array(
        numpy.array([max_length], dtype=numpy.int64),
        output_shape_tensor_name
    )

    reshape_output_name = "reshaped_input"
    reshape_node = helper.make_node(
        'Reshape',
        inputs=[new_input_name, output_shape_tensor_name],
        outputs=[reshape_output_name],
    )

    model.graph.initializer.append(output_shape_tensor)
    model.graph.input.remove(original_input)
    model.graph.input.insert(0, new_input)
    model.graph.node.insert(0, reshape_node)

    connect_node = helper.make_node(
        'Identity',
        inputs=[reshape_output_name],
        outputs=[original_input.name]
    )

    model.graph.node.insert(1, connect_node)
    onnx.save(model, onnx_model_path)

def export_onnx_model(saved_model_path, onnx_model_path, chunk_length):
    max_length = chunk_length * 16000
    os.system(f'python -m tf2onnx.convert --saved-model {saved_model_path} \
                --output {onnx_model_path} --opset 12 --verbose --inputs waveform:0[{max_length}]')
    modify_input_shape(onnx_model_path, max_length)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Export yamnet onnx model', add_help=True)
    parser.add_argument("--chunk_length", type=int, required=True, default= 3,
                        help="audio length, default is 3 seconds")
    args = parser.parse_args()

    saved_model_path = '../model/saved_model'
    export_saved_model(saved_model_path)

    onnx_model_path = '../model/yamnet_{}s.onnx'.format(args.chunk_length)
    export_onnx_model(saved_model_path, onnx_model_path, args.chunk_length)

    # simplify
    original_model = onnx.load(onnx_model_path)
    simplified_model, check = simplify(original_model)
    onnx.save(simplified_model, onnx_model_path)
    print("\nThe model is saved in:", onnx_model_path)