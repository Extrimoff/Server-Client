#include "HtmlView.hpp"
#include "../Fonts/font.hpp"
#include "../SDLContainer.hpp"
#include "../Elements/el_input.hpp"
#include "../../Network/PacketManager/PacketManager.hpp"

#include "../Pages/Page.hpp"
#include "../Pages/BookingsPage/BookingsPage.hpp"
#include "../Pages/LoginPage/LoginPage.hpp"
#include "../Pages/ProfilePage/ProfilePage.hpp"
#include "../Pages/RoomsPage/RoomsPage.hpp"

#include <print>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>

HtmlView::HtmlView(std::shared_ptr<class Client> connection) : m_connection(connection), m_isPagesInit(false), m_scroll_y(0), m_scroll_x(0), m_current_cursor("auto"), m_needsUpdate(true) {}
HtmlView::~HtmlView() = default;

litehtml::element::ptr HtmlView::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc)
{
    //std::println("Called {} tag_name: {}", __FUNCTION__, tag_name);
    if (strcmp(tag_name, "input")) return nullptr;

    auto newTag = std::make_shared<el_input>(doc);
    std::string value = "";
    std::string placeholder = "";
    bool isPassword = false;

    for (const auto& attribute : attributes) {
        if (attribute.first == "value") {
            value = attribute.second;
        }
        else if (attribute.first == "placeholder") {
            placeholder = attribute.second;
        }
        else if (attribute.first == "type") {
            if (attribute.second == "password") 
                isPassword = true;
        }
    }

    newTag->init()
        .value(value)
        .placeholder(placeholder)
        .isPassword(isPassword);

    m_current_page->on_custom_element_create(newTag);

    return newTag;
}

void HtmlView::delete_font(litehtml::uint_ptr f)
{
    /*std::println("Called {}", __FUNCTION__);*/
    TTF_CloseFont(reinterpret_cast<TTF_Font*>(f));
    
}

int HtmlView::text_width(const char* text, litehtml::uint_ptr f)
{
    //std::println("Called {} {}", __FUNCTION__, text);
    if (!f || !text) return 0;

    TTF_Font* font = reinterpret_cast<TTF_Font*>(f);
    int w, h;
    if (TTF_GetStringSize(font, text, 0, &w, &h)) {
        return w;
    }
    else {
        std::println(stderr, "TTF_SizeUTF8 failed: {}", SDL_GetError());
        return 0;
    }
}

void HtmlView::draw_text(litehtml::uint_ptr hdc, const char* text,
    litehtml::uint_ptr f, litehtml::web_color color,
    const litehtml::position& pos)
{
    SDL_Color sdlColor = { color.red, color.green, color.blue, color.alpha };
    auto font = reinterpret_cast<TTF_Font*>(f);

    SDL_Surface* textSurface = TTF_RenderText_Blended(reinterpret_cast<TTF_Font*>(f), text, 0, sdlColor);
    if (!textSurface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(reinterpret_cast<SDL_Renderer*>(hdc), textSurface);
    if (!texture) {
        SDL_DestroySurface(textSurface);
        return;
    }

    SDL_FRect dstRect = { static_cast<float>(pos.left()), static_cast<float>(pos.top()), static_cast<float>(textSurface->w), static_cast<float>(textSurface->h) };
    SDL_RenderTexture(reinterpret_cast<SDL_Renderer*>(hdc), texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(textSurface);
}
litehtml::uint_ptr	HtmlView::create_font(const litehtml::font_description& descr, const litehtml::document* doc, litehtml::font_metrics* fm)
{
    //std::println("Called {} descr.size {}",  __FUNCTION__, descr.size);
    SDL_IOStream* rw = SDL_IOFromConstMem(Arial_Font, sizeof(Arial_Font));
    if (!rw) {
        std::println(stderr, "Failed to create RWops from memory");
        return -1;
    }
    auto font = TTF_OpenFontIO(rw, 1, static_cast<float>(descr.size));
    if (!font) {
        std::println(stderr, "Failed to open font from memory");
        return -1;
    }

    // Получаем метрики

    int ascent = TTF_GetFontAscent(font);
    int descent = -TTF_GetFontDescent(font); // делаем положительным
    int height = TTF_GetFontLineSkip(font);
    int x_height = TTF_GetFontHeight(font) / 2; // приближённо, точного способа нет

    if (fm) {
        fm->ascent = static_cast<int>(ascent);
        fm->descent = static_cast<int>(descent);
        fm->height = static_cast<int>(height);
        fm->x_height = static_cast<int>(x_height);
    }

    return reinterpret_cast<litehtml::uint_ptr>(font);
}
int HtmlView::pt_to_px(int pt) const
{
    /*std::println("Called {}", __FUNCTION__);*/
    return pt / 72 * 98;
}

int HtmlView::get_default_font_size() const
{
    /*std::println("Called {}", __FUNCTION__);*/
    return 16;
}
const char* HtmlView::get_default_font_name() const
{
    /*std::println("Called {}", __FUNCTION__);*/ return "Arial";
}

void HtmlView::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color)
{
    const litehtml::position& pos = layer.clip_box;
    const litehtml::border_radiuses& radii = layer.border_radius;

    int x = pos.left();
    int y = pos.top();
    int w = pos.width;
    int h = pos.height;

    Uint8 r = color.red;
    Uint8 g = color.green;
    Uint8 b = color.blue;
    Uint8 a = 255;

    int radius = static_cast<int>(radii.top_left_x);

    roundedBoxRGBA(reinterpret_cast<SDL_Renderer*>(hdc), x, y, x + w, y + h, radius, r, g, b, a);
}
void HtmlView::draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::linear_gradient& gradient)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::radial_gradient& gradient)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::conic_gradient& gradient)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
    /*std::println("Called {}", __FUNCTION__);*/
    const litehtml::position& pos = draw_pos;
    const litehtml::border_radiuses& radii = borders.radius;

    int x = pos.left();
    int y = pos.top();
    int w = pos.width;
    int h = pos.height;

    Uint8 r = borders.left.color.red;
    Uint8 g = borders.left.color.green;
    Uint8 b = borders.left.color.blue;
    Uint8 a = 255;

    int radius = static_cast<int>(radii.top_left_x);
    roundedRectangleRGBA(reinterpret_cast<SDL_Renderer*>(hdc), x, y, x + w, y + h, radius, r, g, b, a);
}


void HtmlView::set_base_url(const char* base_url)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::on_anchor_click(const char* url, const litehtml::element::ptr& el)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

bool HtmlView::on_element_click(const litehtml::element::ptr& el)
{
    /*std::println("Called {} {}", __FUNCTION__, el->get_tagName());*/

    //std::println("Here");
    if (m_current_page->on_element_click(el)) this->render();

    return false;
}

void HtmlView::on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event)
{
    m_needsUpdate = true;
    /*std::println("Called {} {}", __FUNCTION__, el->get_tagName());*/
}

void HtmlView::set_cursor(const char* cursor)
{
   /* std::println("Called {} cursor: {}", __FUNCTION__, cursor);*/
    if (m_current_cursor == cursor) return;

    SDL_Cursor* sdl_cursor = nullptr;

    if (!strcmp(cursor, "auto") || !strcmp(cursor, "")) {
        sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    }
    else if (!strcmp(cursor, "text")) {
        sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    }
    else if (!strcmp(cursor, "pointer")) {
        sdl_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    }

    SDL_SetCursor(sdl_cursor);
    m_current_cursor = cursor;
}

void HtmlView::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    /*std::println("Called {}", __FUNCTION__);*/
    switch (tt)
    {
    case litehtml::text_transform_none:
        return;
        break;
    case litehtml::text_transform_capitalize:
    {
        bool in_word = false;
        for (size_t i = 0; i < text.length(); ++i) {
            if (std::isalnum(text[i])) {
                if (!in_word) {
                    text[i] = std::toupper(text[i]);
                    in_word = true;
                }
            }
            else {
                in_word = false;
            }
        }
    }
    break;
    case litehtml::text_transform_uppercase:
        std::transform(text.begin(), text.end(), text.begin(), ::toupper);
        break;
    case litehtml::text_transform_lowercase:
        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        break;
    default:
        break;
    }
}

void HtmlView::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::del_clip()
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::get_client_rect(litehtml::position& client) const
{
    /*std::println("Called {}", __FUNCTION__);*/
    int x, y, w, h;
    this->get_rect(&x, &y, &w, &h);
    client.x = x;
    client.y = y;
    client.height = h;
    client.width = w;
}

void HtmlView::import_css(litehtml::string& text, const litehtml::string& url,
    litehtml::string& baseurl)
{
    /*std::println("Called {}", __FUNCTION__);*/
}

void HtmlView::set_caption(const char* caption)
{
    /*std::println("Called {}", __FUNCTION__);*/
    auto window = SDL_GetKeyboardFocus();
    SDL_SetWindowTitle(window, caption);
}
void HtmlView::get_media_features(litehtml::media_features& media) const
{
    /*std::println("Called {}", __FUNCTION__);*/
    int x, y, w, h;
    this->get_rect(&x, &y, &w, &h);
    media.type = litehtml::media_type_screen;
    media.width = w;
    media.height = h;
    media.color = 8;
    media.monochrome = 0;
    media.resolution = 96;
}

void HtmlView::get_language(litehtml::string& language, litehtml::string& culture) const
{
    /*std::println("Called {}", __FUNCTION__);*/
    language = "ru";
    culture = "RU";
}

void HtmlView::on_mouse_move(int x, int y) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_mouse_move(x - m_scroll_x, y - m_scroll_y);
}

void HtmlView::on_key_down(uint32_t vKey, uint16_t keyMode) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_key_down(vKey, keyMode);
}

void HtmlView::on_text_input(const char* text) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_text_input(text);
}

void HtmlView::init_pages()
{
    m_pages.emplace_back(std::make_shared<LoginPage>(shared_from_this()));
    m_pages.emplace_back(std::make_shared<RoomsPage>(shared_from_this()));
    m_pages.emplace_back(std::make_shared<ProfilePage>(shared_from_this()));
    m_pages.emplace_back(std::make_shared<BookingsPage>(shared_from_this()));


    for (const auto& page : m_pages) {
        m_current_page = page;
        if(!page->init()) throw std::runtime_error("Error while initializing pages");
    }

    m_isPagesInit = true;

    m_current_page = this->get_page<LoginPage>();
}

void HtmlView::switch_page(PageID id, nlohmann::json additionData)
{
    auto page = this->get_page(id);

    m_current_page = page;
    m_current_page->on_switch(std::move(additionData));
    
    this->reset_scroll();
    int x, y, w, h;
    this->get_rect(&x, &y, &w, &h);
    this->render(w);
}

void HtmlView::on_key_up(uint32_t vKey, uint16_t keyMode) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_key_up(vKey);
}

void HtmlView::on_lButton_down(int x, int y)
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_lButton_down(x - m_scroll_x, y - m_scroll_y);
}

void HtmlView::on_lButton_up(int x, int y) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_lButton_up(x - m_scroll_x, y - m_scroll_y);
}

void HtmlView::on_mouse_leave() const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->on_mouse_leave();
}

void HtmlView::media_changed() const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->media_changed();
}

uint32_t HtmlView::render(int width)
{
    if (!m_isPagesInit) return 0;// throw std::runtime_error("Pages are not initialized");

    int x, y, w, h;
    this->get_rect(&x, &y, &w, &h);

    if (width == 0) width = w;

    m_needsUpdate = true;
    uint32_t min_width = m_current_page->render(width);


    if (min_width > static_cast<uint32_t>(w)) {
        auto window = SDL_GetKeyboardFocus();
        SDL_SetWindowSize(window, min_width, h);
    }

    return min_width;
}

void HtmlView::draw(litehtml::uint_ptr hdc, const litehtml::position* clip) const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");
    m_current_page->draw(hdc, m_scroll_y, m_scroll_x, clip);
}

void HtmlView::on_mouse_wheel(int scrollY)
{
    m_scroll_y += scrollY * 40;

    litehtml::size doc_size;
    m_current_page->get_doc_size(doc_size);

    int x, y, w, h;
    this->get_rect(&x, &y, &w, &h);

    int max_scroll = std::max(0, doc_size.height - h);

    m_scroll_y = std::clamp(m_scroll_y, -max_scroll, 0);
}

void HtmlView::on_packet_receive(std::unique_ptr<class Packet> packet)
{
    m_current_page->on_packet_receive(std::move(packet));
}

template <typename TRet>
std::shared_ptr<TRet> HtmlView::get_page() const
{
    if (!m_isPagesInit) throw std::runtime_error("Pages are not initialized");

    for (auto& page : m_pages)
        if (auto pRet = std::dynamic_pointer_cast<TRet>(page))
            return pRet;
    

    return nullptr;
}

std::shared_ptr<class Page> HtmlView::get_page(PageID id) const
{
    if (!m_isPagesInit) return nullptr;

    for (auto& page : m_pages)
        if (page->get_id() == id)
            return page;
        
    return nullptr;
}

void HtmlView::get_rect(int* x, int* y, int* w, int* h) const
{
    auto window = SDL_GetKeyboardFocus();
    SDL_GetWindowSize(window, w, h);
    SDL_GetWindowPosition(window, x, y);
}

std::string HtmlView::replace_placeholder(std::string input, const std::string& placeholder, const std::string& content)
{
    size_t pos = input.find(placeholder);
    if (pos != std::string::npos) {
        input.replace(pos, placeholder.length(), content);
    }
    return input;
}
void HtmlView::reset_scroll()
{
    m_scroll_x = 0;
    m_scroll_y = 0;
}