#ifndef STATEMENTS__HPP
#define STATEMENTS__HPP

#include "Node.hpp"

#include <string>
#include <vector>
#include <memory>

class Scope : public Statement {
public:
    Scope (Token _token) : Statement(_token, nullptr, Kind::Scope) {}

    bool addStmt(Statement *stmt) {
        if (!stmt) {
            return false;
        }

        stmt->parent = this;
        statements.emplace_back(stmt);
        return true;
    }

    std::string debugString() {
        return "SCOPE";
    }

    std::string displayString() {
        return "scope";
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::vector<std::unique_ptr<Statement>> statements;
};

class IfStmt : public Statement {
public:
    IfStmt (Token _token, Expression *_condition, Statement *_ifStmt) : Statement(_token, nullptr, Kind::IfStmt), condition(_condition), ifStmt(_ifStmt),
                                                                        elseStmt(nullptr) {}

    IfStmt (Token _token, Expression *_condition, Statement *_ifStmt, Statement *_elseStmt) : Statement(_token, nullptr, Kind::IfStmt), condition(_condition),
                                                                                              ifStmt(_ifStmt), elseStmt(_elseStmt) {}

    std::string debugString() {
        std::string buff = "IFSTMT[condition=" + condition->debugString() + ",ifStmt=" + ifStmt->debugString() + ",elseStmt=";
        if (elseStmt) {
            buff += elseStmt->debugString();
        } else {
            buff += "<none>";
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = "if(" + condition->str() + ") " + ifStmt->str();
        if (elseStmt) {
            buff += " else " + elseStmt->str();
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> ifStmt;
    std::unique_ptr<Statement> elseStmt;
};

class WhileStmt : public Statement {
public:
    WhileStmt (Token _token, std::string _label, Expression *_condition, Statement *_body) : Statement(_token, nullptr, Kind::WhileStmt), label(_label),
                                                                                             condition(_condition), body(_body) {}

    std::string debugString() {
        return std::string("WHILESTMT[label=") + label + ",condition=" + condition->debugString() + ",body=" + body->debugString() + ']';
    }

    std::string displayString() {
        std::string buff;
        if (!label.empty()) {
            buff += label + ": ";
        }

        buff += "while(";
        buff += condition->str();
        buff += ") " + body->str();
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string label;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
};

class ForStmt : public Statement {
public:
    ForStmt (Token _token, std::string _label, Scope *_initScope, Expression *_condition, Expression *_loopExpr, Statement *_body) :
                                                                                            Statement(_token, nullptr, Kind::ForStmt), label(_label),
                                                                                            initScope(_initScope), condition(_condition), loopExpr(_loopExpr),
                                                                                            body(_body) {}

    std::string debugString() {
        std::string buff = "FORSTMT[label=" + label + ",initScope=" + initScope->debugString() + ",condition=";
        if (condition) {
            buff += condition->debugString();
        } else {
            buff += "<none>";
        }
        buff += ",loopExpr=";
        if (loopExpr) {
            buff += loopExpr->debugString();
        } else {
            buff += "<none>";
        }
        buff += ",body=" + body->debugString();
        return buff;
    }

    std::string displayString() {
        std::string buff;

        if (!label.empty()) {
            buff += label + ": ";
        }

        buff += "for(" + initScope->str() + "; ";

        if (condition) {
            buff += condition->str();
        }

        buff += "; ";

        if (loopExpr) {
            buff += loopExpr->str();
        }

        buff += ") ";
        buff += body->str();
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string label;
    std::unique_ptr<Scope> initScope;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> loopExpr;
    std::unique_ptr<Statement> body;
};

class ReturnStmt : public Statement {
public:
    ReturnStmt (Token _token, Expression *_expr) : Statement(_token, nullptr, Kind::ReturnStmt), expr(_expr) {}

    std::string debugString() {
        return std::string("RETURNSTMT[expr=") + expr->displayString() + ']';
    }

    std::string displayString() {
        std::string buff = "return";
        if (expr) {
            buff += ' ' + expr->str();
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> expr;
};

class UsingStmt : public Statement {
public:
    UsingStmt (Token _token, std::string _name, Scope *_scope) : Statement(_token, nullptr, Kind::UsingStmt), name(_name), scope(_scope) {}

    std::string debugString() {
        std::string buff = "USINGSTMT[name=" + name;
        if (scope) {
            buff += ",scope=" + scope->debugString();
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = "using " + name;
        if (scope) {
            buff += ' ' + scope->str();
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::unique_ptr<Scope> scope;
};

class BreakStmt : public Statement {
public:
    BreakStmt (Token _token, std::string _label) : Statement(_token, nullptr, Kind::BreakStmt), label(_label) {}
    BreakStmt (Token _token) : Statement(_token, nullptr, Kind::BreakStmt), label("") {}

    std::string debugString() {
        return std::string("BREAKSTMT[label=") + label + ']';
    }

    std::string displayString() {
        std::string buff = "break";
        if (!label.empty()) {
            buff += ' ' + label;
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string label;
};

class ContinueStmt : public Statement {
public:
    ContinueStmt (Token _token, std::string _label) : Statement(_token, nullptr, Kind::ContinueStmt), label(_label) {}
    ContinueStmt (Token _token) : Statement(_token, nullptr, Kind::ContinueStmt), label("") {}

    std::string debugString() {
        return std::string("CONTINUESTMT[label=") + label + ']';
    }

    std::string displayString() {
        std::string buff = "continue";
        if (!label.empty()) {
            buff += ' ' + label;
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string label;
};

class DeferStmt : public Statement {
public:
    DeferStmt (Token _token, Scope *_scope) : Statement(_token, nullptr, Kind::DeferStmt), scope(_scope) {}

    std::string debugString() {
        return std::string("DEFERSTMT[scope=") + scope->debugString() + ']';
    }

    std::string displayString() {
        return std::string("defer ") + scope->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Scope> scope;
};

// Either a case expr or a case is [tag(exprlist)]
class Case {
public:
    enum class Kind {
        Simple, Is
    };

    Case (Token _token, Expression *_expr, Scope *_body) : token(_token), kind(Kind::Simple), expr(_expr), body(_body) {}
    Case (Token _token, std::string _tag, std::vector<std::unique_ptr<Expression>> _exprs, Scope *_body) : token(_token), kind(Kind::Is), tag(_tag),
                                                                                                           exprs(std::move(_exprs)), body(_body) {}

    std::string debugString() {
        std::string buff = "CASE[kind=";
        if (kind == Kind::Simple) {
            buff += "simple,expr=" + expr->debugString();
        } else {
            buff += "is,tag=" + tag + ",exprs=(";
            if (!exprs.empty()) {
                buff += exprs[0]->debugString();
                for (size_t i = 1; i < exprs.size(); ++i) {
                    buff += ',' + exprs[i]->debugString();
                }
            }
            buff += ')';
        }

        buff += ",body=" + body->debugString();
        return buff;
    }

    Token token;
    Kind kind;

    std::unique_ptr<Expression> expr;

    std::string tag;
    std::vector<std::unique_ptr<Expression>> exprs;

    std::unique_ptr<Scope> body;
};

class MatchStmt : public Statement {
public:
    MatchStmt (Token _token, Expression *_matched_expr, std::vector<Case> _cases, Scope *_else_scope) : Statement(_token, nullptr, Kind::MatchStmt),
                                                                                                       matched_expr(_matched_expr), cases(std::move(_cases)),
                                                                                                       else_scope(_else_scope) {}

    std::string debugString() {
        std::string buff = "MATCHSTMT[matched_expr=" + matched_expr->debugString() + ",cases=(";
        if (!cases.empty()) {
            buff += cases[0].debugString();
            for (size_t i = 1; i < cases.size(); ++i) {
                buff += ',' + cases[i].debugString();
            }
        }
        buff += "),else_scope=";
        if (else_scope) {
            buff += else_scope->debugString();
        } else {
            buff += "<none>";
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        return std::string("match(") + matched_expr->str() + ")";
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> matched_expr;
    std::vector<Case> cases;
    std::unique_ptr<Scope> else_scope;
};

#endif
