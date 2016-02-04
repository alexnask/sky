#ifndef PARSER__HPP
#define PARSER__HPP

#include <memory>
#include <string>

#include <Lexer.hpp>
#include <AST/All.hpp>
#include <Errors.hpp>

// Skylang parser that generates an AST

class Parser {
public:
    Parser (Token *_stream);

    // This will return an AST eventually
    std::unique_ptr<Unit> unit(std::string path, char *contents);
private:
    Token *stream;
    Token *cursor;

    Unit *curr_unit;

    bool accept(int id);
    bool accept_rewind(int id);
    bool consumeAll(int id);

    // checks for: Identifier op_ws COLON op_ws ID
    bool decl_of(int id);

    Use *use();
    Import *import();

    Declaration *declaration();
    NamespaceDeclaration *namespace_();

    TypeDeclaration *typeDecl();
    StructDeclaration *structDecl();
    AliasDeclaration *aliasDecl();
    VariantDeclaration *variantDecl();

    bool templateDef(std::vector<TemplateDeclaration>& templates);

    FunctionDeclaration *funcDecl();

    void func_decl_return_type(FunctionDeclaration *fDecl);
    void arglist_def_man_names(FunctionDeclaration *parent, std::vector<std::unique_ptr<VariableDeclaration>>& arglist);
    VariableDeclaration *opt_name_arg();
    void arglist_def_opt_names(FunctionDeclaration *parent, std::vector<std::unique_ptr<VariableDeclaration>>& arglist);

    VariableDeclaration *simpleVariableDecl();

    Type *type();
    BaseType *baseType();
    FunctionType *functionType();
    ClosureType *closureType();
    TupleType *tupleType();

    bool templateInstance(Node *parent, std::vector<std::unique_ptr<Type>>& templates);
    bool closure_function_type_common(std::vector<std::unique_ptr<Type>>& argTypes, Type **retType);

    Scope *scope();
    Statement *statement();

    VariableDeclaration *variableDecl();

    IfStmt *ifStmt();
    WhileStmt *whileStmt();
    ForStmt *forStmt();
    Statement *returnStmt();
    Statement *usingStmt();
    Statement *controlStmt();
    Statement *deferStmt();
    Statement *matchStmt();

    Statement *forInit();
    bool match_case(std::vector<Case>& cases);
    void variable_decl_modifiers(bool& extern_mod, bool& static_mod);

    bool statement_separator();

    // Expressions
    Expression *expression();
    Expression *assignment();
    Expression *ifExpr();
    Expression *logicalOr();
    Expression *logicalAnd();
    Expression *bitOr();
    Expression *bitXor();
    Expression *bitAnd();
    Expression *equality();
    Expression *relational();
    Expression *shift();
    Expression *additive();
    Expression *multiplicative();
    Expression *cast();
    Expression *prefix();
    Expression *postfix();
    Expression *atom();
    FloatLiteral *floatLiteral();
    IntLiteral *intLiteral();
    CharLiteral *charLiteral();
    StringLiteral *stringLiteral();
    BoolLiteral *boolLiteral();
    NullLiteral *nullLiteral();
    VariableAccess *variableAccess();

    bool function_call_argument(std::vector<Argument>& args);
    void function_call_arg_list(std::vector<Argument>& args);

    void optional_whitespace();
    void optional_whitespace_newline();
    bool mandatory_whitespace();
    bool mandatory_whitespace_newline();
    bool name();

    void raise(const Token& token, const std::string& message, ErrorLevel level = ErrorLevel::Error);
    void raise(const std::string& message, ErrorLevel level = ErrorLevel::Error);

    Token *last();

    std::vector<Token*> token_stack;
};

#endif//PARSER__HPP
