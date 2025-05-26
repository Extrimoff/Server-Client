#include "RoomsPage.hpp"
#include "../../HtmlView/HtmlView.hpp"
#include "../../../Network/PacketManager/PacketManager.hpp"
#include "../../Elements/el_input.hpp"

#include <litehtml/el_tr.h>
#include <litehtml/el_td.h>
#include <regex>
#include <print>

template<typename T>
static T safe_cast(const std::string& str) {
    static_assert(std::is_arithmetic_v<T>, "safe_cast only supports arithmetic types");

    T value{};
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    return (result.ec == std::errc() && result.ptr == str.data() + str.size()) ? value : T{};
}

bool RoomsPage::init()
{
    auto view = m_view.lock();
    if (!view) return false;

    return true;
}

void RoomsPage::delete_data(litehtml::element::ptr const& el)
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

    DeleteDataPacket ddp(TableID::ROOMS, safe_cast<long long>(recordId));
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

        view->switch_page(PageID::ROOMS);
    });
}

void RoomsPage::edit_data(litehtml::element::ptr const& el)
{
    auto recordId = el->get_attr("data-record-id");
    if (!recordId) return;

    const static std::unordered_map<std::string, std::string> mappedLabels = {
        {"room_type", "Тип комнаты"},
        {"price_per_night", "Цена за ночь"},
        {"capacity", "Вместимость"},
        {"availability", "Доступность"},
        {"description", "Описание"},
    };

    auto const& room = m_rooms[recordId];
    std::string rows = std::format
    (R"-(
        <p class="input-wrapper">{}:<input type="text" placeholder="Тип комнаты" value="{}" id="edit-room-type"></p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Цена за ночь" value="{}" id="edit-price-per-night"></p>
        <p style="color:red; font-size: 14px;" id="edit-price-err">Неверный формат цены</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Вместимость" value="{}" id="edit-capacity"></p>
        <p style="color:red; font-size: 14px;" id="edit-capacity-err">Неверный формат данных</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Доступность" value="{}" id="edit-availability"></p>
        <p style="color:red; font-size: 14px;" id="edit-availability-err">Неверный формат данных</p>

        <p class="input-wrapper">{}:<input type="text" placeholder="Описание" value="{}" id="edit-description"></p>
    )-", mappedLabels.at(room[1].first), room[1].second, 
        mappedLabels.at(room[2].first), room[2].second,
        mappedLabels.at(room[3].first), room[3].second,
        mappedLabels.at(room[4].first), room[4].second,
        mappedLabels.at(room[5].first), room[5].second);

    m_room_to_edit = std::move(room);
    this->push_draw_task([rows = std::move(rows), this]() {
        auto view = m_view.lock();
        if (!view) return;

        m_doc = litehtml::document::createFromString(HtmlView::replace_placeholder(m_edit_room_html, "{{ROWS}}", rows), view.get());

        if (auto edition_err = m_doc->root()->select_one("#edition-err")) {
            auto edition_err_text = std::make_shared<el_textholder>("", m_doc);
            edition_err_text->appendTo(edition_err);
        }

        auto edit_price_err = m_doc->root()->select_one("#edit-price-err");
        auto edit_capacity_err = m_doc->root()->select_one("#edit-capacity-err");
        auto edit_availability_err = m_doc->root()->select_one("#edit-availability-err");

        edit_price_err->css_w().set_display(litehtml::display_none);
        edit_capacity_err->css_w().set_display(litehtml::display_none);
        edit_availability_err->css_w().set_display(litehtml::display_none);
        view->render();
    });
}

void RoomsPage::edit_data_action()
{
    auto room_type = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-room-type"));
    auto price_per_night = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-price-per-night"));
    auto capacity = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-capacity"));
    auto availability = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-availability"));
    auto description = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#edit-description"));

    std::string room_type_value = room_type->get_value();
    std::string price_per_night_value = price_per_night->get_value();
    std::string capacity_value = capacity->get_value();
    std::string availability_value = availability->get_value();
    std::string description_value = description->get_value();

    std::regex price_regex(R"(^\d+[.,]\d+$)");
    std::regex capacity_regex(R"(^\d+$)");
    std::regex availability_regex(R"(^[01]$)");

    bool is_price_valid = std::regex_match(price_per_night_value, price_regex);
    bool is_capacity_valid = std::regex_match(capacity_value, capacity_regex);
    bool is_availability_valid = std::regex_match(availability_value, availability_regex);
    bool error = !is_price_valid || !is_capacity_valid || !is_availability_valid;

    auto edit_price_err = m_doc->root()->select_one("#edit-price-err");
    auto edit_capacity_err = m_doc->root()->select_one("#edit-capacity-err");
    auto edit_availability_err = m_doc->root()->select_one("#edit-availability-err");

    edit_price_err->css_w().set_display(litehtml::display_none);
    edit_capacity_err->css_w().set_display(litehtml::display_none);
    edit_availability_err->css_w().set_display(litehtml::display_none);


    if (!is_price_valid) edit_price_err->css_w().set_display(litehtml::display_block);
    if (!is_capacity_valid) edit_capacity_err->css_w().set_display(litehtml::display_block);
    if (!is_availability_valid) edit_availability_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    std::replace(price_per_night_value.begin(), price_per_night_value.end(), ',', '.');

    nlohmann::json newData = {};
    if (m_room_to_edit[1].second != room_type_value)        newData[m_room_to_edit[1].first] = room_type_value;
    if (m_room_to_edit[2].second != price_per_night_value)  newData[m_room_to_edit[2].first] = price_per_night_value; 
    if (m_room_to_edit[3].second != capacity_value)         newData[m_room_to_edit[3].first] = capacity_value;
    if (m_room_to_edit[4].second != availability_value)     newData[m_room_to_edit[4].first] = availability_value;
    if (m_room_to_edit[5].second != description_value)      newData[m_room_to_edit[5].first] = description_value;

    EditDataPacket edp(TableID::ROOMS, safe_cast<long long>(m_room_to_edit[0].second), newData);
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

        view->switch_page(PageID::ROOMS);
    });
}

void RoomsPage::register_room()
{
    this->push_draw_task([this]() {
        auto view = m_view.lock();
        if (!view) return;
        m_doc = litehtml::document::createFromString(m_add_room_html, view.get());

        view->reset_scroll();

        auto price_err = m_doc->root()->select_one("#reg-price-err");
        auto capacity_err = m_doc->root()->select_one("#reg-capacity-err");
        auto availability_err = m_doc->root()->select_one("#reg-availability-err");
        auto reg_err = m_doc->root()->select_one("#reg-err");

        price_err->css_w().set_display(litehtml::display_none);
        capacity_err->css_w().set_display(litehtml::display_none);
        availability_err->css_w().set_display(litehtml::display_none);

        auto reg_err_text = std::make_shared<el_textholder>("", m_doc);
        reg_err_text->appendTo(reg_err);

        view->render();
    });
}

void RoomsPage::register_room_action()
{
    auto room_type = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-type"));
    auto price = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-price-per-night"));
    auto capacity = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-capacity"));
    auto availability = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-availability"));
    auto description = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#reg-description"));

    std::string room_type_value = room_type->get_value();
    std::string price_per_night_value = price->get_value();
    std::string capacity_value = capacity->get_value();
    std::string availability_value = availability->get_value();
    std::string description_value = description->get_value();

    std::regex price_regex(R"(^\d+[.,]\d+$)");
    std::regex capacity_regex(R"(^\d+$)");
    std::regex availability_regex(R"(^[01]$)");

    bool is_price_valid = std::regex_match(price_per_night_value, price_regex);
    bool is_capacity_valid = std::regex_match(capacity_value, capacity_regex);
    bool is_availability_valid = std::regex_match(availability_value, availability_regex);
    bool error = !is_price_valid || !is_capacity_valid || !is_availability_valid;

    auto edit_price_err = m_doc->root()->select_one("#reg-price-err");
    auto edit_capacity_err = m_doc->root()->select_one("#reg-capacity-err");
    auto edit_availability_err = m_doc->root()->select_one("#reg-availability-err");

    edit_price_err->css_w().set_display(litehtml::display_none);
    edit_capacity_err->css_w().set_display(litehtml::display_none);
    edit_availability_err->css_w().set_display(litehtml::display_none);


    if (!is_price_valid) edit_price_err->css_w().set_display(litehtml::display_block);
    if (!is_capacity_valid) edit_capacity_err->css_w().set_display(litehtml::display_block);
    if (!is_availability_valid) edit_availability_err->css_w().set_display(litehtml::display_block);

    if (error) return;

    nlohmann::json newData = {};
    newData["room_type"] = room_type_value;
    newData["price_per_night"] = price_per_night_value;
    newData["capacity"] = capacity_value;
    newData["availability"] = availability_value;
    newData["description"] = description_value;

    AddDataPacket rp(TableID::ROOMS,std::move(newData));
    this->send_packet_async(rp, [=](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;
        auto view = m_view.lock();
        if (!view) return;

        edit_price_err->css_w().set_display(litehtml::display_none);
        edit_capacity_err->css_w().set_display(litehtml::display_none);
        edit_availability_err->css_w().set_display(litehtml::display_none);

        auto response = dynamic_cast<ResponsePacket*>(packet.get());

        if (response->errorCode == ResponseID::Sucess) {
            view->switch_page(PageID::ROOMS);
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

void RoomsPage::sort_data(std::string&& field_name)
{
    auto get_field = [](const room_data_t& data, const std::string& key) -> std::string {
        for (const auto& pair : data) {
            if (pair.first == key)
                return pair.second;
        }
        return "";
    };

    auto view = m_view.lock();
    if (!view) return;

    auto sorted_rooms = sorted_rooms_t(m_rooms.begin(), m_rooms.end());

    auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
    auto& sort_value = input_sort->get_value();

    auto sort_type = m_current_sort.second;

    if (sort_value == m_last_sort_value) {
        if (m_current_sort.first == field_name) {
            sort_type = m_current_sort.second == SortType::ASCENDING ? SortType::DESCENDING : SortType::ASCENDING;
        }
    }
    m_last_sort_value = sort_value;

    std::sort(sorted_rooms.begin(), sorted_rooms.end(), [&](const auto& u1, const auto& u2) {
        auto compare = [&]<typename T>(T const& arg1, T const& arg2) -> bool {
            if (sort_type == SortType::ASCENDING) return arg1 < arg2;
            else return arg1 > arg2;
        };

        if (field_name == "price_per_night") {
            double price1 = std::stod(get_field(u1.second, field_name));
            double price2 = std::stod(get_field(u2.second, field_name));
            return compare(price1, price2);
        }
        else if (field_name == "capacity") {
            auto capacity1 = safe_cast<int>(get_field(u1.second, field_name));
            auto capacity2 = safe_cast<int>(get_field(u2.second, field_name));
            return compare(capacity1, capacity2);
        }
        else if (field_name == "id") {
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
        sorted_rooms.erase(
            std::remove_if(
                sorted_rooms.begin(),
                sorted_rooms.end(),
                [&](const auto& room) {
                    const auto& fields = room.second;
                    return std::none_of(fields.begin(), fields.end(), [&](const auto& field) {
                        if (field.first != field_name) return false;

                        if (field_name == "price_per_night") {
                            auto sort_price = std::stod(sort_value);
                            auto price = std::stod(field.second);
                            return price >= sort_price;
                        }
                        else if (field_name == "capacity") {
                            auto sort_capacity = safe_cast<int>(sort_value);
                            auto capacity = safe_cast<int>(field.second);
                            return capacity >= sort_capacity;
                        }
                        else if (field_name == "id") {
                            auto sort_id = safe_cast<int>(sort_value);
                            auto id = safe_cast<int>(field.second);
                            return id == sort_id;
                        }
                        return field.second.find(sort_value) != std::string::npos;
                });
                }
            ),
            sorted_rooms.end()
        );
    }

    m_current_sort = std::make_pair(std::move(field_name), sort_type);

    this->push_draw_task([sort_value = std::move(sort_value), sorted_rooms = std::move(sorted_rooms), this]() {
        auto view = m_view.lock();
        if (!view) return;

        std::string html;

        for (const auto& room : sorted_rooms) {
            html += "<tr>";

            for (auto const& field : room.second) {
                html += std::format(R"(<td>{}</td>)", field.second);
            }

            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                room.first,
                room.first
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

bool RoomsPage::on_element_click(const litehtml::element::ptr& el)
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

            view->switch_page(PageID::ROOMS);
        });
        return true;
    }
    else if (!strcmp(id, "edit-submit-button")) {
        this->edit_data_action();
        return true;
    }
    else if (!strcmp(id, "add-room-button")) {
        this->register_room();
        return true;
    }
    else if (!strcmp(id, "add-room-submit-button")) {
        this->register_room_action();
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

void RoomsPage::on_switch(nlohmann::json data) {
    GetDataPacket gdp(TableID::ROOMS);
    m_rooms.clear();
    m_room_to_edit = { };

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
            return std::format("{:.2f}", val.get<double>());
        }
        else if (val.is_boolean()) {
            return val.get<bool>() ? "true" : "false";
        }

        return val.dump();
        };

    this->send_packet(gdp, [this, safe_get = safe_get, roomData = std::move(data)](std::unique_ptr<Packet> packet) {
        if (packet->getID() != PacketID::Response) return;

        auto view = m_view.lock();
        if (!view) return;

        auto* response = dynamic_cast<ResponsePacket*>(packet.get());
        auto& data = response->additionalData;

        std::vector<std::string> keys = {
            "id", "room_type", "price_per_night", "capacity", "availability", "description"
        };

        std::string html;

        for (const auto& room : data) {
            html += "<tr>";

            std::vector<std::string> fields;
            for (const auto& key : keys) {
                auto field = safe_get(room, key);
                fields.emplace_back(std::move(field));
            }

            std::vector<std::pair<std::string, std::string>> room;
            for (size_t i = 0; i < fields.size(); ++i) {
                room.push_back(std::make_pair(keys[i], fields[i]));
                html += std::format(R"(<td>{}</td>)", fields[i]);
            }
            
            m_rooms.insert({ fields[0], std::move(room) });
            html += std::format(
                R"(<td class="link" id="edit-record-button" data-record-id="{}">Изменить</td><td class="link" id="delete-record-button" data-record-id="{}">Удалить</td>)",
                fields[0],
                fields[0]
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

        if (roomData.contains("room_id")) {
            auto input_sort = std::dynamic_pointer_cast<el_input>(m_doc->root()->select_one("#input-sort"));
            input_sort->value(safe_get(roomData, "room_id"), true);
            this->sort_data("id");
        }

        view->render();
    });

}

RoomsPage::RoomsPage(std::shared_ptr<HtmlView> view) : Page(view) {
	m_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Управление номерами</title>
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
            .rooms-header{
                display: flex;
                flex-direction: column;
                max-width: 100%;
            }
            .rooms-header>p{
                width: 70%;
            }
            .rooms-header>p>input{
                width: 70%;
            }  
        </style>
    </head>
    <body>
        <header>
            <h1>Управление номерами</h1>
            <button id="back-to-profile-button">Личный кабинет</button>
        </header>
        <div id="admin-panel" class="role-panel">
            <div class="section">
                <div class="rooms-header">
                    <h2>Номера</h2>
                    <p>Сортировать: <input type="text" id="input-sort" placeholder="Введите текст..."></p> 
                </div>
                <table>
                    <thead>
                        <tr>
                            <th id="sort-id">ID</th>
                            <th id="sort-room_type">Тип</th>
                            <th id="sort-price_per_night">Цена за ночь</th>
                            <th id="sort-capacity">Вместимость</th>
                            <th id="sort-availability">Доступность</th>
                            <th id="sort-description">Описание</th>
                            <th id="no-hover" colspan=2></th>
                        </tr>
                    </thead>
                    <tbody id="room-list">
                        {{ROWS}}
                    </tbody>
                </table>
                <div>
                    <p style="color:red; font-size: 14px;" id="deletion-err"></p> 
                    <button id="add-room-button">Добавить номер</button>
                </div>
            </div>

            <div class="section">
                <h2>Управление бронированием</h2>
                <button id="switch-bookings">Перейти к бронированию</button>
            </div>
        </div>
    </body>
    </html>
)-";

    m_edit_room_html = R"-(
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Изменение номера</title>
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
            <h1>Редактирование номера</h1>
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

    m_add_room_html = R"-(
    <!DOCTYPE html>
    <html>
    <head>
    <meta charset="UTF-8">
    <title>Добавление номера</title>
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

        #add-room-submit-button {
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
            <h1>Добавление номера</h1>
            <button id="back-button">Назад</button>
        </header>
        <div class="container">
            <div id="register-form">
                <h2>Добавление</h2>
                <input type="text" id="reg-type" placeholder="Тип комнаты">

                <input type="text" id="reg-price-per-night" placeholder="Цена за ночь">
                <p style="color:red; font-size: 14px;" id="reg-price-err">Неверный формат цены</p>

                <input type="text" id="reg-capacity" placeholder="Вместимость">
                <p style="color:red; font-size: 14px;" id="reg-capacity-err">Неверный формат данных</p>

                <input type="text" id="reg-availability" placeholder="Доступность">
                <p style="color:red; font-size: 14px;" id="reg-availability-err">Неверный формат данных</p>

                <input type="text" id="reg-description" placeholder="Описание">

                <button id="add-room-submit-button">Добавить</button>
                <p style="color:red; font-size: 14px;" id="reg-err"></p>
            </div>
        </div>
    </body>

    </html>
)-";

    m_id = PageID::ROOMS;
}

