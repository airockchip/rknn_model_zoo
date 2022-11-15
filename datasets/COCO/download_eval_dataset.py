import os
import urllib.request
import time
import sys
import traceback
import zipfile


def un_zip(file_name):
    """unzip zip file"""
    zip_file = zipfile.ZipFile(file_name)
    # dir_name = file_name.rstrip('.zip')
    # if os.path.isdir(dir_name):
    #     pass
    # else:
    #     os.mkdir(dir_name)
    for names in zip_file.namelist():
        zip_file.extract(names)
    zip_file.close()


def readable_speed(speed):
    speed_bytes = float(speed)
    speed_kbytes = speed_bytes / 1024
    if speed_kbytes > 1024:
        speed_mbytes = speed_kbytes / 1024
        if speed_mbytes > 1024:
            speed_gbytes = speed_mbytes / 1024
            return "{:.2f} GB/s".format(speed_gbytes)
        else:
            return "{:.2f} MB/s".format(speed_mbytes)
    else:
        return "{:.2f} KB/s".format(speed_kbytes)


def show_progress(blocknum, blocksize, totalsize):
    speed = (blocknum * blocksize) / (time.time() - start_time)
    speed_str = " Speed: {}".format(readable_speed(speed))
    recv_size = blocknum * blocksize

    f = sys.stdout
    progress = (recv_size / totalsize)
    progress_str = "{:.2f}%".format(progress * 100)
    n = round(progress * 50)
    s = ('#' * n).ljust(50, '-')
    f.write(progress_str.ljust(8, ' ') + '[' + s + ']' + speed_str)
    f.flush()
    f.write('\r\n')


def download(file_name, url_path):
    if not os.path.exists(file_name):
        print('--> Download {}'.format(file_name))
        download_file = file_name
        try:
            urllib.request.urlretrieve(url_path, download_file, show_progress)
        except:
            print('Download {} failed.'.format(download_file))
            print(traceback.format_exc())
            exit(-1)
        print('done')

if __name__ == '__main__':
    eval_img_dataset_url = 'http://images.cocodataset.org/zips/val2017.zip'
    dataset_name = 'val2017.zip'
    start_time = time.time()
    download(dataset_name, eval_img_dataset_url)
    un_zip(dataset_name)

    annotation_url = 'http://images.cocodataset.org/annotations/annotations_trainval2017.zip'
    annotation_name = 'annotations_trainval2017.zip'
    start_time = time.time()
    download(annotation_name, annotation_url)
    un_zip(annotation_name)

    # gen path txt for Capi benchmark test
    src_path = 'val2017'
    store_path_on_board = '/userdata/val2017'
    with open('./coco_dataset_path.txt', 'w') as f:
        for file in os.listdir(src_path):
            f.write(os.path.join(store_path_on_board, file)+'\n')
