#ifndef _RKNN_DEMO_BPE_TOOLS_H_
#define _RKNN_DEMO_BPE_TOOLS_H_

#define BPE_DICT_LEN 32005
#define COMMON_DICT_LEN 10000
#define TOKEN_LEN_LIMIT 36808

// A-Z a-z others
#define CHARACTOR_CLASS_NUM 53

#define WORD_LEN_LIMIT 50
#define COMMON_WORD_MAX_BPE_CODE 10
#define LONG_WORD 512

#define BPE_VERSION 0

#define SPLIT_SYM "</w>"
#define UNKNOWN_WORD_TOKEN 3

enum COMMON_DICT_TYPE
{
    CW_BPE=0,
    CW_TOKEN,
};

enum DICT_ORDER_TYPE
{
    NON_ORDERED=0,
    ORDERED,
};

typedef struct _SORTED_DICT_GROUP
{
    char** strings;
    int*  scores;
    int*** qs_index; //quick search index, index include (start index, end index)

    int qs_class_num=CHARACTOR_CLASS_NUM;
    int buf_alloc_num=0;
    int real_num=0;
    int offset=0;
} SORTED_DICT_GROUP;

typedef struct _Search_result
{
    int index;
    int score;
} Search_result;

Search_result get_index_in_sorted_group(SORTED_DICT_GROUP sdg, const char word[WORD_LEN_LIMIT]);


class Bpe_Tools
{
private:
    /* data */
    SORTED_DICT_GROUP bpe_sdg, common_word_sdg, token_sdg;

    char common_word_bpe_result[COMMON_DICT_LEN][COMMON_WORD_MAX_BPE_CODE][WORD_LEN_LIMIT];
    int common_word_token[COMMON_DICT_LEN][COMMON_WORD_MAX_BPE_CODE];

    bool common_word_available=false;

public:
    Bpe_Tools();
    ~Bpe_Tools();

    int prepare_bpe_data(const char* bpe_dict_path, DICT_ORDER_TYPE dict_type);
    int prepare_common_word_data(const char* common_word_dict_path, COMMON_DICT_TYPE dict_type);
    int prepare_token_data(const char* token_dict_path, DICT_ORDER_TYPE dict_type);

    void set_bpe_offset(int offset);
    void set_common_word_offset(int offset);
    void set_token_offset(int offset);

    Search_result common_word_search(const char* word);

    int find_word_token(const char* word);
    int bpe_encode(const char* word, char bpe_word_list[WORD_LEN_LIMIT][WORD_LEN_LIMIT]);
    int bpe_decoder();
    int bpe_and_tokenize(const char* word, int tokens[WORD_LEN_LIMIT]);

    int get_word_by_token(int token, char* word);
};

#endif // _RKNN_DEMO_BPE_TOOLS_H_