#ifndef DECLARATIONS__HPP
#define DECLARATIONS__HPP

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <tuple>

#include "Node.hpp"
#include "Statements.hpp"
#include "Types.hpp"

class TemplateDeclaration : public TypeDeclaration {
public:
    TemplateDeclaration (Token _token, std::string _name ) : TypeDeclaration(_token, nullptr, Kind::TemplateDecl), name(_name) {}

    std::string debugString() {
        return std::string("TEMPLATEDECL[name=") + name + ']';
    }

    std::string displayString() {
        return name;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
};

class NamespaceDeclaration : public Declaration {
public:
    NamespaceDeclaration (Token _token, std::string _name) : Declaration(_token, nullptr, Kind::NamespaceDecl), name(_name) {}

    bool addDeclaration(Declaration *decl) {
        if (!decl) return false;

        decl->parent = this;

        decls.push_back(std::unique_ptr<Declaration>(decl));
        return true;
    }

    std::string debugString() {
        return "NAMESPACE[name=" + name +']';
    }

    std::string displayString() {
        return "namespace " + name;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::vector<std::unique_ptr<Declaration>> decls;
};

class VariableDeclaration: public Declaration {
public:
    // For things like 'foo : Bar = baz'
    VariableDeclaration (Token _token, std::string _name, Type *_type, Expression *_init_expr) : Declaration(_token, nullptr, Kind::VariableDecl),
                                                                                                 name(_name), type(_type), init_expr(_init_expr) {}
    // For things like 'foo : Bar'
    VariableDeclaration (Token _token, std::string _name, Type *_type) : Declaration(_token, nullptr, Kind::VariableDecl), name(_name), type(_type),
                                                                         init_expr(nullptr) {}
    // For things like 'foo := bar'
    VariableDeclaration (Token _token, std::string _name, Expression *_init_expr) : Declaration(_token, nullptr, Kind::VariableDecl),
                                                                                    name(_name), type(nullptr), init_expr(_init_expr) {}

    std::string debugString() {
        std::string buff = "VDECL[name=" + name;
        if (type) {
            buff += ",type=" + type->debugString();
        }

        if (init_expr) {
            buff += ",init_expr=" + init_expr->debugString();
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = name;

        if (type) {
            buff += " : " + type->str();

            if (init_expr) {
                buff += " = " + init_expr->str();
            }
        } else {
            buff += " := " + init_expr->str();
        }

        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::unique_ptr<Type> type;
    std::unique_ptr<Expression> init_expr;

    bool static_mod = false;
    bool extern_mod = false;
};

// TODO: template redefinition checked while building symtable
class StructDeclaration : public TypeDeclaration {
public:
    StructDeclaration (Token _token, std::string _name) : TypeDeclaration(_token, nullptr, Kind::StructDecl), name(_name) {}

    bool addField(VariableDeclaration *vdecl) {
        if (!vdecl) {
            return false;
        }

        vdecl->parent = this;
        fields.emplace_back(vdecl);
        return true;
    }

    bool addSubdecl(TypeDeclaration *tdecl) {
        if (!tdecl) {
            return false;
        }

        tdecl->parent = this;
        subdecls.emplace_back(tdecl);
        return true;
    }

    std::string debugString() {
        std::string buff = "STRUCTDECL[name=" + name;
        if (!templates.empty()) {
            buff += ",templates=<";
            buff += templates[0].debugString();

            for (size_t i = 1; i < templates.size(); ++i) {
                buff += ',' + templates[i].debugString();
            }
            buff += '>';
        }
        buff += ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = name + " : struct";
        if (!templates.empty()) {
            buff += " <";
            buff += templates[0].str();

            for (size_t i = 1; i < templates.size(); ++i) {
                buff += ", " + templates[i].str();
            }
            buff += '>';
        }

        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::vector<std::unique_ptr<VariableDeclaration>> fields;
    std::vector<std::unique_ptr<TypeDeclaration>> subdecls;
    std::vector<TemplateDeclaration> templates;
};

class AliasDeclaration : public TypeDeclaration {
public:
    AliasDeclaration (Token _token, std::string _name, Type *_from_type, std::vector<TemplateDeclaration> _templates) : TypeDeclaration(_token, nullptr, Kind::AliasDecl),
                                                                                                                        name(_name), from_type(_from_type),
                                                                                                                        templates(std::move(_templates)) {
        from_type->parent = this;
    }

    std::string debugString() {
        return "ALIASDECL[name=" + name +",from_type=" + from_type->debugString() + ']';
    }

    std::string displayString() {
        return name + " : alias from " + from_type->str();
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::unique_ptr<Type> from_type;
    std::vector<TemplateDeclaration> templates;
};

class VariantMember {
public:
    VariantMember (std::string _name, TupleType *_type, int64_t _value) : name(_name), type(_type), value(_value) {}

    std::string debugString() {
        return "VARIANTMEM[name=" + name + ",type=" + (type ? type->debugString() : "") + ",value=" + std::to_string(value) + ']';
    }

    std::string str() {
        std::string buff = name;
        if (type) {
            buff += ' ' + type->str();
        }
        buff += " = " + std::to_string(value);
        return buff;
    }

    std::string name;
    std::unique_ptr<TupleType> type;
    int64_t value;
};

class VariantDeclaration : public TypeDeclaration {
public:
    VariantDeclaration (Token _token, std::string _name, Type *_from_type, std::vector<TemplateDeclaration> _templates) :
                                                                                        TypeDeclaration(_token, nullptr, Kind::VariantDecl),
                                                                                        name(_name), from_type(_from_type), templates(std::move(_templates)) {
        if (from_type) {
            from_type->parent = this;
        }
    }

    void addField(std::string fieldName, TupleType *type, int64_t fieldValue) {
        fields.emplace_back(fieldName, type, fieldValue);
    }

    bool addSubdecl(TypeDeclaration *tdecl) {
        if (!tdecl) {
            return false;
        }

        tdecl->parent = this;
        subdecls.emplace_back(tdecl);
        return true;
    }

    std::string debugString() {
        return "VARIANTDECL[name=" + name +",from_type=" + (void_type(&*from_type) ? "void" : from_type->debugString()) + ']';
    }

    std::string displayString() {
        std::string buff = name + " : variant";
        if (!void_type(&*from_type)) {
            buff += " from " + from_type->str();
        }
        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    std::unique_ptr<Type> from_type;

    std::vector<VariantMember> fields;
    std::vector<std::unique_ptr<TypeDeclaration>> subdecls;
    std::vector<TemplateDeclaration> templates;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration (Token _token, std::string _name, bool _is_extern, bool _is_inline) : Declaration(_token, nullptr, Kind::FuncDecl),
                                                                                              name(_name), is_extern(_is_extern), is_inline(_is_inline) {}

    std::string debugString() {
        std::string buff = "FUNCDECL[name=" + name + ",argtypes=(";
        if (!arglist.empty()) {
            buff += arglist[0]->debugString();

            for (size_t i = 1; i < arglist.size(); ++i) {
                buff += ',' + arglist[i]->debugString();
            }
        }
        buff += "),return_type=" + void_type(&*return_type) ? "void" : return_type->debugString() + ",is_extern=" + std::to_string(is_extern) + ']';
        return buff;
    }

    std::string displayString() {
        std::string buff = name + " : func";
        if (!arglist.empty()) {
            buff += " (" + arglist[0]->str();
            for (size_t i = 1; i < arglist.size(); ++i) {
                buff += ", " + arglist[i]->str();
            }
            buff += ")";
        }

        if (!void_type(&*return_type)) {
            buff += " -> " + return_type->str();
        }

        return buff;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string name;
    bool is_extern;
    bool is_inline;

    std::vector<std::unique_ptr<VariableDeclaration>> arglist;
    std::unique_ptr<Type> return_type;
    std::unique_ptr<Scope> body;
    std::vector<TemplateDeclaration> templates;
};

#endif
