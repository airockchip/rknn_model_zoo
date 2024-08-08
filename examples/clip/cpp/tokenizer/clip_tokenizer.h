#ifndef _RKNN_DEMO_CLIP_TOKENIZER_H
#define _RKNN_DEMO_CLIP_TOKENIZER_H

#include <string>
#include <regex>
#include <set>
#include <codecvt>
#include <locale>
#include <map>

#include "clip_vocab.h"


const int UNK_TOKEN_ID = 49407;
const int BOS_TOKEN_ID = 49406;
const int EOS_TOKEN_ID = 49407;
const int PAD_TOKEN_ID = 49407;

std::u32string utf8_to_utf32(const std::string& utf8_str);
std::string utf32_to_utf8(const std::u32string& utf32_str);
std::u32string unicode_value_to_utf32(int unicode_value);
static std::string read_vocab() {
    std::string merges_utf8_str(reinterpret_cast<const char*>(RKNN_DEMO_CLIP_VOCAB_BIN_BUF), sizeof(RKNN_DEMO_CLIP_VOCAB_BIN_BUF));
    return merges_utf8_str;
}

class CLIPTokenizer {
private:
    std::map<int, std::u32string> byte_encoder;
    std::map<std::u32string, int> encoder;
    std::map<std::pair<std::u32string, std::u32string>, int> bpe_ranks;
    std::regex pat;
    std::string merges_utf8_str;

public:
    CLIPTokenizer() {
        merges_utf8_str = read_vocab();
        load_from_merges(merges_utf8_str);
    }

    void load_from_merges(const std::string& merges_utf8_str);

    std::u32string bpe(const std::u32string& token);

    std::vector<int> tokenize(std::string text, size_t max_length = 0, bool padding = false);

    std::vector<int> encode(std::string text);
    
};

#endif // _RKNN_DEMO_CLIP_TOKENIZER_H