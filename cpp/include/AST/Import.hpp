#ifndef IMPORT__HPP
#define IMPORT__HPP

#include "Node.hpp"
#include "Walker.hpp"

class Import : public Node {
public:
    Import (Token _token, const std::string& path) : Node(_token, nullptr, Node::Kind::Import), unit_path(path) {}

    std::string debugString() {
        return "IMPORT[unit_path=" + unit_path +']';
    }

    std::string displayString() {
        return "import " + unit_path;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string unit_path;
};

#endif
