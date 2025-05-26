#include "Page.hpp"
#include "../HtmlView/HtmlView.hpp"
#include "../Elements/el_input.hpp"
#include "../../Network/Client/Client.hpp"
#include "../../Network/PacketManager/PacketManager.hpp"

#include <print>
#include <litehtml/render_item.h>

Page::Page(std::shared_ptr<class HtmlView> view) : m_view(view), m_id(PageID::UNKNOWN) {
	
}

Page::~Page() = default;

uint32_t Page::render(int width) const {
	return m_doc->render(width, litehtml::render_all);
}

void Page::on_mouse_move(int x, int y) const {
	litehtml::position::vector pos;
	m_doc->on_mouse_over(x, y, x, y, pos);
}

void Page::on_key_up(uint32_t vKey) const {

}

void Page::on_lButton_down(int x, int y) {
	auto new_end = std::remove_if(m_custom_elements.begin(), m_custom_elements.end(), [](const std::weak_ptr<custom_element>& wElement) {
		auto element = wElement.lock();
		if (!element || wElement.expired()) {
			return true;
		}

		element->on_focus_lost();
		return false;
		});
	m_custom_elements.erase(new_end, m_custom_elements.end());

	litehtml::position::vector pos;
	m_doc->on_lbutton_down(x, y, x, y, pos);
}

void Page::on_lButton_up(int x, int y) const {
	litehtml::position::vector pos;
	m_doc->on_lbutton_up(x, y, x, y, pos);
}

void Page::on_mouse_leave() const {
	litehtml::position::vector pos;
	m_doc->on_mouse_leave(pos);
}

void Page::on_key_down(uint32_t vKey, uint16_t keyMode) {
	auto new_end = std::remove_if(m_custom_elements.begin(), m_custom_elements.end(), [vKey = vKey, keyMode = keyMode](const std::weak_ptr<custom_element>& wElement) {
		auto element = wElement.lock();
		if (!element || wElement.expired()) {
			return true;
		}

		auto input = std::dynamic_pointer_cast<el_input>(element);
		input->on_key_down(vKey, keyMode);
		return false;
		});

	m_custom_elements.erase(new_end, m_custom_elements.end());
}

void Page::on_text_input(const char* text)
{
	auto new_end = std::remove_if(m_custom_elements.begin(), m_custom_elements.end(), [text = text](const std::weak_ptr<custom_element>& wElement) {
		auto element = wElement.lock();
		if (!element || wElement.expired()) {
			return true;
		}

		auto input = std::dynamic_pointer_cast<el_input>(element);
		input->on_text_input(text);
		return false;
		});

	m_custom_elements.erase(new_end, m_custom_elements.end());
}

void Page::on_custom_element_create(std::weak_ptr<class custom_element> elem) {
	m_custom_elements.push_back(elem);
}

void Page::media_changed() const {
	m_doc->media_changed();
}

void Page::draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip) {
	while (!m_func_queue.empty()) {
		auto& fn = m_func_queue.front();
		fn();

		m_func_queue.pop();
	}
	m_doc->draw(hdc, y, x, clip);
}

void Page::get_doc_size(litehtml::size& size) const
{
	auto root = m_doc->root_render();
	if (!root) return;

	size.height = root->height();
	size.width = root->width();
	
}

void Page::on_packet_receive(std::unique_ptr<class Packet> packet) {
	for (auto it = m_requests.begin(); it < m_requests.end();) {
		if (it->first != packet->getRequestID()) {
			it++;
			continue;
		}
		it->second(std::move(packet));
		it = m_requests.erase(it);
		break;
	}
}

bool Page::send_packet(class Packet const& packet, std::function<void(std::unique_ptr<class Packet>)> callback)
{
	auto view = m_view.lock();
	if (!view) return false;

	auto connection = view->get_connection();

	if (!callback) {
		connection->sendData(packet);
		return true;
	}

	std::mutex mtx;
	std::condition_variable cv;
	std::unique_ptr<Packet> response;
	bool responded = false;

	m_requests.emplace_back(std::pair(packet.getRequestID(), [&](std::unique_ptr<Packet> pkt) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			response = std::move(pkt);
			responded = true;
		}
		cv.notify_one();
	}));

	connection->sendData(packet);

	std::unique_lock<std::mutex> lock(mtx);
	if (!cv.wait_for(lock, std::chrono::milliseconds(5000), [&] { return responded; })) {
		return false;
	}

	if (!response) return false;

	callback(std::move(response));
	return true;
}

bool Page::send_packet_async(class Packet const& packet, std::function<void(std::unique_ptr<class Packet>)> callback)
{
	auto view = m_view.lock();
	if (!view) return false;


	m_requests.emplace_back(std::pair(packet.getRequestID(), [callback = std::move(callback), this](std::unique_ptr<Packet> pkt) {
		if (!pkt) return;
		auto buf = std::make_shared<std::unique_ptr<Packet>>(std::move(pkt));
		m_func_queue.push([buf, cb = std::move(callback), this]() mutable {
			cb(std::move(*buf));
		});
	}));

	auto connection = view->get_connection();
	connection->sendData(packet);

	return true;
}

void Page::push_draw_task(std::function<void()> task)
{
	m_func_queue.push(task);
}

PageID Page::get_id() const {
	return m_id;
}