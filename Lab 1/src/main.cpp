#include <core/Application.hpp>
#include <native/ImGUILayer.hpp>

using namespace retro;

int SDL_main(int argc, char** argv)
{
    core::Application app;
    app.EmplaceLayer<ImGUILayer>("ImGUILayer");

    return app.Run();
}