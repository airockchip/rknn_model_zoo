import tensorflow as tf
from tensorflow.python.framework import graph_util
from tensorflow.python.platform import gfile

class Tensorflow_model_container:
    def __init__(self, model_path, inputs, outputs) -> None:
        self.input_names = []
        for i, item in enumerate(inputs):
            self.input_names.append('import/' + item + ':0')

        self.output_names = []
        for item in outputs:
            self.output_names.append('import/' + item + ':0')

        self.sess = tf.compat.v1.Session()
        with gfile.FastGFile(model_path, 'rb') as f:
            graph_def = tf.compat.v1.GraphDef()
            graph_def.ParseFromString(f.read())
            self.sess.graph.as_default()
            tf.import_graph_def(graph_def)

            # tensor_name_list = [tensor.name for tensor in tf.compat.v1.get_default_graph().as_graph_def().node]
            # print(tensor_name_list)
            self.tf_inputs = list()
            for _name in self.input_names:
                in_tensor = self.sess.graph.get_tensor_by_name(_name)
                self.tf_inputs.append(in_tensor)

            self.tf_outputs = list()
            for _name in self.output_names:
                out_tensor = self.sess.graph.get_tensor_by_name(_name)
                self.tf_outputs.append(out_tensor)

    def run(self, input_datas):

        feed_dict = {}
        for i in range(len(self.tf_inputs)):
            feed_dict[self.tf_inputs[i]] = input_datas[i]

        out_res = self.sess.run(self.tf_outputs, feed_dict=feed_dict)

        return out_res