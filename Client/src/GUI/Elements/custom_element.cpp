#include "custom_element.hpp"

#include <litehtml/render_item.h>

void custom_element::appendTo(litehtml::element::ptr parent, litehtml::element::ptr child)
{
    for (const auto& weak_ri : parent->get_renders()) {
        auto p_ri = weak_ri.lock();
        if (p_ri) {
            auto ri = child->create_render_item(p_ri);
            child->add_render(ri);
            p_ri->add_child(ri);
        }
    }

    parent->appendChild(child);
    parent->compute_styles(true);
}