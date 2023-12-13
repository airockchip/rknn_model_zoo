#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bpe_tools.h"

#define NEXIST -1234

// extern int g_strcmp_count;

int split(char *str, const char *delim, char dst[][WORD_LEN_LIMIT]) {
    char *s = strdup(str);
    char *token;  
    int n = 0;
    for(token = strsep(&s, delim); token != NULL; token = strsep(&s, delim)) {  
        strcpy(dst[n++], token);
    }  
    return n;
}

void remove_newline_sym(char* str, int len){
    for (int j = 0; j < len; j++){
        if (str[j] == '\n'){
            str[j] = '\0';
        }
    }
}

int min(int a, int b){
    return a < b ? a : b;
}

int max(int a, int b){
    return a > b ? a : b;
}

int character_to_index(char c)
{
    if (c >= 'A' && c <= 'Z'){
        return c - 'A';
    }
    else if (c >= 'a' && c <= 'z'){
        return c - 'a' + 26;
    }
    else{
        return 52;
    }
}

char index_to_character(int i)
{
    if (i >= 0 && i <= 25){
        return i + 'A';
    }
    else if (i >= 26 && i <= 51){
        return i - 26 + 'a';
    }
    else{
        return '!';
    }
}

void group_qs_index_init(SORTED_DICT_GROUP sdg){
    // return;
    int c_min_max[CHARACTOR_CLASS_NUM][WORD_LEN_LIMIT][2];
    for (int i = 0; i < CHARACTOR_CLASS_NUM; i++){
        for (int j = 0; j < WORD_LEN_LIMIT; j++){
            c_min_max[i][j][0] = NEXIST;
            c_min_max[i][j][1] = NEXIST;
        }
    }

    for (int i = 0; i < sdg.real_num; i++){
        int fc_int = character_to_index(sdg.strings[i][0]);
        // printf("fc_int = %d, char = %c\n", fc_int, sdg.strings[i][0]);
        int len = strlen(sdg.strings[i]);
        if (c_min_max[fc_int][len][0] == NEXIST){
            c_min_max[fc_int][len][0] = i;
        }
        else{
            c_min_max[fc_int][len][0] = i < c_min_max[fc_int][len][0] ? i : c_min_max[fc_int][len][0];
        }

        if (c_min_max[fc_int][len][1] == NEXIST){
            c_min_max[fc_int][len][1] = i;
        }
        else{
            c_min_max[fc_int][len][1] = i > c_min_max[fc_int][len][1] ? i : c_min_max[fc_int][len][1];
        }
    }

    for (int i = 0; i < CHARACTOR_CLASS_NUM; i++){
        for (int j = 0; j < WORD_LEN_LIMIT; j++){
            sdg.qs_index[i][j][0] = c_min_max[i][j][0];
            sdg.qs_index[i][j][1] = c_min_max[i][j][1];
            }

        char _s[10];
        if (i==CHARACTOR_CLASS_NUM-1){
            sprintf(_s, "%s", "others");
        }
        else{
            sprintf(_s, "%c", index_to_character(i));
        }

        // showing qs sort result
        // printf("For word start with charactor - %s\n", _s);
        // printf("    word lens = ");
        // for (int j = 0; j < WORD_LEN_LIMIT; j++){
        //     printf("%4d ", j);
        //     if (j>12){printf(" ...");break;}
        // }
        // printf("\n        count = ");
        // for (int j = 0; j < WORD_LEN_LIMIT; j++){
        //     if (sdg.qs_index[i][j][0] == NEXIST){
        //         printf("%4d ", 0);
        //     }
        //     else {
        //         printf("%4d ", sdg.qs_index[i][j][1] - sdg.qs_index[i][j][0]+1);
        //     }
        //     if (j>12){printf(" ...");break;}
        // }
        // printf("\n");
    }
    // printf("qs_index_init done\n");
}


Search_result get_index_in_sorted_group(SORTED_DICT_GROUP sdg, const char word[WORD_LEN_LIMIT])
{
    // printf("get_index_in_sorted_group: %s\n", word);
    Search_result sr = {NEXIST, NEXIST};
    int fc_int = character_to_index(word[0]);
    if (fc_int >= sdg.qs_class_num){
        printf("ERROR index_in_sorted_group: fc_int[%d] >= sdg.qs_class_num[%d]\n",fc_int, sdg.qs_class_num);
        return sr;
    }
    int word_len = strlen(word);
    if (word_len >= WORD_LEN_LIMIT){
        printf("WARNING: %s exceed %d charactors\n", word, WORD_LEN_LIMIT);
        return sr;
    }

    // printf("fc_int = %d, char = %c, word_len = %d\n", fc_int, word[0], word_len);
    // printf("start_index = %d, end_index = %d\n", sdg.qs_index[fc_int][word_len][0], sdg.qs_index[fc_int][word_len][1]);
    int hit_index = NEXIST;
    for (int i = max(0, sdg.qs_index[fc_int][word_len][0]); i <= sdg.qs_index[fc_int][word_len][1]; i++)
    {
        // g_strcmp_count++;
        if (strcmp(sdg.strings[i], word) == 0)
        {
            hit_index = i;
            break;
        }
    }

    if (hit_index != NEXIST)
    {
        sr.index = hit_index;
        sr.score = sdg.scores[hit_index];
        return sr;
    }

    return sr;
}


int Bpe_Tools::prepare_token_data(const char* token_dict_path, DICT_ORDER_TYPE dict_type){
    FILE *fp_dict;
    fp_dict = fopen(token_dict_path, "r");
    if (NULL == fp_dict)
    {
        printf("open %s failed.\n", token_dict_path);
        return -1;
    }

    /* basic test version*/
    int dict_line_num = 0;
    char temp_words[WORD_LEN_LIMIT][WORD_LEN_LIMIT];
    char temp_line[WORD_LEN_LIMIT* WORD_LEN_LIMIT];
    while (fgets(temp_line, WORD_LEN_LIMIT*WORD_LEN_LIMIT, fp_dict))
    {
        if (dict_type == NON_ORDERED){
            strcpy(this->token_sdg.strings[dict_line_num], temp_line);
            remove_newline_sym(this->token_sdg.strings[dict_line_num], WORD_LEN_LIMIT);
            this->token_sdg.scores[dict_line_num] = dict_line_num;
        }
        else if (dict_type == ORDERED){
            int word_num = split(temp_line, " ", temp_words);
            strcpy(this->token_sdg.strings[dict_line_num], temp_words[0]);
            remove_newline_sym(temp_words[1], WORD_LEN_LIMIT);
            this->token_sdg.scores[dict_line_num] = atoi(temp_words[1]);
        }

        dict_line_num++;
        if (dict_line_num >= this->token_sdg.buf_alloc_num){
            printf("ERROR: token dict is too large, %d exceed alloc mem %d please check it.\n", dict_line_num, this->token_sdg.buf_alloc_num);
        }
    }

    this->token_sdg.real_num = dict_line_num+1;
    for (int i=0; i<this->token_sdg.qs_class_num; i++)
    {
        for (int j=0; j<WORD_LEN_LIMIT; j++)
        {
            this->token_sdg.qs_index[i][j][1] = dict_line_num;
        }
    }
    group_qs_index_init(this->token_sdg);

    fclose(fp_dict);

    return 0;
}


int Bpe_Tools::prepare_bpe_data(const char* bpe_dict_path, DICT_ORDER_TYPE dict_type){
    FILE *fp_dict;
    fp_dict = fopen(bpe_dict_path, "r");
    if (NULL == fp_dict)
    {
        printf("open %s failed.\n", bpe_dict_path);
        return -1;
    }

    int dict_line_num = 0;
    char temp_words[WORD_LEN_LIMIT][WORD_LEN_LIMIT];
    char temp_line[WORD_LEN_LIMIT* WORD_LEN_LIMIT];
    while (fgets(temp_line, WORD_LEN_LIMIT*WORD_LEN_LIMIT, fp_dict)){
        if (dict_type == NON_ORDERED){
            strcpy(this->bpe_sdg.strings[dict_line_num], temp_line);
            remove_newline_sym(this->bpe_sdg.strings[dict_line_num], WORD_LEN_LIMIT);
            this->bpe_sdg.scores[dict_line_num] = dict_line_num;
        }
        else if (dict_type == ORDERED){
            int word_num = split(temp_line, " ", temp_words);
            sprintf(this->bpe_sdg.strings[dict_line_num], "%s %s", temp_words[0], temp_words[1]);
            remove_newline_sym(temp_words[2], WORD_LEN_LIMIT);
            this->bpe_sdg.scores[dict_line_num] = atoi(temp_words[2]);
        }
        dict_line_num++;
        if (dict_line_num >= this->bpe_sdg.buf_alloc_num){
            printf("ERROR: bpe dict is too large, %d exceed alloc mem %d please check it.\n", dict_line_num, this->bpe_sdg.buf_alloc_num);
        }
    }

    this->bpe_sdg.real_num = dict_line_num+1;
    for (int i=0; i<this->bpe_sdg.qs_class_num; i++)
    {
        for (int j=0; j<WORD_LEN_LIMIT; j++)
        {
            this->bpe_sdg.qs_index[i][j][1] = dict_line_num;
        }
    }
    group_qs_index_init(this->bpe_sdg);

    fclose(fp_dict);
    return 0;
}


int Bpe_Tools::prepare_common_word_data(const char* common_word_dict_path, COMMON_DICT_TYPE dict_type){
    // printf("common_word_dict_path: %s\n", common_word_dict_path);
    FILE *fp_dict;
    fp_dict = fopen(common_word_dict_path, "r");
    if (NULL == fp_dict)
    {
        printf("open %s failed.\n", common_word_dict_path);
        return -1;
    }

    /* basic test version*/
    int dict_line_num = 0;
    char temp_words[WORD_LEN_LIMIT][WORD_LEN_LIMIT];
    char temp_line[WORD_LEN_LIMIT* WORD_LEN_LIMIT];
    while (fgets(temp_line, WORD_LEN_LIMIT*WORD_LEN_LIMIT, fp_dict))
    {
        // printf("\nstrings[%d]: %s", dict_line_num, temp_line);
        int split_len = split(temp_line, " ", temp_words);
        int bpe_len = atoi(temp_words[1]);

        strcpy(this->common_word_sdg.strings[dict_line_num], temp_words[0]);
        this->common_word_sdg.scores[dict_line_num] = bpe_len;

        if (dict_type == CW_BPE){
            for (int i=0; i<bpe_len; i++)
            {
                strcpy(this->common_word_bpe_result[dict_line_num][i], temp_words[i+2]);
            }

            // remove \n for last word
            for (int j = 0; j < WORD_LEN_LIMIT; j++)
            {
                if (this->common_word_bpe_result[dict_line_num][bpe_len-1][j] == '\n')
                {
                    this->common_word_bpe_result[dict_line_num][bpe_len-1][j] = '\0';
                }
            }

            for (int i=0; i<bpe_len; i++)
            {
                this->common_word_token[dict_line_num][i] = find_word_token(this->common_word_bpe_result[dict_line_num][i]);
            }
        }

        else if (dict_type == CW_TOKEN)
        {
            for (int i=0; i<bpe_len; i++)
            {
                this->common_word_token[dict_line_num][i] = atoi(temp_words[i+2]);
            }
        }
        
        // printf("common word: %s\n", this->common_word_sdg.strings[dict_line_num]);
        // printf("bpe len: %d\n", this->common_word_sdg.scores[dict_line_num]);
        // for (int i=0; i<bpe_len; i++)
        // {
        //     printf("bpe result: %s : %d\n", this->common_word_bpe_result[dict_line_num][i], common_word_token[dict_line_num][i]);
        // }

        dict_line_num++;
        if (dict_line_num >= this->common_word_sdg.buf_alloc_num)
        {
            printf("ERROR: common word dict is too large, %d exceed alloc mem %d please check it.\n", dict_line_num, this->common_word_sdg.buf_alloc_num);
        }
    }

    this->common_word_sdg.real_num = dict_line_num+1;
    for (int i=0; i<this->common_word_sdg.qs_class_num; i++)
    {
        // set charactor_end_index to end
        for (int j=0; j<WORD_LEN_LIMIT; j++)
        {
            this->common_word_sdg.qs_index[i][j][1] = dict_line_num;
        }
    }
    group_qs_index_init(this->common_word_sdg);

    fclose(fp_dict);
    this->common_word_available = true;
    return 0;
}


Search_result Bpe_Tools::common_word_search(const char* word){
    return get_index_in_sorted_group(this->common_word_sdg, word);
}

int Bpe_Tools::bpe_encode(const char* word, char bpe_word_list[WORD_LEN_LIMIT][WORD_LEN_LIMIT]){
    int num_char = strlen(word);
    if (num_char < 1 || num_char>WORD_LEN_LIMIT){ return -1;  }

    char sub_word[num_char][WORD_LEN_LIMIT];
    char tmp_word[WORD_LEN_LIMIT];
    for (int i = 0; i < num_char; i++)
    {
        sprintf(sub_word[i], "%c", word[i]);
    }


#if (BPE_VERSION == 1)
        sprintf(sub_word[num_char], SPLIT_SYM);         // add split symbol
        num_char += 1;
#elif (BPE_VERSION == 0)
        memset(tmp_word, 0, WORD_LEN_LIMIT);
        strcpy(tmp_word, sub_word[num_char-1]);
        sprintf(sub_word[num_char-1], "%s%s", tmp_word, SPLIT_SYM);         // add split symbol
#endif

    while (num_char>1)
    {
        char key[WORD_LEN_LIMIT*2+1];
        int score=0;
        int cat_index=0;
        int best_score=12345678; // score越小越好

        for (int j = 0; j < num_char - 1; j++)
        {
            sprintf(key, "%s %s", sub_word[j], sub_word[j + 1]);
            // printf("    temp key: %s - \n", key);
            Search_result sr = get_index_in_sorted_group(this->bpe_sdg, key);
            score = sr.score;
            // printf("%d - %d\n", sr.index, score);
            if (best_score > score and score >= 0) // score越小越好
            {
                best_score = score;
                cat_index = j;
            }
        }

        if (best_score == 12345678)
        {
            // bpe pair not found
            break;
        }

        num_char--;
        strcpy(tmp_word, sub_word[cat_index]);
        sprintf(sub_word[cat_index], "%s%s", tmp_word, sub_word[cat_index + 1]);
        for (int j = cat_index + 1; j < num_char; j++)
        {
            strcpy(sub_word[j], sub_word[j + 1]);
        }
    }

    //删除split符号
    if (strcmp(sub_word[num_char - 1], SPLIT_SYM) == 0){
        num_char -= 1;
    }
    else{
        int w = strlen(sub_word[num_char - 1]);
        for (int j = w - strlen(SPLIT_SYM); j < w; j++) //删除split符号
        {
            sub_word[num_char - 1][j] = '\0';
        }
    }

    //添加连接符号
    if (num_char > 1)
    {
        for (int j = 0; j < num_char - 1; j++)
        {
            strcpy(tmp_word, sub_word[j]);
            sprintf(sub_word[j], "%s%s", tmp_word, "@@");
        }
    }


    // for debug
    printf("    word: %s\n", word);
    printf("    bpe result: ");
    for (int j = 0; j < num_char; j++)
    {
        printf("%s ", sub_word[j]);
        sprintf(bpe_word_list[j], "%s", sub_word[j]);
    }
    printf("\n");

    return num_char;
}


int Bpe_Tools::bpe_decoder(){
    return NEXIST;
}

int Bpe_Tools::find_word_token(const char* word){
    Search_result sr = get_index_in_sorted_group(this->token_sdg, word);
    if (sr.score == NEXIST){
        sr.score = UNKNOWN_WORD_TOKEN;
    }
    return sr.score + this->token_sdg.offset;
}

int Bpe_Tools::get_word_by_token(int token, char* word_get){
    if (token > this->token_sdg.real_num or token < 0){
        return -1;
    }
    int src_position = 0;
    for (int i=0; i<TOKEN_LEN_LIMIT; i++){
        if (this->token_sdg.scores[i] == (token - token_sdg.offset)){
            src_position = i;
            break;
        }
    }
    memcpy(word_get, this->token_sdg.strings[src_position], WORD_LEN_LIMIT);
    return 0;
}

int Bpe_Tools::bpe_and_tokenize(const char* word, int tokens[WORD_LEN_LIMIT]){
    if (this->common_word_available){
        Search_result sr = common_word_search(word);
        if (sr.score >= 0){
            printf("    common_word found: %s, token as: ", word);
            for (int i=0; i< sr.score; i++){
                tokens[i] = this->common_word_token[sr.index][i];
                printf(" %d", tokens[i]);
            }
            printf("\n");
            return sr.score;
        }
    }

    char bpe_word_list[WORD_LEN_LIMIT][WORD_LEN_LIMIT];
    int num_bpe_word = bpe_encode(word, bpe_word_list);

    for (int i = 0; i < num_bpe_word; i++)
    {
        tokens[i] = find_word_token(bpe_word_list[i]);
        // printf("    bpe_word: %s, token: %d\n", bpe_word_list[i], tokens[i]);
    }

    return num_bpe_word;
}


/*
    init Function 
*/
int init_group(SORTED_DICT_GROUP &sdg, int total_len, int qs_class){
    sdg.buf_alloc_num = total_len;
    sdg.qs_class_num = qs_class;
    sdg.strings = (char**)malloc(sizeof(char*) * total_len);
    for (int i = 0; i < total_len; i++)
    {
        sdg.strings[i] = (char*)malloc(sizeof(char) * WORD_LEN_LIMIT);
    }

    sdg.scores = (int*)malloc(sizeof(int) * total_len);
    sdg.qs_index = (int***)malloc(sizeof(int**) * qs_class);
    for (int i = 0; i < qs_class; i++)
    {
        sdg.qs_index[i] = (int**)malloc(sizeof(int*) * WORD_LEN_LIMIT);
        for (int j = 0; j < WORD_LEN_LIMIT; j++)
        {
            sdg.qs_index[i][j] = (int*)malloc(sizeof(int) * 2);
            memset(sdg.qs_index[i][j], 0, sizeof(int) * 2);
        }
    }
    return 0;
}


int release_group(SORTED_DICT_GROUP &sdg){
    // printf("release_group\n");
    for (int i = 0; i < sdg.buf_alloc_num; i++)
    {
        free(sdg.strings[i]);
    }
    free(sdg.strings);

    free(sdg.scores);
    
    for (int i = 0; i < sdg.qs_class_num; i++)
    {
        for (int j = 0; j < WORD_LEN_LIMIT; j++)
        {
            free(sdg.qs_index[i][j]);
        }
        free(sdg.qs_index[i]);
    }
    free(sdg.qs_index);
    return 0;
}

void Bpe_Tools::set_bpe_offset(int offset){
    this->bpe_sdg.offset = offset;
}

void Bpe_Tools::set_common_word_offset(int offset){
    this->common_word_sdg.offset = offset;
}

void Bpe_Tools::set_token_offset(int offset){
    this->token_sdg.offset = offset;
}


Bpe_Tools::Bpe_Tools(/* args */)
{
    init_group(bpe_sdg, BPE_DICT_LEN, CHARACTOR_CLASS_NUM);
    init_group(common_word_sdg, COMMON_DICT_LEN, CHARACTOR_CLASS_NUM);
    init_group(token_sdg, TOKEN_LEN_LIMIT, CHARACTOR_CLASS_NUM);
}

Bpe_Tools::~Bpe_Tools()
{
    release_group(bpe_sdg);
    release_group(common_word_sdg);
    release_group(token_sdg);
}
