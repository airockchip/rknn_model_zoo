
# devices
NPU_V1_0 = ['RK3399PRO', 'RK1808']
NPU_V1_1 = ['RV1109', 'RV1126']
NPU_VERSION_1_DEVICES = NPU_V1_0 + NPU_V1_1

NPU_V2_0 = ['RK3566', 'RK3568']
NPU_V2_1 = ['RK3588']
NPU_V2_2 = ['RV1106', 'RV1103']
NPU_V2_3 = ['RK3562']
NPU_VERSION_2_DEVICES = NPU_V2_0 + NPU_V2_1 + NPU_V2_2 + NPU_V2_3

RKNN_DEVICES_ALL = NPU_VERSION_1_DEVICES + NPU_VERSION_2_DEVICES

PLATFROM_COMPATIBAL_TRANSFORMER_MAP = {
        'RK1808': 'RK1808_3399pro',
        'RK3399PRO': 'RK1808_3399pro',
        'RV1126': 'RV1109_1126',
        'RV1109': 'RV1109_1126',
        'RV1103': 'RV1103_1106',
        'RV1106': 'RV1103_1106',
        'RK3566': 'RK3566_3568',
        'RK3568': 'RK3566_3568',
        'RK3588': 'RK3588',
        'RK3562': 'RK3562',
}
PLATFROM_COMPATIBAL_TRANSFORMER_MAP_REVERSE = {
        'RK1808_3399pro': 'RK1808',
        'RV1109_1126': 'RV1126',
        'RV1103_1106': 'RV1106',
        'RK3566_3568': 'RK3566',
        'RK3588': 'RK3588',
        'RK3562': 'RK3562',
}

def MACRO_toolkit_version(device):
    if device.upper() in NPU_VERSION_1_DEVICES:
        return 1
    elif device.upper() in NPU_VERSION_2_DEVICES:
        return 2
    else:
        assert False, "{} devices is not support. Now support devices list - {}".format(device, NPU_VERSION_2_DEVICES+ NPU_VERSION_1_DEVICES)


# quantize
QUANTIZE_DTYPE_SIM = ['u8', 'i8', 'i16', 'fp']
QUANTIZE_DTYPE_TOOLKIT = ['asymmetric_affine-u8',
                          'dynamic_fixed_point-i8', 
                          'dynamic_fixed_point-i16',
                          'asymmetric_quantized-8',]

def MACRO_qtype_sim_2_toolkit(qtype_sim, device):
    if device in NPU_VERSION_1_DEVICES:
        if qtype_sim == 'u8':
            return 'asymmetric_affine-u8'
        elif qtype_sim == 'i8':
            return 'dynamic_fixed_point-i8'
        elif qtype_sim == 'i16':
            return 'dynamic_fixed_point-i16'
        elif qtype_sim == 'fp':
            return None
        else:
            assert False, "quantize type: {} not support".format(qtype_sim)
    elif device in NPU_VERSION_2_DEVICES:
        if qtype_sim == 'u8':
            return 'asymmetric_quantized-8'
        elif qtype_sim == 'i8':
            return 'asymmetric_quantized-8'
        else:
            assert False, "quantize type: {} not support".format(qtype_sim)

def MACRO_qtype_toolkit_2_sim(qtype_toolkit, device=None):
    if qtype_toolkit == 'asymmetric_affine-u8':
        return 'u8'
    elif qtype_toolkit == 'dynamic_fixed_point-i8':
        return 'i8'
    elif qtype_toolkit == 'dynamic_fixed_point-i16':
        return 'i16'
    elif qtype_toolkit == 'asymmetric_quantized-8':
        return 'i8'
    else:
        assert False, "quantize type: {} not support".format(qtype_toolkit)