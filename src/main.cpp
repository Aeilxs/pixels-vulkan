
#include "app/app.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        ps::app::App app;
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
