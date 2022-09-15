import os
import numpy as np


class classification_helper():
    def __init__(self, threshold=0, need_softmax=False) -> None:
        # TODO support top5
        self.path_list = []
        self.match_list = []
        self.need_softmax = need_softmax
        self.threshold = threshold

    def add_single_record(self, output, label_index, path):
        output = output.reshape(-1)
        if self.need_softmax == True:
            output = np.exp(output)/np.sum(np.exp(output))

        output_sorted = sorted(output, reverse=True)
        value = output_sorted[0]
        index = np.where(output == value)

        top1_str = 'label index is {} while output index is '.format(label_index)
        self.path_list.append(path)

        if value > 0:
            top1_str += '{}:'.format(index[0])
            if label_index == index[0][0] and value >= self.threshold:
                self.match_list.append(1)
                top1_str += 'success\n'
            else:
                self.match_list.append(0)
                top1_str += 'fail\n'
            topi = '{}: {}\n'.format(index[0], value)
        else:
            self.match_list.append(0)
            top1_str += '-1: fail\n'
            topi = '-1: 0.0\n'

        top1_str += topi
        print(top1_str)
        self.conclude()

    def conclude(self):
        precision = np.array(self.match_list).sum()* 100.0 / len(self.match_list)
        print('up to now, on [{}] cases, the top 1 precision is [{}]'.format(len(self.match_list),precision))
        return precision