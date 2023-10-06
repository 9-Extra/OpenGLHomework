#pragma once

#include <assert.h>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// Mingw的ifstream不知道为什么导致了崩溃，手动实现文件读取
std::string read_whole_file(const std::string &path);

namespace SimpleJson {
// 极简json解析库，任何格式错误都可能导致死循环或者崩溃
struct json_null {};

enum JsonType { Null = 0, Bool = 1, Number = 2, String = 3, List = 4, Map = 5 };

class JsonObject {
public:
    std::unique_ptr<std::variant<json_null, bool, double, std::string, std::vector<JsonObject>,
                                 std::unordered_map<std::string, JsonObject>>>
        inner;

    JsonObject() : inner(std::make_unique<decltype(inner)::element_type>()) {}

    JsonType get_type() const { return (JsonType)inner->index(); }
    const JsonObject &operator[](size_t i) const { return std::get<List>(*inner)[i]; }
    const JsonObject &operator[](const std::string &key) const { return std::get<JsonType::Map>(*inner).at(key); }
    bool has(const std::string &key) const {
        const std::unordered_map<std::string, JsonObject> &m = std::get<JsonType::Map>(*inner);
        return m.find(key) != m.end();
    }
    bool is_null() const { return inner->index() == Null; }
    const std::vector<JsonObject> &get_list() const { return std::get<List>(*inner); }
    const std::unordered_map<std::string, JsonObject> &get_map() const { return std::get<JsonType::Map>(*inner); }
    const std::string &get_string() const { return std::get<JsonType::String>(*inner); }
    double get_number() const { return std::get<JsonType::Number>(*inner); }
    uint64_t get_uint() const { return (uint64_t)get_number(); }
    int64_t get_int() const { return (int64_t)get_number(); }
    bool get_bool() const { return std::get<JsonType::Bool>(*inner); }

    friend std::ostream &operator<<(std::ostream &os, const JsonObject &json) {
        switch (json.get_type()) {

        case Null: {
            os << "null";
            break;
        }
        case Bool: {
            os << (json.get_bool() ? "true" : "false");
            break;
        }
        case Number: {
            os << json.get_number();
            break;
        }
        case String: {
            os << '\"' << json.get_string() << '\"';
            break;
        }
        case List: {
            os << "[";
            bool is_first = true;
            for (const auto &i : json.get_list()) {
                if (!is_first) {
                    os << ", ";
                } else {
                    is_first = false;
                }
                os << i;
            }
            os << "]";
            break;
        }
        case Map:
            os << "{";
            bool is_first = true;
            for (const auto &[key, i] : json.get_map()) {
                if (!is_first) {
                    os << ", ";
                } else {
                    is_first = false;
                }
                os << '\"' << key << '\"' << ": " << i;
            }
            os << "}";
            break;
        }
        return os;
    }
};

namespace Impl {
inline void skip_empty(const char **const start) {
    for (; **start == '\n' || **start == '\r' || **start == '\t' || **start == ' '; (*start)++)
        ;
}
inline void match_and_skip(const char **const start, const char *str) {
    for (; *str != '\0'; str++, (*start)++) {
        assert(**start == *str);
    }
}

inline char from_hex(char c) {
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        return c - '0';
    }
    case 'a':
    case 'A': {
        return 0xa;
    }
    case 'b':
    case 'B': {
        return 0xb;
    }
    case 'c':
    case 'C': {
        return 0xc;
    }
    case 'd':
    case 'D': {
        return 0xd;
    }
    case 'e':
    case 'E': {
        return 0xe;
    }
    case 'f':
    case 'F': {
        return 0xf;
    }

    default: {
        assert(false); // 错啦
        return 0;
    }
    }
}

inline std::string parse_string(const char **const start) {
    assert(**start == '\"');
    std::string str;
    (*start)++;
    for (; **start != '\"'; (*start)++) {
        // 转义字符特殊处理
        if (**start == '\\') {
            (*start)++;
            switch (**start) {
            case '\"': {
                str.push_back('\"');
                break;
            }
            case '\\': {
                str.push_back('\\');
                break;
            }

            case 'n': {
                str.push_back('\n');
                break;
            }
            case 'r': {
                str.push_back('\r');
                break;
            }
            case 't': {
                str.push_back('\t');
                break;
            }
            case 'f': {
                str.push_back('\f');
                break;
            }
            case 'b': {
                str.push_back('\b');
                break;
            }
            case 'u': {
                char c1 = (from_hex(*(*start + 1)) << 4) + from_hex(*(*start + 2));
                *start += 2;
                char c2 = (from_hex(*(*start + 1)) << 4) + from_hex(*(*start + 2));
                *start += 2;
                str.push_back(c1);
                str.push_back(c2);
                break;
            }
            }
        } else {
            str.push_back(**start);
        }
    }
    (*start)++; // 跳过后面的"号
    return str;
}

inline bool is_number(char c) { return c <= '9' && c >= '0'; }
inline double parse_number(const char **const start) {
    assert(is_number(**start) || **start == '.' || **start == '-');
    // 解析符号
    bool positive = true;
    if (**start == '-') {
        positive = false;
        (*start)++;
    }
    // 解析整数
    uint64_t base_num = 0;
    int exp_num = 0;
    for (; is_number(**start); (*start)++) {
        base_num *= 10;
        base_num += (**start - '0');
    }
    // 解析小数
    if (**start == '.') {
        (*start)++;
        for (; is_number(**start); (*start)++) {
            base_num *= 10;
            base_num += (**start - '0');
            exp_num--;
        }
    }
    // 解析指数
    size_t given_exp = 0;
    int exp_sign = 1;
    if (**start == 'E' || **start == 'e') {
        (*start)++;
        if (**start == '-' || **start == '+') {
            if (**start == '-')
                exp_sign = -1;
            (*start)++;
        }
        for (; is_number(**start); (*start)++) {
            given_exp *= 10;
            given_exp += (**start - '0');
        }
    }

    exp_num += exp_sign * given_exp;
    double num = base_num * std::pow<double>(10, exp_num);
    return positive ? num : -num;
}

inline JsonObject parse_object(const char **const start);

inline std::unordered_map<std::string, JsonObject> parse_map(const char **const start) {
    assert(**start == '{');
    (*start)++;
    std::unordered_map<std::string, JsonObject> inner_map;
    skip_empty(start);
    while (**start != '}') {
        std::string key = parse_string(start);
        skip_empty(start);
        assert(**start == ':');
        (*start)++;
        skip_empty(start);
        inner_map.emplace(key, parse_object(start));
        skip_empty(start);
        assert(**start == ',' || **start == '}');
        if (**start == ',') {
            (*start)++;
            skip_empty(start);
        }
    }
    (*start)++;

    return inner_map;
}

inline std::vector<JsonObject> parse_list(const char **const start) {
    assert(**start == '[');
    (*start)++;
    skip_empty(start);

    std::vector<JsonObject> list;
    while (**start != ']') {
        list.push_back(parse_object(start));
        skip_empty(start);
        if (**start == ',') {
            (*start)++;
            skip_empty(start);
        }
    }
    (*start)++;

    return list;
}

inline JsonObject parse_object(const char **const start) {
    JsonObject object;
    switch (**start) {
    case '\"': {
        *object.inner = parse_string(start);
        break;
    }
    case '{': {
        *object.inner = parse_map(start);
        break;
    }
    case '[': {
        *object.inner = parse_list(start);
        break;
    }
    case 'n': {
        match_and_skip(start, "null");
        object.inner->emplace<json_null>();
        break;
    }
    case 't': {
        match_and_skip(start, "true");
        *object.inner = true;
        break;
    }
    case 'f': {
        match_and_skip(start, "false");
        *object.inner = false;
        break;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': {
        *object.inner = parse_number(start);
        break;
    }

    default: {
        assert(false);
        break;
    }
    }

    return object;
}
} // namespace Impl

inline JsonObject parse(const std::string &json_str) {
    JsonObject obj;

    const char *start = json_str.c_str();
    Impl::skip_empty(&start);
    if (*start == '\0') {
        *obj.inner = json_null();
    } else {
        obj = Impl::parse_object(&start);
    }

    Impl::skip_empty(&start);
    assert(*start == '\0'); // 检查是否解析到了文件结束

    return obj;
}

inline JsonObject parse_stream(std::istream &stream) {
    std::string str((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return parse(str);
}

inline JsonObject parse_file(const std::string &path) { return parse(read_whole_file(path)); }
} // namespace SimpleJson