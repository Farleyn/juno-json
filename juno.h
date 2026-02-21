#ifndef JUNO_H
#define JUNO_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <cstdint>

namespace juno {

class Value;

using Null = std::monostate;
using Boolean = bool;
using Integer = int64_t;
using Float = double;
using String = std::string;

using Array = std::vector<Value>;
using Object = std::map<String, Value>;

class Value {
public:
    using Data = std::variant<Null, Boolean, Integer, Float, String, Array, Object>;
    Data data;

    Value() : data(Null{}) {}
    Value(std::nullptr_t) : data(Null{}) {}
    Value(bool v) : data(v) {}
    Value(int v) : data(static_cast<Integer>(v)) {}
    Value(int64_t v) : data(v) {}
    Value(double v) : data(v) {}
    Value(const char* v) : data(String(v)) {}
    Value(const String& v) : data(v) {}
    Value(String&& v) : data(std::move(v)) {}
    Value(const Array& v) : data(v) {}
    Value(Array&& v) : data(std::move(v)) {}
    Value(const Object& v) : data(v) {}
    Value(Object&& v) : data(std::move(v)) {}

    bool is_null() const { return std::holds_alternative<Null>(data); }
    bool is_bool() const { return std::holds_alternative<Boolean>(data); }
    bool is_int() const { return std::holds_alternative<Integer>(data); }
    bool is_float() const { return std::holds_alternative<Float>(data); }
    bool is_number() const { return is_int() || is_float(); }
    bool is_string() const { return std::holds_alternative<String>(data); }
    bool is_array() const { return std::holds_alternative<Array>(data); }
    bool is_object() const { return std::holds_alternative<Object>(data); }

    Boolean as_bool() const { return std::get<Boolean>(data); }
    Integer as_int() const { return std::get<Integer>(data); }
    Float as_float() const {
        if (is_float()) return std::get<Float>(data);
        if (is_int()) return static_cast<Float>(std::get<Integer>(data));
        throw std::runtime_error("Not a number");
    }
    const String& as_string() const { return std::get<String>(data); }
    String& as_string() { return std::get<String>(data); }
    const Array& as_array() const { return std::get<Array>(data); }
    Array& as_array() { return std::get<Array>(data); }
    const Object& as_object() const { return std::get<Object>(data); }
    Object& as_object() { return std::get<Object>(data); }

    Value& operator[](size_t i) { return as_array()[i]; }
    const Value& operator[](size_t i) const { return as_array()[i]; }

    Value& operator[](const String& key) { return as_object()[key]; }
    const Value& operator[](const String& key) const { return as_object().at(key); }

    size_t size() const {
        if (is_array()) return as_array().size();
        if (is_object()) return as_object().size();
        return 0;
    }
};

class Parser {
    const char* input;
    size_t pos;
    
    char current() const { return input[pos]; }
    void advance() { if (input[pos]) pos++; }
    void skip_whitespace() {
        while (std::isspace(current())) advance();
    }
    
    Value parse_value() {
        skip_whitespace();
        char c = current();
        
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(c)) return parse_number();
        
        throw std::runtime_error("Unexpected character");
    }
    
    Value parse_null() {
        if (input[pos] == 'n' && input[pos+1] == 'u' && 
            input[pos+2] == 'l' && input[pos+3] == 'l') {
            pos += 4;
            return Value();
        }
        throw std::runtime_error("Expected 'null'");
    }
    
    Value parse_bool() {
        if (input[pos] == 't') {
            if (input[pos+1] == 'r' && input[pos+2] == 'u' && input[pos+3] == 'e') {
                pos += 4;
                return Value(true);
            }
        } else if (input[pos] == 'f') {
            if (input[pos+1] == 'a' && input[pos+2] == 'l' && 
                input[pos+3] == 's' && input[pos+4] == 'e') {
                pos += 5;
                return Value(false);
            }
        }
        throw std::runtime_error("Expected boolean");
    }
    
    Value parse_number() {
        std::string num;
        bool is_float = false;
        
        if (current() == '-') {
            num += current();
            advance();
        }
        
        while (std::isdigit(current())) {
            num += current();
            advance();
        }
        
        if (current() == '.') {
            is_float = true;
            num += current();
            advance();
            while (std::isdigit(current())) {
                num += current();
                advance();
            }
        }
        
        if (current() == 'e' || current() == 'E') {
            is_float = true;
            num += current();
            advance();
            if (current() == '+' || current() == '-') {
                num += current();
                advance();
            }
            while (std::isdigit(current())) {
                num += current();
                advance();
            }
        }
        
        return is_float ? Value(std::stod(num)) : Value(static_cast<Integer>(std::stoll(num)));
    }
    
    Value parse_string() {
        if (current() != '"') throw std::runtime_error("Expected '\"'");
        advance();
        
        std::string result;
        while (current() != '"' && current() != '\0') {
            if (current() == '\\') {
                advance();
                switch (current()) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: throw std::runtime_error("Invalid escape");
                }
                advance();
            } else {
                result += current();
                advance();
            }
        }
        
        if (current() != '"') throw std::runtime_error("Unterminated string");
        advance();
        return Value(result);
    }
    
    Value parse_array() {
        if (current() != '[') throw std::runtime_error("Expected '['");
        advance();
        
        Array arr;
        skip_whitespace();
        
        if (current() == ']') {
            advance();
            return Value(arr);
        }
        
        while (true) {
            arr.push_back(parse_value());
            skip_whitespace();
            
            if (current() == ',') {
                advance();
            } else if (current() == ']') {
                break;
            } else {
                throw std::runtime_error("Expected ',' or ']'");
            }
        }
        
        if (current() != ']') throw std::runtime_error("Expected ']'");
        advance();
        return Value(arr);
    }
    
    Value parse_object() {
        if (current() != '{') throw std::runtime_error("Expected '{'");
        advance();
        
        Object obj;
        skip_whitespace();
        
        if (current() == '}') {
            advance();
            return Value(obj);
        }
        
        while (true) {
            skip_whitespace();
            Value key_val = parse_string();
            String key = key_val.as_string();
            
            skip_whitespace();
            if (current() != ':') throw std::runtime_error("Expected ':'");
            advance();
            
            obj[key] = parse_value();
            skip_whitespace();
            
            if (current() == ',') {
                advance();
            } else if (current() == '}') {
                break;
            } else {
                throw std::runtime_error("Expected ',' or '}'");
            }
        }
        
        if (current() != '}') throw std::runtime_error("Expected '}'");
        advance();
        return Value(obj);
    }

public:
    Parser(const String& str) : input(str.c_str()), pos(0) {}
    
    Value parse() {
        Value result = parse_value();
        skip_whitespace();
        if (current() != '\0') {
            throw std::runtime_error("Unexpected characters after JSON");
        }
        return result;
    }
};

class Serializer {
    std::ostringstream out;
    int indent_level = 0;
    int indent_size = 2;
    bool compact = false;
    
    void write_indent() {
        if (!compact) {
            out << std::string(indent_level * indent_size, ' ');
        }
    }
    
    void write_newline() {
        if (!compact) out << '\n';
    }
    
    String escape_string(const String& str) {
        std::ostringstream oss;
        oss << '"';
        for (char c : str) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b"; break;
                case '\f': oss << "\\f"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c;
            }
        }
        oss << '"';
        return oss.str();
    }
    
    void write_value(const Value& val) {
        if (val.is_null()) {
            out << "null";
        } else if (val.is_bool()) {
            out << (val.as_bool() ? "true" : "false");
        } else if (val.is_int()) {
            out << val.as_int();
        } else if (val.is_float()) {
            out << val.as_float();
        } else if (val.is_string()) {
            out << escape_string(val.as_string());
        } else if (val.is_array()) {
            write_array(val.as_array());
        } else if (val.is_object()) {
            write_object(val.as_object());
        }
    }
    
    void write_array(const Array& arr) {
        out << '[';
        indent_level++;
        
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) out << ',';
            write_newline();
            write_indent();
            write_value(arr[i]);
        }
        
        indent_level--;
        if (!arr.empty()) {
            write_newline();
            write_indent();
        }
        out << ']';
    }
    
    void write_object(const Object& obj) {
        out << '{';
        indent_level++;
        
        size_t i = 0;
        for (const auto& [key, value] : obj) {
            if (i++ > 0) out << ',';
            write_newline();
            write_indent();
            out << escape_string(key) << ':';
            if (!compact) out << ' ';
            write_value(value);
        }
        
        indent_level--;
        if (!obj.empty()) {
            write_newline();
            write_indent();
        }
        out << '}';
    }

public:
    Serializer(bool c = false) : compact(c) {}
    
    String serialize(const Value& val) {
        write_value(val);
        return out.str();
    }
};

inline Value parse(const String& str) {
    return Parser(str).parse();
}

inline String stringify(const Value& val, bool compact = false) {
    return Serializer(compact).serialize(val);
}

}

#endif
