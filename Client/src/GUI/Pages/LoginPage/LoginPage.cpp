#include "LoginPage.hpp"
#include "../../HtmlView/HtmlView.hpp"
#include "../../Elements/el_input.hpp"
#include "../../../Network/PacketManager/PacketManager.hpp"

#include <litehtml/el_para.h>
#include <litehtml/render_item.h>
#include <regex>
#include <print>

template<typename T>
static T safe_cast(const std::string& str) {
    static_assert(std::is_arithmetic_v<T>, "safe_cast only supports arithmetic types");

    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    return (result.ec == std::errc() && result.ptr == str.data() + str.size()) ? value : T{};
}

bool LoginPage::init()
{
    auto view = m_view.lock();
    if (!view) return false;

    m_doc = litehtml::document::createFromString(m_html, view.get());

    auto register_form = m_doc->root()->select_one("#register-form");
    auto email_err = m_doc->root()->select_one("#email-err");
    auto password_err = m_doc->root()->select_one("#password-err");
    auto phone_err = m_doc->root()->select_one("#phone-err");
    auto reg_err = m_doc->root()->select_one("#reg-err");
    auto log_err = m_doc->root()->select_one("#log-err");
    auto log_email_err = m_doc->root()->select_one("#log-email-err");
    auto log_password_err = m_doc->root()->select_one("#log-password-err");

    register_form->css_w().set_display(litehtml::display_none);
    email_err->css_w().set_display(litehtml::display_none);
    password_err->css_w().set_display(litehtml::display_none);
    phone_err->css_w().set_display(litehtml::display_none);
    log_email_err->css_w().set_display(litehtml::display_none);
    log_password_err->css_w().set_display(litehtml::display_none);

    auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
    auto log_err_text = std::make_shared<el_textholder>("", m_doc);

    reg_err_text->appendTo(reg_err);
    log_err_text->appendTo(log_err);
    
    return true;
}

void LoginPage::switch_forms()
{
    auto login_form = m_doc->root()->select_one("#login-form");
    auto register_form = m_doc->root()->select_one("#register-form");

    auto log_email_err = m_doc->root()->select_one("#log-email-err");
    auto log_password_err = m_doc->root()->select_one("#log-password-err");
    auto email_err = m_doc->root()->select_one("#email-err");
    auto password_err = m_doc->root()->select_one("#password-err");
    auto phone_err = m_doc->root()->select_one("#phone-err");
    auto name_err = m_doc->root()->select_one("#name-err");
    auto surname_err = m_doc->root()->select_one("#surname-err");

    log_email_err->css_w().set_display(litehtml::display_none);
    log_password_err->css_w().set_display(litehtml::display_none);
    email_err->css_w().set_display(litehtml::display_none);
    phone_err->css_w().set_display(litehtml::display_none);
    password_err->css_w().set_display(litehtml::display_none);
    name_err->css_w().set_display(litehtml::display_none);
    surname_err->css_w().set_display(litehtml::display_none);

    auto display = login_form->css().get_display();
    login_form->css_w().set_display(display == litehtml::display_block ? litehtml::display_none : litehtml::display_block);

    display = register_form->css().get_display();
    register_form->css_w().set_display(display == litehtml::display_block ? litehtml::display_none : litehtml::display_block);
}

void LoginPage::login_user() 
{
    auto view = m_view.lock();
    if (!view) return;

    auto log_email = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#log-email"));
    auto log_password = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#log-password"));

    std::string email_value = log_email->get_value();
    std::string password_value = log_password->get_value();

    std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    std::regex password_regex(R"(^.{8,}$)");

    bool is_email_valid = std::regex_match(email_value, email_regex);
    bool error = !is_email_valid;

    auto email_err = m_doc->root()->select_one("#log-email-err");
    auto password_err = m_doc->root()->select_one("#log-password-err");

    email_err->css_w().set_display(litehtml::display_none);
    password_err->css_w().set_display(litehtml::display_none);

    if (!is_email_valid)
        email_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    LoginPacket lp(email_value, password_value);
    this->send_packet(lp, [&](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode != ResponseID::Sucess) {
            auto reg_err = m_doc->root()->select_one("#log-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text(std::format("Ошибка {}: {}", static_cast<int>(response->errorCode), response->errorMessage));
            }

            return;
        }

        auto reg_err = m_doc->root()->select_one("#log-err");
        for (auto& child : reg_err->children()) {
            auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
            if (!el_text) continue;

            el_text->set_text("");
        }

        view->switch_page(PageID::PROFILE, std::move(response->additionalData));
    });
}

void LoginPage::register_user()
{
    auto reg_email = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-email"));
    auto reg_password = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-password"));
    auto name = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#name"));
    auto surname = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#surname"));
    auto phone = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#phone"));

    std::string email_value = reg_email->get_value();
    std::string password_value = reg_password->get_value();
    std::string name_value = name->get_value();
    std::string surname_value = surname->get_value();
    std::string phone_value = phone->get_value();

    std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    std::regex phone_regex(R"(^$|^\+?\d{10,15}$)");
    std::regex name_regex(R"(^[A-Za-zА-Яа-яЁё]{1,20}$)");
    std::regex password_regex(R"(^.{8,}$)");

    bool is_email_valid = std::regex_match(email_value, email_regex);
    bool is_phone_valid = std::regex_match(phone_value, phone_regex);
    bool is_password_valid = std::regex_match(password_value, password_regex);
    bool is_name_valid = std::regex_match(name_value, name_regex);
    bool is_surname_valid = std::regex_match(surname_value, name_regex);
    bool error = !is_email_valid || !is_phone_valid || !is_password_valid || !is_name_valid || !is_surname_valid;

    auto email_err = m_doc->root()->select_one("#email-err");
    auto password_err = m_doc->root()->select_one("#password-err");
    auto phone_err = m_doc->root()->select_one("#phone-err");
    auto name_err = m_doc->root()->select_one("#name-err");
    auto surname_err = m_doc->root()->select_one("#surname-err");

    email_err->css_w().set_display(litehtml::display_none);
    phone_err->css_w().set_display(litehtml::display_none);
    password_err->css_w().set_display(litehtml::display_none);
    name_err->css_w().set_display(litehtml::display_none);
    surname_err->css_w().set_display(litehtml::display_none);

    if (!is_email_valid)
        email_err->css_w().set_display(litehtml::display_block);
    if (!is_phone_valid)
        phone_err->css_w().set_display(litehtml::display_block);
    if (!is_password_valid)
        password_err->css_w().set_display(litehtml::display_block);
    if(!is_name_valid)
        name_err->css_w().set_display(litehtml::display_block);
    if (!is_surname_valid)
        surname_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    RegisterPacket rp(email_value, password_value, name_value, surname_value, phone_value);
    this->send_packet(rp, [&](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        email_err->css_w().set_display(litehtml::display_none);
        phone_err->css_w().set_display(litehtml::display_none);
        password_err->css_w().set_display(litehtml::display_none);

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode == ResponseID::Sucess) {
            auto reg_err = m_doc->root()->select_one("#reg-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text("");
            }
            this->switch_forms();
        }
        else {
            auto reg_err = m_doc->root()->select_one("#reg-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text(std::format("Ошибка {}: {}", static_cast<int>(response->errorCode), response->errorMessage));
            }
        }

    });
}


bool LoginPage::on_element_click(const litehtml::element::ptr& el)
{
    if (auto style = el->get_attr("class"); 
        style != nullptr && !strcmp(style, "switch-link")) {
            this->switch_forms();
            return true;
    }

    auto id = el->get_attr("id");
    if (id == nullptr) return false;

    if (!strcmp(id, "login-button")) {
        this->login_user();
        return true;
    }
    else if (!strcmp(id, "register-button")) {
        this->register_user();
        return true;
    }

    return false;
}

void LoginPage::on_switch(nlohmann::json data) {
    auto view = m_view.lock();
    if (!view) return;
    view->reset_scroll();

    return;
}

LoginPage::LoginPage(std::shared_ptr<HtmlView> view) : Page(view){
	m_html = R"-(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <title>Вход</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f4;
                margin: 0;
                padding: 0;
                display: flex;
                flex-direction: column;
                align-items: center;
            }

            .container {
                background-color: white;
                padding: 20px;
                margin-top: 40px;
                border-radius: 8px;
                box-shadow: 0 0 10px rgba(0,0,0,0.1);
                width: 300px;
            }

            h1, h2 {
                text-align: center;
                margin-bottom: 20px;
            }

            .description {
                font-size: 14px;
                margin-bottom: 20px;
                text-align: center;
            }

            input[type="text"],
            input[type="password"],
            input[type="email"],
            button {
                width: 100%;
                padding: 10px;
                margin-top: 10px;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-sizing: border-box;
            }

            input {
              cursor: text;
            }

            button {
                background-color: #4CAF50;
                color: white;
                cursor: pointer;
                text-align: center;
            }

            button:hover {
                background-color: #45a049;
            }

            .switch-link {
                margin-top: 10px;
                text-align: center;
                font-size: 14px;
                color: #007BFF;
                cursor: pointer;
            }

            .hidden {
                display: none;
            }
        </style>
    </head>
    <body>

    <div class="container">
        <h1>Добро пожаловать!</h1>
        <p class="description">Здесь осуществляется управление пользователями, номерами и бронированиями!</p>

        <!-- Login Form -->
        <div id="login-form">
            <h2>Вход</h2>

            <input type="email" placeholder="Email" id="log-email">
            <p style="color:red; font-size: 14px;" id="log-email-err">Неверный формат email</p>

            <input type="password" placeholder="Пароль" id="log-password">
            <p style="color:red; font-size: 14px;" id="log-password-err">Пароль должен быть не менее 8 символов</p>

            <button id="login-button">Войти</button>
            <p style="color:red; font-size: 14px;" id="log-err"></p>

            <div class="switch-link">Зарегистрировать пользователя</div>
        </div>

        <!-- Registration Form -->
        <div id="register-form">
            <h2>Регистрация</h2>

            <input type="email" id="reg-email" placeholder="Email">
            <p style="color:red; font-size: 14px;" id="email-err">Неверный формат email</p>

            <input type="password" id="reg-password" placeholder="Пароль">
            <p style="color:red; font-size: 14px;" id="password-err">Пароль должен быть не менее 8 символов</p>

            <input type="text" id="name" placeholder="Имя">
            <p style="color:red; font-size: 14px;" id="name-err">Неверный формат Имени</p>

            <input type="text" id="surname" placeholder="Фамилия">
            <p style="color:red; font-size: 14px;" id="surname-err">Неверный формат Фамилии</p>

            <input type="text" id="phone" placeholder="Телефон">
            <p style="color:red; font-size: 14px;" id="phone-err">Неверный формат телефона</p>

            <button id="register-button">Зарегистрироваться</button>
            <p style="color:red; font-size: 14px;" id="reg-err"></p>

            <div class="switch-link">Уже есть аккаунт? Войти</div>
        </div>
    </div>

    </body>
    </html> 
)-";

    m_id = PageID::LOGIN;
}