#include "el_textholder.hpp"
#include "../HtmlView/HtmlView.hpp"

#include <litehtml/render_item.h>


void el_textholder::set_text(const std::string& text) {
    m_text = text;
    auto parent = this->parent(); 
    if (parent) { 
        parent->compute_styles(true); 
    }
    else {
        this->compute_styles(true);
    }
    for (const auto& weak_ri : m_renders) {
        auto ri = weak_ri.lock();
        if (ri) {
            auto& ri_pos = ri->pos();
            ri_pos.width = m_size.width;
            ri_pos.height = m_size.height;
        }
    }

    dynamic_cast<HtmlView*>(this->get_document()->container())->render();
}

void el_textholder::appendTo(litehtml::element::ptr parent)
{
    custom_element::appendTo(parent, shared_from_this());
   /* for (const auto& weak_ri : parent->get_renders()) {
        auto p_ri = weak_ri.lock();
        if (p_ri) {
            auto ri = this->create_render_item(p_ri);
            this->add_render(ri);
            p_ri->add_child(ri);
        }
    }

    parent->appendChild(shared_from_this());*/
}

void el_textholder::draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri) {
    el_text::draw(hdc, x, y, clip, ri);
}