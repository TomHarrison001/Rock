#include "examples/modelApp.hpp"
#include "examples/computeApp.hpp"
#include "examples/engineApp.hpp"

int main()
{
    Application* app = new ModelApp();
    //Application* app = new ComputeApp();
    //Application* app = new EngineApp();

    try
    {
        app->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        delete app;
        app = nullptr;
        return EXIT_FAILURE;
    }

    delete app;
    app = nullptr;
    return EXIT_SUCCESS;
}
