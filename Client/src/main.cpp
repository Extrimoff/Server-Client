#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "GUI/SDLContainer.hpp"
#include "Network/Client/Client.hpp"


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    auto container = new SDLContainer(800, 800);
    *appstate = container;
    return container->AppInit(argc, argv);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto container = reinterpret_cast<SDLContainer*>(appstate);
    return container->AppIterate();
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto container = reinterpret_cast<SDLContainer*>(appstate);
    return container->AppEvent(event);
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto container = reinterpret_cast<SDLContainer*>(appstate);
    return container->AppQuit(result);
}