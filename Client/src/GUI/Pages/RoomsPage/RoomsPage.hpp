#pragma once
#include "../Page.hpp"

class RoomsPage : public Page {
	enum class SortType {
		ASCENDING,
		DESCENDING,
	};

	using room_data_t = std::vector<std::pair<std::string, std::string>>;  //std::vector<std::pair<key, value>>
	using sorted_rooms_t = std::vector<std::pair<std::string, room_data_t>>;
	using rooms_list_t = std::unordered_map<std::string, room_data_t>;
	using current_sort_t = std::pair<std::string, SortType>;

private:
	rooms_list_t	m_rooms;
	room_data_t		m_room_to_edit;
	current_sort_t	m_current_sort;
	std::string		m_edit_room_html;
	std::string		m_add_room_html;
	std::string		m_last_sort_value;
public:
	RoomsPage(std::shared_ptr<HtmlView> view);
	~RoomsPage() = default;

	void delete_data(litehtml::element::ptr const& elem);
	void edit_data(litehtml::element::ptr const& elem);
	void register_room();
	void register_room_action();
	void edit_data_action();
	void sort_data(std::string&& field);

	bool init() override;
	bool on_element_click(const litehtml::element::ptr& el) override;
	void on_switch(nlohmann::json = nlohmann::json::object()) override;

};