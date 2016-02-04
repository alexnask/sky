#ifndef ASTDUMPER__HPP
#define ASTDUMPER__HPP

#include <AST/All.hpp>
#include <AST/Walker.hpp>

#include <cstdio>
#include <initializer_list>

// Dumps a unit AST into a .dot file
class ASTDumper : public Walker {
public:
    ASTDumper (Node *root, std::string outpath);

    void walk(Unit *u);
    void walk(Use *u);
    void walk(Import *i);

    void walk(TemplateDeclaration *decl);
    void walk(NamespaceDeclaration *decl);
    void walk(VariableDeclaration *decl);
    void walk(FunctionDeclaration *decl);
    void walk(StructDeclaration *decl);
    void walk(AliasDeclaration *decl);
    void walk(VariantDeclaration *decl);

    void walk(BaseType *type);
    void walk(PointerType *type);
    void walk(ArrayType *type);
    void walk(FunctionType *type);
    void walk(ClosureType *type);
    void walk(TupleType *type);

    void walk(Scope *scope);
    void walk(IfStmt *ifStmt);
    void walk(WhileStmt *whileStmt);
    void walk(ForStmt *forStmt);
    void walk(ReturnStmt *ret);
    void walk(UsingStmt *usingStmt);
    void walk(DeferStmt *defer);
    void walk(MatchStmt *match);

    void walk(BreakStmt *breakStmt);
    void walk(ContinueStmt *contStmt);

    void walk(VariableAccess *vAcc);
    void walk(FieldAccess *fAcc);
    void walk(BoolLiteral *lit);
    void walk(StringLiteral *lit);
    void walk(CharLiteral *lit);
    void walk(IntLiteral *lit);
    void walk(NullLiteral *lit);
    void walk(FloatLiteral *lit);

    void walk(ArrayIndexing *ai);
    void walk(FunctionCall *call);
    void walk(Sizeof *sof);

    void walk(UnaryOperator *op);
    void walk(Cast *cast);
    void walk(IsExpr *is);
    void walk(BinaryOperator *op);
    void walk(Assignment *ass);

    void walk(IfExpr *ifExpr);

private:
    FILE *file;
    int nodeCounter;
    int parent_id;
    std::string desc;

    void child(const std::string& _desc);

    void write(const std::string& str);
    void writeln(const std::string& str);
    int node(const std::string& text);
    int record(std::initializer_list<std::string> values);
    int current();
    void edge(int a, int b);
};


#endif
