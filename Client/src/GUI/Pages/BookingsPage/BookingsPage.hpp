#pragma once
#include "../Page.hpp"


class BookingsPage : public Page {
	enum class SortType {
		ASCENDING,
		DESCENDING,
	};

	using booking_data_t = std::vector<std::pair<std::string, std::string>>;  //std::vector<std::pair<key, value>>
	using sorted_bookings_t = std::vector<std::pair<std::string, booking_data_t>>;
	using bookings_list_t = std::unordered_map<std::string, booking_data_t>;
	using current_sort_t = std::pair<std::string, SortType>;

private:
	bookings_list_t		m_bookings;
	booking_data_t		m_booking_to_edit;
	current_sort_t		m_current_sort;
	std::string			m_edit_booking_html;
	std::string			m_add_booking_html;
	std::string			m_last_sort_value;
public:
	BookingsPage(std::shared_ptr<HtmlView> view);
	~BookingsPage() = default;

	bool init() override;
	bool on_element_click(const litehtml::element::ptr& el) override;
	void on_switch(nlohmann::json = nlohmann::json::object()) override;

private:
	void delete_data(litehtml::element::ptr const& elem);
	void edit_data(litehtml::element::ptr const& elem);
	void edit_data_action();
	void register_booking();
	void register_booking_action();
	void sort_data(std::string&& field);
};