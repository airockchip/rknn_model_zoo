import sys
import os
import re
import numpy as np

realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)

binary_dir = '/{}/capi_tools/toolkit1/rknn_capi_test/install'.format(_sep.join(realpath[: realpath.index('rknn_model_zoo')+1]))
scaling_file = '/{}/utils/scaling_frequency.sh'.format(_sep.join(realpath[: realpath.index('common')+1]))


require_map = {
    'RV1126': os.path.join(binary_dir, 'RV1109_1126/rknn_capi_test'),
    'RV1109': os.path.join(binary_dir, 'RV1109_1126/rknn_capi_test'),
    'RK1808': os.path.join(binary_dir, 'RK1808/rknn_capi_test'),
}

api_dict = {
    'normal': 'rknn_capi_test',
    'zero_copy': 'rknn_capi_test_zero_copy',
}

def get_device_system_type(device_id=None):
    if device_id is None:
        cmd = "adb shell cat /proc/version"
    else:
        cmd = "adb -s {} shell cat /proc/version".format(device_id)
    
    query = os.popen(cmd)
    result = query.readlines()

    if "android" in result[0] or \
        "Android" in result[0] or \
        "ANDROID" in result[0]:
        system_type = 'android'
    else:
        system_type = 'linux'
    # print('SYSTEM is {}'.format(system_type))
    return system_type


_debug = True
def my_os_system(cmd):
    if _debug:
        return os.system(cmd)
    else:
        return os.system(cmd + ' > /dev/null 2>&1')


class tk1_capi_excuter(object):
    # --------------------------------------------------------------------------------------------------
    # Capi type: normal
    #       feature:
    #           1.multi-input available
    #           2.float/uint8 allow
    #           3.output get as float
    #       return:
    #           1.npy result
    #           2.(rknn)input_set cost time
    #           3.(rknn)run cost time
    #           4.(rknn)output_get cost time
    #
    #
    # Capi type: zero_copy
    #       feature:
    #           1.only support single input
    #           2.only allow uint8 input
    #           3.output get as uint8, but using cpu dequantize output to float32
    #       return:
    #           1.npy result
    #           2.(rknn)input sync cost time
    #           3.(rknn)run cost time
    #           4.(rknn)output_sync cost time
    #           5.cpu dequantize time
    #
    # --------------------------------------------------------------------------------------------------
    def __init__(self, model_path, model_config_dict):
        self.model_path = model_path
        self.model_config_dict = model_config_dict

        self.remote_tmp_path = '/userdata/capi_test/'
        self.platform = model_config_dict.get('RK_device_platform', 'RK1808').upper()
        if self.platform not in ['RV1126', 'RV1109', 'RK1808']:
            raise Exception('Unsupported platform: {}'.format(self.platform))
        self.device_system = get_device_system_type(model_config_dict['RK_device_id'])
        print("\b System: {}".format(self.device_system))
        if self.device_system == 'android':
            self.remote_tmp_path = '/data/capi_test/'
        #     my_os_system("adb root & adb remount")
        else:
            self.remote_tmp_path = '/userdata/capi_test/'

        self.test_model_name = 'test.rknn'

        self.input_name = ['{}.npy'.format(key) for key in model_config_dict['inputs'].keys()]
        self.input_line = '#'.join(self.input_name)

        self.capi_record_file_name = 'capi_record.txt'

        self.remote_output_name = None
        self.output_name = None

        self._check_file()
        self._push_model()

        self.scaling_freq()


    def scaling_freq(self):
        my_os_system("adb push {} {}".format(scaling_file, self.remote_tmp_path))
        if self.device_system == 'android':
            my_os_system("adb shell {}/scaling_frequency.sh -c {}".format(self.remote_tmp_path, self.platform.lower()))
        else:
            my_os_system("adb shell bash {}/scaling_frequency.sh -c {}".format(self.remote_tmp_path, self.platform.lower()))


    def _get_output_name(self, number):
        self.remote_output_name = []
        self.output_name = []
        for i in range(number):
            self.remote_output_name.append(os.path.join(self.remote_tmp_path, 'output_{}.npy'.format(i)) )
            self.output_name.append('output_{}.npy'.format(i))


    def _push_input(self, inputs):
        print('---> push input')
        for i in range(len(inputs)):
            tmp_path = os.path.join(self.result_store_dir, self.input_name[i])
            np.save(tmp_path, inputs[i])
            my_os_system("adb push {} {}".format(tmp_path, os.path.join(self.remote_tmp_path, self.input_name[i])))
        

    def _check_file(self):
        # file_state = my_os_system("adb shell '(ls {} && echo exists) || (echo miss)'".format(self.remote_tmp_path))
        # if file_state == 'miss':
        self._push_require()


    def _push_require(self):
        # TODO smart push
        # import subprocess
        # result = subprocess.getoutput("adb shell md5sum {xxx}")
        print('---> push require')
        my_os_system("adb push {}/* {}".format(require_map[self.platform], self.remote_tmp_path))


    def _push_model(self):
        print('---> push model')
        my_os_system("adb push {} {}".format(self.model_path, self.remote_tmp_path))
        self.test_model_name = os.path.basename(self.model_path)

    def _run_commond(self, loop, api_type):
        if api_type in api_dict:
            # TODO record ddr
            command_in_shell = [
                'cd {}'.format(self.remote_tmp_path),
                'chmod 777 {}'.format(api_dict[api_type]),
                './{} {} {} {}'.format(api_dict[api_type], self.test_model_name, self.input_line, loop),
            ]

            running_command = ' adb shell "\n {}"'.format("\n ".join(command_in_shell))                
            my_os_system(running_command)
        else:
            raise Exception('Unsupported api_type: {}'.format(api_type))

    def _init_time_dict(self, api_type='normal'):
        if api_type == 'normal':
            self.time_dict = {
                'input_set': 0,
                'run': 0,
                'output_get': 0,
            }
        elif api_type == 'zero_copy':
            self.time_dict = {
                'input_sync': 0,
                'run': 0,
                'output_sync': 0,
                'cpu_dequantize': 0,
            }

    def _pull_and_parse(self, api_type):
        my_os_system("adb pull {} {}".format(os.path.join(self.remote_tmp_path, self.capi_record_file_name), self.result_store_dir))

        self._init_time_dict(api_type)
        with open(os.path.join(self.result_store_dir, self.capi_record_file_name), 'r') as f:
            lines = f.readlines()

        assert len(lines)>0, "{} is blank, run failed".format(self.capi_record_file_name)
        pattern = re.compile(r'\d+')
        output_number = int(pattern.findall(lines[1])[0])
        if api_type == 'normal':
            time_line = {'input_set': 4, 'run': 5, 'output_get': 6}
        elif api_type == 'zero_copy':
            time_line = {'run': 4, 'cvt_type_time': 5, 'total': 6}

        for _key, _value in time_line.items():
            self.time_dict[_key] = float('.'.join(pattern.findall(lines[_value])))

        self._get_output_name(output_number)
        for i in range(len(self.output_name)):
            my_os_system("adb pull {} {}".format(os.path.join(self.remote_tmp_path, self.output_name[i]), self.result_store_dir))
        capi_result = []
        for i in range(len(self.output_name)):
            capi_result.append(np.load(os.path.join(self.result_store_dir, self.output_name[i])))
        return capi_result, self.time_dict


    def _clear(self):
        print('---> clear and create result store dir')
        self.result_store_dir = './tmp'
        if os.path.exists(self.result_store_dir):
            my_os_system("rm -r {}".format(self.result_store_dir))
        os.makedirs(self.result_store_dir)


    def excute(self, inputs, loop, api_type):
        self._clear()
        self._push_input(inputs)
        self._run_commond(loop, api_type)
        capi_result, time_set = self._pull_and_parse(api_type)

        return capi_result, time_set


if __name__ == '__main__':
    etk = tk1_capi_excuter('./', {})
    etk._run_commond(1, 'normal')