#include <Parser.hpp>
#include <Token_ids.hpp>

#include <Options.hpp>

// This is a handwritten parser subject to tonnes of modification.
// Its performance is probably horrible.

Parser::Parser(Token *_stream) : stream(_stream), cursor(stream), curr_unit(nullptr) {}

inline bool Parser::accept(int id) {
    return (cursor++)->id == id;
}

void Parser::raise(const std::string& message, ErrorLevel level) {
    Options::get().err_handler->report(message, level);
}

void Parser::raise(const Token& token, const std::string& message, ErrorLevel level) {
    Options::get().err_handler->report(curr_unit, token, message, level);
}

// Convenience function to auto-rewinding if the accept fails
// Very convenient for accepts in while loops (along with other productions)
inline bool Parser::accept_rewind(int id) {
    auto ok = accept(id);

    if (!ok) {
        cursor--;
    }

    return ok;
}

// This is equivalent to '*' or '+' (you chose depending on the return value)
inline bool Parser::consumeAll(int id) {
    Token *start = cursor;

    while (cursor->id == id) {
        cursor++;
    }

    return start != cursor;
}

// Parses a code unit (file)
std::unique_ptr<Unit> Parser::unit(std::string path, char *contents) {
    auto unit = std::make_unique<Unit>(path, contents);

    curr_unit = unit.get();

    Token *accepted = cursor;

    // unit <- op_ws VERSION op_ws PAREN_OPEN VersionSpec PAREN_CLOSE op_ws_nl OPEN_CURLY op_ws_nl unit op_ws_nl

    // unit <- (Use | Import | Newline | Whitespace)* (Declaration | Newline | Whitespace)* END

    while (unit->addUse(use()) || unit->addImport(import()) || accept_rewind(NEWLINE) || accept_rewind(WHITESPACE)) { accepted = cursor; }
    cursor = accepted;

    while (unit->addDeclaration(declaration()) || accept_rewind(NEWLINE) || accept_rewind(WHITESPACE)) { accepted = cursor; }
    cursor = accepted;

    if (!accept_rewind(END)) {
        raise(*cursor, "Unexpected token at the end of input.");
        return nullptr;
    }

    return unit;
}

inline Declaration *Parser::declaration() {
    // declaration <- functionDecl | variableDecl | structDecl | dataDecl | aliasDecl | namespaceDecl
    // Try namespace first, then inf-ass variabledecl, then the rest?
    Declaration *ret;

    if ((ret = namespace_())) {
        return ret;
    } else if ((ret = typeDecl())) {
        return ret;
    } else if ((ret = funcDecl())) {
        return ret;
    } else if ((ret = variableDecl())) {
        return ret;
    } else {
        return nullptr;
    }
}

// optional whitespace then semicolon or newline
inline bool Parser::statement_separator() {
    bool has = false;

    while (true) {
        if (accept_rewind(SEMICOLON) || accept_rewind(NEWLINE)) {
            has = true;
        } else if(!accept_rewind(WHITESPACE)) {
            break;
        }
    }

    return has;
}

inline TypeDeclaration *Parser::typeDecl() {
    // TODO: add data
    TypeDeclaration *ret;
    if ((ret = structDecl())) {
        return ret;
    } else if ((ret = variantDecl())) {
        return ret;
    } else if ((ret = aliasDecl())) {
        return ret;
    }

    return nullptr;
}

inline bool Parser::decl_of(int id) {
    Token *start = cursor;

    if (!accept(IDENTIFIER)) {
        cursor = start;
        return false;
    }

    optional_whitespace();

    if (!accept(COLON)) {
        cursor = start;
        return false;
    }

    optional_whitespace();

    if (!accept(id)) {
        cursor = start;
        return false;
    }

    return true;
}

inline VariantDeclaration *Parser::variantDecl() {
    Token *start = cursor;

    if (!decl_of(VARIANT)) {
        return nullptr;
    }

    std::string name = start->value();

    Token *afterEnum = cursor;
    BaseType *fromType = nullptr;

    std::vector<TemplateDeclaration> templates;

    optional_whitespace();
    templateDef(templates);
    if (templates.empty()) {
        cursor = afterEnum;
    }

    // op_ws_nl CURLY_OPEN | man_ws FROM man_ws Type op_ws_nl CURLY_OPEN
    if (mandatory_whitespace() && accept(FROM)) {
        if (!mandatory_whitespace()) {
            raise(*last(), "Expected whitespace after variant from.");
            cursor = start;
            return nullptr;
        }

        fromType = baseType();
        if (!fromType) {
            raise(*last(), "Expected base type after variant from.");
            cursor = start;
            return nullptr;
        }
    } else {
        cursor = afterEnum;
    }

    optional_whitespace_newline();

    if (!accept(CURLY_OPEN)) {
        raise(*last(), "Expected curly brace to open variant declaration.");
        cursor = start;
        return nullptr;
    }

    auto vDecl = new VariantDeclaration(Token::concat(start, last()), name, fromType, std::move(templates));

    optional_whitespace_newline();

    // (Identifier op_ws (OP_ASS op_ws IntLiteral)? StmtSep)*
    int64_t enumCounter = 0;

    while (true) {
        if (vDecl->addSubdecl(typeDecl())) {
            optional_whitespace_newline();
            continue;
        }

        if (!accept_rewind(IDENTIFIER)) {
            break;
        }

        std::string fieldName = last()->value();

        optional_whitespace();

        // Try to get a tuple
        auto maybeTuple = tupleType();
        if (maybeTuple) {
            optional_whitespace();
        }

        if (accept_rewind(OP_ASS)) {
            optional_whitespace();

            if (!accept(INT_LITERAL)) {
                raise(*last(), "Variant member tag can only be initialized to integer literal.");
                cursor = start;
                return nullptr;
            }

            // Parse int literal, set its value as enumCounter
            std::string value = last()->value();
            toupper(value);
            bool positive = num_is_positive(value);
            int base = int_base(value);
            int_type(value);

            enumCounter = int_value(base, value);
            if (!positive) {
                enumCounter = -enumCounter;
            }
        }

        vDecl->addField(fieldName, maybeTuple, enumCounter);
        enumCounter++;

        if (!statement_separator()) {
            break;
        }
    }

    if (!accept(CURLY_CLOSE)) {
        raise(*last(), "Expected enum field, newline or semicolon in enum declaration.");
        cursor = start;
        return nullptr;
    }

    return vDecl;
}

inline AliasDeclaration *Parser::aliasDecl() {
    Token *start = cursor;

    if (!decl_of(ALIAS)) {
        return nullptr;
    }

    std::string name = start->value();

    std::vector<TemplateDeclaration> templates;
    Token *afterAlias = cursor;

    optional_whitespace();
    templateDef(templates);

    if (templates.empty()) {
        cursor = afterAlias;
    }

    if (!mandatory_whitespace()) {
        raise(*last(), "Expected whitespace after alias keyword and template declarations.");
        cursor = start;
        return nullptr;
    }

    if (!accept(FROM)) {
        raise(*last(), "Expected from keyword in alias declaration.");
        cursor = start;
        return nullptr;
    }

    if (!mandatory_whitespace()) {
        raise(*last(), "Expected whitespace after from keyword.");
        cursor = start;
        return nullptr;
    }

    auto maybeType = type();
    if (!maybeType) {
        raise(*last(), "Alias declaration needs a type to alias.");
        cursor = start;
        return nullptr;
    }

    return new AliasDeclaration(Token::concat(start, last()), name, maybeType, std::move(templates));
}

inline StructDeclaration *Parser::structDecl() {
    Token *start = cursor;

    if (!decl_of(STRUCT)) {
        return nullptr;
    }

    std::string name = start->value();

    optional_whitespace();

    auto *decl = new StructDeclaration(Token::concat(start, last()), name);

    // Ok, we sure are in a struct declaration.
    // Try for templates
    templateDef(decl->templates);

    optional_whitespace_newline();

    if (!accept(CURLY_OPEN)) {
        raise(*cursor, "Expected opening curly brace to start structure declaration.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    while (true) {
        if (decl->addSubdecl(typeDecl())) {
            optional_whitespace_newline();
            continue;
        }

        if (decl->addField(simpleVariableDecl())) {
            if (!statement_separator()) {
                break;
            }
        } else {
            break;
        }
    }

    optional_whitespace_newline();

    if (!accept(CURLY_CLOSE)) {
        raise(*last(), "Expected semicolon, newline or closing curly brace in structure declaration.");
        cursor = start;
        return nullptr;
    }

    return decl;

    // TODO: add packing control to language syntax

}

inline BaseType *Parser::baseType() {
    Token *start = cursor;

    if (!name()) {
        return nullptr;
    }

    Token nameToken = Token::concat(start, last());

    auto _type = new BaseType(nameToken, nameToken.value());

    optional_whitespace();

    // Perhaps we have templates?
    templateInstance(_type, _type->templates);

    return _type;
}

// (PAREN_OPEN (Type (COMMA Type)* PAREN_CLOSE)? (ARROW Type)? )

inline bool Parser::closure_function_type_common(std::vector<std::unique_ptr<Type>>& argTypes, Type **retType) {
    Token *start = cursor;

    optional_whitespace();

    // Optional arguments
    if (accept_rewind(PAREN_OPEN)) {
        optional_whitespace();

        Type *curr;
        if ((curr = type())) {
            argTypes.emplace_back(curr);

            // op_ws COMMA op_ws TYPE
            while (true) {
                optional_whitespace();
                if (!accept_rewind(COMMA)) {
                    break;
                }

                optional_whitespace();

                curr = type();
                if (!curr) {
                    raise(*cursor, "Excpected type in function type argument list.");
                    cursor = start;
                    return false;
                }
                argTypes.emplace_back(curr);
            }
        }

        optional_whitespace();

        if (!accept(PAREN_CLOSE)) {
            raise(Token::concat(start, last()), "Expected closing parenthesis of funciton type.");
            cursor = start;
            return false;
        }
    }

    optional_whitespace();

    // Optional return type
    if (accept_rewind(ARROW)) {
        optional_whitespace();

        *retType = type();
        if (!*retType) {
            raise(*cursor, "Expected function type return type after arrow.");
            cursor = start;
            return false;
        }
    }

    return true;
}

inline FunctionType *Parser::functionType() {
    Token *start = cursor;

    if (!accept(FUNC_TYPE)) {
        cursor = start;
        return nullptr;
    }

    std::vector<std::unique_ptr<Type>> argTypes;
    Type *retType = nullptr;

    if (!closure_function_type_common(argTypes, &retType)) {
        cursor = start;
        return nullptr;
    }

    return new FunctionType(Token::concat(start, last()), std::move(argTypes), retType);
}

inline ClosureType *Parser::closureType() {
    Token *start = cursor;

    if (!accept(CLOSURE)) {
        cursor = start;
        return nullptr;
    }

    std::vector<std::unique_ptr<Type>> argTypes;
    Type *retType = nullptr;

    if (!closure_function_type_common(argTypes, &retType)) {
        cursor = start;
        return nullptr;
    }

    return new ClosureType(Token::concat(start, last()), std::move(argTypes), retType);
}

inline TupleType *Parser::tupleType() {
    Token *start = cursor;

    if (!accept_rewind(PAREN_OPEN)) {
        return nullptr;
    }

    std::vector<std::unique_ptr<Type>> types;
    Type *maybeType = type();

    if (maybeType) {
        types.emplace_back(maybeType);

        while (true) {
            Token *loopStart = cursor;
            optional_whitespace();

            if (!accept_rewind(COMMA)) {
                cursor = loopStart;
                break;
            }

            optional_whitespace();
            maybeType = type();
            if (!maybeType) {
                raise(*last(), "Expected type in tuple type list.");
                cursor = start;
                return nullptr;
            }
            types.emplace_back(maybeType);
        }
    }

    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected tuple closing parenthesis.");
        cursor = start;
        return nullptr;
    }

    return new TupleType(Token::concat(start, last()), std::move(types));
    // TODO: Add packing
}

inline Type *Parser::type() {
    // So, these are the types we can have:
    // Func type, Closure type, array type, pointer type
    // And finally, "just" a type (known as base type)
    Token *start = cursor;

    Type *ret = baseType();

    if (!ret) {
        ret = functionType();
        if (!ret) {
            ret = closureType();
            if (!ret) {
                ret = tupleType();
            }
        }
    }

    if (!ret) {
        cursor = start;
        return nullptr;
    }

    // Ok, we have some kind of type, check if it's a pointer or array type (or any combination) now.
    while (true) {
        if (accept_rewind(OP_TIMES)) {
            ret = new PointerType(Token::concat(start, last()), ret);
        } else if (accept_rewind(BRACK_OPEN)) {
            optional_whitespace();

            if (!accept(BRACK_CLOSE)) {
                raise(*last(), "Expected closing bracket of array type.");
                cursor = start;
                return nullptr;
            }

            ret = new ArrayType(Token::concat(start, last()), ret);
        } else {
            break;
        }
    }

    return ret;
}

inline Token *Parser::last() {
    return cursor - 1;
}

// Simplest form, just 'name : Type'
inline VariableDeclaration *Parser::simpleVariableDecl() {
    Token *start = cursor;

    if (!accept(IDENTIFIER)) {
        cursor = start;
        return nullptr;
    }

    std::string name = last()->value();

    optional_whitespace();

    if (!accept(COLON)) {
        cursor = start;
        return nullptr;
    }

    optional_whitespace();

    auto _type = type();
    if (!_type) {
        raise(*cursor, "Expected type in variable declaration.");

        cursor = start;
        return nullptr;
    }

    return new VariableDeclaration(Token::concat(start, last()), name, _type);
}

inline bool Parser::templateInstance(Node *parent, std::vector<std::unique_ptr<Type>>& templates) {
    Token *start = cursor;

    if (!accept(OP_LESS)) {
        cursor = start;
        return false;
    }

    // op_ws Type (op_ws COMMA op_ws Type)*
    optional_whitespace();

    auto maybeType = type();
    if (!maybeType) {
        cursor = start;
        return false;
    }

    maybeType->parent = parent;
    templates.emplace_back(maybeType);

    while (true) {
        optional_whitespace();

        if (!accept_rewind(COMMA)) {
            break;
        }

        optional_whitespace();

        maybeType = type();
        if (!maybeType) {
            cursor = start;
            return false;
        }

        maybeType->parent = parent;
        templates.emplace_back(maybeType);
    }

    optional_whitespace();

    if (!accept(OP_GREATER)) {
        cursor = start;
        return false;
    }

    return true;
}

inline bool Parser::templateDef(std::vector<TemplateDeclaration>& templates) {
    Token *start = cursor;

    if (!accept(OP_LESS)) {
        cursor = start;
        return false;
    }

    // op_ws IDENT (op_ws COMMA op_ws IDENT)*

    optional_whitespace();

    if (!accept(IDENTIFIER)) {
        cursor = start;
        return false;
    }

    templates.emplace_back(*last(), last()->value());

    while (true) {
        optional_whitespace();

        if (!accept_rewind(COMMA)) {
            break;
        }

        optional_whitespace();

        if (!accept(IDENTIFIER)) {
            // comma but no identifier, this is an error for sure
            raise(Token::concat(start, last()), "Expected identifier after comma in template list.");
            cursor = start;
            return false;
        }

        templates.emplace_back(*last(), last()->value());
    }

    optional_whitespace();

    if (!accept(OP_GREATER)) {
        raise(Token::concat(start, last()), "Template definition list should end with >");
        cursor = start;
        return false;
    }

    return true;
}

inline NamespaceDeclaration *Parser::namespace_() {
    // namespaceDecl <- op_ws NAMESPACE man_ws NAME op_ws_nl CURLY_OPEN op_ws_nl (Declaration | WHITESPACE | NEWLINE)* CURLY_CLOSE
    Token *start = cursor;

    optional_whitespace();
    if (!accept(NAMESPACE)) {
        cursor = start;
        return nullptr;
    }

    if (!mandatory_whitespace()) {
        raise(*cursor, "Malformed namespace declaration, expected whitespace after namespace keyword.");
        return nullptr;
    }

    Token *nameStart = cursor;

    if (!name()) {
        raise(*cursor, "Malformed namespace declaration, expected namespace identifier.");
        return nullptr;
    }

    auto *decl = new NamespaceDeclaration(Token::concat(start, last()), Token::concat(nameStart, last()).value());

    optional_whitespace_newline();

    if (!accept(CURLY_OPEN)) {
        raise(*cursor, "Namespace declaration should be followed by a declaration block.");
        return nullptr;
    }

    optional_whitespace_newline();

    Token *accepted = cursor;
    while (decl->addDeclaration(declaration()) || accept_rewind(WHITESPACE) || accept_rewind(NEWLINE)) { accepted = cursor; }
    cursor = accepted;

    if (!accept(CURLY_CLOSE)) {
        raise(*cursor, "Expected declaration or closing bracket in namespace declaration.");
        return nullptr;
    }

    return decl;
}

inline void Parser::optional_whitespace() {
    Token *start = cursor;

    if (!consumeAll(WHITESPACE)) {
        cursor = start;
    }
}

inline void Parser::optional_whitespace_newline() {
    while (accept_rewind(WHITESPACE) || accept_rewind(NEWLINE)) {}
}

inline bool Parser::mandatory_whitespace_newline() {
    bool ok = false;
    while (accept_rewind(WHITESPACE) || accept_rewind(NEWLINE)) { ok = true; }
    return ok;
}

inline bool Parser::mandatory_whitespace() {
    return consumeAll(WHITESPACE);
}

// Parses a use directive
inline Use *Parser::use() {
    // use <- op_ws USE man_ws USE_LIB UNIT_PATH?

    Token *start = cursor;
    optional_whitespace();

    if (!accept(USE) || !mandatory_whitespace() || !accept(USE_LIB)) {
        cursor = start;
        return nullptr;
    }

    auto useLib = last()->value();
    std::string usePath = "";

    Token *newStart = cursor;
    if (!accept(UNIT_PATH)) {
        cursor = newStart;
    } else {
        usePath = last()->value();
    }

    return new Use(Token::concat(start, last()), useLib, usePath);
}

inline Import *Parser::import() {
    // import <- op_ws IMPORT man_ws UNIT_PATH

    Token *start = cursor;
    optional_whitespace();

    if (!accept(IMPORT) || !mandatory_whitespace() || !accept(UNIT_PATH)) {
        cursor = start;
        return nullptr;
    }

    return new Import(Token::concat(start, last()), last()->value());
}

// Parses a 'name' (namespace'd identifier)
inline bool Parser::name() {
    // name <- (IDENTIFIER DOUBLE_COLON)* IDENTIFIER

    Token *start = cursor;

    bool inWildcard = true;

    while (inWildcard) {
        Token *wcStart = cursor;

        if (!accept(IDENTIFIER) || !accept(DOUBLE_COLON)) {
            cursor = wcStart;
            inWildcard = false;
        }
    }

    if (!accept(IDENTIFIER)) {
        cursor = start;
        return false;
    }

    return true;
}

inline FunctionDeclaration *Parser::funcDecl() {
    Token *start = cursor;

    if (!accept(IDENTIFIER)) {
        cursor = start;
        return nullptr;
    }

    std::string funcName = last()->value();

    optional_whitespace();

    if (!accept(COLON)) {
        cursor = start;
        return nullptr;
    }

    optional_whitespace();

    // Ok, let's check extern functions first
    // EXTERN man_ws FUNC op_ws_nl ARGLIST_DEF_OPT_NAMES op_ws (ARROW op_ws Type)?
    if (accept_rewind(EXTERN)) {
        if (!mandatory_whitespace()) {
            raise(*last(), "Expected whitespace after extern keyword.");
            cursor = start;
            return nullptr;
        }

        if (accept_rewind(INLINE)) {
            raise(*last(), "Extern function cannot bet inline.");
            cursor = start;
            return nullptr;
        }

        if (!accept(FUNC)) {
            // Not a function after all, woops!
            cursor = start;
            return nullptr;
        }

        // We are actually an extern function!
        auto fDecl = new FunctionDeclaration(Token::concat(start, last()), funcName, true, false);
        optional_whitespace_newline();

        templateDef(fDecl->templates);

        if (!fDecl->templates.empty()) {
            raise(Token::concat(start, last()), "Extern functions cannot define templates.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();

        arglist_def_opt_names(fDecl, fDecl->arglist);
        func_decl_return_type(fDecl);

        // Skip and try to find curly braces, throw a better error if there are
        Token *afterExtern = cursor;
        optional_whitespace_newline();
        if (accept(CURLY_OPEN)) {
            raise(fDecl->token, "Extern functions cannot have a function body.");
            cursor = start;
            return nullptr;
        }
        cursor = afterExtern;

        return fDecl;
    }

    bool is_inline = false;

    if (accept_rewind(INLINE)) {
        is_inline = true;
        optional_whitespace();
    }

    // Not extern, make sure we actually are a function
    // FUNC op_ws_nl ARGLIST_DEF_MAN_NAMES op_ws (ARROW op_ws Type)? op_ws_nl BODY
    if (!accept(FUNC)) {
        cursor = start;
        return nullptr;
    }

    auto fDecl = new FunctionDeclaration(Token::concat(start, last()), funcName, false, is_inline);

    optional_whitespace_newline();

    templateDef(fDecl->templates);
    optional_whitespace_newline();

    arglist_def_man_names(fDecl, fDecl->arglist);
    func_decl_return_type(fDecl);

    optional_whitespace_newline();

    auto maybeBody = scope();
    if (!maybeBody) {
        raise(fDecl->token, "Function declaration is missing body");
        cursor = start;
        return nullptr;
    }

    maybeBody->parent = fDecl;
    fDecl->body = std::unique_ptr<Scope>(maybeBody);

    return fDecl;
}

inline void Parser::func_decl_return_type(FunctionDeclaration *fDecl) {
    optional_whitespace();

    Type *retType = nullptr;
    if (accept_rewind(ARROW)) {
        optional_whitespace();

        retType = type();
        if (!retType) {
            raise(*last(), "Expected type after return arrow in extern function declaration.");
            return;
        }
    }

    fDecl->return_type = std::unique_ptr<Type>(retType);
}

inline void Parser::arglist_def_man_names(FunctionDeclaration *parent, std::vector<std::unique_ptr<VariableDeclaration>>& arglist) {
    // (PAREN_OPEN op_ws (simpleVariableDecl (op_ws COMMA op_ws simpleVariableDecl)*)? op_ws PAREN_CLOSE)?
    Token *start = cursor;

    // No args
    if (!accept_rewind(PAREN_OPEN)) {
        return;
    }

    optional_whitespace_newline();

    auto maybeArg = simpleVariableDecl();
    if (maybeArg) {
        maybeArg->parent = parent;
        arglist.emplace_back(maybeArg);

        while (true) {
            optional_whitespace_newline();

            if (!accept_rewind(COMMA)) {
                break;
            }

            optional_whitespace_newline();
            maybeArg = simpleVariableDecl();
            if (!maybeArg) {
                raise(*last(), "Expected argument declaration in argument list of fucntion.");
                cursor = start;
                return;
            }

            maybeArg->parent = parent;
            arglist.emplace_back(maybeArg);
        }
    }

    optional_whitespace_newline();
    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis of function argument list.");
        cursor = start;
        return;
    }
}

inline VariableDeclaration *Parser::opt_name_arg() {
    Token *start = cursor;

    auto maybeVdecl = simpleVariableDecl();
    if (maybeVdecl) {
        return maybeVdecl;
    }

    auto maybeType = type();
    if (maybeType) {
        return new VariableDeclaration(Token::concat(start, last()), "", maybeType);
    }

    return nullptr;
}

inline void Parser::arglist_def_opt_names(FunctionDeclaration *parent, std::vector<std::unique_ptr<VariableDeclaration>>& arglist) {
    // opt_name_arg = simpleVariableDecl | Type
    // (PAREN_OPEN op_ws (opt_name_arg (op_ws COMMA op_ws opt_name_arg)*)? op_ws PAREN_CLOSE)?

    Token *start = cursor;

    // No args
    if (!accept_rewind(PAREN_OPEN)) {
        return;
    }

    optional_whitespace_newline();

    auto maybeArg = opt_name_arg();
    if (maybeArg) {
        maybeArg->parent = parent;
        arglist.emplace_back(maybeArg);

        while (true) {
            optional_whitespace_newline();

            if (!accept_rewind(COMMA)) {
                break;
            }

            optional_whitespace_newline();
            maybeArg = opt_name_arg();
            if (!maybeArg) {
                raise(*last(), "Expected argument declaration in argument list of fucntion.");
                cursor = start;
                return;
            }

            maybeArg->parent = parent;
            arglist.emplace_back(maybeArg);
        }
    }

    optional_whitespace_newline();
    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis of function argument list.");
        cursor = start;
        return;
    }
}

inline Scope *Parser::scope() {
    // CURLY_OPEN op_ws_nl (Stmt stmt_separator)* op_ws_nl CURLY_CLOSE
    Token *start = cursor;

    if (!accept_rewind(CURLY_OPEN)) {
        return nullptr;
    }

    optional_whitespace_newline();

    auto scope = new Scope(*start);

    while (true) {
        if (!scope->addStmt(statement())) {
            break;
        }

        if (!statement_separator()) {
            break;
        }
    }

    optional_whitespace_newline();

    if (!accept(CURLY_CLOSE)) {
        raise(*last(), "Expected statement or closing curly brace in scope.");
        cursor = start;
        return nullptr;
    }

    return scope;
}

// TODO: Statements
inline Statement *Parser::statement() {
    Statement *ret;

    if ((ret = matchStmt())) {
        return ret;
    } else if ((ret = deferStmt())) {
        return ret;
    } else if ((ret = returnStmt())) {
        return ret;
    } else if ((ret = usingStmt())) {
        return ret;
    } else if ((ret = controlStmt())) {
        return ret;
    } else if ((ret = forStmt())) {
        return ret;
    } else if ((ret = whileStmt())) {
        return ret;
    } else if ((ret = declaration())) {
        return ret;
    } else if ((ret = scope())) {
        return ret;
    } else if ((ret = variableDecl())) {
        return ret;
    } else if ((ret = ifStmt())) {
        return ret;
    } else if ((ret = expression())) {
        // Last resort: expression.
        return ret;
    }

    return nullptr;
}

inline Statement *Parser::deferStmt() {
    Token *start = cursor;

    // This one is pretty straightforward
    if (!accept_rewind(DEFER)) {
        return nullptr;
    }

    optional_whitespace_newline();

    auto maybeScope = scope();
    if (!maybeScope) {
        raise(*last(), "Expected scope after defer directive.");
        cursor = start;
        return nullptr;
    }

    return new DeferStmt(Token::concat(start, last()), maybeScope);
}

inline Statement *Parser::matchStmt() {
    // MATCH op_ws_nl PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE op_ws_nl CURLY_OPEN op_ws_nl (CASE (op_ws_nl CASE)* (op_ws_nl ELSE_CASE)?)? op_ws_nl CURLY_CLOSE
    Token *start = cursor;

    if (!accept_rewind(MATCH)) {
        return nullptr;
    }

    optional_whitespace_newline();
    if (!accept(PAREN_OPEN)) {
        raise(*last(), "Expected parentheses surrounded expression after match keyword.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();
    auto maybeExpr = expression();
    if (!maybeExpr) {
        raise(*last(), "Expected expression between parentheses after match keyword.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();
    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesesi after match statement expression.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();
    if (!accept(CURLY_OPEN)) {
        raise(*last(), "Expected opening curly brace after match statement expression.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    std::vector<Case> cases;
    Scope *elseScope = nullptr;

    if (match_case(cases)) {
        while (true) {
            Token *loopStart = cursor;
            optional_whitespace_newline();
            if (!match_case(cases)) {
                cursor = loopStart;
                break;
            }
        }
    }

    optional_whitespace_newline();
    if (accept_rewind(ELSE)) {
        optional_whitespace_newline();
        elseScope = scope();

        if (!elseScope) {
            raise(*last(), "Expected scope after else case in match statement.");
            cursor = start;
            return nullptr;
        }
    }

    optional_whitespace_newline();
    if (!accept(CURLY_CLOSE)) {
        raise(*last(), "Expected closing curly brace for match statement.");
        cursor = start;
        return nullptr;
    }

    return new MatchStmt(Token::concat(start, last()), maybeExpr, std::move(cases), elseScope);
}

inline bool Parser::match_case(std::vector<Case>& cases) {
    //   CASE man_ws Expression op_ws_nl Scope
    // | CASE man_ws IS man_ws Identifier op_ws_nl (PAREN_OPEN op_ws_nl (Expression (op_ws_nl COMMA op_ws_nl Expression)*)? PAREN_CLOSE)? op_ws_nl Scope

    Token *start = cursor;

    if (!accept_rewind(CASE)) {
        return false;
    }

    if (!mandatory_whitespace()) {
        raise(*last(), "Expected whitespace after case directive.");
        cursor = start;
        return false;
    }

    // Try for an expression first!
    auto maybeExpr = expression();
    if (maybeExpr) {
        // Good!
        optional_whitespace_newline();
        auto maybeScope = scope();
        if (!maybeScope) {
            raise(*last(), "Expected scope after case expression.");
            cursor = start;
            return false;
        }

        cases.emplace_back(Token::concat(start, last()), maybeExpr, maybeScope);
        return true;
    }

    // Nope, try for case is
    if (!accept(IS)) {
        raise(*last(), "Expected expression or is keyword after case directive.");
        cursor = start;
        return false;
    }

    if (!mandatory_whitespace()) {
        raise(*last(), "Expected whitespace after case is.");
        cursor = start;
        return false;
    }

    if (!accept(IDENTIFIER)) {
        raise(*last(), "Expected identifier after case is.");
        cursor = start;
        return false;
    }

    std::string isTag = last()->value();

    // Try to find DOUBLE_COLON for a better error message
    Token *afterIdent = cursor;
    if (accept(DOUBLE_COLON)) {
        raise(*last(), "Expected single identifier in case is directive, you don't need to specify the ADT name.");
        cursor = start;
        return false;
    }
    cursor = afterIdent;

    // (PAREN_OPEN op_ws_nl (Expression (op_ws_nl COMMA op_ws_nl Expression)*)? PAREN_CLOSE)? op_ws_nl Scope

    optional_whitespace_newline();

    std::vector<std::unique_ptr<Expression>> isExprs;
    if (accept_rewind(PAREN_OPEN)) {
        optional_whitespace_newline();
        auto maybeExpr = expression();
        if (maybeExpr) {
            isExprs.emplace_back(maybeExpr);

            while (true) {
                Token *loopStart = cursor;
                optional_whitespace_newline();
                if (!accept(COMMA)) {
                    cursor = loopStart;
                    break;
                }
                optional_whitespace_newline();
                maybeExpr = expression();
                if (!maybeExpr) {
                    raise(*last(), "Expected expression after comma in case is rule list.");
                    cursor = start;
                    return false;
                }
                isExprs.emplace_back(maybeExpr);
            }
        }

        if (!accept(PAREN_CLOSE)) {
            raise(*last(), "Expected closing parenthesis after case is rule list.");
            cursor = start;
            return false;
        }
    }

    optional_whitespace_newline();
    auto maybeScope = scope();
    if (!maybeScope) {
        raise(*last(), "Expected scope in case is of match statement.");
        cursor = start;
        return false;
    }

    cases.emplace_back(Token::concat(start, last()), isTag, std::move(isExprs), maybeScope);
    return true;
}

inline Statement *Parser::controlStmt() {
    if (accept_rewind(BREAK)) {
        Token *start = cursor;

        // Maybe we have a label?
        optional_whitespace();
        if (accept(IDENTIFIER)) {
            return new BreakStmt(Token::concat(start - 1, last()), last()->value());
        }
        cursor = start;
        return new BreakStmt(Token::concat(start - 1, last()));
    } else if (accept_rewind(CONTINUE)) {
        Token *start = cursor;

        // Maybe we have a label?
        optional_whitespace();
        if (accept(IDENTIFIER)) {
            return new ContinueStmt(Token::concat(start - 1, last()), last()->value());
        }
        cursor = start;
        return new ContinueStmt(Token::concat(start - 1, last()));
    } else {
        return nullptr;
    }
}

inline Statement *Parser::usingStmt() {
    Token *start = cursor;
    if (!accept_rewind(USING)) {
        return nullptr;
    }

    if (!mandatory_whitespace()) {
        raise(*last(), "Expected whitespace after using keyword.");
        cursor = start;
        return nullptr;
    }

    Token *nameStart = cursor;
    if (!name()) {
        raise(*last(), "Expected namespace name after using directive.");
        cursor = start;
        return nullptr;
    }

    std::string usingName = Token::concat(nameStart, last()).value();

    Token *afterUsing = cursor;
    optional_whitespace_newline();

    auto maybeScope = scope();
    if (!maybeScope) {
        cursor = afterUsing;
    }

    return new UsingStmt(Token::concat(start, last()), usingName, maybeScope);
}

inline Statement *Parser::returnStmt() {
    Token *start = cursor;
    if (!accept_rewind(RETURN)) {
        return nullptr;
    }

    optional_whitespace_newline();

    auto expr = expression();
    return new ReturnStmt(Token::concat(start, last()), expr);
}

inline Statement *Parser::forInit() {
    Statement *ret = variableDecl();
    if (ret) {
        return ret;
    } else if ((ret = expression())) {
        return ret;
    }

    return nullptr;
}

inline ForStmt *Parser::forStmt() {
    // ForInit = VariableDecl | Expression
    // (IDENTIFIER op_ws COLON op_ws_nl)? FOR op_ws_nl PAREN_OPEN op_ws_nl ForInit (op_ws_nl COMMA op_ws_nl ForInit)
    // op_ws_nl SEMICOLON op_ws_nl Expression op_ws_nl SEMICOLON op_ws_nl Expression op_ws_nl PAREN_CLOSE op_ws_nl Statement

    Token *start = cursor;

    std::string label = "";

    if (accept_rewind(IDENTIFIER)) {
        label = last()->value();

        optional_whitespace();
        if (accept(COLON)) {
            optional_whitespace_newline();
        } else {
            // If we started with an identifier and had no colon afterwards, we are not a while statement.
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();
    }

    if (!accept(FOR)) {
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    if (!accept(PAREN_OPEN)) {
        raise(*last(), "Expected parenthesis after for statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto initScope = new Scope(*last());

    Token *beforeInit = cursor;

    auto maybeInit = forInit();
    if (!maybeInit) {
        cursor = beforeInit;
    } else {
        initScope->addStmt(maybeInit);

        while (true) {
            Token *loopStart = cursor;
            optional_whitespace_newline();

            if (!accept(COMMA)) {
                cursor = loopStart;
                break;
            }

            optional_whitespace_newline();

            maybeInit = forInit();
            if (!maybeInit) {
                raise(*last(), "Expected variable declaration or expression in for initialization list.");
                cursor = start;
                return nullptr;
            }
            initScope->addStmt(maybeInit);
        }
    }

    optional_whitespace_newline();

    if (!accept(SEMICOLON)) {
        raise(*last(), "Expected semicolon after for initialization list.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto condition = expression();

    optional_whitespace_newline();

    if (!accept(SEMICOLON)) {
        raise(*last(), "Expected semicolon after for loop condition.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto loopExpr = expression();

    optional_whitespace_newline();

    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis after for loop expression.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto body = statement();
    if (!body) {
        raise(*last(), "Expected body or statement of for loop.");
        cursor = start;
        return nullptr;
    }

    return new ForStmt(Token::concat(start, last()), label, initScope, condition, loopExpr, body);
}

inline WhileStmt *Parser::whileStmt() {
    // (IDENTIFIER op_ws COLON op_ws_nl)? WHILE op_ws_nl PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE op_ws_nl Statement
    Token *start = cursor;

    std::string label = "";

    if (accept_rewind(IDENTIFIER)) {
        label = last()->value();

        optional_whitespace();
        if (accept(COLON)) {
            optional_whitespace_newline();
        } else {
            // If we started with an identifier and had no colon afterwards, we are not a while statement.
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();
    }

    if (!accept(WHILE)) {
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    if (!accept(PAREN_OPEN)) {
        raise(*last(), "Expected parentheses surrounded loop condition in while statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto maybeExpr = expression();
    if (!maybeExpr) {
        raise(*last(), "Expected loop condition between parentheses in while statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis after while statement loop condition.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto maybeStmt = statement();
    if (!maybeStmt) {
        raise(*last(), "Expected while statement body or loop statement.");
        cursor = start;
        return nullptr;
    }

    return new WhileStmt(Token::concat(start, last()), label, maybeExpr, maybeStmt);
}

inline IfStmt *Parser::ifStmt() {
    // IF op_ws_nl PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE op_ws_nl Statement (man_ws_nl ELSE op_ws_nl Statement);
    Token *start = cursor;

    if (!accept_rewind(IF)) {
        return nullptr;
    }

    optional_whitespace_newline();

    if (!accept(PAREN_OPEN)) {
        raise(*last(), "Excpected parentheses surrounded condition in if statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto maybeExpr = expression();
    if (!maybeExpr) {
        raise(*last(), "Expected condition expression in if statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis after condition expression in if statement.");
        cursor = start;
        return nullptr;
    }

    optional_whitespace_newline();

    auto ifStmt = statement();
    if (!ifStmt) {
        raise(*last(), "Expected if branch statement.");
        cursor = start;
        return nullptr;
    }

    Token *afterIf = cursor;

    if (mandatory_whitespace_newline() && accept(ELSE)) {
        optional_whitespace_newline();
        auto elseStmt = statement();
        if (!elseStmt) {
            raise(*last(), "Expected else branch statement.");
            cursor = start;
            return nullptr;
        }

        return new IfStmt(Token::concat(start, last()), maybeExpr, ifStmt, elseStmt);
    }

    cursor = afterIf;

    return new IfStmt(Token::concat(start, last()), maybeExpr, ifStmt);
}

inline void Parser::variable_decl_modifiers(bool& extern_mod, bool& static_mod) {
    Token *start = cursor;
    while (true) {
        Token *after = cursor;
        if (mandatory_whitespace()) {
            if (accept_rewind(EXTERN)) {
                if (extern_mod) {
                    raise(*last(), "Extern modifier has already been specified.");
                    cursor = start;
                    return;
                }
                extern_mod = true;
            } else if (accept_rewind(STATIC)) {
                if (static_mod) {
                    raise(*last(), "Static modifier has already been specified.");
                    cursor = start;
                    return;
                }
                static_mod = true;
            } else {
                cursor = after;
                break;
            }
        } else {
            cursor = after;
            break;
        }
    }
}

inline VariableDeclaration *Parser::variableDecl() {
    Token *start = cursor;

    // Identifier op_ws COLON (man_ws (EXTERN | STATIC))* op_ws Type (op_ws OP_ASS op_ws_nl Expression)?

    if (!accept_rewind(IDENTIFIER)) {
        return nullptr;
    }

    std::string name = last()->value();
    optional_whitespace();

    bool extern_mod = false;
    bool static_mod = false;

    if (accept_rewind(COLON)) {
        variable_decl_modifiers(extern_mod, static_mod);

        optional_whitespace();
        auto maybeType = type();
        if (!maybeType) {
            raise(*last(), "Expected type in variable declaration.");
            cursor = start;
            return nullptr;
        }

        auto decl = new VariableDeclaration(Token::concat(start, last()), name, maybeType);
        decl->extern_mod = extern_mod;
        decl->static_mod = static_mod;

        // See if we have an initial expression
        Token *after = cursor;
        optional_whitespace();
        if (accept(OP_ASS)) {
            optional_whitespace_newline();
            auto maybeExpr = expression();
            if (!maybeExpr) {
                raise(*last(), "Expected expression as variable declaration initializer.");
                cursor = start;
                return nullptr;
            }

            if (extern_mod) {
                raise(decl->token, "Initialized variable declaration cannot possibly be extern.");
                cursor = start;
                return nullptr;
            }

            decl->init_expr = std::unique_ptr<Expression>(maybeExpr);
        } else {
            cursor = after;
        }

        return decl;
    }

    if (!accept(OP_INF_ASS)) {
        cursor = start;
        return nullptr;
    }

    // Identifier op_ws OP_INF_ASS (man_ws (EXTERN | STATIC))* op_ws Expression

    variable_decl_modifiers(extern_mod, static_mod);
    optional_whitespace();

    auto maybeExpr = expression();
    if (!maybeExpr) {
        raise(*last(), "Expected expression after infer assign operator.");
        cursor = start;
        return nullptr;
    }

    if (extern_mod) {
        raise(Token::concat(start, last()), "Infer assign cannot possibly be extern.");
        cursor = start;
        return nullptr;
    }

    auto decl = new VariableDeclaration(Token::concat(start, last()), name, maybeExpr);
    decl->static_mod = static_mod;

    return decl;
}

// Expressions bellow, tread with caution

inline Expression *Parser::expression() {
    return assignment();
}

inline Expression *Parser::assignment() {
    // Assignment = IfExpr | IfExpr op_ws_nl (OP_ASS | OP_PLUS_EQ | OP_MINUS_EQ | OP_TIMES_EQ | OP_DIV_EQ | OP_MOD_EQ | OP_BIT_AND_EQ | OP_BIT_XOR_EQ | OP_BIT_OR_EQ) op_ws_nl IfExpr
    Token *start = cursor;
    auto left = ifExpr();

    if (!left) {
        return nullptr;
    }
    Token *afterLeft = cursor;

    optional_whitespace_newline();

    if (accept_rewind(OP_ASS) || accept_rewind(OP_PLUS_EQ) || accept_rewind(OP_MINUS_EQ) || accept_rewind(OP_TIMES_EQ) || 
        accept_rewind(OP_DIV_EQ) || accept_rewind(OP_MOD_EQ) || accept_rewind(OP_BIT_AND_EQ) || accept_rewind(OP_BIT_XOR_EQ) ||
        accept_rewind(OP_BIT_OR_EQ)) {

        auto opid = last()->id;

        optional_whitespace_newline();
        auto right = ifExpr();
        if (!right) {
            raise(*last(), "Expected right hand side of assignment.");
            cursor = start;
            return nullptr;
        } 

        return new Assignment(Token::concat(start, last()), left, right, opid);
    } else {
        cursor = afterLeft;
        return left;
    }
}

inline Expression *Parser::ifExpr() {
    // IfExpr = LogicalOr | IF op_ws_nl PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE op_ws_nl (Expression | Scope) op_ws_nl ELSE op_ws_nl (Expression | Scope)
    Token *start = cursor;

    // Check wether we actually have an 'if'
    if (accept_rewind(IF)) {
        // Bingo!
        optional_whitespace_newline();

        if (!accept(PAREN_OPEN)) {
            raise(*last(), "Expected parentheses surrounded if condition.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();

        auto condition = expression();
        if (!condition) {
            raise(*last(), "Expected expression in if expression condition parentheses.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();
        if (!accept(PAREN_CLOSE)) {
            raise(*last(), "Expected closing parenthesis after expression in if expression condition.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();

        auto ifScope = scope();

        if (!ifScope) {
            // Well, we must have an expression then
            auto maybeExpr = expression();
            if (!maybeExpr) {
                raise(*last(), "If expression body should either be a scope or a single expression.");
                cursor = start;
                return nullptr;
            }

            ifScope = new Scope(maybeExpr->token);
            ifScope->addStmt(maybeExpr);
        }

        if (!mandatory_whitespace_newline()) {
            raise(*last(), "Excpected whitespace or newline after if scope.");
            cursor = start;
            return nullptr;
        }

        // Ok, look for the else branch.
        if (!accept(ELSE)) {
            raise(*last(), "If expressions must always have an else branch.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();

        auto elseScope = scope();
        if (!elseScope) {
            auto maybeExpr = expression();
            if (!maybeExpr) {
                raise(*last(), "Else branch of if expression must either have a scope or expression body.");
                cursor = start;
                return nullptr;
            }

            elseScope = new Scope(maybeExpr->token);
            elseScope->addStmt(maybeExpr);
        }

        return new IfExpr(Token::concat(start, last()), condition, ifScope, elseScope);
    }

    // Not an if expression, pass on!
    return logicalOr();
}

inline Expression *Parser::logicalOr() {
    // LogicalOr = LogicalAnd | LogicalOr op_ws_nl OP_LOG_OR op_ws_nl LogicalAnd
    Token *start = cursor;

    auto curr = logicalAnd();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept(OP_LOG_OR)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeLAnd = logicalAnd();
            if (!maybeLAnd) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeLAnd, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::logicalAnd() {
    // LogicalAnd = BitOr | LogicalAnd op_ws_nl OP_LOG_AND op_ws_nl BitOr
    Token *start = cursor;

    auto curr = bitOr();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept(OP_LOG_AND)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeBOr = bitOr();
            if (!maybeBOr) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeBOr, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::bitOr() {
    // BitOr = BitXor | BitOr op_ws_nl OP_BIT_OR op_ws_nl BitXor
    Token *start = cursor;

    auto curr = bitXor();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept(OP_BIT_OR)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeBXor = bitXor();
            if (!maybeBXor) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeBXor, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::bitXor() {
    // BitXor = BitAnd | BitXor op_ws_nl OP_BIT_XOR op_ws_nl BitAnd
    Token *start = cursor;

    auto curr = bitAnd();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept(OP_BIT_XOR)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeBAnd = bitAnd();
            if (!maybeBAnd) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeBAnd, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::bitAnd() {
    // BitAnd = Equality | BitAnd op_ws_nl OP_BIT_AND op_ws_nl Equality
    Token *start = cursor;

    auto curr = equality();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept(OP_BIT_AND)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeEq = equality();
            if (!maybeEq) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeEq, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::equality() {
    // Equality = Relational | Equality op_ws_nl (OP_EQ | OP_NEQ) op_ws_nl Relational
    Token *start = cursor;

    auto curr = relational();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept_rewind(OP_EQ) || accept(OP_NEQ)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeRel = relational();
            if (!maybeRel) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeRel, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::relational() {
    // Relational = Shift | Relational op_ws_nl (OP_LESSER | OP_GREATER | OP_LESSER_EQ | OP_GREATER_EQ) op_ws_nl Shift
    Token *start = cursor;

    auto curr = shift();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept_rewind(OP_LESS) || accept_rewind(OP_GREATER) || accept_rewind(OP_LESS_EQ) || accept(OP_GREATER_EQ)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeShift = shift();
            if (!maybeShift) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeShift, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::shift() {
    // Shift = Additive | Shift op_ws_nl (OP_LOGICAL_SHIFT_RIGHT | OP_LOGICAL_SHIFT_LEFT | OP_ARITHM_SHIFT_RIGHT | OP_ARITHM_SHIFT_LEFT) op_ws_nl Additive
    Token *start = cursor;

    auto curr = additive();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept_rewind(OP_LOGICAL_SHIFT_RIGHT) || accept_rewind(OP_LOGICAL_SHIFT_LEFT) || accept_rewind(OP_ARITHM_SHIFT_RIGHT) ||
            accept(OP_ARITHM_SHIFT_LEFT)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeAdd = additive();
            if (!maybeAdd) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeAdd, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::additive() {
    // Additive = Multiplicative | Additive op_ws_nl (OP_PLUS | OP_MINUS) op_ws_nl Multiplicative
    Token *start = cursor;

    auto curr = multiplicative();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept_rewind(OP_PLUS) || accept(OP_MINUS)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeMult = multiplicative();
            if (!maybeMult) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeMult, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::multiplicative() {
    // Multiplicative = Cast | Multiplicative op_ws_nl (OP_TIMES | OP_DIV | OP_MOD) op_ws_nl Cast
    Token *start = cursor;

    auto curr = cast();

    if (!curr) {
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace_newline();
        if (accept_rewind(OP_TIMES) || accept_rewind(OP_DIV) || accept(OP_MOD)) {
            auto opid = last()->id;

            optional_whitespace_newline();

            auto maybeCast = cast();
            if (!maybeCast) {
                raise(*last(), "Expected right hand side of operator.");
                cursor = start;
                return nullptr;
            }

            curr = new BinaryOperator(Token::concat(start, last()), curr, maybeCast, opid);
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::cast() {
    Token *start = cursor;
    // Cast = Prefix | Cast man_ws AS man_ws Type | IsExpr
    // A cast always start with a prefix expression.
    auto curr = prefix();

    if (!curr) {
        return nullptr;
    }

    // It is in fact legal to make a chain of casts, though I don't see why you would do it.
    while (true) {
        Token *after = cursor;
        if (mandatory_whitespace() && accept(AS)) {
            if (!mandatory_whitespace()) {
                raise(*last(), "as keyword must be followed by whitespace.");
                cursor = start;
                return nullptr;
            }

            auto maybeType = type();
            if (!maybeType) {
                raise(*last(), "as casting keyword should be followed by a type.");
                cursor = start;
                return nullptr;
            }

            curr = new Cast(Token::concat(start, last()), curr, maybeType);
            continue;
        }

        cursor = after;
        // Check for is-expr
        // IsExpr = Cast man_ws IS man_ws IDENTIFIER op_ws_nl (PAREN_OPEN op_ws_nl (Expression (op_ws_nl COMMA op_ws_nl Expression)*)? op_ws_nl PAREN_CLOSE)?
        if (mandatory_whitespace() && accept(IS)) {
            if (!mandatory_whitespace()) {
                raise(*last(), "is keyword must be followed by whitespace.");
                cursor = start;
                return nullptr;
            }

            if (!accept(IDENTIFIER)) {
                raise(*last(), "is keyword must be followed by an identifier.");
                cursor = start;
                return nullptr;
            }

            std::string isTag = last()->value();

            // Special error checking!
            if (accept_rewind(DOUBLE_COLON)) {
                raise(*last(), "is expressions must be followed by identifiers, you don't need to specify the ADT name.");
                cursor = start;
                return nullptr;
            }

            optional_whitespace_newline();

            std::vector<std::unique_ptr<Expression>> isExprs;
            if (accept_rewind(PAREN_OPEN)) {
                optional_whitespace_newline();

                auto maybeExpr = expression();
                if (maybeExpr) {
                    isExprs.emplace_back(maybeExpr);

                    while (true) {
                        Token *loopStart = cursor;
                        optional_whitespace_newline();
                        if (!accept(COMMA)) {
                            cursor = loopStart;
                            break;
                        }

                        optional_whitespace_newline();
                        maybeExpr = expression();
                        if (!maybeExpr) {
                            raise(*last(), "Expected expression in is expression rule list.");
                            cursor = start;
                            return nullptr;
                        }

                        isExprs.emplace_back(maybeExpr);
                    }
                }

                optional_whitespace_newline();
                if (!accept(PAREN_CLOSE)) {
                    raise(*last(), "Expected closing parenthesis after is expression rule list.");
                    cursor = start;
                    return nullptr;
                }
            }

            curr = new IsExpr(Token::concat(start, last()), curr, isTag, std::move(isExprs));
        } else {
            cursor = after;
            break;
        }
    }

    return curr;
}

inline Expression *Parser::prefix() {
    // Prefix = Postfix | (OP_PLUS | OP_MINUS | OP_BANG | OP_BIT_NOT | OP_TIMES | OP_BIT_AND) Prefix | SIZEOF op_ws_nl PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE
    Token *start = cursor;

    // Here is our strategy:
    // We advance our cursor until we stop finding prefix operators.
    // There, we check that we have a postfix or a sizeof expression.
    // If we don't, we do not have a prefix expression.
    // If we do, we reverse the cursor until our start, building up an expression, then return that.

    while (cursor->id == OP_PLUS || cursor->id == OP_MINUS || cursor->id == OP_BANG || cursor->id == OP_BIT_NOT || cursor->id == OP_TIMES ||
           cursor->id == OP_BIT_AND) {
        cursor++;
    }

    Token *afterSkip = cursor;

    auto curr = postfix();

    if (!curr) {
        // Check for sizeof
        if (!accept_rewind(SIZEOF)) {
            // We don't have sizeof either, so we are not a prefix expression
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();
        if (!accept(PAREN_OPEN)) {
            raise(*last(), "Expected parenthesis after sizeof keyword.");
            cursor = start;
            return nullptr;
        }

        optional_whitespace_newline();
        auto maybeExpr = expression();

        if (!maybeExpr) {
            auto maybeType = type();
            if (!maybeType) {
                raise(*last(), "Expected expression or type in sizeof parentheses.");
                cursor = start;
                return nullptr;
            }

            curr = new Sizeof(Token::concat(start, last()), maybeType);
        } else {
            curr = new Sizeof(Token::concat(start, last()), maybeExpr);
        }

        optional_whitespace_newline();
        if (!accept(PAREN_CLOSE)) {
            raise(*last(), "Expected closing parenthesis after single expression in sizeof operator.");
            cursor = start;
            return nullptr;
        }
    }

    Token *afterCurr = cursor;

    cursor = afterSkip - 1;
    while (cursor >= start) {
        curr = new UnaryOperator(Token::concat(start, cursor), curr, cursor->id);

        cursor--;
    }

    cursor = afterCurr;
    return curr;
}

inline Expression *Parser::postfix() {
    // Postfix = Atom | FunctionCall | ArrayIndex | FieldAccess

    // Every postfix expression begins with an atom
    Token *start = cursor;

    Expression *curr = atom();
    if (!curr) {
        cursor = start;
        return nullptr;
    }

    while (true) {
        Token *after = cursor;

        optional_whitespace();

        // Check for array indexing
        // ArrayIndex = Postfix op_ws BRACK_OPEN op_ws_nl Expression op_ws_nl BRACK_CLOSE
        if (accept_rewind(BRACK_OPEN)) {
            optional_whitespace_newline();

            auto maybeIndex = expression();
            if (!maybeIndex) {
                raise(*last(), "Expected index in array indexing expression.");
                cursor = start;
                return nullptr;
            }

            optional_whitespace_newline();
            if (!accept(BRACK_CLOSE)) {
                raise(*last(), "Expected closing bracker in array indexing expression.");
                cursor = start;
                return nullptr;
            }

            curr = new ArrayIndexing(Token::concat(start, last()), curr, maybeIndex);
            continue;
        }

        optional_whitespace_newline();
        if (accept(PAREN_OPEN)) {
            // Actual function call!
            std::vector<Argument> args;

            optional_whitespace_newline();
            function_call_arg_list(args);

            curr = new FunctionCall(Token::concat(start, last()), curr, std::move(args));
            continue;
        }

        // Neither array indexing or a function call.
        // It must be a field access, right?

        // We need to reset our cursor since we don't want any whitespace
        // FieldAccess = Postfix DOT Identifier
        cursor = after;
        if (accept_rewind(DOT)) {
            if (!accept(IDENTIFIER)) {
                raise(*last(), "Expected identifier after dot for a field access.");
                cursor = start;
                return nullptr;
            }

            std::string fieldName = last()->value();
            curr = new FieldAccess(Token::concat(start, last()), curr, fieldName);
        } else {
            // None of the above!
            cursor = after;
            break;
        }
    }

    return curr;
}

inline void Parser::function_call_arg_list(std::vector<Argument>& args) {
    Token *start = cursor;
    // ArgList = (Arg (op_ws_nl COMMA op_ws_nl Arg)*)? op_ws_nl PAREN_CLOSE

    if (function_call_argument(args)) {
        while (true) {
            optional_whitespace_newline();

            if (!accept_rewind(COMMA)) {
                break;
            }

            optional_whitespace_newline();
            if (!function_call_argument(args)) {
                break;
            }
        }
    }

    optional_whitespace_newline();

    if (!accept(PAREN_CLOSE)) {
        raise(*last(), "Expected closing parenthesis after argument list of function call.");
        cursor = start;
        return;
    }
}

inline bool Parser::function_call_argument(std::vector<Argument>& args) {
    // Arg = Identifier COLON op_ws_nl Expression | Expression
    Token *start = cursor;

    if (accept(IDENTIFIER) && accept(COLON) && mandatory_whitespace_newline()) {
        std::string argname = start->value();

        auto maybeExpr = expression();
        if (!maybeExpr) {
            cursor = start;
            return false;
        }

        args.emplace_back(argname, maybeExpr);
        return true;
    } else {
        cursor = start;

        auto maybeExpr = expression();
        if (!maybeExpr) {
            cursor = start;
            return false;
        }

        args.emplace_back(maybeExpr);
        return true;
    }
}

inline Expression *Parser::atom() {
    // Atom = IntLiteral | CharLiteral | StringLiteral | BoolLiteral | VariableAccess | OperatorAccess | PAREN_OPEN op_ws_nl Expression op_ws_nl PAREN_CLOSE
    Expression *ret;

    if ((ret = nullLiteral())) {
        return ret;
    } else if ((ret = floatLiteral())) {
        return ret;
    } else if ((ret = intLiteral())) {
        return ret;
    } else if ((ret = charLiteral())) {
        return ret;
    } else if((ret = stringLiteral())) {
        return ret;
    } else if ((ret = boolLiteral())) {
        return ret;
    } else if ((ret = variableAccess())) {
        return ret;
    } else if (accept_rewind(PAREN_OPEN)) {
        auto maybeExpr = expression();
        if (!maybeExpr) {
            raise(*last(), "Expected expression between parenthesis.");
            return nullptr;
        }

        if (!accept_rewind(PAREN_CLOSE)) {
            raise(*last(), "Expected closing parenthesis or expression.");
            return nullptr;
        }

        return maybeExpr;
    }

    return nullptr;
}

inline FloatLiteral *Parser::floatLiteral() {
    if (accept_rewind(FLOAT_LITERAL)) {
        return new FloatLiteral(*last(), last()->value());
    }

    return nullptr;
}

inline IntLiteral *Parser::intLiteral() {
    if (accept_rewind(INT_LITERAL)) {
        return new IntLiteral(*last(), last()->value());
    }

    return nullptr;
}

inline CharLiteral *Parser::charLiteral() {
    if (accept_rewind(CHARACTER_LITERAL)) {
        return new CharLiteral(*last(), last()->value());
    }

    return nullptr;
}

inline StringLiteral *Parser::stringLiteral() {
    if (accept_rewind(STRING_LITERAL)) {
        return new StringLiteral(*last(), last()->value());
    }

    return nullptr;
}

inline BoolLiteral *Parser::boolLiteral() {
    if (accept_rewind(BOOL_LITERAL)) {
        return new BoolLiteral(*last(), last()->value());
    }

    return nullptr;
}

inline NullLiteral *Parser::nullLiteral() {
    if (accept_rewind(NULL_LITERAL)) {
        return new NullLiteral(*last());
    }

    return nullptr;
}

inline VariableAccess *Parser::variableAccess() {
    Token *start = cursor;
    if (!name()) {
        return nullptr;
    }

    Token nameToken = Token::concat(start, last());

    Token *afterName = cursor;
    auto vAcc = new VariableAccess(nameToken, nameToken.value());

    optional_whitespace();
    templateInstance(vAcc, vAcc->templates);

    if (vAcc->templates.empty()) {
        cursor = afterName;
    }

    return vAcc;
}
