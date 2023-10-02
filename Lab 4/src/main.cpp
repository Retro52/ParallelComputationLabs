#include <ImGUILayer.hpp>
#include <core/include/Application.hpp>

#ifndef _OPENMP
# error "OpenMP is not supported"
#endif

using namespace retro;

int SDL_main(int argc, char** argv)
{
    core::Application app;
    app.EmplaceLayer<ImGUILayer>("ImGUILayer");

    return app.Run();
}
