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
    return result[0].split(' ')[0]


def _check_file_exist(file, device_id=None, remote=False):
    cmd = 'test -f {} && echo "exist"'.format(file)
    result = run_shell_command([cmd], device_id, remote)
    if len(result) == 0:
        return False
    else:
        return True

def _check_file_size(file, device_id=None, remote=False):
    cmd = 'ls {} -l'.format(file)
    result = run_shell_command([cmd], device_id, remote)
    result = int(result[-1].split(' ')[4])/1000/1000
    return result

def check_file(file_path, check_type, device_id=None, remote=False):
    mapping = {
        'md5': _check_file_md5,
        'exits': _check_file_exist,
        'size': _check_file_size,
    }
    return mapping[check_type](file_path, device_id, remote)


if __name__ == '__main__':
    result = run_shell_command(['uname'])
    print(result)

    holdon = 1