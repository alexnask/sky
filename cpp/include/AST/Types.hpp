#ifndef TYPES__HPP
#define TYPES__HPP

#include <vector>
#include <memory>
#include <string>

#include "Node.hpp"

class TupleType : public Type {
public:
    TupleType (Token _token, std::vector<std::unique_ptr<Type>> _types) : Type (_token, nullptr, Kind::TupleType), types(std::move(_types)) {}

    bool isVoid() {
        // TODO: check inside types
        return types.empty();
    }

    int size() {
        return -1;
    }

    std::string debugString() {
        std::string buff = "TUPLETYPE[types=(";
        if (!types.empty()) {
            buff += types[0]->debugString();

            for (size_t i = 1; i < types.size(); ++i) {
                buff += ',' + types[i]->debugString();
            }
        }
        buff += ")]";
        return buff;
    }

    std::string displayString() {
        std::string buff = "(";
        if (!types.empty()) {
            buff += types[0]->str();

            for (size_t i = 1; i < types.size(); ++i) {
                buff += ", " + types[i]->str();
            }
        }
        buff += ')';
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::vector<std::unique_ptr<Type>> types;
};

class ClosureType : public Type {
public:
    ClosureType (Token _token, std::vector<std::unique_ptr<Type>> _argTypes, Type *_returnType) : Type (_token, nullptr, Kind::ClosureType),
                                                                                                  argTypes(std::move(_argTypes)),
                                                                                                  returnType(_returnType) {
        if (returnType) {
            returnType->parent = this;
        }

        for (auto& arg : argTypes) {
            arg->parent = this;
        }
    }

    bool isVoid() {
        return false;
    }

    // Pair of pointers (context, thunk)
    int size() {
        return 16;
    }

    std::string debugString() {
        std::string buff = "CLOSURETYPE[argTypes=(";
        if (!argTypes.empty()) {
            buff += argTypes[0]->debugString();

            if (argTypes.size() > 1) {
                for (size_t i = 1; i < argTypes.size(); ++i) {
                    buff += ',' + argTypes[i]->debugString();
                }
            }
        }

        buff += "),returnType=";
        if (void_type(&*returnType)) {
            buff += "void";
        } else {
            buff += returnType->debugString();
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = "Closure (";
        if (!argTypes.empty()) {
            buff += argTypes[0]->str();

            if (argTypes.size() > 1) {
                for (size_t i = 1; i < argTypes.size(); ++i) {
                    buff += ", " + argTypes[i]->str();
                }
            }
        }

        buff += ')';
        if (!void_type(&*returnType)) {
            buff += " -> " + returnType->str();
        }

        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::vector<std::unique_ptr<Type>> argTypes;
    std::unique_ptr<Type> returnType;
};

class FunctionType : public Type {
public:
    FunctionType (Token _token, std::vector<std::unique_ptr<Type>> _argTypes, Type *_returnType) : Type (_token, nullptr, Kind::FuncType),
                                                                                                   argTypes(std::move(_argTypes)),
                                                                                                   returnType(_returnType) {
        if (returnType) {
            returnType->parent = this;
        }

        for (auto& arg : argTypes) {
            arg->parent = this;
        }
    }

    bool isVoid() {
        return false;
    }

    int size() {
        return 8;
    }

    std::string debugString() {
        std::string buff = "FUNCTYPE[argTypes=(";
        if (!argTypes.empty()) {
            buff += argTypes[0]->debugString();

            if (argTypes.size() > 1) {
                for (size_t i = 1; i < argTypes.size(); ++i) {
                    buff += ',' + argTypes[i]->debugString();
                }
            }
        }

        buff += "),returnType=";
        if (void_type(&*returnType)) {
            buff += "void";
        } else {
            buff += returnType->debugString();
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = "Func (";
        if (!argTypes.empty()) {
            buff += argTypes[0]->str();

            if (argTypes.size() > 1) {
                for (size_t i = 1; i < argTypes.size(); ++i) {
                    buff += ", " + argTypes[i]->str();
                }
            }
        }

        buff += ')';
        if (!void_type(&*returnType)) {
            buff += " -> " + returnType->str();
        }

        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::vector<std::unique_ptr<Type>> argTypes;
    std::unique_ptr<Type> returnType;
};

class PointerType : public Type {
public:
    PointerType (Token _token, Type *_inner) : Type (_token, nullptr, Kind::PointerType), inner(_inner) {
        inner->parent = this;
    }

    bool isVoid() {
        return false;
    }

    int size() {
        return 8;
    }

    std::string debugString() {
        return "PointerType[inner=" + inner->debugString() +']';
    }

    std::string displayString() {
        return inner->str() + "*";
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Type> inner;
};

class ArrayType : public Type {
public:
    ArrayType (Token _token, Type *_inner) : Type (_token, nullptr, Kind::ArrayType), inner(_inner) {
        inner->parent = this;
    }

    bool isVoid() {
        return false;
    }

    int size() {
        // pointer + integer
        return 16;
    }

    std::string debugString() {
        return "ArrayType[inner=" + inner->debugString() +']';
    }

    std::string displayString() {
        return inner->str() + "[]";
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::unique_ptr<Type> inner;
};

class BaseType : public Type {
public:
    BaseType (Token _token, std::string _name) : Type (_token, nullptr, Kind::BaseType), name(_name) {}

    bool isVoid() {
        return name == "void";
    }

    int size() {
        if (ref) {
            return ref->size;
        }

        return -1;
    }

    std::string debugString() {
        std::string buff = "BaseType[name=" + name;
        if (!templates.empty()) {
            buff += ",templates=<";
            buff += templates[0]->debugString();

            if (templates.size() > 1) {
                for (size_t i = 1; i < templates.size(); ++i) {
                    buff += ',' + templates[i]->debugString();
                }
            }
            buff += '>';
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = name;
        if (!templates.empty()) {
            buff += "<";
            buff += templates[0]->str();

            if (templates.size() > 1) {
                for (size_t i = 1; i < templates.size(); ++i) {
                    buff += ", " + templates[i]->str();
                }
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
};

#endif
