#include "AddDataPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"

#include <print>
#include <regex>

bool AddDataPacket::handleRoomAdd(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName)
{
	std::regex price_regex(R"(^\d+[.,]\d+$)");
	std::regex capacity_regex(R"(^\d+$)");
	std::regex availability_regex(R"(^[01]$)");

	for (const auto& [key, value] : m_data.items()) {
		if (!value.is_string()) continue;

		const std::string str = value.get<std::string>();

		if (key == "price_per_night" && !std::regex_match(str, price_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Invalid price value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "capacity" && !std::regex_match(str, capacity_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Invalid capacity value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "availability" && !std::regex_match(str, availability_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Invalid availability value", m_requestID);
			client.sendData(resp);
			return false;
		}
	}
	return true;
}

bool AddDataPacket::handleBookingAdd(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName)
{
	std::regex id_regex(R"(^\d+$)");
	std::regex date_regex(R"-(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$)-");
	std::regex status_regex(R"-(^(подтверждено|отменено|завершено)$)-");

	for (const auto& [key, value] : m_data.items()) {
		if (!value.is_string()) continue;

		const std::string str = value.get<std::string>();

		if (key == "user_id" && !std::regex_match(str, id_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid user ID value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "room_id" && !std::regex_match(str, id_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid room ID value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "check_in_date" && !std::regex_match(str, date_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid check in date value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "check_out_date" && !std::regex_match(str, date_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid check out date value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "status" && !std::regex_match(str, status_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid status value", m_requestID);
			client.sendData(resp);
			return false;
		}
	}

	SQLite::Statement query(db,
		"SELECT "
		"  EXISTS(SELECT 1 FROM Users WHERE id = ?) AS user_ok, "
		"  EXISTS(SELECT 1 FROM Rooms WHERE id = ?) AS room_ok;"
	);
	query.bind(1, m_data["user_id"].get<std::string>());
	query.bind(2, m_data["room_id"].get<std::string>());

	if (!query.executeStep()) {
		ResponsePacket resp(ResponseID::EditionError, "Unknown error", m_requestID);
		client.sendData(resp);
		return false;
	}

	bool userExists = query.getColumn(0).getInt() != 0;
	bool roomExists = query.getColumn(1).getInt() != 0;
	if (!userExists) {
		ResponsePacket resp(ResponseID::EditionError, "User does not exist", m_requestID);
		client.sendData(resp);
		return false;
	}
	else if (!roomExists) {
		ResponsePacket resp(ResponseID::EditionError, "Room does not exist", m_requestID);
		client.sendData(resp);
		return false;
	}

	return true;
}

void AddDataPacket::handlePacket(class Server& server, class RemoteClient& client)
{
	auto join = [](const std::vector<std::string>& parts, const std::string& delim) -> std::string {
		std::ostringstream oss;
		for (size_t i = 0; i < parts.size(); ++i) {
			if (i != 0) oss << delim;
			oss << parts[i];
		}
		return oss.str();
	};
	try {
		
		if (client.clientData.role != UserRole::ADMIN) {
			ResponsePacket resp(ResponseID::AccessDenied, "Access Denied", m_requestID);
			client.sendData(resp);
			return;
		}

		auto& db = server.getDatabase();

		auto const& tableName = s_tableIDmap.at(m_table);

		if (m_table == TableID::ROOMS && !this->handleRoomAdd(server, client, db, tableName)) return;
		else if(m_table == TableID::BOOKINGS && !this->handleBookingAdd(server, client, db, tableName)) return;

		std::vector<std::string> fields;
		std::vector<std::string> placeholders;
		std::vector<std::string> keys;

		for (auto& [key, value] : m_data.items()) {
			fields.push_back(key);
			placeholders.push_back("?");
			keys.push_back(key);
		}

		if (fields.empty()) {
			ResponsePacket resp(ResponseID::EditionError, "No fields to insert", m_requestID);
			client.sendData(resp);
			return;
		}

		std::string fieldsClause = join(fields, ", ");
		std::string valuesClause = join(placeholders, ", ");
		std::string sql = std::format("INSERT INTO {} ({}) VALUES ({})", tableName, fieldsClause, valuesClause);

		SQLite::Statement insertQuery(db, sql);

		int bindIndex = 1;
		for (const auto& key : keys) {
			const auto& val = m_data[key];

			if (val.is_null()) {
				insertQuery.bind(bindIndex++, nullptr);
			}
			else if (val.is_number_integer()) {
				insertQuery.bind(bindIndex++, val.get<int64_t>());
			}
			else if (val.is_number_float()) {
				insertQuery.bind(bindIndex++, val.get<double>());
			}
			else if (val.is_string()) {
				insertQuery.bind(bindIndex++, val.get<std::string>());
			}
			else {
				ResponsePacket resp(ResponseID::RegErrInvalidData, "Unsupported data type", m_requestID);
				client.sendData(resp);
				return;
			}
		}

		insertQuery.exec();

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID);
		client.sendData(resp);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Server error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}