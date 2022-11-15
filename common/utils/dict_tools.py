import os

def _dict_to_str(tar_dict, indent=2, new_line_between_dict=False):
    def _parse_dict(_d, _srt_list, _depth=0):
        _blank = ' '*indent
        first_dict = True
        for _key, _value in _d.items():
            if isinstance(_value, dict):
                if (new_line_between_dict is True) and (first_dict is False):
                    _str.append('\n')
                _str.append(_blank*_depth + _key + ':\n')
                _parse_dict(_value, _srt_list, _depth+1)
                first_dict = False
            else:
                _srt_list.append(_blank*_depth + _key + ': ' + str(_value) + '\n')

    if not isinstance(tar_dict, dict):
        print('{} is not a dict'.format(tar_dict))

    _str = []
    _parse_dict(tar_dict, _str)
    return _str
