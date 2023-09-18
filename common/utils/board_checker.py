import os
import sys

# add path
realpath = os.path.abspath(__file__)
_sep = os.path.sep
realpath = realpath.split(_sep)
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:-1]))
sys.path.append(os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')+1]))

from utils.shell_utils import run_shell_command, check_file, check_devices_available

# devices_type
NPU_VERSION_MAP = {
    1:  ['RK3399PRO', 'RK1808', 'RV1109', 'RV1126'],
    2:  ['RK3566', 'RK3568', 'RK3588', 'RV1106', 'RV1103', 'RK3562'],
}

SK_FILE = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')]) + '/capi_tools/scaling_frequency.sh'

RV1106_LIB_PATH = os.path.join(realpath[0]+_sep, *realpath[1:realpath.index('common')], 'capi_tools/toolkit2/rknn_capi_test/install/rv110x/Linux/rknn_capi_test/lib/librknnmrt.so')

class Board_checker:
    def __init__(self, platform, device_id=None) -> None:
        platform = platform.upper()
        self.platform = platform
        for _v, _p in NPU_VERSION_MAP.items(): 
            if platform in _p:
                self.npu_version = _v

        self.device_id = device_id
        self.get_device_system_type()
        if check_devices_available() and self.system_type == 'android':
            run_shell_command(['adb root && adb remount'], remote=False)

    def _d_run_shell_command(self, cmd):
        return run_shell_command(cmd, self.device_id, remote=True)


    def _get_device_system_type_for_rk3399pro_hack(self):
        result = self._d_run_shell_command(['ls vendor/lib64'])
        if len(result) == 0 or "No such file" in result[0]:
            return 'linux'
        else:
            return 'android'

    def get_device_system_type(self):
        if self.platform.upper() in ['RK3399PRO', 'RK3566', 'RK3568', 'RK3588', 'RK3562']:
            self.system_type = self._get_device_system_type_for_rk3399pro_hack()
            return self.system_type

        result = self._d_run_shell_command(['cat /proc/version'])
        if len(result) == 0:
            print("WARNING: no adb devices. All the capi test may failed")
            return
        if "android" in result[0] or \
            "Android" in result[0] or \
            "ANDROID" in result[0]:
            self.system_type = 'android'
        else:
            self.system_type = 'linux'
        return self.system_type

    def get_librknn_version(self):
        if self.npu_version == 1:
            if self.platform.upper() in ['RK3399PRO','RK3399']:
                result = 'not support currently, please check with rknn.init_runtime api'
            else: 
                result = self._d_run_shell_command(["strings /usr/lib/librknn_runtime.so | grep 'librknn_runtime version'"])
                _result = result[0].rstrip('\n').split('version ')[1].replace(':', '-')
                result = _result if len(_result)>0 else result

        elif self.npu_version == 2:
            if self.platform.upper() in ['RV1106', 'RV1103']:
                result = run_shell_command(["strings {} | grep 'librknnmrt version'".format(RV1106_LIB_PATH)])
                return result[0].rstrip('\n').split('version: ')[1]
            if self.system_type == 'linux':
                result = self._d_run_shell_command(["strings /usr/lib/librknnrt.so | grep 'librknnrt version'"])
            else:
                result = self._d_run_shell_command(["strings /vendor/lib/librknnrt.so | grep 'librknnrt version'"])
            try:
                _result = result[0].rstrip('\n').split('version: ')[1]
                result = _result if len(_result)>0 else result
            except:
                result = "query failed"
        return result

    def get_device_id(self):
        cmd = ['adb devices']
        for _ in range(3):
            result = run_shell_command(cmd)
        device_id_list = []
        for i in range(len(result)-1):
            _l = result[i+1]
            if _l == '\n':
                continue
            else:
                device_id_list.append(_l.split('\t')[0])
        return device_id_list

    def scaling_freq(self):
        remote_sk = {'linux': '/userdata/scaling_frequency.sh', 'android': '/data/scaling_frequency.sh'}[self.system_type]
        if not check_file(remote_sk, 'exists', self.device_id, remote=True):
            os.system('adb push {} {}'.format(SK_FILE, remote_sk))
        
        result = self._d_run_shell_command([
            'chmod 777 {}'.format(remote_sk),
            '{} -c {}'.format(remote_sk, self.platform.lower())
            ])

        info = {}
        device_key = None
        for _l in result:
            if _l[:4] in ['CPU:', 'NPU:', 'DDR:']:
                device_key = _l[:3]
            if device_key is not None:
                if len(_l.split('try set'))>1:
                    info[device_key + '_freq.try_set'] = int(_l.split('try set')[1])
                
                elif len(_l.split('query'))>1:
                    info[device_key + '_freq.query'] = int(_l.split('query')[1])

                elif _l.startswith('Firmware seems not support'):
                    info[device_key + '_freq.try_set'] = 'not support setting'

                # elif len(_l.split('check'))>1:
                #     holdon = 1

        return info


if __name__ == '__main__':
    bc = Board_checker('rv1126')
    print(bc.get_librknn_version())
    print(bc.scaling_freq())
    holdon = 1

