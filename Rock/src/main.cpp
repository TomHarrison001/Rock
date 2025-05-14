/** \file main.cpp */

#include "modelApp.hpp"
#include "computeShaderApp.hpp"

int main() {
    Application* app = new ModelApp();
    //Application* app = new ComputeShaderApp();
    
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
