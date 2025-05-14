/** \file main.cpp */

#include "examples/modelApp.hpp"
#include "examples/computeApp.hpp"

int main() {
    Application* app = new ModelApp();
    //Application* app = new ComputeApp();
    
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
