#ifndef OPTIONS__HPP
#define OPTIONS__HPP

#include <Errors.hpp>

class Options {
public:
    Options(Options const&) = delete;
    void operator=(Options const&) = delete;

    static Options& get() {
        static Options instance;
        return instance;
    }

    void read(int argc, char *argv[]) {
        err_handler = new DefaultErrorHandler();
    }

    ErrorHandler *err_handler;
private:
    Options() {};
};

#endif
