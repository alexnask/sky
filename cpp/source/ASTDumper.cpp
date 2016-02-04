#include <ASTDumper.hpp>

ASTDumper::ASTDumper (Node *root, std::string outpath) {
    file = fopen(outpath.c_str(), "w");
    nodeCounter = 0;

    writeln("digraph {");

    root->accept(*this);

    writeln("}");
    fclose(file);
}

void ASTDumper::write(const std::string& str) {
    fwrite(str.data(), str.size(), 1, file);
}

void ASTDumper::writeln(const std::string& str) {
    write(str + '\n');
}

int ASTDumper::node(const std::string& text) {
    writeln("node" + std::to_string(nodeCounter) + " [label=\"" + text + "\"]");
    return nodeCounter++;
}

int ASTDumper::record(std::initializer_list<std::string> values) {
    std::string buff;
    buff += "node" + std::to_string(nodeCounter) + "[shape=record, label=\"{";

    bool first = true;

    for (auto& val: values) {
        if (first) {
            first =  false;
        } else {
            buff += '|';
        }

        buff += val;
    }

    buff += "}\"]";
    writeln(buff);
    return nodeCounter++;
}

int ASTDumper::current() {
    return nodeCounter;
}

void ASTDumper::child(const std::string& _desc) {
    desc = _desc;
}

void ASTDumper::edge(int a, int b) {
    std::string buff = "node" + std::to_string(a) + " -> node" + std::to_string(b);
    if (!desc.empty()) {
        buff += " [label=\"" + desc + "\"]";
        desc.clear();
    }
    writeln(buff);
}

void ASTDumper::walk(Unit *unit) {
    parent_id = node(unit->unit_path);

    for (auto& use: unit->uses) {
        child("use");
        use->accept(*this);
    }

    for (auto& import: unit->imports) {
        child("import");
        import->accept(*this);
    }

    for (auto& decl: unit->decls) {
        child("decl");
        decl->accept(*this);
    }
}

void ASTDumper::walk(Use *use) {
    edge(parent_id, node(use->str()));
}

void ASTDumper::walk(Import *import) {
    edge(parent_id, node(import->str()));
}

void ASTDumper::walk(TemplateDeclaration *temp) {
    edge(parent_id, node(temp->name));
}

void ASTDumper::walk(NamespaceDeclaration *ns) {
    int parent = parent_id;

    parent_id = node(ns->str());
    edge(parent, parent_id);

    for (auto& decl: ns->decls) {
        child("decl");
        decl->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(VariableDeclaration *vDecl) {
    int parent = parent_id;

    parent_id = node("var_decl " + vDecl->name);
    edge(parent, parent_id);

    child("modifiers");
    edge(parent_id, record({ "extern: " + std::to_string(vDecl->extern_mod), "static: " + std::to_string(vDecl->static_mod) }));

    if (vDecl->type) {
        child("type");
        vDecl->type->accept(*this);
    }

    if (vDecl->init_expr) {
        child("init_expr");
        vDecl->init_expr->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(StructDeclaration *decl) {
    int parent = parent_id;

    parent_id = node("struct_decl " + decl->name);
    edge(parent, parent_id);

    for (auto& temp: decl->templates) {
        child("template");
        temp.accept(*this);
    }

    for (auto& sdecl: decl->subdecls) {
        child("sub_decl");
        sdecl->accept(*this);
    }

    for (auto& field: decl->fields) {
        child("field");
        field->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(AliasDeclaration *decl) {
    int parent = parent_id;

    parent_id = node("alias_decl " + decl->name);
    edge(parent, parent_id);

    child("from_type");
    decl->from_type->accept(*this);

    for (auto& temp: decl->templates) {
        child("template");
        temp.accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(VariantDeclaration *decl) {
    int parent = parent_id;

    parent_id = node("variant_decl " + decl->name);
    edge(parent, parent_id);

    if (decl->from_type) {
        child("from_type");
        decl->from_type->accept(*this);
    }

    for (auto& sdecl: decl->subdecls) {
        child("sub_decl");
        sdecl->accept(*this);
    }

    for (auto& field: decl->fields) {
        child("member");
        edge(parent_id, node(field.str()));
    }

    parent_id = parent;
}

void ASTDumper::walk(FunctionDeclaration *decl) {
    int parent = parent_id;

    parent_id = node("func_decl " + decl->name);
    edge(parent, parent_id);

    child("modifiers");
    edge(parent_id, record({ "extern: " + std::to_string(decl->is_extern), "inline: " + std::to_string(decl->is_inline) }));

    for (auto& temp: decl->templates) {
        child("template");
        temp.accept(*this);
    }

    for (auto& arg: decl->arglist) {
        child("arg");
        arg->accept(*this);
    }

    if (decl->return_type) {
        child("return_type");
        decl->return_type->accept(*this);
    }

    if (decl->body) {
        child("body");
        decl->body->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(Scope *scope) {
    int parent = parent_id;

    parent_id = node("scope");
    edge(parent, parent_id);

    for (auto& stmt: scope->statements) {
        child("stmt");
        stmt->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(IfStmt *ifS) {
    int parent = parent_id;

    parent_id = node("if");
    edge(parent, parent_id);

    child("condition");
    ifS->condition->accept(*this);

    child("if_stmt");
    ifS->ifStmt->accept(*this);

    if (ifS->elseStmt) {
        child("else_stmt");
        ifS->elseStmt->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(WhileStmt *whSt) {
    int parent = parent_id;

    parent_id = node("while " + whSt->label);
    edge(parent, parent_id);

    child("condition");
    whSt->condition->accept(*this);

    child("body");
    whSt->body->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(ForStmt *fS) {
    int parent = parent_id;

    parent_id = node("for " + fS->label);
    edge(parent, parent_id);

    child("init_scope");
    fS->initScope->accept(*this);

    if (fS->condition) {
        child("condition");
        fS->condition->accept(*this);
    }

    if (fS->loopExpr) {
        child("loop_expr");
        fS->loopExpr->accept(*this);
    }

    child("body");
    fS->body->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(ReturnStmt *ret) {
    int parent = parent_id;

    parent_id = node("return");
    edge(parent, parent_id);

    child("expr");
    ret->expr->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(UsingStmt *us) {
    int parent = parent_id;

    parent_id = node("using " + us->name);
    edge(parent, parent_id);

    if (us->scope) {
        child("body");
        us->scope->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(BreakStmt *bs) {
    edge(parent_id, node(bs->str()));
}

void ASTDumper::walk(ContinueStmt *cs) {
    edge(parent_id, node(cs->str()));
}

void ASTDumper::walk(DeferStmt *defer) {
    int parent = parent_id;

    parent_id = node("defer");
    edge(parent, parent_id);

    child("body");
    defer->scope->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(MatchStmt *match) {
    int parent = parent_id;

    parent_id = node("match");
    edge(parent, parent_id);

    child("matched_expr");
    match->matched_expr->accept(*this);

    for (auto& ca: match->cases) {
        int loop_parent = parent_id;

        if (ca.kind == Case::Kind::Simple) {
            parent_id = node("case");
            child("expr");
            ca.expr->accept(*this);
        } else {
            parent_id = node("case is");
            child("tag");
            edge(parent_id, node(ca.tag));

            for (auto& rule: ca.exprs) {
                child("rule");
                rule->accept(*this);
            }
        }

        child("body");
        ca.body->accept(*this);

        child("case");
        edge(loop_parent, parent_id);

        parent_id = loop_parent;
    }

    if (match->else_scope) {
        child("else_body");
        match->else_scope->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(ClosureType *ct) {
    int parent = parent_id;

    parent_id = node("closure_type");
    edge(parent, parent_id);

    for (auto& arg_type: ct->argTypes) {
        child("arg_type");
        arg_type->accept(*this);
    }

    if (ct->returnType) {
        child("return_type");
        ct->returnType->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(TupleType *type) {
    int parent = parent_id;

    parent_id = node("tuple_type");
    edge(parent, parent_id);

    for (auto& t: type->types) {
        child("type");
        t->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(FunctionType *ft) {
    int parent = parent_id;

    parent_id = node("function_type");
    edge(parent, parent_id);

    for (auto& arg_type: ft->argTypes) {
        child("arg_type");
        arg_type->accept(*this);
    }

    if (ft->returnType) {
        child("return_type");
        ft->returnType->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(PointerType *pt) {
    int parent = parent_id;

    parent_id = node("pointer_type");
    edge(parent, parent_id);

    child("inner");
    pt->inner->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(ArrayType *at) {
    int parent = parent_id;

    parent_id = node("array_type");
    edge(parent, parent_id);

    child("inner");
    at->inner->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(BaseType *bt) {
    edge(parent_id, node(bt->str()));
}

void ASTDumper::walk(VariableAccess *vAcc) {
    int parent = parent_id;
    parent_id = node(vAcc->name);
    edge(parent, parent_id);

    for (auto& temp: vAcc->templates) {
        child("template");
        temp->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(BoolLiteral *bl) {
    edge(parent_id, node(bl->value ? "true" : "false"));
}

void ASTDumper::walk(StringLiteral *sl) {
    edge(parent_id, node("string_literal"));
}

void ASTDumper::walk(CharLiteral *cl) {
    edge(parent_id, node(std::string("'") + cl->value + "'"));
}

void ASTDumper::walk(NullLiteral *nl) {
    edge(parent_id, node("null"));
}

void ASTDumper::walk(IntLiteral *il) {
    int parent = parent_id;

    parent_id = node("int_literal");
    edge(parent, parent_id);

    child("value");
    edge(parent_id, node(std::to_string(il->value)));

    child("type");
    il->type->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(FloatLiteral *fl) {
    int parent = parent_id;

    parent_id = node("float_literal");
    edge(parent, parent_id);

    child("value");
    edge(parent_id, node(std::to_string(fl->value)));

    child("type");
    fl->type->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(ArrayIndexing *ai) {
    int parent = parent_id;

    parent_id = node("array_indexing");
    edge(parent, parent_id);

    child("base");
    ai->base->accept(*this);

    child("index");
    ai->index->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(FunctionCall *fCall) {
    int parent = parent_id;

    parent_id = node("function_call");
    edge(parent, parent_id);

    child("base");
    fCall->base->accept(*this);

    for (auto& arg: fCall->args) {
        int loop_parent = parent_id;

        parent_id = node("arg");
        edge(loop_parent, parent_id);

        if (!arg.name.empty()) {
            child("name");
            edge(parent_id, node(arg.name));
        }

        child("expr");
        arg.expr->accept(*this);

        parent_id = loop_parent;
    }

    parent_id = parent;
}

void ASTDumper::walk(Sizeof *sf) {
    int parent = parent_id;

    parent_id = node("sizeof");
    edge(parent, parent_id);

    if (sf->expr) {
        child("expr");
        sf->expr->accept(*this);
    } else if (sf->arg_type) {
        child("arg_type");
        sf->arg_type->accept(*this);
    }

    parent_id = parent;
}

void ASTDumper::walk(UnaryOperator *op) {
    int parent = parent_id;

    parent_id = node(std::string() + op->opChar());
    edge(parent, parent_id);

    child("expr");
    op->expr->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(Cast *cast) {
    int parent = parent_id;

    parent_id = node("cast");
    edge(parent, parent_id);

    child("expr");
    cast->expr->accept(*this);

    child("type");
    cast->type->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(BinaryOperator *op) {
    int parent = parent_id;

    parent_id = node(op->opStr());
    edge(parent, parent_id);

    child("left");
    op->left->accept(*this);

    child("right");
    op->right->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(IfExpr *ie) {
    int parent = parent_id;

    parent_id = node("if_expr");
    edge(parent, parent_id);

    child("condition");
    ie->condition->accept(*this);

    child("if_scope");
    ie->ifScope->accept(*this);

    child("else_scope");
    ie->elseScope->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(Assignment *ass) {
    int parent = parent_id;

    parent_id = node(ass->assStr());
    edge(parent, parent_id);

    child("left");
    ass->left->accept(*this);

    child("right");
    ass->right->accept(*this);

    parent_id = parent;
}

void ASTDumper::walk(FieldAccess *fa) {
    int parent = parent_id;

    parent_id = node("field_access");
    edge(parent, parent_id);

    child("expr");
    fa->expr->accept(*this);

    child("field_name");
    edge(parent_id, node(fa->field_name));

    parent_id = parent;
}

void ASTDumper::walk(IsExpr *is) {
    int parent = parent_id;

    parent_id = node("is");
    edge(parent, parent_id);

    child("base");
    is->base->accept(*this);

    child("tag");
    edge(parent_id, node(is->tag));

    for (auto& expr: is->exprs) {
        child("expr");
        expr->accept(*this);
    }

    parent_id = parent;
}
