/** \file main.cpp */

#include "computeShaderApp.hpp"

int main() {
    ComputeShaderApp* app = new ComputeShaderApp();
    
    try {
        app->run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        delete app;
        app = nullptr;
        return EXIT_FAILURE;
    }

    delete app;
    app = nullptr;
    return EXIT_SUCCESS;
}
