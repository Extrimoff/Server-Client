#pragma once
#include <litehtml.h>
#include "../../Utils/Json.hpp"

class HtmlView : public litehtml::document_container, public std::enable_shared_from_this<HtmlView>
{
    using document_ptr = std::shared_ptr<litehtml::document>;
    using page = std::shared_ptr<class Page>;
    using pages_list = std::vector<page>;
private:
    std::shared_ptr<class Client>  m_connection;
    pages_list                     m_pages;
    page                           m_current_page;
    bool                           m_isPagesInit;
    int                            m_scroll_y;
    int                            m_scroll_x;
    std::string                    m_current_cursor;
    bool                           m_needsUpdate;

public:
    HtmlView(std::shared_ptr<class Client> connection);
    ~HtmlView();

    void set_needs_update(bool val) { m_needsUpdate = val; }
    bool get_needs_update() const { return m_needsUpdate; }
    __declspec(property(get = get_needs_update, put = set_needs_update)) bool needsUpdate;

public:
    static std::string replace_placeholder(std::string input, const std::string& placeholder, const std::string& content);

    void init_pages();
    void on_mouse_wheel(int scrollY);
    void switch_page(enum class PageID, nlohmann::json = nlohmann::json::object());
    uint32_t render(int width = 0);
    void reset_scroll();

    void on_key_down(uint32_t vKey, uint16_t keyMode) const;
    void on_text_input(const char* text) const;
    void on_mouse_move(int x, int y) const;
    void on_key_up(uint32_t vKey, uint16_t keyMode) const;
    void on_lButton_down(int x, int y);
    void on_lButton_up(int x, int y) const;
    void on_mouse_leave() const;
    void on_packet_receive(std::unique_ptr<class Packet> packet);
    void media_changed() const;
    void draw(litehtml::uint_ptr hdc, const litehtml::position* clip) const;

    std::shared_ptr<class Client> get_connection() const { return m_connection; }

public:
    litehtml::element::ptr create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;
    litehtml::uint_ptr create_font(const litehtml::font_description& descr, const litehtml::document* doc, litehtml::font_metrics* fm) override;

    const char* get_default_font_name() const override;
    int get_default_font_size() const override;
    void get_client_rect(litehtml::position& client) const override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    void get_language(litehtml::string& language, litehtml::string& culture) const override;
    void get_media_features(litehtml::media_features& media) const override;

    void delete_font(litehtml::uint_ptr f) override;
    void del_clip() override;

    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;
    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::conic_gradient& gradient) override;
    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const std::string& url, const std::string& base_url) override;
    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::linear_gradient& gradient) override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::background_layer::radial_gradient& gradient) override;
    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr f, litehtml::web_color color, const litehtml::position& pos) override;

    int pt_to_px(int pt) const override;
    void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
    void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;

    int text_width(const char* text, litehtml::uint_ptr f) override;
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override;

    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    bool on_element_click(const litehtml::element::ptr& el) override;
    void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override;

    void set_base_url(const char* base_url) override;
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    void set_cursor(const char* cursor) override;
    void set_caption(const char* caption) override;

private:
    template <typename TRet>
    std::shared_ptr<TRet> get_page() const;
    std::shared_ptr<class Page> get_page(enum class PageID) const;

    void get_rect(int* x, int* y, int* w, int* h) const;
};