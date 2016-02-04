#include <Lexer.hpp>
#include <Token_ids.hpp>
#include <Parser.hpp>

#include <Options.hpp>

#include <ASTDumper.hpp>

#include <cstdio>
#include <iostream>
#include <vector>
#include <stdexcept>

void dump_token_stream(const std::vector<Token>& stream) {
    for (auto& tok: stream) {
        std::cout << tokNames[tok.id] << ' ';
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "rb");

        if (file) {
            Options::get().read(argc, argv);

            fseek(file, 0, SEEK_END);

            int length = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *buffer = new char[length + 1];

            fread(buffer, 1, length, file);
            buffer[length] = 0;

            fclose(file);

            Lexer lexer(buffer);
            std::vector<Token> tokens;

            Token curr;
            do {
                curr = lexer.nextToken();
                tokens.emplace_back(curr);
            } while (curr.id != END);

            // Version pass here.
            // Evaluates versions, keeps tokens we want
            // This means version is context free, you can put it anywhere in your code
            // (just be careful with it please)

            std::cout << "Token stream (" << tokens.size() << " tokens): " << std::endl;
            dump_token_stream(tokens);
            std::cout << std::endl;

            Parser parser(&tokens.front());
            auto u = parser.unit(std::string(argv[1]), buffer);

            ASTDumper(&*u, "out.dot");
        }
    }

    return 0;
}

