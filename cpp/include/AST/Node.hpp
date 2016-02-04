#ifndef NODE__HPP
#define NODE__HPP

#include <Lexer.hpp>
#include <string>
#include <cstdint>
#include <memory>

// These are the base types of our AST.

class Node {
public:
    enum class Kind {
        Unit, Use, Import, NamespaceDecl, FuncDecl,
        TemplateDecl,
        StructDecl, VariableDecl, AliasDecl, VariantDecl,
        BaseType, ArrayType, PointerType, ClosureType, FuncType, TupleType,
        Scope, IfStmt, WhileStmt, ForStmt, ReturnStmt, UsingStmt,
        BreakStmt, ContinueStmt,
        DeferStmt, MatchStmt,
        VariableAcc, FieldAcc,
        BoolLit, StringLit, CharLit, IntLit, NullLit, FloatLit,
        ArrayIndexing, FuncCall, Sizeof, UnaryOp, BinaryOp,
        Cast, IfExpr, IsExpr,
        Ass
    };

    Token token;

    Node *parent;
    Kind kind;

    Node (Token _token, Node *_parent, Kind _kind) : token(_token), parent(_parent), kind(_kind) {}

    // This is a string to be used for internal compiler debugging
    // It should show information like the node kind, etc.
    virtual std::string debugString() = 0;

    // This is used for printing info to the user.
    virtual std::string displayString() = 0;

    inline std::string str() {
        return displayString();
    }

    virtual void accept(class Walker& w) = 0;
};

class Statement : public Node {
public:
    Statement (Token _token, Node *_parent, Kind _kind) : Node(_token, _parent, _kind) {}
};

class Declaration : public Statement {
public:
    Declaration (Token _token, Node *_parent, Kind _kind) : Statement(_token, _parent, _kind) {}
};

class TypeDeclaration : public Declaration {
public:
    TypeDeclaration (Token _token, Node *_parent, Kind _kind) : Declaration(_token, _parent, _kind), size(-1) {}

    int size;
};

class Type : public Node {
public:
    TypeDeclaration *ref;

    Type (Token _token, Node *_parent, Kind _kind) : Node(_token, _parent, _kind), ref(nullptr) {}

    virtual bool isVoid() = 0;
    virtual int size() = 0;
};

class Expression : public Statement {
public:
    std::unique_ptr<Type> type;

    Expression (Token _token, Node *_parent, Kind _kind, Type *_type) : Statement(_token, _parent, _kind), type(_type) {}
};

// Some convenience functions
bool void_type(Type *type);
std::string unescape_string(std::string input);

void toupper(std::string& in);

int int_base(std::string& in);
std::string int_type(std::string& in);
int64_t int_value(int base, const std::string& in);

std::string float_type(std::string& in);
long double float_value(const std::string& in);

bool num_is_positive(std::string& in);

#endif//NODE__HPP
