import cv2
import argparse

points = []

def mouse_callback(event, x, y, flags, param):
    global points

    if event == cv2.EVENT_LBUTTONDOWN:
        points.append((x, y))
        temp_image = image.copy()
        cv2.circle(temp_image, (x, y), 5, (0, 0, 255), -1)
        cv2.imshow('get point coords', temp_image)
        print(f'Point coordinates: ({x}, {y})')
        cv2.destroyAllWindows()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='mobilesam get point coords')
    parser.add_argument('--img',
                        type=str,
                        help="input image",
                        default='../model/picture.jpg')
    args = parser.parse_args()

    image = cv2.imread(args.img)
    cv2.namedWindow('get point coords')
    cv2.setMouseCallback('get point coords', mouse_callback)
    cv2.imshow('get point coords', image)

    cv2.waitKey(0)

