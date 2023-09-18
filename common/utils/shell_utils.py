import os
import sys
import subprocess


def run_shell_command(command, device_id=None, remote=False):
    if remote == True:
        if device_id is not None:
            running_command = ' adb -s {} shell "\n {}"'.format(device_id, "\n ".join(command))
        else:
            running_command = ' adb shell "\n {}"'.format("\n ".join(command))
    else:
        running_command = "\n ".join(command)

    p = os.popen(running_command)
    result = p.readlines()
    p.close()
    return result


def _check_file_md5(file, device_id=None, remote=False):
    cmd = "md5sum '{}'".format(file)
    result = run_shell_command([cmd], device_id, remote)
    if len(result)>0:
        result = result[0].split(' ')[0]
    return result

def _check_file_exist(file, device_id=None, remote=False):
    cmd = 'test -f {} && echo "exist"'.format(file)
    result = run_shell_command([cmd], device_id, remote)
    if len(result) > 0:
        return True

    cmd = 'test -d {} && echo "exist"'.format(file)
    result = run_shell_command([cmd], device_id, remote)
    if len(result) > 0:
        return True

    return False

def _check_file_size(file, device_id=None, remote=False):
    cmd = 'ls {} -l'.format(file)
    result = run_shell_command([cmd], device_id, remote)
    result = int(result[-1].split(' ')[4])/1000/1000
    return result

def check_file(file_path, check_type, device_id=None, remote=False):
    mapping = {
        'md5': _check_file_md5,
        'exists': _check_file_exist,
        'size': _check_file_size,
    }
    return mapping[check_type](file_path, device_id, remote)

def check_devices_available(device_id=None, remote=False):
    cmd = 'adb devices'
    result = run_shell_command([cmd], None, remote)
    return len(result)>2

if __name__ == '__main__':
    result = run_shell_command(['uname'])
    print(result)

    holdon = 1