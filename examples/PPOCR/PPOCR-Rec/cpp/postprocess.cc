#include <cmath>
#include <math.h>
#include <algorithm>
#include "ppocr_rec.h"
#include "dict.h"

using namespace std;

int rec_postprocess(float* out_data, int out_channel, int out_seq_len, ppocr_rec_result* text)
{
    std::string str_res;
    float score = 0.f;
    int argmax_idx;
    int last_index = 0;
    int count = 0;
    float max_value = 0.0f;

    for (int n = 0; n < out_seq_len; n++) {
        argmax_idx = int(
            std::distance(&out_data[n * out_channel], std::max_element(&out_data[n * out_channel], &out_data[(n + 1) * out_channel]))
        );

        max_value = float(*std::max_element(&out_data[n * out_channel], &out_data[(n + 1) * out_channel]));

        if (argmax_idx > 0 && (!(n > 0 && argmax_idx == last_index))) {
            score += max_value;
            count += 1;
            if(argmax_idx > MODEL_OUT_CHANNEL) {
                printf("The output index: %d is larger than the size of label_list: %d. Please check the label file!\n",  argmax_idx, MODEL_OUT_CHANNEL);
                return -1; 
            }
            // printf("str argmax_idx: %d ", argmax_idx);
            str_res += ocr_dict[argmax_idx];
        }
        last_index = argmax_idx;
    }
    score /= (count + 1e-6);
    if (count == 0 || std::isnan(score)) {
        score = 0.f;
    }
    // printf("\ntext result is %s, score is %f\n", str_res.c_str(), score);

    // copy result to text
    strcpy(text->str, str_res.c_str());
    text->str_size = count;
    text->score = score;
    return 0;
}