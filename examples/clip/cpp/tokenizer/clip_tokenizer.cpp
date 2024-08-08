#include "clip_tokenizer.h"

std::u32string utf8_to_utf32(const std::string& utf8_str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.from_bytes(utf8_str);
}

std::string utf32_to_utf8(const std::u32string& utf32_str) {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    return converter.to_bytes(utf32_str);
}

std::u32string unicode_value_to_utf32(int unicode_value) {
    std::u32string utf32_string = {static_cast<char32_t>(unicode_value)};
    return utf32_string;
}

std::vector<std::pair<int, std::u32string>> bytes_to_unicode() {
    std::vector<std::pair<int, std::u32string>> byte_unicode_pairs;
    std::set<int> byte_set;
    for (int b = static_cast<int>('!'); b <= static_cast<int>('~'); ++b) {
        byte_set.insert(b);
        byte_unicode_pairs.push_back(std::pair<int, std::u32string>(b, unicode_value_to_utf32(b)));
    }
    for (int b = 161; b <= 172; ++b) {
        byte_set.insert(b);
        byte_unicode_pairs.push_back(std::pair<int, std::u32string>(b, unicode_value_to_utf32(b)));
    }
    for (int b = 174; b <= 255; ++b) {
        byte_set.insert(b);
        byte_unicode_pairs.push_back(std::pair<int, std::u32string>(b, unicode_value_to_utf32(b)));
    }
    int n = 0;
    for (int b = 0; b < 256; ++b) {
        if (byte_set.find(b) == byte_set.end()) {
            byte_unicode_pairs.push_back(std::pair<int, std::u32string>(b, unicode_value_to_utf32(n + 256)));
            ++n;
        }
    }
    // LOG_DEBUG("byte_unicode_pairs %d", byte_unicode_pairs.size());
    return byte_unicode_pairs;
}

static std::string strip(const std::string& str) {
    std::string::size_type start = str.find_first_not_of(" \t\n\r\v\f");
    std::string::size_type end   = str.find_last_not_of(" \t\n\r\v\f");

    if (start == std::string::npos) {
        // String contains only whitespace characters
        return "";
    }

    return str.substr(start, end - start + 1);
}

static std::string whitespace_clean(std::string text) {
    text = std::regex_replace(text, std::regex(R"(\s+)"), " ");
    text = strip(text);
    return text;
}

static std::set<std::pair<std::u32string, std::u32string>> get_pairs(const std::vector<std::u32string>& subwords) {
    std::set<std::pair<std::u32string, std::u32string>> pairs;
    if (subwords.size() == 0) {
        return pairs;
    }
    std::u32string prev_subword = subwords[0];
    for (int i = 1; i < subwords.size(); i++) {
        std::u32string subword = subwords[i];
        std::pair<std::u32string, std::u32string> pair(prev_subword, subword);
        pairs.insert(pair);
        prev_subword = subword;
    }
    return pairs;
}

void CLIPTokenizer::load_from_merges(const std::string& merges_utf8_str) {
        auto byte_unicode_pairs = bytes_to_unicode();
        byte_encoder = std::map<int, std::u32string>(byte_unicode_pairs.begin(), byte_unicode_pairs.end());
        // for (auto & pair: byte_unicode_pairs) {
        //     std::cout << pair.first << ": " << pair.second << std::endl;
        // }
        std::vector<std::u32string> merges;
        size_t start = 0;
        size_t pos;
        std::u32string merges_utf32_str = utf8_to_utf32(merges_utf8_str);
        while ((pos = merges_utf32_str.find('\n', start)) != std::string::npos) {
            merges.push_back(merges_utf32_str.substr(start, pos - start));
            start = pos + 1;
        }
        // LOG_DEBUG("merges size %llu", merges.size());
        // GGML_ASSERT(merges.size() == 48895);
        merges = std::vector<std::u32string>(merges.begin() + 1, merges.end());
        std::vector<std::pair<std::u32string, std::u32string>> merge_pairs;
        for (const auto& merge : merges) {
            size_t space_pos = merge.find(' ');
            merge_pairs.emplace_back(merge.substr(0, space_pos), merge.substr(space_pos + 1));
            // LOG_DEBUG("%s", utf32_to_utf8(merge.substr(space_pos + 1)).c_str());
        }
        std::vector<std::u32string> vocab;
        for (const auto& pair : byte_unicode_pairs) {
            vocab.push_back(pair.second);
        }
        for (const auto& pair : byte_unicode_pairs) {
            vocab.push_back(pair.second + utf8_to_utf32("</w>"));
        }
        for (const auto& merge : merge_pairs) {
            vocab.push_back(merge.first + merge.second);
        }
        vocab.push_back(utf8_to_utf32("<|startoftext|>"));
        vocab.push_back(utf8_to_utf32("<|endoftext|>"));
        // LOG_DEBUG("vocab size: %llu", vocab.size());
        int i = 0;
        for (const auto& token : vocab) {
            encoder[token] = i++;
        }

        int rank = 0;
        for (const auto& merge : merge_pairs) {
            bpe_ranks[merge] = rank++;
        }
    }

std::u32string CLIPTokenizer::bpe(const std::u32string& token) {
    std::vector<std::u32string> word;

    for (int i = 0; i < token.size() - 1; i++) {
        word.emplace_back(1, token[i]);
    }
    word.push_back(token.substr(token.size() - 1) + utf8_to_utf32("</w>"));

    std::set<std::pair<std::u32string, std::u32string>> pairs = get_pairs(word);

    if (pairs.empty()) {
        return token + utf8_to_utf32("</w>");
    }

    while (true) {
        auto min_pair_iter = std::min_element(pairs.begin(),
                                                pairs.end(),
                                                [&](const std::pair<std::u32string, std::u32string>& a,
                                                    const std::pair<std::u32string, std::u32string>& b) {
                                                    if (bpe_ranks.find(a) == bpe_ranks.end()) {
                                                        return false;
                                                    } else if (bpe_ranks.find(b) == bpe_ranks.end()) {
                                                        return true;
                                                    }
                                                    return bpe_ranks.at(a) < bpe_ranks.at(b);
                                                });

        const std::pair<std::u32string, std::u32string>& bigram = *min_pair_iter;

        if (bpe_ranks.find(bigram) == bpe_ranks.end()) {
            break;
        }

        std::u32string first  = bigram.first;
        std::u32string second = bigram.second;
        std::vector<std::u32string> new_word;
        int32_t i = 0;

        while (i < word.size()) {
            auto it = std::find(word.begin() + i, word.end(), first);
            if (it == word.end()) {
                new_word.insert(new_word.end(), word.begin() + i, word.end());
                break;
            }
            new_word.insert(new_word.end(), word.begin() + i, it);
            i = static_cast<int32_t>(std::distance(word.begin(), it));

            if (word[i] == first && i < static_cast<int32_t>(word.size()) - 1 && word[i + 1] == second) {
                new_word.push_back(first + second);
                i += 2;
            } else {
                new_word.push_back(word[i]);
                i += 1;
            }
        }

        word = new_word;

        if (word.size() == 1) {
            break;
        }
        pairs = get_pairs(word);
    }

    std::u32string result;
    for (int i = 0; i < word.size(); i++) {
        result += word[i];
        if (i != word.size() - 1) {
            result += utf8_to_utf32(" ");
        }
    }

    return result;
}

std::vector<int> CLIPTokenizer::tokenize(std::string text, size_t max_length, bool padding) {
    std::vector<int32_t> tokens = encode(text);
    tokens.insert(tokens.begin(), BOS_TOKEN_ID);
    if (max_length > 0) {
        if (tokens.size() > max_length - 1) {
            tokens.resize(max_length - 1);
            tokens.push_back(EOS_TOKEN_ID);
        } else {
            tokens.push_back(EOS_TOKEN_ID);
            if (padding) {
                int pad_token_id = PAD_TOKEN_ID;
                tokens.insert(tokens.end(), max_length - tokens.size(), pad_token_id);
            }
        }
    }
    return tokens;
}

std::vector<int> CLIPTokenizer::encode(std::string text) {
    std::string original_text = text;
    std::vector<int32_t> bpe_tokens;
    text = whitespace_clean(text);
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });

    std::regex pat(R"(<\|startoftext\|>|<\|endoftext\|>|'s|'t|'re|'ve|'m|'ll|'d|[[:alpha:]]+|[[:digit:]]|[^[:space:][:alpha:][:digit:]]+)",
                    std::regex::icase);

    std::smatch matches;
    std::string str = text;
    std::vector<std::string> token_strs;
    while (std::regex_search(str, matches, pat)) {
        for (auto& token : matches) {
            std::string token_str = token.str();
            std::u32string utf32_token;
            for (int i = 0; i < token_str.length(); i++) {
                char b = token_str[i];
                utf32_token += byte_encoder[b];
            }
            auto bpe_strs = bpe(utf32_token);
            size_t start  = 0;
            size_t pos;
            while ((pos = bpe_strs.find(' ', start)) != std::u32string::npos) {
                auto bpe_str = bpe_strs.substr(start, pos - start);
                bpe_tokens.push_back(encoder[bpe_str]);
                token_strs.push_back(utf32_to_utf8(bpe_str));

                start = pos + 1;
            }
            auto bpe_str = bpe_strs.substr(start, bpe_strs.size() - start);
            bpe_tokens.push_back(encoder[bpe_str]);
            token_strs.push_back(utf32_to_utf8(bpe_str));
        }
        str = matches.suffix();
    }
    return bpe_tokens;
}