import cv2
import numpy as np
import os
import sys

temp_dir = './_tmp_for_caffe'
docker_dir = '/_tmp_for_caffe'

_debug = True
def my_os_system(cmd):
    if _debug:
        return os.system(cmd)
    else:
        return os.system(cmd + ' > /dev/null 2>&1')

class Caffe_model_container:
    def __init__(self, prototxt, caffemodel, output_nodes, mean_values, std_values) -> None:
        # self.net = cv2.dnn.readNetFromCaffe(prototxt, caffemodel)
        self.output_nodes = output_nodes
        self.mean_values = mean_values
        self.std_values = std_values
        self.prototxt = prototxt
        self.caffemodel = caffemodel

    def run(self, input_datas):
        #! now only support single input, input also should be image
        #! std values should have same values for each
        
        # add path
        # run caffe on docker
        realpath = os.path.abspath(__file__)
        _sep = os.path.sep
        realpath = realpath.split(_sep)
        _executor_file = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]) + \
                        '/utils/run_and_gen_result.py'

        if not os.path.exists(temp_dir):
            os.mkdir(temp_dir)
        my_os_system("cp {} {}/".format(_executor_file, temp_dir))
        my_os_system("cp {} {}/".format(self.prototxt, temp_dir))
        my_os_system("cp {} {}/".format(self.caffemodel, temp_dir))
        # save npy
        # TODO support multi in
        # np.save(os.path.join(temp_dir, 'in0.npy'), input_datas[0])
        input_datas[0] = (input_datas[0].astype(np.float) - np.array(self.mean_values))/self.std_values
        np.save(os.path.join(temp_dir, 'in0.npy'), input_datas[0].reshape(1, *input_datas[0].shape).transpose(0,3,1,2))

        my_os_system("docker run -v {host_path}:{docker_path} -w /{docker_path} caffe:cpu python run_and_gen_result.py {proto} {weight} {inputs}".format(
            host_path=os.path.abspath(temp_dir),
            docker_path=docker_dir,
            proto=self.prototxt,
            weight=self.caffemodel,
            inputs='in0.npy'
        ))

        outputs = []
        for i in range(len(self.output_nodes)):
            my_os_system("chmod 777 {}/caffe_out_{}.npy".format(temp_dir, i))
            outputs.append(np.load("{}/caffe_out_{}.npy".format(temp_dir, i)))

        '''
        docker run -v /home/xz/Documents/git_rk/rknn-toolkit/examples/others/redmine/350054/deconv/Deconv_exp/caffe:/tmp -w /tmp caffe:cpu python run_and_gen_result.py 
        '''

        '''
        # opencv 
        blob = cv2.dnn.blobFromImage(input_datas[0], 
                                     1.0/self.std_values[0], 
                                     (input_datas[0].shape[1], input_datas[0].shape[0]), 
                                     tuple(self.mean_values))
        self.net.setInput(blob)
        outputs = self.net.forward(self.output_nodes)
        '''

        return outputs

    def gen_blobs(self):
        realpath = os.path.abspath(__file__)
        _sep = os.path.sep
        realpath = realpath.split(_sep)
        # _executor_file = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]) + \
        #                 '/utils/gen_caffe_blobs.py'

        _executor_file = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('nn_tools')+1]) + \
                        "/caffe_tools/gen_blobs/gen_caffe_blobs.py"

        if not os.path.exists(temp_dir):
            os.mkdir(temp_dir)
        my_os_system("cp {} {}/".format(_executor_file, temp_dir))
        my_os_system("cp {} {}/".format(self.prototxt, temp_dir))
        # save npy
        # TODO support multi in

        my_os_system("docker run -v {host_path}:{docker_path} -w {docker_path} caffe:torch python gen_caffe_blobs.py {proto}".format(
            host_path=os.path.abspath(temp_dir),
            docker_path=docker_dir,
            proto=self.prototxt,
        ))

        blobs_name = os.path.basename(self.prototxt).split('.')
        blobs_name = '.'.join(blobs_name[:-1]) + '.caffemodel'
        my_os_system("chmod 777 {}/{}".format(temp_dir, os.path.basename(self.prototxt)))



if __name__ == '__main__':
    # direct use for gen blobs
    if len(sys.argv) < 1:
        print("Direct use for gen blobs")
        print("Usage: python caffe_excute.py {proto_path}")
    
    cmc = Caffe_model_container(sys.argv[1], [], [], [], [])
    cmc.gen_blobs()