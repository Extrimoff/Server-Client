#pragma once
#include "../Page.hpp"

class ProfilePage : public Page {

	struct CurrentUser {
		uint64_t	id;
		std::string name;
		std::string surname;
		std::string role;
	};

	enum class SortType {
		ASCENDING,
		DESCENDING,
	};

	using user_data_t = std::vector<std::pair<std::string, std::string>>;
	using sorted_users_t = std::vector<std::pair<std::string, user_data_t>>;
	using users_list_t = std::unordered_map<std::string, user_data_t>;
	using current_sort_t = std::pair<std::string, SortType>;

private:
	users_list_t	m_users;
	user_data_t		m_user_to_edit;
	current_sort_t	m_current_sort;
	std::string		m_edit_user_html;
	std::string		m_add_user_html;
	std::string		m_last_sort_value;
	CurrentUser		m_current_user;

	void delete_data(litehtml::element::ptr const& elem);
	void edit_data(litehtml::element::ptr const& elem);
	void edit_data_action();
	void register_user();
	void register_user_action();
	void sort_data(std::string&& field);
public:
	ProfilePage(std::shared_ptr<HtmlView> view);
	~ProfilePage() = default;

	bool init() override;
	bool on_element_click(const litehtml::element::ptr& el) override;
	void on_switch(nlohmann::json = nlohmann::json::object()) override;

};