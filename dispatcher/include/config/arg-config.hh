#pragma once
#include <cstdlib>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

std::vector<std::string> arg_config_split(std::string input, char delimiter) {
    std::vector<std::string> answer;
    std::stringstream ss(input);
    std::string temp;

    while (getline(ss, temp, delimiter)) {
        answer.push_back(temp);
    }

    return answer;
}

#define ARG_STR_DEFAULT(NAME, DEFAULT, NOTE)                        \
    static constexpr std::string_view NAME##_note = NOTE;           \
    static inline std::string NAME##_usage() {                      \
        return "--" #NAME " <string>";                              \
    }                                                               \
    std::string NAME = DEFAULT;                                     \
    inline void NAME##_parse(const char *arg) { this->NAME = arg; } \
    inline std::string NAME##_str() { return this->NAME; }

#define ARG_STR(NAME, NOTE) ARG_STR_DEFAULT(NAME, "", NOTE)

#define ARG_INT_DEFAULT(NAME, DEFAULT, NOTE)                                 \
    static constexpr std::string_view NAME##_note = NOTE;                    \
    static inline std::string NAME##_usage() { return "--" #NAME " <int>"; } \
    long long NAME = DEFAULT;                                                \
    inline void NAME##_parse(const char *arg) {                              \
        this->NAME = std::atoll(arg);                                        \
    }                                                                        \
    inline std::string NAME##_str() { return std::to_string(this->NAME); }

#define ARG_INT(NAME, NOTE) ARG_INT_DEFAULT(NAME, 0, NOTE)

#define ARG_OPTIONS_DEFAULT(NAME, DEFAULT, NOTE, ...)                       \
    static constexpr std::string_view NAME##_note = NOTE;                   \
    static constexpr std::initializer_list<const char *> NAME##_options = { \
        __VA_ARGS__};                                                       \
    static inline std::string NAME##_usage() {                              \
        std::string __arg_options = "";                                     \
        for (auto &__arg_option : NAME##_options) {                         \
            __arg_options += __arg_option;                                  \
            __arg_options += "|";                                           \
        }                                                                   \
        if (!__arg_options.empty()) {                                       \
            __arg_options.pop_back();                                       \
        }                                                                   \
        return std::string("--") + #NAME + " <" + __arg_options + ">";      \
    }                                                                       \
    std::string NAME = DEFAULT;                                             \
    inline void NAME##_parse(const char *arg) { this->NAME = arg; }         \
    inline std::string NAME##_str() { return this->NAME; }

#define ARG_OPTIONS(NAME, NOTE, ...) \
    ARG_OPTIONS_DEFAULT(NAME, "", NOTE, __VA_ARGS__)

#define ARG_INT_LIST_CUSTOM(NAME, NOTE, DEL, BASE, ...)                \
    static constexpr std::string_view NAME##_note = NOTE;              \
    static inline std::string NAME##_usage() {                         \
        return "--" #NAME " <list of int, " #DEL "-separated>";        \
    }                                                                  \
    std::vector<long long> NAME = {__VA_ARGS__};                       \
    inline void NAME##_parse(const char *arg) {                        \
        auto splited = arg_config_split(arg, DEL);                     \
        if (!splited.empty()) {                                        \
            this->NAME.clear();                                        \
            for (auto &stem : splited)                                 \
                this->NAME.push_back(std::stoll(stem, nullptr, BASE)); \
        }                                                              \
    }                                                                  \
    inline std::string NAME##_str() {                                  \
        if (this->NAME.empty()) {                                      \
            return "";                                                 \
        }                                                              \
        std::string result = "";                                       \
        for (int i = 0; i < this->NAME.size() - 1; ++i) {              \
            result += std::to_string(this->NAME[i]);                   \
            result += DEL;                                             \
        }                                                              \
        result += std::to_string(this->NAME[this->NAME.size() - 1]);   \
        return result;                                                 \
    }

#define ARG_INT_LIST(NAME, NOTE) ARG_INT_LIST_CUSTOM(NAME, NOTE, ',', 10)

#define ARG_PARSE_START(argc, argv)                    \
    int __arg_parsing_argc = argc;                     \
    char **__arg_parsing_argv = argv;                  \
    int __arg_parsing_arg_i = 1; /* except for cmd*/   \
    while (__arg_parsing_arg_i < __arg_parsing_argc) { \
        std::string __arg_parsing = __arg_parsing_argv[__arg_parsing_arg_i];
#define ARG_PARSE_END()    \
    ++__arg_parsing_arg_i; \
    }

#define ARG_PARSE(NAME)                                        \
    if (__arg_parsing == "--" #NAME &&                         \
        __arg_parsing_arg_i + 1 < __arg_parsing_argc) {        \
        ++__arg_parsing_arg_i;                                 \
        NAME##_parse(__arg_parsing_argv[__arg_parsing_arg_i]); \
    }
#define ARG_USAGE_START(argc, argv)                    \
    std::string __arg_usage = argc > 0 ? argv[0] : ""; \
    std::string __arg_note = "\n\n"
#define ARG_USAGE(NAME)                  \
    __arg_usage += " ";                  \
    __arg_usage += NAME##_usage();       \
    if (!NAME##_note.empty()) {          \
        __arg_note += "\t--" #NAME ": "; \
        __arg_note += NAME##_note;       \
        __arg_note += "\n";              \
    }

#define ARG_USAGE_END() (__arg_usage + __arg_note);

#define ARG_PRINT_START() std::string __arg_print = ""
#define ARG_PRINT(NAME)              \
    __arg_print += " - " #NAME ": "; \
    __arg_print += NAME##_str();     \
    __arg_print += "\n";

#define ARG_PRINT_END() __arg_print
