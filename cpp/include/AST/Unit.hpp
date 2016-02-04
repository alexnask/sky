#ifndef UNIT__HPP
#define UNIT__HPP

#include "Node.hpp"
#include "Use.hpp"
#include "Import.hpp"

#include <vector>
#include <memory>

class Unit : public Node {
public:
    Unit (const std::string& path, char *_contents) : Node(Token::empty, nullptr, Node::Kind::Unit), unit_path(path), contents(_contents) {}

    ~Unit () {
        delete[] contents;
    }

    bool addUse(Use *use) {
        if (!use) return false;

        use->parent = this;

        uses.push_back(std::unique_ptr<Use>(use));
        return true;
    }

    bool addImport(Import *import) {
        if (!import) return false;

        import->parent = this;

        imports.push_back(std::unique_ptr<Import>(import));
        return true;
    }

    bool addDeclaration(Declaration *decl) {
        if (!decl) return false;

        decl->parent = this;

        decls.push_back(std::unique_ptr<Declaration>(decl));
        return true;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string debugString() {
        return "Unit[path=" + unit_path + ']';
    }

    std::string displayString() {
        return unit_path;
    }

    std::string unit_path;

    char *contents;

    std::vector<std::unique_ptr<Use>> uses;
    std::vector<std::unique_ptr<Import>> imports;

    std::vector<std::unique_ptr<Declaration>> decls;
};

#endif
