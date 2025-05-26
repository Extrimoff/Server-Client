#pragma once
#include <litehtml.h>

enum class element_id 
{
	input,
	input_content,
	textholder
};

class custom_element {
protected:
	bool		m_has_focus;
	element_id	m_id;

public:
	custom_element(element_id id) : m_has_focus(false), m_id(id) { }
	custom_element() = delete;
	~custom_element() = default;

	static void appendTo(litehtml::element::ptr parent, litehtml::element::ptr child);

	bool is_focused() const { return m_has_focus; }
	element_id get_element_id() const { return m_id; } 

public:
	virtual void on_key_down(uint32_t vKey, uint16_t keyMode) { return; }
	virtual void on_text_input(const char* text) { return; }
	virtual void on_focus() { m_has_focus = true; }
	virtual void on_focus_lost() { m_has_focus = false; }
};