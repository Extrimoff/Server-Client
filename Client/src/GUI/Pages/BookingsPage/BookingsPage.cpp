#include "BookingsPage.hpp"
#include "../../HtmlView/HtmlView.hpp"
#include "../../../Network/PacketManager/PacketManager.hpp"
#include "../../Elements/el_input.hpp"

#include <litehtml/el_tr.h>
#include <litehtml/el_td.h>
#include <unordered_set>
#include <regex>
#include <print>

template<typename T>
static T safe_cast(const std::string& str) {
    static_assert(std::is_arithmetic_v<T>, "safe_cast only supports arithmetic types");

    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    return (result.ec == std::errc() && result.ptr == str.data() + str.size()) ? value : T{};
}

bool BookingsPage::init()
{
    auto view = m_view.lock();
    if (!view) return false;

    return true;
}

void BookingsPage::delete_data(litehtml::element::ptr const& el)
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

    DeleteDataPacket ddp(TableID::BOOKINGS, safe_cast<long long>(recordId));
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

        view->switch_page(PageID::BOOKINGS);
    });
}

void BookingsPage::edit_data(litehtml::element::ptr const& el)
{
    auto recordId = el->get_attr("data-record-id");
    if (!recordId) return;

    const static std::unordered_map<std::string, std::string> mappedLabels = {
        {"user_id", "ID пользователя"},
        {"room_id", "ID номера"},
        {"check_in_date", "Дата заезда (ГГГГ-ММ-ДД)"},
        {"check_out_date", "Дата выезда (ГГГГ-ММ-ДД)"},
        {"status", "Статус"},
    };

    auto const& booking = m_bookings[recordId];
    std::string rows = std::format
    (R"-(
        <p class="input-wrapper">{}:<input type="text" placeholder="ID пользователя" value="{}" id="edit-user"></p>
        <p style="color:red; font-size: 14px;" id="edit-user-err">Неверный формат данных</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="ID номера" value="{}" id="edit-room"></p>
        <p style="color:red; font-size: 14px;" id="edit-room-err">Неверный формат данных</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Дата заезда" value="{}" id="edit-checkin"></p>
        <p style="color:red; font-size: 14px;" id="edit-checkin-err">Дата должна быть в формате ГГГГ-ММ-ДД</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Дата выезда" value="{}" id="edit-checkout"></p>
        <p style="color:red; font-size: 14px;" id="edit-checkout-err">Дата должна быть в формате ГГГГ-ММ-ДД</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Статус" value="{}" id="edit-status"></p>
    )-", mappedLabels.at(booking[1].first), booking[1].second,
        mappedLabels.at(booking[2].first), booking[2].second,
        mappedLabels.at(booking[3].first), booking[3].second,
        mappedLabels.at(booking[4].first), booking[4].second,
        mappedLabels.at(booking[6].first), booking[6].second);

    m_booking_to_edit = std::move(booking);
    this->push_draw_task([rows = std::move(rows), this]() {
        auto view = m_view.lock();
        if (!view) return;

        view->reset_scroll();

        m_doc = litehtml::document::createFromString(HtmlView::replace_placeholder(m_edit_booking_html, "{{ROWS}}", rows), view.get());

        if (auto edition_err = m_doc->root()->select_one("#edition-err")) {
            auto edition_err_text = std::make_shared<el_textholder>("", m_doc);
            edition_err_text->appendTo(edition_err);
        }

        auto edit_user_err = m_doc->root()->select_one("#edit-user-err");
        auto edit_room_err = m_doc->root()->select_one("#edit-room-err");
        auto edit_checkin_err = m_doc->root()->select_one("#edit-checkin-err");
        auto edit_checkout_err = m_doc->root()->select_one("#edit-checkout-err");

        edit_user_err->css_w().set_display(litehtml::display_none);
        edit_room_err->css_w().set_display(litehtml::display_none);
        edit_checkin_err->css_w().set_display(litehtml::display_none);
        edit_checkout_err->css_w().set_display(litehtml::display_none);
        view->render();
    });
}

void BookingsPage::edit_data_action()
{
    auto user = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-user"));
    auto room = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-room"));
    auto checkin = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-checkin"));
    auto checkout = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-checkout"));
    auto status = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-status"));

    std::string user_value = user->get_value();
    std::string room_value = room->get_value();
    std::string checkin_value = checkin->get_value();
    std::string checkout_value = checkout->get_value();
    std::string status_value = status->get_value();

    std::regex id_regex(R"(^\d+$)");
    std::regex date_regex(R"-(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$)-");

    bool is_user_valid = std::regex_match(user_value, id_regex);
    bool is_room_valid = std::regex_match(room_value, id_regex);
    bool is_checkin_valid = std::regex_match(checkin_value, date_regex);
    bool is_checkout_valid = std::regex_match(checkout_value, date_regex);
    bool error = !is_user_valid || !is_room_valid || !is_checkin_valid || !is_checkout_valid;

    auto edit_user_err = m_doc->root()->select_one("#edit-user-err");
    auto edit_room_err = m_doc->root()->select_one("#edit-room-err");
    auto edit_checkin_err = m_doc->root()->select_one("#edit-checkin-err");
    auto edit_checkout_err = m_doc->root()->select_one("#edit-checkout-err");

    edit_user_err->css_w().set_display(litehtml::display_none);
    edit_room_err->css_w().set_display(litehtml::display_none);
    edit_checkin_err->css_w().set_display(litehtml::display_none);
    edit_checkout_err->css_w().set_display(litehtml::display_none);


    if (!is_user_valid) edit_user_err->css_w().set_display(litehtml::display_block);
    if (!is_room_valid) edit_room_err->css_w().set_display(litehtml::display_block);
    if (!is_checkin_valid) edit_checkin_err->css_w().set_display(litehtml::display_block);
    if (!is_checkout_valid) edit_checkout_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    nlohmann::json newData = {};
    if (m_booking_to_edit[1].second != user_value)      newData[m_booking_to_edit[1].first] = user_value;
    if (m_booking_to_edit[2].second != room_value)      newData[m_booking_to_edit[2].first] = room_value;
    if (m_booking_to_edit[3].second != checkin_value)   newData[m_booking_to_edit[3].first] = checkin_value;
    if (m_booking_to_edit[4].second != checkout_value)  newData[m_booking_to_edit[4].first] = checkout_value;
    if (m_booking_to_edit[6].second != status_value)    newData[m_booking_to_edit[6].first] = status_value;

    EditDataPacket edp(TableID::BOOKINGS, safe_cast<long long>(m_booking_to_edit[0].second), newData);
    this->send_packet_async(edp, [this](std::unique_ptr<Packet> packet) {
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

        view->switch_page(PageID::BOOKINGS);
    });
}

void BookingsPage::register_booking()
{
    this->push_draw_task([this]() {
        auto view = m_view.lock();
        if (!view) return;
        m_doc = litehtml::document::createFromString(m_add_booking_html, view.get());

        view->reset_scroll();

        auto user_err = m_doc->root()->select_one("#reg-user-err");
        auto room_err = m_doc->root()->select_one("#reg-room-err");
        auto checkin_err = m_doc->root()->select_one("#reg-checkin-err");
        auto checkout_err = m_doc->root()->select_one("#reg-checkout-err");
        auto reg_err = m_doc->root()->select_one("#reg-err");

        user_err->css_w().set_display(litehtml::display_none);
        room_err->css_w().set_display(litehtml::display_none);
        checkin_err->css_w().set_display(litehtml::display_none);
        checkout_err->css_w().set_display(litehtml::display_none);

        auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
        reg_err_text->appendTo(reg_err);

        view->render();
    });
}

void BookingsPage::register_booking_action()
{
    auto user = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-user"));
    auto room = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-room"));
    auto checkin = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-checkin"));
    auto checkout = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-checkout"));
    auto status = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-status"));

    std::string user_value = user->get_value();
    std::string room_value = room->get_value();
    std::string checkin_value = checkin->get_value();
    std::string checkout_value = checkout->get_value();
    std::string status_value = status->get_value();

    std::regex id_regex(R"(^\d+$)");
    std::regex date_regex(R"-(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$)-");

    bool is_user_valid = std::regex_match(user_value, id_regex);
    bool is_room_valid = std::regex_match(room_value, id_regex);
    bool is_checkin_valid = std::regex_match(checkin_value, date_regex);
    bool is_checkout_valid = std::regex_match(checkout_value, date_regex);
    bool error = !is_user_valid || !is_room_valid || !is_checkin_valid || !is_checkout_valid;

    auto edit_user_err = m_doc->root()->select_one("#reg-user-err");
    auto edit_room_err = m_doc->root()->select_one("#reg-room-err");
    auto edit_checkin_err = m_doc->root()->select_one("#reg-checkin-err");
    auto edit_checkout_err = m_doc->root()->select_one("#reg-checkout-err");

    edit_user_err->css_w().set_display(litehtml::display_none);
    edit_room_err->css_w().set_display(litehtml::display_none);
    edit_checkin_err->css_w().set_display(litehtml::display_none);
    edit_checkout_err->css_w().set_display(litehtml::display_none);


    if (!is_user_valid) edit_user_err->css_w().set_display(litehtml::display_block);
    if (!is_room_valid) edit_room_err->css_w().set_display(litehtml::display_block);
    if (!is_checkin_valid) edit_checkin_err->css_w().set_display(litehtml::display_block);
    if (!is_checkout_valid) edit_checkout_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    nlohmann::json newData = {};
    newData["user_id"] = user_value;
    newData["room_id"] = room_value;
    newData["check_in_date"] = checkin_value;
    newData["check_out_date"] = checkout_value;
    newData["status"] = status_value;

    AddDataPacket rp(TableID::BOOKINGS, std::move(newData));
    this->send_packet_async(rp, [=](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;
        auto view = m_view.lock();
        if (!view) return;

        edit_user_err->css_w().set_display(litehtml::display_none);
        edit_room_err->css_w().set_display(litehtml::display_none);
        edit_checkin_err->css_w().set_display(litehtml::display_none);
        edit_checkout_err->css_w().set_display(litehtml::display_none);

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode == ResponseID::Sucess) {
            view->switch_page(PageID::BOOKINGS);
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

void BookingsPage::sort_data(std::string&& field_name)
{
    auto get_field = [](const booking_data_t& data, const std::string& key) -> std::string {
        for (const auto& pair : data) {
            if (pair.first == key)
                return pair.second;
        }
        return "";
        };

    auto view = m_view.lock();
    if (!view) return;

    auto sorted_bookings = sorted_bookings_t(m_bookings.begin(), m_bookings.end());

    auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
    auto& sort_value = input_sort->get_value();

    auto sort_type = m_current_sort.second;

    if (sort_value == m_last_sort_value) {
        if (m_current_sort.first == field_name) {
            sort_type = m_current_sort.second == SortType::ASCENDING ? SortType::DESCENDING : SortType::ASCENDING;
        }
    }
    m_last_sort_value = sort_value;

    std::sort(sorted_bookings.begin(), sorted_bookings.end(), [&](const auto& u1, const auto& u2) {
        auto compare = [&]<typename T>(T const& arg1, T const& arg2) -> bool {
            if (sort_type == SortType::ASCENDING) return arg1 < arg2;
            else return arg1 > arg2;
        };

        if (const static std::unordered_set<std::string> fields_to_check = { "id", "user_id", "room_id" }; fields_to_check.contains(field_name)) {
            auto num1 = safe_cast<int>(get_field(u1.second, field_name));
            auto num2 = safe_cast<int>(get_field(u2.second, field_name));
            return compare(num1, num2);
        }

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


    if (sort_value != "") {
        sorted_bookings.erase(
            std::remove_if(
                sorted_bookings.begin(),
                sorted_bookings.end(),
                [&](const auto& booking) {
                    const auto& fields = booking.second;
                    return std::none_of(fields.begin(), fields.end(), [&](const auto& field) {
                        if (field.first != field_name) return false;

                        if (const static std::unordered_set<std::string> fields_to_check = { "check_in_date", "check_out_date", "booking_date" }; fields_to_check.contains(field_name)) {
                            return field.second >= sort_value;
                        }

                        if (field_name == "id") {
                            auto sort_id = safe_cast<int>(sort_value);
                            auto id = safe_cast<int>(field.second);
                            return id == sort_id;
                        }

                        return field.second.find(sort_value) != std::string::npos;
                        });
                }
            ),
            sorted_bookings.end()
        );
    }

    m_current_sort = std::make_pair(std::move(field_name), sort_type);

    this->push_draw_task([sort_value = std::move(sort_value), sorted_bookings = std::move(sorted_bookings), this]() {
        auto view = m_view.lock();
        if (!view) return;

        std::string html;

        for (const auto& booking : sorted_bookings) {
            html += "<tr>";

            for (auto const& field : booking.second) {
                if (const static std::unordered_set<std::string> testedStr = { "user_id", "room_id" }; testedStr.contains(field.first)) {
                    html += std::format(R"(<td class="link" id="switch-{}" data-record-id="{}">{}</td>)", field.first, field.second, field.second);
                }
                else {
                    html += std::format(R"(<td>{}</td>)", field.second);
                }
            }

            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                booking.first,
                booking.first
            );

            html += "</tr>";
        }

        html = HtmlView::replace_placeholder(m_html, "{{ROWS}}", html);
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

bool BookingsPage::on_element_click(const litehtml::element::ptr& el)
{
    auto view = m_view.lock();
    if (!view) return false;

    auto id = el->get_attr("id");
    if (id == nullptr) return false;

    if (!strcmp(id, "delete-record-button")) {
        this->delete_data(el);
        return true;
    }
    else if (!strcmp(id, "edit-record-button")) {
        this->edit_data(el);
        return true;
    }
    else if (!strcmp(id, "back-to-profile-button")) {
        this->push_draw_task([this]() {
            auto view = m_view.lock();
            if (!view) return;

            view->switch_page(PageID::PROFILE);
            });
        return true;
    }
    else if (!strcmp(id, "back-button")) {
        this->push_draw_task([this]() {
            auto view = m_view.lock();
            if (!view) return;

            view->switch_page(PageID::BOOKINGS);
            });
        return true;
    }
    else if (!strcmp(id, "edit-submit-button")) {
        this->edit_data_action();
        return true;
    }
    else if (!strcmp(id, "add-booking-button")) {
        this->register_booking();
        return true;
    }
    else if (!strcmp(id, "add-booking-submit-button")) {
        this->register_booking_action();
        return true;
    }
    else if (!strcmp(id, "switch-rooms")) {
        this->push_draw_task([this]() {
            auto view = m_view.lock();
            if (!view) return;

            view->switch_page(PageID::ROOMS);
            });
        return true;
    }
    else if (!strcmp(id, "switch-user_id")) {
        this->push_draw_task([this, el = el]() {
            auto view = m_view.lock();
            if (!view) return;

            nlohmann::json data;
            data["user_id"] = el->get_attr("data-record-id");
            view->switch_page(PageID::PROFILE, std::move(data));
            });
        return true;
    }
    else if (!strcmp(id, "switch-room_id")) {
        this->push_draw_task([this, el = el]() {
            auto view = m_view.lock();
            if (!view) return;

            nlohmann::json data;
            data["room_id"] = el->get_attr("data-record-id");
            view->switch_page(PageID::ROOMS, std::move(data));
            });
        return true;
    }
    else if (std::string field = id, prefix = "sort-"; field.find(prefix) == 0) {
        field = field.substr(prefix.length());
        this->sort_data(std::move(field));
        return true;
    }
   
}

void BookingsPage::on_switch(nlohmann::json data) {
    GetDataPacket gdp(TableID::BOOKINGS);

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

    this->send_packet(gdp, [this, safe_get = safe_get, data = std::move(data)](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto* response = dynamic_cast<ResponsePacket*>(packet.get());
        auto& data = response->additionalData;

        std::vector<std::string> keys = {
            "id", "user_id", "room_id", "check_in_date", "check_out_date", "booking_date", "status"
        };

        std::string html;

        for (const auto& booking : data) {
            html += "<tr>";

            std::vector<std::string> fields;
            for (const auto& key : keys) {
                auto field = safe_get(booking, key);
                fields.emplace_back(std::move(field));
            }

            std::vector<std::pair<std::string, std::string>> booking;
            for (size_t i = 0; i < fields.size(); ++i) {
                booking.push_back(std::make_pair(keys[i], fields[i]));
                if (const static std::unordered_set<std::string> testedStr = { "user_id", "room_id" }; testedStr.contains(keys[i])) {
                    html += std::format(R"(<td class="link" id="switch-{}" data-record-id="{}">{}</td>)", keys[i], fields[i], fields[i]);
                }
                else {
                    html += std::format(R"(<td>{}</td>)", fields[i]);
                }
            }
            m_bookings.insert({ fields[0], std::move(booking) });

            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                fields[0],
                fields[0]
            );

            html += "</tr>";
        }

        html = HtmlView::replace_placeholder(m_html, "{{ROWS}}", html);
        m_doc = litehtml::document::createFromString(html, view.get());

        if (auto deletion_err = m_doc->root()->select_one("#deletion-err")) {
            auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
            reg_err_text->appendTo(deletion_err);
        }

    });
}

BookingsPage::BookingsPage(std::shared_ptr<HtmlView> view) : Page(view) {
	m_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Управление бронированием</title>
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
            .bookings-header{
                display: flex;
                flex-direction: column;
                max-width: 100%;
            }
            .bookings-header>p{
                width: 70%;
            }
            .bookings-header>p>input{
                width: 70%;
            }  

        </style>
    </head>
    <body>
        <header>
            <h1>Управление бронированием</h1>
            <button id="back-to-profile-button">Личный кабинет</button>
        </header>
        <div id="admin-panel" class="role-panel">
            <div class="section">
                <div class="bookings-header">
                    <h2>Бронирования</h2>
                    <p>Сортировать: <input type="text" id="input-sort" placeholder="Введите текст..."></p> 
                </div>
                <table>
                    <thead>
                        <tr>
                            <th id="sort-id">ID</th>
                            <th id="sort-user_id">ID пользователя</th>
                            <th id="sort-room_id">Номер</th>
                            <th id="sort-check_in_date">Дата заезда</th>
                            <th id="sort-check_out_date">Дата выезда</th>
                            <th id="sort-booking_date">Дата бронирования</th>
                            <th id="sort-status">Статус</th>
                            <th id="no-hover" colspan=2></th>
                        </tr>
                    </thead>
                    <tbody id="bookings-list">
                        {{ROWS}}
                    </tbody>
                </table>
                <div>
                    <p style="color:red; font-size: 14px;" id="deletion-err"></p> 
                    <button id="add-booking-button">Добавить бронирование</button>
                </div>
            </div>

            <div class="section">
                <h2>Управление номерами</h2>
                <button id="switch-rooms">Перейти к номерам</button>
            </div>
        </div>
    </body>
    </html>
)-";

    m_edit_booking_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Изменение бронирования</title>
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
            <h1>Редактирование бронирования</h1>
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

    m_add_booking_html = R"-(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <title>Добавление бронирования</title>
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
                box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
                width: 300px;
                margin: auto;
            }

            h1,
            h2 {
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
            input[type="email"] {
                width: 100%;
                padding: 10px;
                margin-top: 10px;
                border: 1px solid #ddd;
                border-radius: 4px;
                box-sizing: border-box;
            }

            #add-booking-submit-button {
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
            <h1>Добавление бронирования</h1>
            <button id="back-button">Назад</button>
        </header>
        <div class="container">
            <div id="register-form">
                <h2>Добавление</h2>
                <input type="text" placeholder="ID пользователя" id="reg-user">
                <p style="color:red; font-size: 14px;" id="reg-user-err">Неверный формат данных</p>

                <input type="text" placeholder="ID номера" id="reg-room">
                <p style="color:red; font-size: 14px;" id="reg-room-err">Неверный формат данных</p>

                <input type="text" placeholder="Дата заезда"  id="reg-checkin">
                <p style="color:red; font-size: 14px;" id="reg-checkin-err">Дата должна быть в формате ГГГГ-ММ-ДД</p>

                <input type="text" placeholder="Дата выезда" id="reg-checkout">
                <p style="color:red; font-size: 14px;" id="reg-checkout-err">Дата должна быть в формате ГГГГ-ММ-ДД</p>

                <input type="text" placeholder="Статус" id="reg-status">

                <button id="add-booking-submit-button">Добавить</button>
                <p style="color:red; font-size: 14px;" id="reg-err"></p>
            </div>
        </div>
    </body>

    </html>
)-";
    m_id = PageID::BOOKINGS;
}