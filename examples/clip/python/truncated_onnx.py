import onnx
import argparse


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Export clip onnx model', add_help=True)
    parser.add_argument('--model', type=str, required=True,
                        help='onnx model path')
    args = parser.parse_args()

    output_path = '../model/clip_text.onnx'
    input_names = ['input_ids', 'attention_mask']
    output_names = ['text_embeds']
    onnx.utils.extract_model(args.model, output_path, input_names, output_names)

    output_path = '../model/clip_images.onnx'
    input_names = ['pixel_values']
    output_names = ['image_embeds']
    onnx.utils.extract_model(args.model, output_path, input_names, output_names)