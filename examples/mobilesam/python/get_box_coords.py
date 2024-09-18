import cv2
import argparse

start_point = None
end_point = None
drawing = False

def mouse_callback(event, x, y, flags, param):
    global start_point, end_point, drawing

    if event == cv2.EVENT_LBUTTONDOWN:
        start_point = (x, y)
        drawing = True

    elif event == cv2.EVENT_MOUSEMOVE:
        if drawing:
            end_point = (x, y)
            temp_image = image.copy()
            cv2.rectangle(temp_image, start_point, end_point, (0, 255, 0), 2)
            cv2.imshow('get box coords', temp_image)

    elif event == cv2.EVENT_LBUTTONUP:
        end_point = (x, y)
        drawing = False
        cv2.rectangle(image, start_point, end_point, (0, 255, 0), 2)
        cv2.imshow('get box coords', image)
        print(f'Box coordinates: Top-left: {start_point}, Bottom-right: {end_point}')
        cv2.destroyAllWindows()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='mobilesam get box coords')
    parser.add_argument('--img',
                        type=str,
                        help="input image",
                        default='../model/picture.jpg')
    args = parser.parse_args()

    image = cv2.imread(args.img)
    cv2.namedWindow('get box coords')
    cv2.setMouseCallback('get box coords', mouse_callback)
    cv2.imshow('get box coords', image)

    cv2.waitKey(0)
