#include "opencv2/opencv.hpp"
#include <vector>

#include "ppocr_det.h"
#include "clipper.h"

using namespace std;

bool XsortFp32(std::vector<float> a, std::vector<float> b) {
    if (a[0] != b[0]) return a[0] < b[0];
    return false;
}

bool XsortInt(std::vector<int> a, std::vector<int> b) {
    if (a[0] != b[0]) return a[0] < b[0];
    return false;
}

std::vector<std::vector<float>> Mat2Vector(cv::Mat mat) {
    std::vector<std::vector<float>> img_vec;
    std::vector<float> tmp;

    for (int i = 0; i < mat.rows; ++i) {
        tmp.clear();
        for (int j = 0; j < mat.cols; ++j) {
            tmp.push_back(mat.at<float>(i, j));
        }
        img_vec.push_back(tmp);
    }
    return img_vec;
}

std::vector<std::vector<float>> GetMiniBoxes(cv::RotatedRect box, float &ssid) {
    ssid = std::max(box.size.width, box.size.height);

    cv::Mat points;
    cv::boxPoints(box, points);

    auto array = Mat2Vector(points);
    std::sort(array.begin(), array.end(), XsortFp32);

    std::vector<float> idx1 = array[0], idx2 = array[1], idx3 = array[2], idx4 = array[3];
    if (array[3][1] <= array[2][1]) {
        idx2 = array[3];
        idx3 = array[2];
    } else {
        idx2 = array[2];
        idx3 = array[3];
    }
    if (array[1][1] <= array[0][1]) {
        idx1 = array[1];
        idx4 = array[0];
    } else {
        idx1 = array[0];
        idx4 = array[1];
    }

    array[0] = idx1;
    array[1] = idx2;
    array[2] = idx3;
    array[3] = idx4;

    return array;
}

int clamp(int x, int min, int max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

float clampf(float x, float min, float max) {
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

float PolygonScoreAcc(std::vector<cv::Point> contour, cv::Mat pred) {
    int width = pred.cols;
    int height = pred.rows;
    std::vector<float> box_x;
    std::vector<float> box_y;
    for (int i = 0; i < contour.size(); ++i) {
        box_x.push_back(contour[i].x);
        box_y.push_back(contour[i].y);
    }

    int xmin = clamp(int(std::floor(*(std::min_element(box_x.begin(), box_x.end())))), 0, width - 1);
    int xmax = clamp(int(std::ceil(*(std::max_element(box_x.begin(), box_x.end())))), 0, width - 1);
    int ymin = clamp(int(std::floor(*(std::min_element(box_y.begin(), box_y.end())))), 0, height - 1);
    int ymax = clamp(int(std::ceil(*(std::max_element(box_y.begin(), box_y.end())))), 0, height - 1);

    cv::Mat mask;
    mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);

    cv::Point *rook_point = new cv::Point[contour.size()];

    for (int i = 0; i < contour.size(); ++i) {
        rook_point[i] = cv::Point(int(box_x[i]) - xmin, int(box_y[i]) - ymin);
    }
    const cv::Point *ppt[1] = {rook_point};
    int npt[] = {int(contour.size())};

    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));

    cv::Mat croppedImg;
    pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1)).copyTo(croppedImg);
    float score = cv::mean(croppedImg, mask)[0];

    delete[] rook_point;
    return score;
}

float BoxScoreFast(std::vector<std::vector<float>> box_array, cv::Mat pred) {
    auto array = box_array;
    int width = pred.cols;
    int height = pred.rows;

    float box_x[4] = {array[0][0], array[1][0], array[2][0], array[3][0]};
    float box_y[4] = {array[0][1], array[1][1], array[2][1], array[3][1]};

    int xmin = clamp(int(std::floor(*(std::min_element(box_x, box_x + 4)))), 0, width - 1);
    int xmax = clamp(int(std::ceil(*(std::max_element(box_x, box_x + 4)))), 0, width - 1);
    int ymin = clamp(int(std::floor(*(std::min_element(box_y, box_y + 4)))), 0, height - 1);
    int ymax = clamp(int(std::ceil(*(std::max_element(box_y, box_y + 4)))), 0, height - 1);

    cv::Mat mask;
    mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);

    cv::Point root_point[4];
    root_point[0] = cv::Point(int(array[0][0]) - xmin, int(array[0][1]) - ymin);
    root_point[1] = cv::Point(int(array[1][0]) - xmin, int(array[1][1]) - ymin);
    root_point[2] = cv::Point(int(array[2][0]) - xmin, int(array[2][1]) - ymin);
    root_point[3] = cv::Point(int(array[3][0]) - xmin, int(array[3][1]) - ymin);
    const cv::Point *ppt[1] = {root_point};
    int npt[] = {4};
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));

    cv::Mat croppedImg;
    pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1)).copyTo(croppedImg);

    float score = cv::mean(croppedImg, mask)[0];
    return score;
}

void GetContourArea(const std::vector<std::vector<float>> &box, float unclip_ratio, float &distance) {
    int pts_num = box.size();
    float area = 0.0f;
    float dist = 0.0f;
    for (int i = 0; i < pts_num; i++) {
        area += box[i][0] * box[(i + 1) % pts_num][1] - box[i][1] * box[(i + 1) % pts_num][0];
        dist += sqrtf((box[i][0] - box[(i + 1) % pts_num][0]) * (box[i][0] - box[(i + 1) % pts_num][0]) +
                    (box[i][1] - box[(i + 1) % pts_num][1]) * (box[i][1] - box[(i + 1) % pts_num][1]));
    }
    area = fabs(float(area / 2.0));

    distance = area * unclip_ratio / dist;
}

cv::RotatedRect UnClip(std::vector<std::vector<float>>& box, const float &unclip_ratio) {
    float distance = 1.0;

    GetContourArea(box, unclip_ratio, distance);

    ClipperLib::ClipperOffset offset;
    ClipperLib::Path p;
    int pts_num = box.size();
    for (int i = 0; i < pts_num; i++) {
        p << ClipperLib::IntPoint(int(box[i][0]), int(box[i][1]));
    }
    offset.AddPath(p, ClipperLib::jtRound, ClipperLib::etClosedPolygon);

    ClipperLib::Paths soln;
    offset.Execute(soln, distance);
    std::vector<cv::Point2f> points;

    for (int j = 0; j < soln.size(); j++) {
        for (int i = 0; i < soln[soln.size() - 1].size(); i++) {
            points.emplace_back(soln[j][i].X, soln[j][i].Y);
        }
    }
    cv::RotatedRect res;
    if (points.size() <= 0) {
        res = cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    } else {
        res = cv::minAreaRect(points);
    }
    return res;
}

std::vector<std::vector<int>> OrderPointsClockwise(std::vector<std::vector<int>> pts) {
    std::vector<std::vector<int>> box = pts;
    std::sort(box.begin(), box.end(), XsortInt);

    std::vector<std::vector<int>> leftmost = {box[0], box[1]};
    std::vector<std::vector<int>> rightmost = {box[2], box[3]};

    if (leftmost[0][1] > leftmost[1][1]) std::swap(leftmost[0], leftmost[1]);

    if (rightmost[0][1] > rightmost[1][1]) std::swap(rightmost[0], rightmost[1]);

    std::vector<std::vector<int>> rect = {leftmost[0], rightmost[0], rightmost[1], leftmost[1]};
    return rect;
}

int dbnet_postprocess(float* output, int det_out_w, int det_out_h, float db_threshold, float db_box_threshold, bool use_dilation,
                                                const std::string &db_score_mode, const float &db_unclip_ratio, const std::string &db_box_type,
                                                float scale_w, float scale_h, ppocr_det_result* results)
{
    // printf("[Info] db_threshold=%f, db_box_threshold=%f, use_dilation=%d, db_score_mode=%s, db_unclip_ratio=%f, db_box_type=%s\n",
    //                 db_threshold, db_box_threshold, use_dilation, db_score_mode.c_str(), db_unclip_ratio, db_box_type.c_str());
    int n = det_out_w * det_out_h;

    // prepare bitmap
    std::vector<float> pred(n, 0.0);
    std::vector<unsigned char> cbuf(n, ' ');

    for (int i = 0; i < n; i++) {
        pred[i] = float(output[i]);
        cbuf[i] = (unsigned char)((output[i]) * 255);
    }
    cv::Mat cbuf_map(det_out_h, det_out_w, CV_8UC1, (unsigned char*)cbuf.data());
    cv::Mat pred_map(det_out_h, det_out_w, CV_32F, (float*)pred.data());

    float threshold = db_threshold * 255;
    float maxvalue = 255;
    cv::Mat bit_map;
    cv::threshold(cbuf_map, bit_map, threshold, maxvalue, cv::THRESH_BINARY);
    if (use_dilation) {
        cv::Mat dila_ele = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::dilate(bit_map, bit_map, dila_ele);
    }
    // cv::imwrite("binary.jpg", bit_map);

    // find polygon Contours
    const int min_size = 3;
    const int max_candidates = 1000;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;

    cv::findContours(bit_map, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    int num_contours = contours.size() >= max_candidates ? max_candidates : contours.size();
    // printf("[Info] num_contours=%d\n", num_contours);

    std::vector<std::vector<std::vector<int>>> boxes;
    std::vector<float> scores;

    for (int _i = 0; _i < num_contours; _i++) {
        float score;
        if (db_box_type == "poly"){
            // printf("[OK] Starting poly postprocess\n");
            float epsilon = 0.002 * cv::arcLength(contours[_i], true);
            std::vector<cv::Point> points;
            cv::approxPolyDP(contours[_i], points, epsilon, true);
            if (points.size() < 4) {
                continue;
            }

            score = PolygonScoreAcc(points, pred_map);
            // printf("[Info] epsilon=%f, polyscore=%f\n", epsilon, score);
            if (score < db_box_threshold) continue;

            std::vector<std::vector<float>> box_for_unclip;
            for(int _k = 0; _k < points.size(); _k++) {
                std::vector<float> _box;
                _box.push_back(points[_k].x);
                _box.push_back(points[_k].y);
                box_for_unclip.push_back(_box);
            }
            // start for unclip
            cv::RotatedRect clipbox = UnClip(box_for_unclip, db_unclip_ratio);
            if (clipbox.size.height < 1.001 && clipbox.size.width < 1.001) {
                continue;
            }
            // end for unclip
            cv::Point2f vertex[4];
	        clipbox.points(vertex);
            // for (int i = 0; i < 4; i++)
            // {
            //     cv::line(bit_map, vertex[i], vertex[(i + 1) % 4], cv::Scalar(255, 100, 200), 2);
            // }
            // cv::imwrite("binary-rotatedrect.jpg", bit_map);

            float ssid;
            auto cliparray = GetMiniBoxes(clipbox, ssid);
            if (ssid < min_size + 2) {
                continue;
            }

            std::vector<std::vector<int>> intcliparray;

            for (int num_pt = 0; num_pt < 4; num_pt++) {
                std::vector<int> a{int(clampf(vertex[num_pt].x, 0, float(det_out_w))), int(clampf(vertex[num_pt].y, 0, float(det_out_h)))};
                intcliparray.push_back(a);
            }
            // printf("[Info] rotateRect: [(%f, %f), (%f, %f), (%f, %f), (%f, %f)]\n", vertex[0].x, vertex[0].y, vertex[1].x, vertex[1].y, 
            //                 vertex[2].x, vertex[2].y, vertex[3].x, vertex[3].y);
            boxes.push_back(intcliparray);
        }
        else
        {
            // printf("[OK] Starting quad postprocess\n");
            if (contours[_i].size() <= 2) {
                continue;
            }

            float ssid;
            cv::RotatedRect box = cv::minAreaRect(contours[_i]);
            auto array = GetMiniBoxes(box, ssid);
            auto box_for_unclip = array;
            // end get_mini_box

            if (ssid < min_size) {
                continue;
            }

            cv::Point2f vertex[4];
	        box.points(vertex);
            // for (int i = 0; i < 4; i++)
            // {
            //     cv::line(bit_map, vertex[i], vertex[(i + 1) % 4], cv::Scalar(255, 100, 200), 2);
            // }
            // cv::imwrite("binary-rotatedrect.jpg", bit_map);

            if (db_score_mode == "slow") /* compute using polygon*/
                score = PolygonScoreAcc(contours[_i], pred_map);
            else
                score = BoxScoreFast(array, pred_map);
            // printf("[Info] polyscore=%f\n", score);
            if (score < db_box_threshold) continue;

            // start for unclip
            cv::RotatedRect points = UnClip(box_for_unclip, db_unclip_ratio);
            if (points.size.height < 1.001 && points.size.width < 1.001) {
                continue;
            }
            // end for unclip

	        points.points(vertex);
            // for (int i = 0; i < 4; i++)
            // {
            //     cv::line(bit_map, vertex[i], vertex[(i + 1) % 4], cv::Scalar(255, 100, 200), 2);
            // }
            // cv::imwrite("binary-unclipRect.jpg", bit_map);

            cv::RotatedRect clipbox = points;
            auto cliparray = GetMiniBoxes(clipbox, ssid);

            if (ssid < min_size + 2) continue;

            std::vector<std::vector<int>> intcliparray;

            for (int num_pt = 0; num_pt < 4; num_pt++) {
                std::vector<int> a{
                    int(clampf(cliparray[num_pt][0], 0, float(det_out_w))),
                    int(clampf(cliparray[num_pt][1], 0, float(det_out_h)))};
                intcliparray.push_back(a);
            }
            boxes.push_back(intcliparray);
        }
        scores.push_back(score);
    }

    std::vector<std::vector<std::vector<int>>> root_points;
    std::vector<float> root_scores;
    for (int n = 0; n < boxes.size(); n++) {
        boxes[n] = OrderPointsClockwise(boxes[n]);
        for (int m = 0; m < boxes[0].size(); m++) {
            boxes[n][m][0] = int(min(max(boxes[n][m][0], 0), det_out_w - 1));
            boxes[n][m][1] = int(min(max(boxes[n][m][1], 0), det_out_h - 1));
        }

        // printf("[Info] boxes: [(%d, %d), (%d, %d), (%d, %d), (%d, %d)]\n", boxes[n][0][0], boxes[n][0][1], boxes[n][1][0], boxes[n][1][1], 
        //                 boxes[n][2][0], boxes[n][2][1], boxes[n][3][0], boxes[n][3][1]);
        int rect_width, rect_height;
        rect_width = int(sqrt(pow(boxes[n][0][0] - boxes[n][1][0], 2) +
                            pow(boxes[n][0][1] - boxes[n][1][1], 2)));
        rect_height = int(sqrt(pow(boxes[n][0][0] - boxes[n][3][0], 2) +
                            pow(boxes[n][0][1] - boxes[n][3][1], 2)));
        // printf("[Info] rect_width=%d, rect_height=%d\n", rect_width, rect_height);
        if (rect_width <= 4 || rect_height <= 4) continue;
        root_points.push_back(boxes[n]);
        root_scores.push_back(scores[n]);
    }

    results->count = 0;
    for (int n = 0; n < root_points.size(); n++) {
        if (results->count >= 1000) break;
        results->box[n].left_top.x = root_points[n][0][0] * scale_w;
        results->box[n].left_top.y = root_points[n][0][1] * scale_h;
        results->box[n].right_top.x = root_points[n][1][0] * scale_w;
        results->box[n].right_top.y = root_points[n][1][1] * scale_h;
        results->box[n].right_bottom.x = root_points[n][2][0] * scale_w;
        results->box[n].right_bottom.y = root_points[n][2][1] * scale_h;
        results->box[n].left_bottom.x = root_points[n][3][0] * scale_w;
        results->box[n].left_bottom.y = root_points[n][3][1] * scale_h;
        results->box[n].score = root_scores[n];
        results->count ++;
    }

    return 0;
}