#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>

struct Token {
    int id;

    int line;
    int column;

    char *start;
    int length;

    Token concat(const Token& other) {
        return { id, line, column, start, length + other.length };
    }

    static Token concat(Token *start, Token *end) {
        int length = 0;

        for (Token *i = start; i <= end; ++i) {
            length += i->length;
        }

        return { start->id, start->line, start->column, start->start, length };
    }

    std::string value() const {
        return std::string(start, length);
    }

    static Token empty;
};

class Lexer {
public:

    enum YYCONDTYPE {
      yycCODE,
      yycSTRING,
      yycSINGLE_LINE_COMM,
      yycMULTI_LINE_COMM,
      yycUSE_LIB,
      yycUNIT_PATH
    };

    Lexer (const char *_input);
    Token nextToken();
    bool done() const;
    void nextLine();
    void unknown_error();
    void string_newline_error();
    void string_end_error();
    void multi_line_comm_end_error();

private:
    const char *input;
    const char *end;

    const char *cursor;

    // Used by re2c
    const char *marker;
    const char *ctxmarker;

    YYCONDTYPE condition = yycCODE;
    int line = 1;
    int column = 1;
};

#endif
