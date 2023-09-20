#pragma once

#include <assert.h>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <fstream>

namespace SimpleJson {
//极简json解析库，并没有完全实现json的所有解析功能，任何格式错误都可能导致死循环或者崩溃
struct json_null {};

enum JsonType { Null = 0, Bool = 1, Number = 2, String = 3, List = 4, Map = 5 };

class JsonObject {
public:
    std::variant<json_null, bool, double, std::string, std::vector<std::unique_ptr<JsonObject>>,
                 std::unordered_map<std::string, std::unique_ptr<JsonObject>>>
        inner;

    JsonType get_type() const { return (JsonType)inner.index(); }

    const std::unique_ptr<JsonObject>& operator[](size_t i) const{
        return std::get<List>(inner)[i];
    }

    const std::unique_ptr<JsonObject>& operator[](const std::string& key) const{
        return std::get<JsonType::Map>(inner).at(key);
    }

    bool is_null() const{
        return inner.index() == Null;
    }

    const std::vector<std::unique_ptr<JsonObject>>& get_list() const{
        return std::get<List>(inner);
    }

    const std::unordered_map<std::string, std::unique_ptr<JsonObject>>& get_map() const{
        return std::get<JsonType::Map>(inner);
    }

    const std::string& get_string() const{
        return std::get<JsonType::String>(inner);
    }

    double get_number() const{
        return std::get<JsonType::Number>(inner);
    }

    bool get_bool() const{
        return std::get<JsonType::Bool>(inner);
    }

    friend std::ostream &operator<<(std::ostream &os, const JsonObject &json) {
        switch (json.get_type()) {

        case Null:{
            os << "null";
            break;
        }
        case Bool:{
            os << (json.get_bool() ? "true" : "false");
            break;
        }
        case Number:{
            os << json.get_number();
            break;
        }
        case String:{
            os << '\"' << json.get_string() << '\"';
            break;
        }
        case List:{
            os << "[";
            bool is_first = true;
            for(const auto& i : json.get_list()){
                if (!is_first){
                    os << ", ";
                } else {
                    is_first = false;
                }
                os << *i;
            }
            os << "]";
            break;
        }
        case Map:
            os << "{";
            bool is_first = true;
            for(const auto& [key, i] : json.get_map()){
                if (!is_first){
                    os << ", ";
                } else {
                    is_first = false;
                }
                os << '\"' << key << '\"' << ": " << *i;
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

inline std::string parse_string(const char **const start) {
    assert(**start == '\"');
    std::string str;
    (*start)++;
    for (; **start != '\"'; (*start)++) {
        str.push_back(**start);
    }
    (*start)++; // 跳过后面的"号
    return str;
}

inline bool is_number(char c) { return c <= '9' && c >= '0'; }
inline double parse_number(const char **const start) {
    assert(is_number(**start) || **start == '.' || **start == '-');
    double sign = 1.0;
    if (**start == '-'){
        sign = -1.0;
        (*start)++;
    }
    size_t before_dot = 0;
    for (; is_number(**start); (*start)++) {
        before_dot *= 10;
        before_dot += (**start - '0');
    }
    double after_dot = 0;
    double scale = 0.1;
    if (**start == '.') {
        (*start)++;
        for (; is_number(**start); (*start)++) {
            after_dot += (**start - '0') * scale;
            scale /= 10.0;
        }
    }

    return (before_dot + after_dot) * sign;
}

inline std::unique_ptr<JsonObject> parse_object(const char **const start);

inline std::unordered_map<std::string, std::unique_ptr<JsonObject>> parse_map(const char **const start) {
    assert(**start == '{');
    (*start)++;
    std::unordered_map<std::string, std::unique_ptr<JsonObject>> inner_map;
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

inline std::vector<std::unique_ptr<JsonObject>> parse_list(const char **const start) {
    assert(**start == '[');
    (*start)++;
    skip_empty(start);

    std::vector<std::unique_ptr<JsonObject>> list;
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

inline std::unique_ptr<JsonObject> parse_object(const char **const start) {
    std::unique_ptr<JsonObject> object = std::make_unique<JsonObject>();
    switch (**start) {
    case '\"': {
        object->inner = parse_string(start);
        break;
    }
    case '{': {
        object->inner = parse_map(start);
        break;
    }
    case '[': {
        object->inner = parse_list(start);
        break;
    }
    case 'n': {
        match_and_skip(start, "null");
        object->inner.emplace<json_null>();
        break;
    }
    case 't': {
        match_and_skip(start, "true");
        object->inner = true;
        break;
    }
    case 'f': {
        match_and_skip(start, "false");
        object->inner = false;
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
    case '.':
    case '-':
    {
        object->inner = parse_number(start);
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

inline std::unique_ptr<JsonObject> parse(const std::string &json_str) {
    std::unique_ptr<JsonObject> obj;

    const char* start = json_str.c_str();
    Impl::skip_empty(&start);
    if (*start == '\0'){
        obj = std::make_unique<JsonObject>();
    } else {
        obj = Impl::parse_object(&start);
    }

    assert(*start == '\0');

    return obj;
}

inline std::unique_ptr<JsonObject> parse_stream(std::istream& stream) {
    std::string str((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return parse(str);
}

inline std::unique_ptr<JsonObject> parse_file(const std::string &path) {
    std::ifstream file(path);
    return parse_stream(file);
}
} // namespace SimpleJson