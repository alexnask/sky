#ifndef USE__HPP
#define USE__HPP

#include "Node.hpp"
#include "Walker.hpp"

class Use : public Node {
public:
    Use (Token _token, const std::string& lib, const std::string& path) : Node(_token, nullptr, Node::Kind::Use), lib_name(lib), unit_path(path) {}

    std::string debugString() {
        return "USE[lib_name=" + lib_name + ", unit_path=" + unit_path +']';
    }

    std::string displayString() {
        return "use " + lib_name + '/' + unit_path;
    }

    void accept(Walker& w) {
        w.walk(this);
    }

    std::string lib_name;
    std::string unit_path;
};

#endif
