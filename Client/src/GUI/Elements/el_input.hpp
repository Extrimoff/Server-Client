#pragma once
#include "el_textholder.hpp"

class el_input_content : public el_textholder
{;
private:
    bool                    m_isPlaceholder;
public:
    explicit el_input_content(const char* text, const litehtml::document::ptr& doc) : el_textholder(text, doc), m_isPlaceholder(false) { }

    void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri) override;
    void set_text(const std::string& text, bool placeholder = false);
};

class el_input : public custom_element, public litehtml::html_tag
{
private:
    std::string                         m_value;    
    std::string                         m_placeholder;
    std::shared_ptr<el_input_content>   m_content;
    bool                                m_isPassword;

public:
    explicit el_input(const std::shared_ptr<litehtml::document>& doc, bool isPassword = false);
    el_input& init();
    el_input& value(const std::string& value, bool force = false);
    el_input& placeholder(const std::string& placeholder);
    el_input& isPassword(bool isPassword);
    const std::string& get_value() const;


    void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri) override;
    bool on_lbutton_down() override;
    bool on_lbutton_up(bool is_click) override;
    void on_key_down(uint32_t vKey, uint16_t keyMode) override;
    void on_text_input(const char* text) override;
    void on_focus_lost() override;
    void on_focus() override;

private:
    void set_content(const std::string& content, bool placeholder = false);
};