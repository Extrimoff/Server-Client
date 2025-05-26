#pragma once
#include "custom_element.hpp"

#include <litehtml/el_text.h>

class el_textholder : public custom_element, public litehtml::el_text
{
public:
	explicit el_textholder(const char* text, const litehtml::document::ptr& doc) : custom_element(element_id::textholder),
		litehtml::el_text(text, doc) { }

	
	
	void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip, const std::shared_ptr<litehtml::render_item>& ri) override;

	virtual void set_text(const std::string& text);
	virtual void appendTo(litehtml::element::ptr parent);
};