import cv2 


def read_rtsp(rtsp_url):
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    output = cv2.VideoWriter("{0}".format("output.avi"), fourcc, 12.5, (1920, 1080))


    cap = cv2.VideoCapture(rtsp_url)
    if not cap.isOpened():
        print("Cannot open camera")
        exit()        

    c = 1
    while True:

        try:
            ret,frame = cap.read()
            if not ret:
                print("Can't receive frame (stream end?). Exiting ...")
                break
            print('frame: {0}'.format(c))
            c += 1
        
            output.write(frame)
        except BaseException as e:
            if isinstance(e, KeyboardInterrupt):
                cap.release()
                output.release()
                print("normal exit")
                break


if __name__ == "__main__":
    rtsp_url = "rtsp://192.168.17.12:554/11"
    read_rtsp(rtsp_url)