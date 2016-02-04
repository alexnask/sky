#ifndef EXPRESSIONS__HPP
#define EXPRESSIONS__HPP

#include <vector>
#include <string>
#include <memory>

#include <cstdint>

#include <Token_ids.hpp>

#include "Node.hpp"
#include "Types.hpp"
#include "Statements.hpp"

class VariableAccess : public Expression {
public:
    VariableAccess (Token _token, std::string _name) : Expression(_token, nullptr, Kind::VariableAcc, nullptr), name(_name), ref(nullptr) {}

    std::string debugString() {
        std::string buff = "VARIABLEACCESS[name=" + name + ",templates=<";
        if (!templates.empty()) {
            buff += templates[0]->debugString();

            for (size_t i = 1; i < templates.size(); ++i) {
                buff += ',' + templates[i]->debugString();
            }
        }
        buff += ">]";
        return buff;
    }

    std::string displayString() {
        std::string buff = name;

        if (!templates.empty()) {
            buff += " <" + templates[0]->str();
            for (size_t i = 1; i < templates.size(); ++i) {
                buff += ", " + templates[i]->str();
            }
            buff += '>';
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::vector<std::unique_ptr<Type>> templates;

    Declaration *ref;
};

class BoolLiteral : public Expression {
public:
    BoolLiteral (Token _token, std::string _value) : Expression(_token, nullptr, Kind::BoolLit, new BaseType(_token, "bool")) {
        value = _value == "true";
    }

    std::string debugString() {
        return "BOOLLITERAL[value=" + std::to_string(value) + ']';
    }

    std::string displayString() {
        return std::to_string(value);
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    bool value;
};

class StringLiteral : public Expression {
public:
    StringLiteral (Token _token, std::string _value) : Expression(_token, nullptr, Kind::StringLit, new BaseType(_token, "string")),
                                                       value(unescape_string(_value)) {}

    std::string debugString() {
        return "STRINGLITERAL[value=\"" + value + +"\"]";
    }

    std::string displayString() {
        return '"' + value + '"';
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string value;
};

class CharLiteral : public Expression {
public:
    CharLiteral (Token _token, std::string _value) : Expression(_token, nullptr, Kind::CharLit, new BaseType(_token, "byte")) {
        _value = unescape_string(_value);
        if (_value.size() > 1) {
            // :(
            // TODO: error out.
        } else {
            value = _value[0];
        }
    }

    std::string debugString() {
        return std::string("CHARLITERAL[value='") + value + +"']";
    }

    std::string displayString() {
        return std::string("'") + value + '\'';
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    char value;
};

class NullLiteral : public Expression {
public:
    NullLiteral (Token _token) : Expression (_token, nullptr, Kind::NullLit, new PointerType(_token, new BaseType(_token, "void"))) {}

    std::string debugString() {
        return "NULLLIT";
    }

    std::string displayString() {
        return "null";
    }

    void accept(Walker& w) {
        w.walk(this);
    }
};

class IntLiteral : public Expression {
public:
    IntLiteral (Token _token, std::string _value) : Expression(_token, nullptr, Kind::IntLit, nullptr) {
        // TODO: parse prefix, suffix, set value and type
        toupper(_value);
        bool positive = num_is_positive(_value);
        int base = int_base(_value);
        type = std::unique_ptr<Type>(new BaseType(_token, int_type(_value)));
        value = int_value(base, _value);
        if (!positive) {
            value = -value;
        }
    }

    std::string debugString() {
        return "INTLITERAL[value='" + std::to_string(value) + +",type=" + type->debugString() + "']";
    }

    std::string displayString() {
        return std::to_string(value);
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    int64_t value;
};

class FloatLiteral : public Expression {
public:
    FloatLiteral (Token _token, std::string _value) : Expression(_token, nullptr, Kind::FloatLit, nullptr) {
        type = std::unique_ptr<Type>(new BaseType(_token, float_type(_value)));
        bool positive = num_is_positive(_value);
        value = float_value(_value);
        if (!positive) {
            value = -value;
        }
    }

    std::string debugString() {
        return "FLOATLITERAL[value='" + std::to_string(value) + +",type=" + type->debugString() + "']";
    }

    std::string displayString() {
        return std::to_string(value);
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    long double value;
};

class ArrayIndexing : public Expression {
public:
    ArrayIndexing (Token _token, Expression *_base, Expression *_index) : Expression(_token, nullptr, Kind::ArrayIndexing, nullptr), base(_base), index(_index) {}

    std::string debugString() {
        return "ARRAYINDEXING[base='" + base->debugString() + +",index=" + index->debugString() + "']";
    }

    std::string displayString() {
        return base->str() + '[' + index->str() + ']';
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> base;
    std::unique_ptr<Expression> index;
};

class Argument {
public:
    Argument (std::string _name, Expression *_expr) : name(_name), expr(_expr) {}
    Argument (Expression *_expr) : name(), expr(_expr) {}

    inline bool hasName() {
        return !name.empty();
    }

    std::string debugString() {
        std::string buff = "ARG[";
        if (hasName()) {
            buff += "name=" + name + ',';
        }
        buff += "expr=" + expr->debugString() + ']';
        return buff;
    }

    std::string str() {
        std::string buff;
        if (hasName()) {
            buff += name + ": ";
        }
        buff += expr->str();
        return buff;
    }

    std::string name;
    std::unique_ptr<Expression> expr;
};

class FunctionCall : public Expression {
public:
    FunctionCall (Token _token, Expression *_base, std::vector<Argument> _args) : Expression(_token, nullptr, Kind::FuncCall, nullptr), base(_base),
                                                                                  args(std::move(_args)) {}


    std::string debugString() {
        std::string buff = "FUNCTIONCALL[base=" + base->debugString() +",args=(";
        if (!args.empty()) {
            buff += args[0].debugString();
            if (args.size() > 1) {
                for (size_t i = 1; i < args.size(); ++i) {
                    buff += ',' + args[i].debugString();
                }
            }
        }
        buff += ")]";
        return buff;
    }

    std::string displayString() {
        std::string buff = base->str();

        buff += " (";
        if (!args.empty()) {
            buff += args[0].str();
            if (args.size() > 1) {
                for (size_t i = 1; i < args.size(); ++i) {
                    buff += ", " + args[i].str();
                }
            }
        }
        buff += ')';
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> base;
    std::vector<Argument> args;
};

// sizeof can take an expression or a type
// TODO: type could be inferred by surrounding, int64 for now
class Sizeof : public Expression {
public:
    Sizeof (Token _token, Expression *_expr) : Expression(_token, nullptr, Kind::Sizeof, new BaseType(_token, "int64")), expr(_expr) {}
    Sizeof (Token _token, Type *_type) : Expression(_token, nullptr, Kind::Sizeof, new BaseType(_token, "int64")), arg_type(_type) {}

    std::string debugString() {
        std::string buff = "SIZEOF[";
        if (expr) {
            buff += "expr=" + expr->debugString();
        } else if (arg_type) {
            buff += "arg_type=" + arg_type->debugString();
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = "sizeof(";
        if (expr) {
            buff += expr->str();
        } else if (arg_type) {
            buff += arg_type->str();
        }
        buff += ')';
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> expr;
    std::unique_ptr<Type> arg_type;
};

class UnaryOperator : public Expression {
public:
    enum class OpKind {
        Plus, Minus, Bang, BitNot, Deref, Addrof
    };

    UnaryOperator (Token _token, Expression *_expr, int tok_id) : Expression(_token, nullptr, Kind::UnaryOp, nullptr), expr(_expr) {
        if (tok_id == OP_PLUS) {
            opopkind = OpKind::Plus;
        } else if (tok_id == OP_MINUS) {
            opopkind = OpKind::Minus;
        } else if (tok_id == OP_BANG) {
            opopkind = OpKind::Bang;
        } else if (tok_id == OP_BIT_NOT) {
            opopkind = OpKind::BitNot;
        } else if (tok_id == OP_TIMES) {
            opopkind = OpKind::Deref;
        } else if (tok_id == OP_BIT_AND) {
            opopkind = OpKind::Addrof;
        }
    }

    char opChar() {
        if (opopkind == OpKind::Plus) {
            return '+';
        } else if (opopkind == OpKind::Minus) {
            return '-';
        } else if (opopkind == OpKind::Bang) {
            return '!';
        } else if (opopkind == OpKind::BitNot) {
            return '~';
        } else if (opopkind == OpKind::Deref) {
            return '*';
        } else if (opopkind == OpKind::Addrof) {
            return '&';
        }

        return ' ';
    }

    std::string debugString() {
        return std::string("UNARYOP[opkind=") + opChar() + ",expr=" + expr->debugString() + ']';
    }

    std::string displayString() {
        return opChar() + expr->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> expr;
    OpKind opopkind;
};

class Cast : public Expression {
public:
    Cast (Token _token, Expression *_expr, Type *_type) : Expression(_token, nullptr, Kind::Cast, _type), expr(_expr) {}

    std::string debugString() {
        return "CAST[expr=" + expr->debugString() + ",type=" + type->debugString() + ']';
    }

    std::string displayString() {
        return expr->str() + " as " + type->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> expr;
};

class BinaryOperator : public Expression {
public:
    enum class OpKind {
        Times, Div, Mod, Plus, Minus, ShLogRight, ShLogLeft, ShArRight, ShArLeft,
        Lesser, Greater, LesserEq, GreaterEq, Equals, NotEquals, BitAnd, BitXor,
        BitOr, LogAnd, LogOr
    };

    BinaryOperator (Token _token, Expression *_left, Expression *_right, int tok_id) : Expression(_token, nullptr, Kind::BinaryOp, nullptr), left(_left), right(_right) {
        if (tok_id == OP_TIMES) {
            opkind = OpKind::Times;
        } else if (tok_id == OP_DIV) {
            opkind = OpKind::Div;
        } else if (tok_id == OP_MOD) {
            opkind = OpKind::Mod;
        } else if (tok_id == OP_PLUS) {
            opkind = OpKind::Plus;
        } else if (tok_id == OP_MINUS) {
            opkind = OpKind::Minus;
        } else if (tok_id == OP_LOGICAL_SHIFT_RIGHT) {
            opkind = OpKind::ShLogRight;
        } else if (tok_id == OP_LOGICAL_SHIFT_LEFT) {
            opkind = OpKind::ShLogRight;
        } else if (tok_id == OP_ARITHM_SHIFT_RIGHT) {
            opkind = OpKind::ShArRight;
        } else if (tok_id == OP_ARITHM_SHIFT_LEFT) {
            opkind = OpKind::ShArLeft;
        } else if (tok_id == OP_LESS) {
            opkind = OpKind::Lesser;
        } else if (tok_id == OP_GREATER) {
            opkind = OpKind::Greater;
        } else if (tok_id == OP_LESS) {
            opkind = OpKind::LesserEq;
        } else if (tok_id == OP_GREATER_EQ) {
            opkind = OpKind::GreaterEq;
        } else if (tok_id == OP_EQ) {
            opkind = OpKind::Equals;
        } else if (tok_id == OP_NEQ) {
            opkind = OpKind::NotEquals;
        } else if (tok_id == OP_BIT_AND) {
            opkind = OpKind::BitAnd;
        } else if (tok_id == OP_BIT_XOR) {
            opkind = OpKind::BitXor;
        } else if (tok_id == OP_BIT_OR) {
            opkind = OpKind::BitOr;
        } else if (tok_id == OP_LOG_AND) {
            opkind = OpKind::LogAnd;
        } else if (tok_id == OP_LOG_OR) {
            opkind = OpKind::LogOr;
        }
    }

    std::string opStr() {
        if (opkind == OpKind::Times) {
            return "*";
        } else if (opkind == OpKind::Div) {
            return "/";
        } else if (opkind == OpKind::Mod) {
            return "%";
        } else if (opkind == OpKind::Plus) {
            return "+";
        } else if (opkind == OpKind::Minus) {
            return "-";
        } else if (opkind == OpKind::ShLogRight) {
            return "shr";
        } else if (opkind == OpKind::ShLogLeft) {
            return "shl";
        } else if (opkind == OpKind::ShArLeft) {
            return "sal";
        } else if (opkind == OpKind::ShArRight) {
            return "sar";
        } else if (opkind == OpKind::Lesser) {
            return "<";
        } else if (opkind == OpKind::Greater) {
            return ">";
        } else if (opkind == OpKind::LesserEq) {
            return "<=";
        } else if (opkind == OpKind::GreaterEq) {
            return ">=";
        } else if (opkind == OpKind::Equals) {
            return "==";
        } else if (opkind == OpKind::NotEquals) {
            return "!=";
        } else if (opkind == OpKind::BitAnd) {
            return "&";
        } else if (opkind == OpKind::BitXor) {
            return "^";
        } else if (opkind == OpKind::BitOr) {
            return "|";
        } else if (opkind == OpKind::LogAnd) {
            return "&&";
        } else if (opkind == OpKind::LogOr) {
            return "||";
        }

        return "";
    }

    std::string debugString() {
        return "BINARYOPERATOR[left=" + left->debugString() + ",right=" + right->debugString() + ",op=" + opStr() + ']';
    }

    std::string displayString() {
        return left->str() + ' ' + opStr() + ' ' + right->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    OpKind opkind;
};

class IfExpr : public Expression {
public:
    IfExpr (Token _token, Expression *_condition, Scope *_ifScope, Scope *_elseScope) : Expression(_token, nullptr, Kind::IfExpr, nullptr),
                                                                                        condition(_condition), ifScope(_ifScope), elseScope(_elseScope) {}

    std::string debugString() {
        return "IFEXPR[condition=" + condition->debugString() + ",ifScope=" + ifScope->debugString() + ",elseScope=" + elseScope->debugString() + ']';
    }

    std::string displayString() {
        return "if(" + condition->str() + ") " + ifScope->str() + " else " + elseScope->str(); 
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Scope> ifScope;
    std::unique_ptr<Scope> elseScope;
};

class Assignment : public Expression {
public:
    enum class AssKind {
        Bare, Plus, Minus, Times, Div, Mod,
        BitAnd, BitXor, BitOr
    };

    Assignment (Token _token, Expression *_left, Expression *_right, int tok_id) : Expression(_token, nullptr, Node::Kind::Ass, nullptr),
                                                                                   left(_left), right(_right) {
        if (tok_id == OP_ASS) {
            asskind = AssKind::Bare;
        } else if (tok_id == OP_PLUS_EQ) {
            asskind = AssKind::Plus;
        } else if (tok_id == OP_MINUS_EQ) {
            asskind = AssKind::Minus;
        } else if (tok_id == OP_TIMES_EQ) {
            asskind = AssKind::Times;
        } else if (tok_id == OP_DIV_EQ) {
            asskind = AssKind::Div;
        } else if (tok_id == OP_MOD_EQ) {
            asskind = AssKind::Mod;
        } else if (tok_id == OP_BIT_AND_EQ) {
            asskind = AssKind::BitAnd;
        } else if (tok_id == OP_BIT_XOR_EQ) {
            asskind = AssKind::BitXor;
        } else if (tok_id == OP_BIT_OR_EQ) {
            asskind = AssKind::BitOr;
        }
    }

    std::string assStr() {
        if (asskind == AssKind::Bare) {
            return "=";
        } else if (asskind == AssKind::Plus) {
            return "+=";
        } else if (asskind == AssKind::Minus) {
            return "-=";
        } else if (asskind == AssKind::Times) {
            return "*=";
        } else if (asskind == AssKind::Div) {
            return "/=";
        } else if (asskind == AssKind::Mod) {
            return "%=";
        } else if (asskind == AssKind::BitAnd) {
            return "&=";
        } else if (asskind == AssKind::BitXor) {
            return "^=";
        } else if (asskind == AssKind::BitOr) {
            return "|=";
        }

        return "";
    }

    std::string debugString() {
        return "ASSIGNMENT[left=" + left->debugString() + ",right=" + right->debugString() + ",ass=" + assStr() + ']';
    }

    std::string displayString() {
        return left->str() + ' ' + assStr() + ' ' + right->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    AssKind asskind;
};

class FieldAccess : public Expression {
public:
    FieldAccess (Token _token, Expression *_expr, std::string _field_name) : Expression(_token, nullptr, Kind::FieldAcc, nullptr), expr(_expr),
                                                                             field_name(_field_name) {}

    std::string debugString() {
        return std::string("FIELDACCESS[expr=") + expr->debugString() + ",field_name=" + field_name + ']';
    }

    std::string displayString() {
        return expr->str() + '.' + field_name;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> expr;
    std::string field_name;
};

class IsExpr : public Expression {
public:
    IsExpr (Token _token, Expression *_base, std::string _tag, std::vector<std::unique_ptr<Expression>> _exprs) : Expression(_token, nullptr, Kind::IfExpr, nullptr),
                                                                                                                  base(_base), tag(_tag), exprs(std::move(_exprs)) {}

    std::string debugString() {
        std::string buff = "ISEXPR[base=" + base->debugString() + "tag=" + tag + ",exprs=(";
        if (!exprs.empty()) {
            buff += exprs[0]->debugString();
            for (size_t i = 1; i < exprs.size(); ++i) {
                buff += ',' + exprs[i]->debugString();
            }
        }
        buff += ")]";
        return buff;
    }

    std::string displayString() {
        std::string buff = base->str() + " is " + tag;
        if (!exprs.empty()) {
            buff += '(' + exprs[0]->str();
            for (size_t i = 1; i < exprs.size(); ++i) {
                buff += ", " + exprs[i]->str();
            }
            buff += ')';
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Expression> base;
    std::string tag;
    std::vector<std::unique_ptr<Expression>> exprs;
};

#endif
