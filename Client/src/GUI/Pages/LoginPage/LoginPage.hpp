#pragma once
#include "../Page.hpp"

class LoginPage : public Page {
public:
	LoginPage(std::shared_ptr<HtmlView> view);
	~LoginPage() = default;

	bool init() override;
	bool on_element_click(const litehtml::element::ptr& el) override;
	void on_switch(nlohmann::json = nlohmann::json::object()) override;

private:
	void switch_forms();
	void register_user();
	void login_user();

};