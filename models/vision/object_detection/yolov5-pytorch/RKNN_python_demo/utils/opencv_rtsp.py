from cgi import test
import cv2 
import time


def read_rtsp(rtsp_url):
    try:
        # (cv2.CAP_PROP_HW_ACCELERATION, 1)
        cap = cv2.VideoCapture(rtsp_url, cv2.CAP_ANY)
        if not cap.isOpened():
            print("Cannot open camera")
            exit()        
        # print(cv2.videoio_registry.getBackends())
        fps = max(cap.get(cv2.CAP_PROP_FPS) % 100, 0) or 30.0

        fourcc = cv2.VideoWriter_fourcc(*'XVID')
        output = cv2.VideoWriter("{0}".format("output.avi"), fourcc, fps, (640, 640))

        print(cap.get(cv2.CAP_PROP_FOURCC))
        print(cap.get(cv2.CAP_PROP_MODE))
        # print(cap.set(50, 1))
        # print(cap.open(rtsp_url, cv2.CAP_ANY, (50, 1)))
        # 1900:ffmpeg
        print(cap.get(cv2.CAP_PROP_BACKEND))
        # enum: 50
        print(cap.get(cv2.CAP_PROP_HW_ACCELERATION))
        print(cap.get(cv2.CAP_PROP_HW_DEVICE))
        # print(cap.get(cv2.CAP_PROP_HW_ACCELERATION_USE_OPENCL))
        c = 1
        while True:
            ret,frame = cap.read()
            if not ret:
                print("Can't receive frame (stream end?). Exiting ...")
                break
            print('frame: {0}'.format(c))
            c += 1
            frame = test_opencl(frame)
            output.write(frame)
            
            time.sleep(1/fps)
    except BaseException as e:
        if isinstance(e, KeyboardInterrupt):
            cap.release()
            # output.release()
            print("normal exit")
        else:
            cap.release()
            print(e)
            


def test_opencl(img):
    print('OpenCL available:', cv2.ocl.haveOpenCL())
    # img = cv2.UMat(img)
    # img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    # img = cv2.GaussianBlur(img, (7, 7), 1.5)
    # img = cv2.Canny(img, 0, 50)
    # img = cv2.resize(img, (640, 640))
    # if type(img) == cv2.UMat:
        # print("yes")
        # print(type(img))
        # img = cv2.UMat.get(img)
        # print(type(img) == cv2.UMat)
        # print(type(img))
    return img



if __name__ == "__main__":
    # rtsp_url = "rtsp://admin:admin123@192.168.172.105:554/cam/realmonitor?channel=1&subtype=0"
    # ! videoconvert ! video/x-raw,format=(string)BGR
    # rtsp_url = "rtspsrc location=rtsp://admin:admin123@192.168.172.105:554/cam/realmonitor?channel=1&subtype=0 latency=0 ! rtph264depay ! h264parse ! mppvideodec ! rgaconvert ! video/x-raw,format=BGR,width=640,height=640 ! appsink sync=false"
    # rtsp_url = "rtspsrc location=rtsp://admin:admin123@192.168.172.105:554/cam/realmonitor?channel=1&subtype=0 latency=0 ! rtph264depay ! h264parse ! mppvideodec ! videoconvert ! video/x-raw,format=BGR ! appsink sync=false"
    rtsp_url = "filesrc location=huoyan1.mp4 ! qtdemux ! h264parse ! mppvideodec ! rgaconvert ! video/x-raw,format=BGR,width=640,height=640 ! appsink sync=false"
    read_rtsp(rtsp_url)