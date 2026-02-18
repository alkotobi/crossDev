#include "../include/app_runner.h"
#include <iostream>

int main(int argc, const char* argv[]) {
    try {
        AppRunner app(argc, argv);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
