#include "EditDataPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"
#include "../GetDataPacket/GetDataPacket.hpp"

#include <bcrypt_.h>
#include <unordered_set>
#include <regex>
#include <print>

bool EditDataPacket::handleUserEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName)
{
	auto query = SQLite::Statement(db, std::format("SELECT email FROM {} WHERE id = ?", tableName));

	query.bind(1, m_recordID);

	if (!query.executeStep()) {
		std::string errStr = std::format("Can't find record id = {} in table {}.", m_recordID, tableName);
		std::println("{}", errStr);
		ResponsePacket resp(ResponseID::EditionError, errStr, m_requestID);
		client.sendData(resp);
		return false;
	}

	std::string targetEmail = query.getColumn(0).getString();

	const bool isEditingSelf = (client.clientData.login == targetEmail);

	if (isEditingSelf) {
		static const std::unordered_set<std::string> allowedFields = {
			"first_name", "last_name", "phone_number", "password_hash"
		};

		for (auto it = m_newData.begin(); it != m_newData.end(); ++it) {
			const std::string& key = it.key();
			if (!allowedFields.contains(key)) {
				ResponsePacket resp(ResponseID::AccessDenied,
					std::format("Admins cannot change their own {}", key),
					m_requestID);
				client.sendData(resp);
				return false;
			}
		}
	}

	std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
	std::regex phone_regex(R"(^$|^\+?\d{10,15}$)");
	std::regex password_regex(R"(^.{8,}$)");

	for (const auto& [key, value] : m_newData.items()) {
		if (!value.is_string()) continue;

		const std::string str = value.get<std::string>();

		if (key == "email" && !std::regex_match(str, email_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid email format", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "phone_number" && !std::regex_match(str, phone_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid phone number", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "password_hash" && !std::regex_match(str, password_regex)) {
			ResponsePacket resp(ResponseID::EditionError,
				"Password must be at least 8 characters", m_requestID);
			client.sendData(resp);
			return false;
		}
	}

	return true;
}


bool EditDataPacket::handleRoomEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName) 
{
	std::regex price_regex(R"(^\d+[.,]\d+$)");
	std::regex capacity_regex(R"(^\d+$)");
	std::regex availability_regex(R"(^[01]$)");

	for (const auto& [key, value] : m_newData.items()) {
		if (!value.is_string()) continue;

		const std::string str = value.get<std::string>();

		if (key == "price_per_night" && !std::regex_match(str, price_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid price value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "capacity" && !std::regex_match(str, capacity_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid capacity value", m_requestID);
			client.sendData(resp);
			return false;
		}
		if (key == "availability" && !std::regex_match(str, availability_regex)) {
			ResponsePacket resp(ResponseID::EditionError, "Invalid availability value", m_requestID);
			client.sendData(resp);
			return false;
		}
	}
	return true;
}
bool EditDataPacket::handleBookingEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName) 
{
	std::regex id_regex(R"(^\d+$)");
	std::regex date_regex(R"-(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$)-");
	std::regex status_regex(R"-(^(подтверждено|отменено|завершено)$)-");

	for (const auto& [key, value] : m_newData.items()) {
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
	query.bind(1, m_newData["user_id"].get<std::string>());
	query.bind(2, m_newData["room_id"].get<std::string>());

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

void EditDataPacket::handlePacket(class Server& server, class RemoteClient& client)
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
		const auto& tableName = s_tableIDmap.at(m_tableID);

		if (m_tableID == TableID::USERS && !this->handleUserEdit(server, client, db, tableName)) return;
		else if (m_tableID == TableID::ROOMS && !this->handleRoomEdit(server, client, db, tableName)) return;
		else if (m_tableID == TableID::BOOKINGS && !this->handleBookingEdit(server, client, db, tableName)) return;

		std::vector<std::string> setParts;
		std::vector<std::string> keys;
		for (auto& [key, value] : m_newData.items()) {
			setParts.push_back(std::format("{} = ?", key));
			keys.push_back(key);
		}

		if (setParts.empty()) {
			ResponsePacket resp(ResponseID::EditionError, "No fields to update", m_requestID);
			client.sendData(resp);
			return;
		}

		std::string setClause = join(setParts, ", ");
		std::string sql = std::format("UPDATE {} SET {} WHERE id = ?", tableName, setClause);

		SQLite::Statement updateQuery(db, sql);

		int bindIndex = 1;
		for (const auto& key : keys) {
			const auto& val = m_newData[key];

			if (val.is_null()) {
				updateQuery.bind(bindIndex++, nullptr);
			}
			else if (val.is_number_integer()) {
				updateQuery.bind(bindIndex++, val.get<int64_t>());
			}
			else if (val.is_number_float()) {
				updateQuery.bind(bindIndex++, val.get<double>());
			}
			else if (val.is_string()) {
				if(key == "password_hash") updateQuery.bind(bindIndex++, bcrypt::generateHash(val.get<std::string>()));
				else updateQuery.bind(bindIndex++, val.get<std::string>());
			}
			else {
				ResponsePacket resp(ResponseID::EditionError, "Unsupported data type", m_requestID);
				client.sendData(resp);
				return;
			}
		}

		updateQuery.bind(bindIndex, m_recordID);
		updateQuery.exec();

		std::println("Updated record {} in {}.", m_recordID, tableName);

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID);
		client.sendData(resp);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Edit error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}