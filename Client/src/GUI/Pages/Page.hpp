#pragma once
#include <litehtml.h>
#include "../../Utils/Json.hpp"
#include <queue>

enum class PageID
{
	UNKNOWN,
	LOGIN,
	PROFILE,
	BOOKINGS,
	ROOMS
};

class Page {
	using weak_custom_elements_list = std::vector<std::weak_ptr<class custom_element>>;
	using requests_list = std::vector<std::pair<uint64_t, std::function<void(std::unique_ptr<class Packet>)>>>;
	using functions_queue = std::queue<std::function<void()>>;
protected:
	std::string							m_html;
	litehtml::document::ptr				m_doc;
	std::weak_ptr<class HtmlView>		m_view;
	weak_custom_elements_list			m_custom_elements;
	PageID								m_id;
	requests_list						m_requests;
	functions_queue						m_func_queue;

	bool send_packet(class Packet const& packet, std::function<void(std::unique_ptr<class Packet>)> callback = nullptr);
	bool send_packet_async(class Packet const& packet, std::function<void(std::unique_ptr<class Packet>)> callback);
	void push_draw_task(std::function<void()> task);
public:
	Page(std::shared_ptr<class HtmlView> view);
	Page() = delete;

	virtual ~Page();
	virtual bool init() = 0;
	virtual bool on_element_click(const litehtml::element::ptr& el) = 0;
	virtual void on_switch(nlohmann::json = nlohmann::json::object()) = 0;

	virtual void on_packet_receive(std::unique_ptr<class Packet> packet);

	void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip);
	void on_key_down(uint32_t vKey, uint16_t keyMode);
	void on_text_input(const char* text);
	void on_custom_element_create(std::weak_ptr<class custom_element>);

	uint32_t render(int width) const;
	void on_mouse_move(int x, int y) const;
	void on_key_up(uint32_t vKey) const;
	void on_lButton_down(int x, int y);
	void on_lButton_up(int x, int y) const;
	void on_mouse_leave() const;
	void media_changed() const;
	void get_doc_size(litehtml::size& size) const;
	PageID get_id() const;

};