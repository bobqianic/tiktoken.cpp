#pragma once

#include "unicode.h"

class llm_regex {
public:
    std::vector<std::string> gpt2_style(const std::string & str) {
        std::vector<std::string> results;
        results.reserve(str.size());
        std::vector<uint32_t> codepoints;
        codepoints.reserve(str.size());
        std::vector<uint32_t> codepoints_buffer;
        codepoints_buffer.reserve(str.size());

        codepoints = unicode_engine.to_codepoints(str);
        size_t offset = 0;

        auto codepoint_rules_1 = unicode_engine.to_codepoints({"'s", "'t", "'re", "'ve", "'m", "ll", "'d"});
        auto codepoint_rules_2 = unicode_engine.to_category_code({"WHITESPACE", "LETTER", "NUMBER"});

        while (offset < codepoints.size()) {
            codepoints_buffer.clear();
            uint32_t codepoint = codepoints[offset];
            uint32_t codepoint_next = (offset + 1 < codepoints.size()) ? codepoints[offset + 1] : 0xFFFFFFFF;

            //'s|'t|'re|'ve|'m|'ll|'d
            if (basic_match(codepoint_rules_1, codepoints, results, offset)) {
                continue;
            }
            // ?\p{L}+
            else if (unicode_engine.is_category(codepoint, "LETTER") || codepoint == 32 && unicode_engine.is_category(codepoint_next, "LETTER")) {
                codepoints_buffer.push_back(codepoint);
                offset++;
                while (offset < codepoints.size() && unicode_engine.is_category(codepoints[offset], "LETTER")) {
                    codepoints_buffer.push_back(codepoints[offset]);
                    offset++;
                }
            }
            // ?\p{N}+
            else if (unicode_engine.is_category(codepoint, "NUMBER") || codepoint == 32 && unicode_engine.is_category(codepoint_next, "NUMBER")) {
                codepoints_buffer.push_back(codepoint);
                offset++;
                while (offset < codepoints.size() && unicode_engine.is_category(codepoints[offset], "NUMBER")) {
                    codepoints_buffer.push_back(codepoints[offset]);
                    offset++;
                }
            }
            // ?[^\s\p{L}\p{N}]+
            else if (!unicode_engine.is_category(codepoint, codepoint_rules_2) || codepoint == 32 && !unicode_engine.is_category(codepoint_next, codepoint_rules_2)) {
                codepoints_buffer.push_back(codepoint);
                offset++;
                while (offset < codepoints.size() && !unicode_engine.is_category(codepoints[offset], codepoint_rules_2)) {
                    codepoints_buffer.push_back(codepoints[offset]);
                    offset++;
                }
            }
            //\s+(?!\S)|\s+
            else if (unicode_engine.is_category(codepoint, "WHITESPACE")) {
                codepoints_buffer.push_back(codepoint);
                offset++;
                while (offset < codepoints.size() && unicode_engine.is_category(codepoints[offset], "WHITESPACE")) {
                    if (offset + 1 < codepoints.size() && !unicode_engine.is_category(codepoints[offset+1], "WHITESPACE")) { break;}
                    codepoints_buffer.push_back(codepoints[offset]);
                    offset++;
                }
            } else {
                offset++;
            }

            if (!codepoints_buffer.empty()) {
                results.push_back(unicode_engine.to_string(codepoints_buffer));
            }
        }

        return results;
    }

    llm_regex() {
        unicode_engine.overload_category(REGEX_RANGES::Whitespace, "WHITESPACE");
    }
private:
    UNICODE unicode_engine;

    // Very basic match no metacharacter support
    bool basic_match(const std::vector<std::vector<uint32_t>> & codepoint_rules,
                     const std::vector<uint32_t> & codepoints,
                     std::vector<std::string> & output,
                     size_t & offset) {

        for (auto & codepoint_rule : codepoint_rules) {
            bool satisfy = true;
            for (size_t ru_index = 0; ru_index < codepoint_rule.size(); ru_index++) {
                if (offset + ru_index >= codepoints.size() || codepoint_rule[ru_index] != codepoints[offset + ru_index]) {
                    satisfy = false;
                    break;
                }
            }
            if (satisfy) {
                output.push_back(unicode_engine.to_string(codepoint_rule));
                offset += codepoint_rule.size();
                return true;
            }
        }

        return false;
    }
};
