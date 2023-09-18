import os
import sys

WEIGHT_ZOO_PATH = "/home/xz/Documents/gitlab_model_zoo/weight_zoom/models/CV/object_detection/yolo"
clear_type = ['txt', 'yml', 'yaml', 'bmp', 'jpg', 'png']
# clear_type.append('rknn')

def get_all_sub_folder(path):
    all_folder = []
    for root, dirs, files in os.walk(path):
        for dir in dirs:
            all_folder.append(os.path.join(root, dir))
    return all_folder

def main():
    all_sub_folder = get_all_sub_folder(WEIGHT_ZOO_PATH)

    for sub_folder in all_sub_folder:
        if os.path.basename(sub_folder) != 'model_cvt':
            continue

        for platform_name in os.listdir(sub_folder):
            platform_dir = os.path.join(sub_folder, platform_name)

            # if platform_name.upper() not in ['RK1808_3399PRO', 'RV1109_1126']:
            #     continue
            
            cache_files = os.listdir(platform_dir)
            for _f in cache_files:
                f_type = _f.split('.')[-1]
                if f_type in clear_type:
                    os.remove(os.path.join(platform_dir, _f))
                    print("Remove {}".format(os.path.join(platform_dir, _f)))
    
    print("CLEAR!")


if __name__ == '__main__':
    main()
