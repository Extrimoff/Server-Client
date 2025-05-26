#include "SDLContainer.hpp"
#include "Elements/el_input.hpp"
#include "HtmlView/HtmlView.hpp"
#include "../Network/Client/Client.hpp"
#include "../Network/PacketManager/PacketManager.hpp"

#include <print>
#include <SDL3_ttf/SDL_ttf.h>


SDL_AppResult SDLContainer::AppInit(int argc, char** argv)
{
    std::setlocale(LC_ALL, "ru_RU.UTF-8");

    m_connection = std::make_shared<Client>(this);
    if (m_connection->connectTo("127.0.0.1", 8081) == SocketStatus::connected) {
        std::println(stderr,
            "Connected to remote server!\n"
            "Client handling thread pool size: {}",
            m_connection->getThreadPool().getThreadCount());
    }
    else {
        std::println(stderr, "Client isn't connected");
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println(stderr, "SDL init failed");
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        std::println(stderr, "Failed to init TTF");
        return SDL_APP_FAILURE;
    }
    SDL_CreateWindowAndRenderer("Гостиница", m_width, m_height, SDL_WINDOW_RESIZABLE, &m_window, &m_renderer);
    if (!m_window || !m_renderer) {
        std::println(stderr, "SDL window/renderer failed");
        return SDL_APP_FAILURE;
    }

    int displayIndex = SDL_GetDisplayForWindow(m_window);
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayIndex);
    m_refresh_rate = mode->refresh_rate;

    SDL_SetRenderVSync(m_renderer, SDL_RENDERER_VSYNC_ADAPTIVE);

    m_view = std::make_shared<HtmlView>(m_connection);
    m_view->init_pages();

    m_view->render(m_width);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLContainer::AppIterate()
{
    if (!m_view->needsUpdate) { 
        SDL_Delay(10); 
        return SDL_APP_CONTINUE; 
    }
    Uint32 frameStart = SDL_GetTicks();

    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);
    m_last_tick = m_current_tick;
    m_current_tick = SDL_GetTicks();
    m_delta_time = (m_current_tick - m_last_tick) / 1000.f;

    m_view->render(m_width);
    
    litehtml::position pos(0, 0, m_width, m_height);
    m_view->draw(reinterpret_cast<litehtml::uint_ptr>(m_renderer), &pos);

    SDL_RenderPresent(m_renderer);

    //m_view->needsUpdate = false;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLContainer::AppEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        m_connection->disconnect(false);
        return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_RESIZED:
    {
        this->size_changed();
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    {
        auto mouseEvent = reinterpret_cast<SDL_MouseMotionEvent*>(event);
        m_view->on_mouse_move(static_cast<int>(mouseEvent->x), static_cast<int>(mouseEvent->y));
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        auto mouseEvent = reinterpret_cast<SDL_MouseButtonEvent*>(event);
        m_view->on_lButton_up(static_cast<int>(mouseEvent->x), static_cast<int>(mouseEvent->y));
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        m_view->needsUpdate = true;
        auto mouseEvent = reinterpret_cast<SDL_MouseButtonEvent*>(event);
        m_view->on_lButton_down(static_cast<int>(mouseEvent->x), static_cast<int>(mouseEvent->y));
        break;
    }
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    {
        auto windowEvent = reinterpret_cast<SDL_WindowEvent*>(event);
        m_view->on_mouse_leave();
        break;
    }
    case SDL_EVENT_KEY_UP:
    {
        auto keyboardEvent = reinterpret_cast<SDL_KeyboardEvent*>(event);
        m_view->on_key_up(keyboardEvent->key, keyboardEvent->mod);
        break;
    }
    case SDL_EVENT_KEY_DOWN:
    {
        m_view->needsUpdate = true;
        auto keyboardEvent = reinterpret_cast<SDL_KeyboardEvent*>(event);
        m_view->on_key_down(keyboardEvent->key, keyboardEvent->mod);
        break;
    }
    case SDL_EVENT_TEXT_INPUT:
    {
        auto textInputEvent = reinterpret_cast<SDL_TextInputEvent*>(event);
        m_view->on_text_input(textInputEvent->text);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
    {
        m_view->needsUpdate = true;
        auto mouseWheelEvent = reinterpret_cast<SDL_MouseWheelEvent*>(event);
        m_view->on_mouse_wheel(static_cast<int>(mouseWheelEvent->y));
        break;
    }
    default:
        break;
    }

    return SDL_APP_CONTINUE;
}

void SDLContainer::AppQuit(SDL_AppResult result)
{
    TTF_Quit();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void SDLContainer::size_changed() {
    int h, w;
    SDL_GetWindowSize(m_window, &w, &h);
    m_height = h;
    m_width = w;
    m_view->media_changed();
    m_view->render(m_width);
}

void SDLContainer::on_packet_receive(std::unique_ptr<class Packet> packet)
{
    m_view->on_packet_receive(std::move(packet));
}

void SDLContainer::render()
{
    m_view->needsUpdate = true;
}

SDLContainer::SDLContainer(uint32_t width, uint32_t height) : m_renderer(nullptr), m_window(nullptr), m_connection(nullptr), m_last_tick(0),
m_current_tick(0), m_delta_time(0.f), m_view(nullptr), m_height(height), m_width(width) {

}

SDLContainer::~SDLContainer() = default;