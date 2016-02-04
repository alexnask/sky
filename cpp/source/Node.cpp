#include <AST/Node.hpp>

#include <algorithm>

std::string tokNames[] = { "WHITESPACE", "NEWLINE", "SEMICOLON", "COMMA", "COLON", "DOUBLE_COLON", "ELLIPSIS", "BRACK_OPEN", "BRACK_CLOSE", "PAREN_OPEN", "PAREN_CLOSE", "DOT", "CURLY_OPEN", "CURLY_CLOSE", "ARROW", "MATCH", "CASE", "IS", "ALIAS", "FROM", "STRUCT", "VARIANT", "IF", "ELSE", "WHILE", "FOR", "BREAK", "CONTINUE", "FUNC", "OPERATOR", "DEFER", "USING", "NAMESPACE", "RETURN", "INLINE", "EXTERN", "STATIC", "USE", "IMPORT", "VERSION", "UNARY", "BINARY", "SIZEOF", "AS", "FUNC_TYPE", "CLOSURE", "OP_PLUS", "OP_MINUS", "OP_TIMES", "OP_DIV", "OP_MOD", "OP_BANG", "OP_NEQ", "OP_EQ", "OP_PLUS_EQ", "OP_MINUS_EQ", "OP_TIMES_EQ", "OP_DIV_EQ", "OP_MOD_EQ", "OP_LESS", "OP_GREATER", "OP_LESS_EQ", "OP_GREATER_EQ", "OP_BIT_OR", "OP_LOG_OR", "OP_LOG_AND", "OP_BIT_AND", "OP_BIT_NOT", "OP_ASS", "OP_BIT_XOR", "OP_LOGICAL_SHIFT_RIGHT", "OP_LOGICAL_SHIFT_LEFT", "OP_ARITHM_SHIFT_RIGHT", "OP_ARITHM_SHIFT_LEFT", "OP_BIT_AND_EQ", "OP_BIT_XOR_EQ", "OP_BIT_OR_EQ", "OP_INF_ASS", "STRING_LITERAL", "INT_LITERAL", "FLOAT_LITERAL", "CHARACTER_LITERAL", "BOOL_LITERAL", "NULL_LITERAL", "IDENTIFIER", "USE_LIB", "UNIT_PATH", "END" };

bool void_type(Type *type) {
    return !type || type->isVoid();
}

char char_toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return 'A' + c - 'a';
    }
    return c;
}

void toupper(std::string& seq) {
    std::transform(seq.begin(), seq.end(), seq.begin(), char_toupper);
}

std::string float_type(std::string& in) {
    int size = in.size();

    if (size <= 4) {
        return "float64";
    }

    auto letter = in[size - 3];
    if (letter == 'F') {
        return "float" + in.substr(size - 2);
    }

    return "float64";
}

long double float_value(const std::string& in) {
    std::string value;

    for (auto c: in) {
        if (c == '_') {
            continue;
        }

        value += c;
    }

    return std::stold(value);
}

int int_base(std::string& in) {
    if (in[0] == '0') {
        if (in.size() == 1) {
            return 10;
        }

        auto base = in[1];

        if (base >= '0' && base <= '9') {
            return 10;
        }

        in = in.substr(2);

        if (base == 'O') {
            return 8;
        } else if (base == 'X') {
            return 16;
        } else if (base == 'B') {
            return 2;
        }
    }

    return 10;
}

std::string int_type(std::string& in) {
    int size = in.size();

    if (size <= 4) {
        return "int64";
    }

    auto letter = in[size - 3];
    bool _signed;
    if (letter == 'S') {
        _signed = true;
    } else if (letter == 'U') {
        _signed = false;
    } else {
        return "int64";
    }

    std::string buff;
    if (!_signed) {
        buff += 'u';
    }

    buff += "int";
    buff += in.substr(size - 2);

    return buff;
}
#include <iostream>
int64_t int_value(int base, const std::string& in) {
    std::string value;

    for (auto c: in) {
        if (c == '_') {
            continue;
        }

        value += c;
    }

    return std::stoll(value, 0, base);
}

bool num_is_positive(std::string& in) {
    if (in[0] == '+') {
        in = in.substr(1);
        return true;
    } else if (in[0] == '-') {
        in = in.substr(1);
        return false;
    }

    return true;
}

bool getCharacter(std::string seq, char *chr) {
    switch (seq[0]) {
        case '\'':
            *chr = '\'';
        break;
        case '"':
            *chr = '"';
        break;
        case '0':
            *chr = '\0';
        break;
        case 'b':
            *chr = '\b';
        break;
        case 'f':
            *chr = '\f';
        break;
        case 'n':
            *chr = '\n';
        break;
        case 'r':
            *chr = '\r';
        break;
        case 't':
            *chr = '\t';
        break;
        case 'v':
            *chr = '\v';
        break;
        case 'x':
            if (seq.size() >= 3) {
                toupper(seq);
                *chr = '\0';
                int power = 1;
                for (int i = 0; i < 2; ++i) {
                    char value = '\0';
                    if (seq[2 - i] >= 'A' && seq[2 - i] <= 'F') {
                        value = 10 + (seq[2 - i] - 'A');
                    } else if (seq[2 - i] >= '0' && seq[2 - i] <= '9') {
                        value = seq[2 - i] - '0';
                    } else {
                        return false;
                    }
                    *chr += power * value;
                    power *= 16;
                }
                return true;
            } else {
                return false;
            }
        break;
        default:
            if (seq[0] >= '0' && seq[0] < '8') {
                *chr = '\0';
                int octLen = seq.size() - 1;
                int power = 1;
                for (int i = 0; i <= octLen; ++i) {
                    char value = '\0';
                    if (seq[octLen - i] >= '0' && seq[octLen - i] < '8') {
                        value = seq[octLen - i] - '0';
                    } else {
                        return false;
                    }
                    *chr += power * value;
                    power *= 8;
                }
                return true;
            }
        break;
    }
    return false;
}

std::string unescape_string(std::string input) {
    std::string buff;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\\') {
            // Escape sequence starting
            i++;
            auto j = i;
        
            if (input[i] >= '0' && input[i] < '8') {
                // Octal
                while (j < input.size() && input[j] >= '0' && input[j] < '8') {
                    j++;
                }
            } else if (input[i] == 'x') {
                // Hexadecimal
                j += 3;
            } else {
                // One character
                j++;
            }

            char chr;
            if (getCharacter(input.substr(i, j), &chr) == true) {
                // Valid escape sequence
                buff += chr;
            } else {
                // Welp, just append what comes after '\'
                buff += input.substr(i, j);
            }
            i = j;
        } else {
            buff += input[i];
            i++;
        }
    }
    return buff;
}
