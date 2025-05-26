#include "el_input.hpp"
#include "../HtmlView/HtmlView.hpp"

#include <SDL3/SDL.h>
#include <SDL3_gfx/SDL3_gfxPrimitives.h>
#include <litehtml/render_item.h>
#include <print>
#include <iostream>

el_input::el_input(const std::shared_ptr<litehtml::document>& doc, bool isPassword)
    : custom_element(element_id::input), litehtml::html_tag(doc), m_content(std::make_shared<el_input_content>("", doc)), m_isPassword(isPassword) {
}

el_input& el_input::init() {
    this->appendChild(m_content);
    return *this;
}

el_input& el_input::value(const std::string& value, bool force)
{
    auto place = this->get_placement();
    auto font = this->css().get_font();
    auto doc = this->get_document();
    auto width = doc->container()->text_width(value.c_str(), font);
    if (width > place.width && !force) return *this;

    m_value = value;
    if (m_value == "") this->set_content(m_placeholder, true);
    else this->set_content(m_value);
    return *this;
}

el_input& el_input::placeholder(const std::string& placeholder)
{
    auto place = this->get_placement();
    auto font = this->css().get_font();
    auto doc = this->get_document();
    auto width = doc->container()->text_width(placeholder.c_str(), font);
    if (width > place.width) return *this;

    m_placeholder = placeholder;
    if (m_value == "") this->set_content(m_placeholder, true);
    else this->set_content(m_value);
    return *this;
}

el_input& el_input::isPassword(bool isPassword)
{
    m_isPassword = isPassword;
    return *this;
}

void el_input::set_content(const std::string& content, bool placeholder) 
{  
    if (m_isPassword && !placeholder)  m_content->set_text(std::string(content.length(), '*'), placeholder);
    else m_content->set_text(content, placeholder);
}

const std::string& el_input::get_value() const
{
    return m_value;
}

void el_input::draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri) 
{
    litehtml::html_tag::draw(hdc, x, y, clip, ri); 
}

bool el_input::on_lbutton_down() 
{
    this->on_focus();
    m_has_focus = true;
    return true;
}

bool el_input::on_lbutton_up(bool is_click) 
{
    this->on_focus();
    return true;
}

void el_input::on_key_down(uint32_t vKey, uint16_t keyMode)
{
    if (!m_has_focus) return;
    
    auto removeCharacter = [](std::string& str, size_t pos) {
        size_t bytePos = 0;
        size_t charCount = 0;

        while (bytePos < str.length()) {

            unsigned char byte1 = str[bytePos];
            size_t charSize = 0;

            if (byte1 <= 0x7F) {
                charSize = 1;
            }
            else if ((byte1 >> 5) == 0x6) {
                charSize = 2;
            }
            else if ((byte1 >> 4) == 0xE) {
                charSize = 3;
            }
            else if ((byte1 >> 3) == 0x1E) {
                charSize = 4;
            }

            if (charCount == pos) {
                str.erase(bytePos, charSize);
                return;
            }

            bytePos += charSize;
            charCount++;
        }
    };

    auto countCharacters = [](const std::string& str) -> size_t {
        size_t count = 0;
        size_t bytePos = 0;

        while (bytePos < str.length()) {
            unsigned char byte1 = str[bytePos];
            size_t charSize = 0;

            if (byte1 <= 0x7F) {
                charSize = 1;
            }
            else if ((byte1 >> 5) == 0x6) {
                charSize = 2;
            }
            else if ((byte1 >> 4) == 0xE) {
                charSize = 3;
            }
            else if ((byte1 >> 3) == 0x1E) {
                charSize = 4;
            }

            bytePos += charSize;
            count++;
        }

        return count;
    };

    if (vKey == SDLK_BACKSPACE && !m_value.empty()) {
        auto len = countCharacters(m_value);
        removeCharacter(m_value, len - 1);

        this->value(m_value);
    }
    
}

void el_input::on_text_input(const char* text) 
{
    if (!m_has_focus) return; 

    this->value(m_value + text);
    
}

void el_input::on_focus() {
    m_has_focus = true;
    auto window = SDL_GetKeyboardFocus();
    SDL_StartTextInput(window);
    m_content->on_focus();
}

void el_input::on_focus_lost() {
    m_has_focus = false;
    auto window = SDL_GetKeyboardFocus();
    SDL_StopTextInput(window);
    m_content->on_focus_lost();
}

void el_input_content::draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri)
{
    if (is_white_space() && !m_draw_spaces) return;

    auto pos = ri->pos();
    pos.x += x;
    pos.y += y;

    if (!pos.does_intersect(clip)) return;
    
    auto el_parent = parent();
    if (!el_parent) return;
        
    auto doc = get_document();
    auto font = el_parent->css().get_font();
    if (!font) return;
            
    auto color = m_isPlaceholder ? litehtml::web_color(117,117,117) : el_parent->css().get_color();
    doc->container()->draw_text(hdc, m_use_transformed ? m_transformed_text.c_str() : m_text.c_str(), font,
        color, pos);
    
    if (m_has_focus) {
    thickLineRGBA(reinterpret_cast<SDL_Renderer*>(hdc),
        m_isPlaceholder ? pos.left() : pos.right(), pos.top(),
        m_isPlaceholder ? pos.left() : pos.right(), pos.bottom(),
        1,
        0, 0, 0, 255);

    }
}

void el_input_content::set_text(const std::string& text, bool placeholder) {
    m_isPlaceholder = placeholder;
    m_text = text;
    this->compute_styles(true);
    for (const auto& weak_ri : m_renders) {
        auto ri = weak_ri.lock();
        if (ri) {
            auto& ri_pos = ri->pos();
            ri_pos.width = m_size.width;
            ri_pos.height = m_size.height;
        }
    }

    //dynamic_cast<HtmlView*>(this->get_document()->container())->render();
}