#include "ProfilePage.hpp"
#include "../../HtmlView/HtmlView.hpp"
#include "../../../Network/PacketManager/PacketManager.hpp"
#include "../../Elements/el_input.hpp"

#include <litehtml/el_tr.h>
#include <litehtml/el_td.h>
#include <regex>
#include <print>
#include <algorithm>

template<typename T>
static T safe_cast(const std::string& str) {
    static_assert(std::is_arithmetic_v<T>, "safe_cast only supports arithmetic types");

    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    return (result.ec == std::errc() && result.ptr == str.data() + str.size()) ? value : T{};
}

bool ProfilePage::init()
{
    auto view = m_view.lock();
    if (!view) return false;

    return true;
}

void ProfilePage::delete_data(litehtml::element::ptr const& el)
{
    auto recordId = el->get_attr("data-record-id");
    if (!recordId) return;

    auto reg_err = m_doc->root()->select_one("#deletion-err");
    if (!reg_err) return;

    for (auto& child : reg_err->children()) {
        auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
        if (!el_text) continue;

        el_text->set_text("");
    }

    DeleteDataPacket ddp(TableID::USERS, safe_cast<long long>(recordId));
    this->send_packet_async(ddp, [this](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode != ResponseID::Sucess) {
            auto reg_err = m_doc->root()->select_one("#deletion-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text(std::format("Ошибка {}: {}", static_cast<int>(response->errorCode), response->errorMessage));
            }

            return;
        }
        view->switch_page(PageID::PROFILE);
    });
}

void ProfilePage::register_user()
{
    this->push_draw_task([this]() {
        auto view = m_view.lock();
        if (!view) return;
        m_doc = litehtml::document::createFromString(m_add_user_html, view.get());

        view->reset_scroll();

        auto email_err = m_doc->root()->select_one("#email-err");
        auto password_err = m_doc->root()->select_one("#password-err");
        auto phone_err = m_doc->root()->select_one("#phone-err");
        auto name_err = m_doc->root()->select_one("#name-err");
        auto surname_err = m_doc->root()->select_one("#surname-err");
        auto reg_err = m_doc->root()->select_one("#reg-err");
       
        email_err->css_w().set_display(litehtml::display_none);
        phone_err->css_w().set_display(litehtml::display_none);
        password_err->css_w().set_display(litehtml::display_none);
        name_err->css_w().set_display(litehtml::display_none);
        surname_err->css_w().set_display(litehtml::display_none);

        auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
        reg_err_text->appendTo(reg_err);

        view->render();
    });
}

void ProfilePage::register_user_action()
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
    if (!is_name_valid)
        name_err->css_w().set_display(litehtml::display_block);
    if (!is_surname_valid)
        surname_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    RegisterPacket rp(email_value, password_value, name_value, surname_value, phone_value);
    this->send_packet_async(rp, [=](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;
        auto view = m_view.lock();
        if (!view) return;

        email_err->css_w().set_display(litehtml::display_none);
        phone_err->css_w().set_display(litehtml::display_none);
        password_err->css_w().set_display(litehtml::display_none);

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode == ResponseID::Sucess) {
            view->switch_page(PageID::PROFILE);
        }
        else {
            auto reg_err = m_doc->root()->select_one("#reg-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text(std::format("Ошибка {}: {}", static_cast<int>(response->errorCode), response->errorMessage));
            }
            view->render();
        }

    });
}

void ProfilePage::edit_data(litehtml::element::ptr const& el)
{
    auto recordId = el->get_attr("data-record-id");
    if (!recordId) return;

    const static std::unordered_map<std::string, std::string> mappedLabels = {
        {"email", "Email"},
        {"first_name", "Имя"},
        {"last_name", "Фамилия"},
        {"role", "Роль"},
        {"phone_number", "Номер телефона"},
    };

    auto const& user = m_users[recordId];
    std::string rows = std::format
    (R"-(
        <p class="input-wrapper">{}:<input type="email" placeholder="Email" value="{}" id="edit-email"></p>
        <p style="color:red; font-size: 14px;" id="edit-email-err">Неверный формат email</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Имя" value="{}" id="edit-name"></p>
        <p style="color:red; font-size: 14px;" id="edit-name-err">Неверный формат Имени</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Фамилия" value="{}" id="edit-surname"></p>
        <p style="color:red; font-size: 14px;" id="edit-surname-err">Неверный формат Фамилии</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Роль" value="{}" id="edit-role"></p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Номер телефона" value="{}" id="edit-phone"></p>
        <p style="color:red; font-size: 14px;" id="edit-phone-err">Неверный формат телефона</p>

        <p class="input-wrapper">Пароль:<input type="password" placeholder="Пароль" id="edit-password"></p>
        <p style="color:red; font-size: 14px;" id="edit-password-err">Пароль должен быть не менее 8 символов</p>
    )-", mappedLabels.at(user[1].first), user[1].second,
        mappedLabels.at(user[2].first), user[2].second,
        mappedLabels.at(user[3].first), user[3].second,
        mappedLabels.at(user[4].first), user[4].second,
        mappedLabels.at(user[5].first), user[5].second);

    m_user_to_edit = std::move(user);
    this->push_draw_task([rows = std::move(rows), this]() {
        auto view = m_view.lock();
        if (!view) return;

        m_doc = litehtml::document::createFromString(HtmlView::replace_placeholder(m_edit_user_html, "{{ROWS}}", rows), view.get());

        view->reset_scroll();


        if (auto edition_err = m_doc->root()->select_one("#edition-err")) {
            auto edition_err_text = std::make_shared<el_textholder>("", m_doc);
            edition_err_text->appendTo(edition_err);
        }

        auto edit_phone_err = m_doc->root()->select_one("#edit-phone-err");
        auto edit_email_err = m_doc->root()->select_one("#edit-email-err");
        auto edit_password_err = m_doc->root()->select_one("#edit-password-err");
        auto name_err = m_doc->root()->select_one("#edit-name-err");
        auto surname_err = m_doc->root()->select_one("#edit-surname-err");

        edit_phone_err->css_w().set_display(litehtml::display_none);
        edit_email_err->css_w().set_display(litehtml::display_none);
        edit_password_err->css_w().set_display(litehtml::display_none);
        name_err->css_w().set_display(litehtml::display_none);
        surname_err->css_w().set_display(litehtml::display_none);
        view->render();
    });
}

void ProfilePage::edit_data_action()
{
    auto reg_email = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-email"));
    auto reg_password = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-password"));
    auto name = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-name"));
    auto surname = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-surname"));
    auto phone = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-phone"));
    auto role = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-role"));

    std::string email_value = reg_email->get_value();
    std::string password_value = reg_password->get_value();
    std::string name_value = name->get_value();
    std::string surname_value = surname->get_value();
    std::string phone_value = phone->get_value();
    std::string role_value = role->get_value();

    std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    std::regex phone_regex(R"(^$|^\+?\d{10,15}$)");
    std::regex password_regex(R"(^.{8,}$)");

    bool is_email_valid = std::regex_match(email_value, email_regex);
    bool is_phone_valid = std::regex_match(phone_value, phone_regex);
    bool is_password_valid = std::regex_match(password_value, password_regex) || password_value == "";
    bool error = !is_email_valid || !is_phone_valid || !is_password_valid;

    auto email_err = m_doc->root()->select_one("#edit-email-err");
    auto password_err = m_doc->root()->select_one("#edit-password-err");
    auto phone_err = m_doc->root()->select_one("#edit-phone-err");


    email_err->css_w().set_display(litehtml::display_none);
    password_err->css_w().set_display(litehtml::display_none);
    phone_err->css_w().set_display(litehtml::display_none);

    if (!is_email_valid)
        email_err->css_w().set_display(litehtml::display_block);
    if (!is_phone_valid)
        phone_err->css_w().set_display(litehtml::display_block);
    if (!is_password_valid)
        password_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    nlohmann::json newData = {};
    
    if (m_user_to_edit.size() < 5) return;

    if (m_user_to_edit[1].second != email_value)    newData[m_user_to_edit[1].first] = email_value;
    if (m_user_to_edit[2].second != name_value)     newData[m_user_to_edit[2].first] = name_value;
    if (m_user_to_edit[3].second != surname_value)  newData[m_user_to_edit[3].first] = surname_value;
    if (m_user_to_edit[4].second != role_value)     newData[m_user_to_edit[4].first] = role_value;
    if (m_user_to_edit[5].second != phone_value)    newData[m_user_to_edit[5].first] = phone_value;
    if ("" != password_value)                       newData[m_user_to_edit[6].first] = password_value;

    EditDataPacket edp(TableID::USERS, safe_cast<long long>(m_user_to_edit[0].second), newData);
    this->send_packet_async(edp, [this](std::unique_ptr<Packet> packet){
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode != ResponseID::Sucess) {
            auto reg_err = m_doc->root()->select_one("#edition-err");
            for (auto& child : reg_err->children()) {
                auto el_text = std::dynamic_pointer_cast<el_textholder>(child);
                if (!el_text) continue;

                el_text->set_text(std::format("Ошибка {}: {}", static_cast<int>(response->errorCode), response->errorMessage));
            }

            return;
        }

        view->switch_page(PageID::PROFILE);
    });
}

void ProfilePage::sort_data(std::string&& field_name)
{
    auto get_field = [](const user_data_t& data, const std::string& key) -> std::string {
        for (const auto& pair : data) {
            if (pair.first == key)
                return pair.second;
        }
        return "";
    };

    auto view = m_view.lock();
    if (!view) return;

    auto sorted_users = sorted_users_t(m_users.begin(), m_users.end());

    auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
    auto& sort_value = input_sort->get_value();

    auto sort_type = m_current_sort.second;

    if (sort_value == m_last_sort_value) {
        if (m_current_sort.first == field_name) {
            sort_type = m_current_sort.second == SortType::ASCENDING  ? SortType::DESCENDING : SortType::ASCENDING;
        }
    }
    m_last_sort_value = sort_value;

    std::sort(sorted_users.begin(), sorted_users.end(), [&](const auto& u1, const auto& u2) {
        auto compare = [&]<typename T>(T const& arg1, T const& arg2) -> bool {
            if (sort_type == SortType::ASCENDING) return arg1 < arg2;
            else return arg1 > arg2;
        };

        if (field_name == "id") {
            auto id1 = safe_cast<int>(get_field(u1.second, field_name));
            auto id2 = safe_cast<int>(get_field(u2.second, field_name));
            return compare(id1, id2);
        }
        std::string a = get_field(u1.second, field_name);
        std::string b = get_field(u2.second, field_name);
        std::string lowerA, lowerB;
        std::transform(a.begin(), a.end(), std::back_inserter(lowerA), ::tolower);
        std::transform(b.begin(), b.end(), std::back_inserter(lowerB), ::tolower);
        return compare(lowerA, lowerB);
    });

    if (sort_value != "")  {
        sorted_users.erase(
            std::remove_if(
                sorted_users.begin(),
                sorted_users.end(),
                [&](const auto& user) {
                    const auto& fields = user.second;
                    return std::none_of(fields.begin(), fields.end(), [&](const auto& field) {
                        if (field.first != field_name) return false;

                        if (field_name == "id") {
                            auto sort_id = safe_cast<int>(sort_value);
                            auto id = safe_cast<int>(field.second);
                            return id == sort_id;
                        }
                        return field.second.find(sort_value) != std::string::npos;
                    });
                }
            ),
            sorted_users.end()
        );
    }

    m_current_sort = std::make_pair(std::move(field_name), sort_type);

    this->push_draw_task([sort_value = std::move(sort_value), sorted_users = std::move(sorted_users), this]() {
        auto view = m_view.lock();
        if (!view) return;

        std::string html;

        for (const auto& user : sorted_users) {
            html += "<tr>";

            for (auto const& field : user.second) {
                html += std::format(R"(<td>{}</td>)", field.second);
            }

            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                user.first,
                user.first
            );

            html += "</tr>";
        }

        std::string user_data = std::format(R"-(
                <p>Имя: {}</p>
                <p>Фамилия: {}</p>
                <p>Роль: {}</p>
            )-", m_current_user.name, m_current_user.surname, m_current_user.role);

        html = HtmlView::replace_placeholder(m_html, "{{ROWS}}", html);
        html = HtmlView::replace_placeholder(html, "{{UserData}}", user_data);
        m_doc = litehtml::document::createFromString(html, view.get());

        view->reset_scroll();

        if (auto deletion_err = m_doc->root()->select_one("#deletion-err")) {
            auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
            reg_err_text->appendTo(deletion_err);
        }
        auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
        input_sort->value(sort_value, true);

        view->render();
    });
}

bool ProfilePage::on_element_click(const litehtml::element::ptr& el)
{
    auto view = m_view.lock();
    if (!view) return false;

    auto id = el->get_attr("id");
    if(id == nullptr) return false;

    if (!strcmp(id, "logout-button")) {
        this->send_packet(LogoutPacket());
        view->switch_page(PageID::LOGIN);
        return true;
    }
    else if (!strcmp(id, "delete-record-button")) {
        this->delete_data(el);
        return true;
    }
    else if (!strcmp(id, "edit-record-button")) {
        this->edit_data(el);
        return true;
    }
    else if (!strcmp(id, "back-button")) {
        this->push_draw_task([this]() {
            auto view = m_view.lock();
            if (!view) return;

            view->switch_page(PageID::PROFILE);
            });
        return true;
    }
    else if (!strcmp(id, "edit-submit-button")) {
        this->edit_data_action();
        return true;
    }
    else if (!strcmp(id, "add-user-button")) {
        this->register_user();
        return true;
    }
    else if (!strcmp(id, "add-user-submit-button")) {
        this->register_user_action();
        return true;
    }
    else if (!strcmp(id, "switch-rooms")) {
        view->switch_page(PageID::ROOMS);
        return true;
    }
    else if (!strcmp(id, "switch-bookings")) {
        view->switch_page(PageID::BOOKINGS);
        return true;
    }
    else if (std::string field = id, prefix = "sort-"; field.find(prefix) == 0) {
        field = field.substr(prefix.length());
        this->sort_data(std::move(field));
        return true;
    }

    return false;
}

void ProfilePage::on_switch(nlohmann::json userData) {
    GetDataPacket gdp(TableID::USERS);
    m_users.clear();
    m_user_to_edit = { };

    auto safe_get = [](const nlohmann::json& obj, const std::string& key) -> std::string {
        if (!obj.contains(key) || obj[key].is_null()) return "";

        const auto& val = obj[key];

        if (val.is_string()) {
            return val.get<std::string>();
        }
        else if (val.is_number_integer()) {
            return std::to_string(val.get<int64_t>());
        }
        else if (val.is_number_unsigned()) {
            return std::to_string(val.get<uint64_t>());
        }
        else if (val.is_number_float()) {
            return std::to_string(val.get<double>());
        }
        else if (val.is_boolean()) {
            return val.get<bool>() ? "true" : "false";
        }

        return val.dump();
    };



    if (userData.contains("id") && userData.contains("name") && userData.contains("surname") && userData.contains("role")) {
        m_current_user = CurrentUser(safe_cast<long long>(safe_get(userData, "id")), safe_get(userData, "name"), safe_get(userData, "surname"), safe_get(userData, "role"));
    }

    this->send_packet(gdp, [this, safe_get = safe_get, userData = std::move(userData)](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto* response = dynamic_cast<ResponsePacket*>(packet.get());
        auto& data = response->additionalData;

        std::vector<std::string> keys = {
            "id", "email", "first_name", "last_name", "role", "phone_number",
            "password_hash", "created_at", "updated_at"
        };

        std::string html;

        for (const auto& user : data) {
            html += "<tr>";

            std::vector<std::string> fields;
            for (const auto& key : keys) {
                auto field = safe_get(user, key);
                if (key == "id" && safe_cast<long long>(field) == m_current_user.id)
                    m_current_user = CurrentUser(safe_cast<long long>(safe_get(user, "id")), safe_get(user, "first_name"), safe_get(user, "last_name"), safe_get(user, "role"));
                fields.emplace_back(std::move(field));
            }

            std::vector<std::pair<std::string, std::string>> user;
            for (size_t i = 0; i < fields.size(); ++i) {
                user.push_back(std::make_pair(keys[i], fields[i]));
                html += std::format(R"(<td>{}</td>)", fields[i]);
            }
            m_users.insert({ fields[0], std::move(user) });

            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                fields[0],
                fields[0]
            );

            html += "</tr>";
        }

        std::string user_data = std::format(R"-(
            <p>Имя: {}</p>
            <p>Фамилия: {}</p>
            <p>Роль: {}</p>
        )-", m_current_user.name, m_current_user.surname, m_current_user.role);

        html = HtmlView::replace_placeholder(m_html, "{{ROWS}}", html);
        html = HtmlView::replace_placeholder(html, "{{UserData}}", user_data);
        m_doc = litehtml::document::createFromString(html, view.get());
      
        view->reset_scroll();

        if (auto deletion_err = m_doc->root()->select_one("#deletion-err")) {
            auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
            reg_err_text->appendTo(deletion_err);
        }

        if (userData.contains("user_id")) {
            auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
            input_sort->value(safe_get(userData, "user_id"), true);
            this->sort_data("id");
        }

        view->render();

    });
}

ProfilePage::ProfilePage(std::shared_ptr<HtmlView> view) : Page(view), m_current_sort(std::make_pair("id", SortType::ASCENDING)) {
	m_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Личный кабинет</title>
        <style>
             body {
                font-family: Arial, sans-serif;
                margin: 20px;
            }

            .section {
                margin-bottom: 30px;
            }

            h2 {
                color: #333;
            }

            table {
                width: 100%;
                border-collapse: collapse;
                margin-top: 10px;
            }

            th {
                background-color: #d8f3dc;
                cursor: pointer;
            }
            th:hover{
                background-color: #b7e4c7;
            }
            #no-hover:hover {
                background-color: #d8f3dc;
                cursor: auto;
            }

            th,
            td {
                border: 1px solid #ccc;
                padding: 8px;
                text-align: left;
            }

            input,
            select,
            button {
                padding: 6px;
                margin-top: 5px;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-sizing: border-box;
            }

            input {
                width: 50%;
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

		    header {
			    display: flex;
			    justify-content: space-between;
			    align-items: center;
		    }

            .role-panel {
                margin-top: 30px;
            }

            .link {
                text-align: center;
                font-size: 14px;
                color: #007BFF;
                cursor: pointer;
            }

            .link:hover {
                color: #0056b3;
                text-decoration: underline;
            }
            .users-header{
                display: flex;
                flex-direction: column;
                max-width: 100%;
            }
            .users-header>p{
                width: 70%;
            }
            .users-header>p>input{
                width: 70%;
            }  
        </style>
    </head>
    <body>
        <header>
            <h1>Личный кабинет</h1>
            <button id="logout-button">Выйти</button>
        </header>
        <div class="section">
            {{UserData}}
        </div>
        <div id="admin-panel" class="role-panel">
            <div class="section">
                <div class="users-header">
                    <h2>Пользователи</h2>
                    <p>Сортировать: <input type="text" id="input-sort" placeholder="Введите текст..."></p> 
                </div>
                <table>
                    <thead>
                        <tr>
                            <th id="sort-id">ID</th>
                            <th id="sort-email">Email</th>
                            <th id="sort-first_name">Имя</th>
                            <th id="sort-last_name">Фамилия</th>
                            <th id="sort-role">Роль</th>
                            <th id="sort-phone_number">Номер телефона</th>
                            <th id="sort-password_hash">Пароль</th>
                            <th id="sort-created_at">Создан</th>
                            <th id="sort-updated_at">Изменён</th>
                            <th id="no-hover" colspan=2></th>
                        </tr>
                    </thead>
                    <tbody id="user-list">
                        {{ROWS}}
                    </tbody>
                </table>
                <div>
                    <p style="color:red; font-size: 14px;" id="deletion-err"></p> 
                    <button id="add-user-button">Добавить пользователя</button>
                </div>
            </div>

            <div class="section">
                <h2>Управление номерами</h2>
                <button id="switch-rooms">Перейти к номерам</button>
            </div>

            <div class="section">
                <h2>Управление бронированием</h2>
                <button id="switch-bookings">Перейти к бронированию</button>
            </div>
        </div>
    </body>
    </html>
)-";

    m_edit_user_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Изменение пользователя</title>
        <style>
             body {
                font-family: Arial, sans-serif;
                margin: 20px;
            }

            input,
            button {
                padding: 6px;
                margin-top: 5px;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-sizing: border-box;
            }

            input {
                width: 50%;
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

	        header {
		        display: flex;
		        justify-content: space-between;
		        align-items: center;
	        }
            .input-wrapper {
                display: flex;
                flex-direction: column;
                margin-bottom: 8px;
            }

            .input-wrapper input {
                margin-top: 4px;
                padding: 8px;
                width: 100%;
                box-sizing: border-box;
            }
        </style>
    </head>
    <body>
        <header>
            <h1>Редактирование пользователя</h1>
            <button id="back-button">Назад</button>
        </header>
        <div>
            {{ROWS}}
        </div>
        <button id="edit-submit-button">Подтвердить</button>
        <p style="color:red; font-size: 14px;" id="edition-err"></p>
    </body>
    </html>
    )-";

    m_add_user_html = R"-(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="UTF-8">
    <title>Регистрация пользователя</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            margin: 20px;

        }

        .container {
            background-color: white;
            padding: 20px;
            margin-top: 40px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            width: 300px;
            margin: auto;
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
        input[type="email"]
        {
            width: 100%;
            padding: 10px;
            margin-top: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }

        #add-user-submit-button{
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
            padding: 6px;
            margin-top: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
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

        header {
            display: flex;
	        justify-content: space-between;
	        align-items: center;
        }

    </style>
</head>
<body>

<header>
    <h1>Регистрация пользователя</h1>
    <button id="back-button">Назад</button>
</header>
<div class="container">
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

        <button id="add-user-submit-button">Зарегистрировать</button>
        <p style="color:red; font-size: 14px;" id="reg-err"></p>
    </div>
</div>

</body>
</html> 
)-";
    m_id = PageID::PROFILE;
}