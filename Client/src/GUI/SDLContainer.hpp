#pragma once
#include <SDL3/SDL.h>
#include <memory>

class SDLContainer {
private:
    std::shared_ptr<class Client>   m_connection;
    std::shared_ptr<class HtmlView> m_view;
    struct SDL_Renderer*            m_renderer;
    struct SDL_Window*              m_window;
    float                           m_delta_time;
    float                           m_refresh_rate;
    uint32_t                        m_height;
    uint32_t                        m_width;
    uint64_t                        m_current_tick;
    uint64_t                        m_last_tick; 


public:
    SDLContainer(uint32_t width, uint32_t height);
    ~SDLContainer();

    SDL_AppResult AppInit(int argc, char** argv);
    SDL_AppResult AppIterate();
    SDL_AppResult AppEvent(SDL_Event* event);
    void AppQuit(SDL_AppResult result);

    void on_packet_receive(std::unique_ptr<class Packet> packet);
    void render();

private:
    void size_changed();
};