#include <Lexer.hpp>
#include <Token_ids.hpp>

#include <cstring>
#include <string>

#include <iostream>
#include <stdexcept>

Token Token::empty = { -1, -1, -1, nullptr, -1 };

Lexer::Lexer (const char *_input) : input(_input), end(_input + strlen(_input)), cursor(input), marker(nullptr), ctxmarker(nullptr) {}

void Lexer::nextLine() {
    column = 1;
    line++;
}

void Lexer::string_newline_error() {
    throw std::runtime_error("String literal at line " + std::to_string(line) + ", column " + std::to_string(column) + " interrupted by newline.");
}

void Lexer::multi_line_comm_end_error() {
    throw std::runtime_error("Multi line comment at line " + std::to_string(line) + ", column " + std::to_string(column) + " never ends (expected closing */).");
}

void Lexer::unknown_error() {
    // Try to read until a space, newline or EOF
    const char *errEnd = cursor - 1;
    while (*errEnd != ' ' && *errEnd != '\t' && *errEnd != '\r' && *errEnd != '\n' && *errEnd != 0) {
        errEnd++;
    }

    std::string errTok(cursor - 1, errEnd - cursor + 1);

    throw std::runtime_error("At line " + std::to_string(line) + ", column " + std::to_string(column) + ", unexpected token \"" + errTok + '"');
}

void Lexer::string_end_error() {
    throw std::runtime_error("String literal at line " + std::to_string(line) + ", column " + std::to_string(column) + " never ends (expected closing \").");
}

/*!max:re2c */

Token Lexer::nextToken() {
    const char *start = cursor;

    #define YYCTYPE     char
    #define YYCURSOR    cursor
    #define YYLIMIT     end
    #define YYMARKER    marker
    #define YYCTXMARKER ctxmarker
    #define YYGETCONDITION() condition
    #define YYSETCONDITION(c) condition = c

    #define TOKEN(id) (column += cursor - start, Token { id, line, column - (cursor - start), (char*) start, cursor - start })
    #define TOKEN_(id) { id, line, column, (char*) start, cursor - start }

    /*!re2c
        re2c:indent:top      = 1;
        re2c:yyfill:enable   = 0;

        newline = "\r\n" | '\r' | '\n';
        whitespace = ' ' | '\t';

        // "System" rules
        <*> *                           { unknown_error();   }
        <CODE, SINGLE_LINE_COMM> '\000' { return TOKEN(END); }

        // Comment ignoring
        <CODE> "//"    :=> SINGLE_LINE_COMM
        <CODE> [/] [*] :=> MULTI_LINE_COMM

        // Miscellaneous
        <CODE> (' ' | '\t')+ { return TOKEN(WHITESPACE);           }
        <CODE> ';'           { return TOKEN(SEMICOLON);            }
        <CODE> newline       { nextLine(); return TOKEN_(NEWLINE); }

        <CODE> ','   { return TOKEN(COMMA);       }
        <CODE> '::'  { return TOKEN(DOUBLE_COLON);}
        <CODE> ':'   { return TOKEN(COLON);       }
        <CODE> '...' { return TOKEN(ELLIPSIS);    }
        <CODE> '['   { return TOKEN(BRACK_OPEN);  }
        <CODE> ']'   { return TOKEN(BRACK_CLOSE); }
        <CODE> '('   { return TOKEN(PAREN_OPEN);  }
        <CODE> ')'   { return TOKEN(PAREN_CLOSE); }
        <CODE> '.'   { return TOKEN(DOT);         }
        <CODE> '{'   { return TOKEN(CURLY_OPEN);  }
        <CODE> '}'   { return TOKEN(CURLY_CLOSE); }
        <CODE> '->'  { return TOKEN(ARROW);       }

        // Keywords
        <CODE> "match"     { return TOKEN(MATCH);     }
        <CODE> "case"      { return TOKEN(CASE);      }
        <CODE> "is"        { return TOKEN(IS);        }
        <CODE> "alias"     { return TOKEN(ALIAS);     }
        <CODE> "from"      { return TOKEN(FROM);      }
        <CODE> "struct"    { return TOKEN(STRUCT);    }
        <CODE> "variant"   { return TOKEN(VARIANT);   }
        <CODE> "if"        { return TOKEN(IF);        }
        <CODE> "else"      { return TOKEN(ELSE);      }
        <CODE> "while"     { return TOKEN(WHILE);     }
        <CODE> "for"       { return TOKEN(FOR);       }
        <CODE> "break"     { return TOKEN(BREAK);     }
        <CODE> "continue"  { return TOKEN(CONTINUE);  }
        <CODE> "func"      { return TOKEN(FUNC);      }
        <CODE> "operator"  { return TOKEN(OPERATOR);  }
        <CODE> "defer"     { return TOKEN(DEFER);     }
        <CODE> "using"     { return TOKEN(USING);     }
        <CODE> "namespace" { return TOKEN(NAMESPACE); }
        <CODE> "return"    { return TOKEN(RETURN);    }
        <CODE> "inline"    { return TOKEN(INLINE);    }
        <CODE> "extern"    { return TOKEN(EXTERN);    }
        <CODE> "static"    { return TOKEN(STATIC);    }
        <CODE> "version"   { return TOKEN(VERSION);   }
        <CODE> "unary"     { return TOKEN(UNARY);     }
        <CODE> "binary"    { return TOKEN(BINARY);    }
        <CODE> "sizeof"    { return TOKEN(SIZEOF);    }
        <CODE> "as"        { return TOKEN(AS);        }
        <CODE> "Func"      { return TOKEN(FUNC_TYPE); }
        <CODE> "Closure"   { return TOKEN(CLOSURE);   }
        <CODE> "use"    => USE_LIB   { return TOKEN(USE);    }
        <CODE> "import" => UNIT_PATH { return TOKEN(IMPORT); }

        // Operators
        <CODE> '+'   { return TOKEN(OP_PLUS);                }
        <CODE> '-'   { return TOKEN(OP_MINUS);               }
        <CODE> '*'   { return TOKEN(OP_TIMES);               }
        <CODE> '/'   { return TOKEN(OP_DIV);                 }
        <CODE> '%'   { return TOKEN(OP_MOD);                 }
        <CODE> '!'   { return TOKEN(OP_BANG);                }
        <CODE> '!='  { return TOKEN(OP_NEQ);                 }
        <CODE> '=='  { return TOKEN(OP_EQ);                  }
        <CODE> '+='  { return TOKEN(OP_PLUS_EQ);             }
        <CODE> '-='  { return TOKEN(OP_MINUS_EQ);            }
        <CODE> '*='  { return TOKEN(OP_TIMES_EQ);            }
        <CODE> '/='  { return TOKEN(OP_DIV_EQ);              }
        <CODE> '%='  { return TOKEN(OP_MOD_EQ);              }
        <CODE> '<'   { return TOKEN(OP_LESS);                }
        <CODE> '>'   { return TOKEN(OP_GREATER);             }
        <CODE> '<='  { return TOKEN(OP_LESS_EQ);             }
        <CODE> '>='  { return TOKEN(OP_GREATER_EQ);          }
        <CODE> '|'   { return TOKEN(OP_BIT_OR);              }
        <CODE> '||'  { return TOKEN(OP_LOG_OR);              }
        <CODE> '&&'  { return TOKEN(OP_LOG_AND);             }
        <CODE> '&'   { return TOKEN(OP_BIT_AND);             }
        <CODE> '~'   { return TOKEN(OP_BIT_NOT);             }
        <CODE> '='   { return TOKEN(OP_ASS);                 }
        <CODE> '^'   { return TOKEN(OP_BIT_XOR);             }
        <CODE> "shr" { return TOKEN(OP_LOGICAL_SHIFT_RIGHT); }
        <CODE> "shl" { return TOKEN(OP_LOGICAL_SHIFT_LEFT);  }
        <CODE> "sar" { return TOKEN(OP_ARITHM_SHIFT_RIGHT);  }
        <CODE> "sal" { return TOKEN(OP_ARITHM_SHIFT_LEFT);   }
        <CODE> '&='  { return TOKEN(OP_BIT_AND_EQ);          }
        <CODE> '^='  { return TOKEN(OP_BIT_XOR_EQ);          }
        <CODE> '|='  { return TOKEN(OP_BIT_OR_EQ);           }
        <CODE> ':='  { return TOKEN(OP_INF_ASS);             }

        // Literals
        <CODE> '"' :=> STRING

        octalLiteral   = '0o' ([0 - 7] ('_' [0 - 7])*)+;
        hexLiteral     = '0x' ([0-9a-fA-F] ('_' [0-9a-fA-F])*)+;
        binaryLiteral  = '0b' ([0-1] ('_' [0-1])*)+;
        // This is a bit complex because doing it like above results in matching to zero + identifier for octal hex and binary numbers.
        decimalLiteral = ('0' \ [oxbd]) | ('0d'? ([0-9] ('_' [0-9])*)+);
        intSuffix = 's8' | 's16' | 's32' | 's64' | 'u8' | 'u16' | 'u32' | 'u64';

        <CODE> ('-' | '+')? (decimalLiteral | octalLiteral | hexLiteral | binaryLiteral) ('_'? intSuffix)?  { return TOKEN(INT_LITERAL);       }
        <CODE> ('-' | '+')? ([0-9] ('_' [0-9])*)* '.' ([0-9] ('_' [0-9])*)+ ('_'? ('f32' | 'f64' | 'f80'))? { return TOKEN(FLOAT_LITERAL);     }
        <CODE> "true" | "false"                                                                { return TOKEN(BOOL_LITERAL);      }
        <CODE> "'" (. | "\\'" | "\\" .) "'"                                                    { return TOKEN(CHARACTER_LITERAL); }
        <CODE> "null"                                                                          { return TOKEN(NULL_LITERAL);      }

        // Identifiers
        <CODE> [_a-zA-Z]+ [0-9_a-zA-Z]* { return TOKEN(IDENTIFIER); }

        // Comments
        <SINGLE_LINE_COMM> ("\r\n" | '\n' | '\r') => CODE { nextLine(); start = cursor; goto yyc_CODE; }
        <SINGLE_LINE_COMM> *                              { goto yyc_SINGLE_LINE_COMM;                 }

        <MULTI_LINE_COMM> [*] [/] => CODE { start = cursor; goto yyc_CODE;        }
        <MULTI_LINE_COMM> newline         { nextLine(); goto yyc_MULTI_LINE_COMM; }
        <MULTI_LINE_COMM> '\000'          { multi_line_comm_end_error();          }
        <MULTI_LINE_COMM> *               { column++; goto yyc_MULTI_LINE_COMM;   }

        // String literal
        <STRING> '"' => CODE { return TOKEN(STRING_LITERAL); }
        <STRING> "\\\""      { goto yyc_STRING;              }
        <STRING> '\000'      { string_end_error();           }
        <STRING> newline     { string_newline_error();       }
        <STRING> *           { goto yyc_STRING;              }

        basePath = ([0-9a-zA-Z] | '-' | '.')+;

        // Use library spec
        // We accept characters [0-9a-zA-Z_] '-' '.' and colon separators, until we hit a '/' slash
        <USE_LIB> whitespace+ { return TOKEN(WHITESPACE); }
        <USE_LIB> basePath (':' basePath)* { return TOKEN(USE_LIB); }
        <USE_LIB> [/] [*] :=> MULTI_LINE_COMM
        <USE_LIB> '/' :=> UNIT_PATH
        <USE_LIB> newline => CODE { nextLine(); goto yyc_CODE; }

        // Use library path
        <USE_LIB> [/] [*] :=> MULTI_LINE_COMM
        <UNIT_PATH> newline => CODE { nextLine(); goto yyc_CODE; }
        <UNIT_PATH> whitespace+ { return TOKEN(WHITESPACE); }
        <UNIT_PATH> basePath ('/' basePath)* ('/' whitespace* '[' whitespace* basePath (whitespace* ',' whitespace* basePath)* whitespace* ']')? => CODE { return TOKEN(UNIT_PATH); }
    */

    #undef TOKEN_
    #undef TOKEN
}

bool Lexer::done() const {
    return end + 1 == cursor;
}
