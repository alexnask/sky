#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <Lexer.hpp>
#include <AST/Unit.hpp>

#include <stdexcept>
#include <iostream>
#include <string>

enum class ErrorLevel {
    Error,
    Warning
};

class ErrorHandler {
public:
    virtual void report(const std::string& message, ErrorLevel level = ErrorLevel::Error) = 0;
    virtual void report(Unit *unit, const Token& token, const std::string& message, ErrorLevel level = ErrorLevel::Error) = 0;
};

class ErrorGobbler : public ErrorHandler {
public:
    ErrorGobbler() {};

    void report(const std::string& message, ErrorLevel level = ErrorLevel::Error) {}
    void report(Unit *unit, const Token& token, const std::string& message, ErrorLevel level = ErrorLevel::Error) {}
};

class DefaultErrorHandler : public ErrorHandler {
public:
    DefaultErrorHandler() {};

    void report(const std::string& message, ErrorLevel level = ErrorLevel::Error) {
        if (level == ErrorLevel::Error) {
            throw std::runtime_error(message);
        } else {
            std::cout << message << std::endl;
        }
    }

    void report(Unit *unit, const Token& token, const std::string& message, ErrorLevel level = ErrorLevel::Error) {
        std::string buff;

        buff += "In unit " + unit->unit_path + ':' + std::to_string(token.line) + ':' + std::to_string(token.column) + ", ";

        if (level == ErrorLevel::Error) {
            buff += "error";
        } else {
            buff += "warning";
        }

        buff += ": \n" + message + '\n';

        // Add a clang-style token view
        char *contents = unit->contents;
        char *start = token.start;

        if (contents && start) {
            std::string tokenview;
            int offset = start - contents;

            // This tries to find a portion of the string before the token's start to append
            if (offset > 0) {
                for (char *i = start; i >= contents; --i) {
                    if (*i == '\n' || ((*i == ' ' || *i == '\t') && start - i >= 10)) {
                        tokenview += std::string(i + 1, start - i - 1);
                        offset = start - i - 1;
                        break;
                    }
                }
            }

            tokenview += std::string(token.start, token.length);

            // Same as above, but after the token
            start = start + token.length;
            char *i = start;
            while (*i != '\0') {
                if (*i == '\n' || ((*i == '\t' || *i == ' ') && i - start >= 10)) {
                    tokenview += std::string(start, i - start);
                    break;
                }

                ++i;
            }

            buff += '\n' + tokenview + '\n';

            if (offset > 0) {
                buff += std::string(offset, ' ');
            }

            buff += std::string(token.length, '~');
            buff += '\n';
        }

        report(buff, level);
    }
};

#endif
