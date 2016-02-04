#ifndef WALKER__HPP
#define WALKER__HPP

class Unit;
class Use;
class Import;

class TemplateDeclaration;
class NamespaceDeclaration;
class VariableDeclaration;
class FunctionDeclaration;
class StructDeclaration;
class AliasDeclaration;
class VariantDeclaration;

class BaseType;
class PointerType;
class ArrayType;
class FunctionType;
class ClosureType;
class TupleType;

class Scope;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class UsingStmt;
class DeferStmt;
class MatchStmt;

class BreakStmt;
class ContinueStmt;

class OperatorAccess;
class VariableAccess;
class FieldAccess;
class BoolLiteral;
class StringLiteral;
class CharLiteral;
class IntLiteral;
class NullLiteral;
class FloatLiteral;

class ArrayIndexing;
class FunctionCall;
class Sizeof;

class UnaryOperator;
class Cast;
class IsExpr;
class BinaryOperator;
class Assignment;

class IfExpr;

class Walker {
public:
    virtual void walk(Unit *u) = 0;
    virtual void walk(Use *u) = 0;
    virtual void walk(Import *i) = 0;

    virtual void walk(TemplateDeclaration *decl) = 0;
    virtual void walk(NamespaceDeclaration *decl) = 0;
    virtual void walk(VariableDeclaration *decl) = 0;
    virtual void walk(FunctionDeclaration *decl) = 0;
    virtual void walk(StructDeclaration *decl) = 0;
    virtual void walk(AliasDeclaration *decl) = 0;
    virtual void walk(VariantDeclaration *decl) = 0;

    virtual void walk(BaseType *type) = 0;
    virtual void walk(PointerType *type) = 0;
    virtual void walk(ArrayType *type) = 0;
    virtual void walk(FunctionType *type) = 0;
    virtual void walk(ClosureType *type) = 0;
    virtual void walk(TupleType *type) = 0;

    virtual void walk(Scope *scope) = 0;
    virtual void walk(IfStmt *ifStmt) = 0;
    virtual void walk(WhileStmt *whileStmt) = 0;
    virtual void walk(ForStmt *forStmt) = 0;
    virtual void walk(ReturnStmt *ret) = 0;
    virtual void walk(UsingStmt *usingStmt) = 0;
    virtual void walk(DeferStmt *defer) = 0;
    virtual void walk(MatchStmt *match) = 0;

    virtual void walk(BreakStmt *breakStmt) = 0;
    virtual void walk(ContinueStmt *contStmt) = 0;

    virtual void walk(VariableAccess *vAcc) = 0;
    virtual void walk(FieldAccess *fAcc) = 0;
    virtual void walk(BoolLiteral *lit) = 0;
    virtual void walk(StringLiteral *lit) = 0;
    virtual void walk(CharLiteral *lit) = 0;
    virtual void walk(IntLiteral *lit) = 0;
    virtual void walk(NullLiteral *lit) = 0;
    virtual void walk(FloatLiteral *lit) = 0;

    virtual void walk(ArrayIndexing *ai) = 0;
    virtual void walk(FunctionCall *call) = 0;
    virtual void walk(Sizeof *sof) = 0;

    virtual void walk(UnaryOperator *op) = 0;
    virtual void walk(Cast *cast) = 0;
    virtual void walk(IsExpr *is) = 0;
    virtual void walk(BinaryOperator *op) = 0;
    virtual void walk(Assignment *ass) = 0;

    virtual void walk(IfExpr *ifExpr) = 0;
};

#endif
