#include "RegisterPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"

#include <print>
#include <regex>
#include <bcrypt_.h>

void RegisterPacket::handlePacket(class Server& server, class RemoteClient& client)
{
	try {
		std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
		std::regex phone_regex(R"(^$|^\+?\d{10,15}$)");
		std::regex password_regex(R"(^.{8,}$)");

		if (!std::regex_match(m_login, email_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Invalid email format", m_requestID);
			client.sendData(resp);
			return;
		}

		if (!std::regex_match(m_password, password_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Password must be at least 8 characters", m_requestID);
			client.sendData(resp);
			return;
		}

		if (!std::regex_match(m_phoneNumber, phone_regex)) {
			ResponsePacket resp(ResponseID::RegErrInvalidData, "Invalid phone number", m_requestID);
			client.sendData(resp);
			return;
		}

		auto& db = server.getDatabase();

		auto query = SQLite::Statement(db, "SELECT * FROM Users WHERE email = ?");

		query.bind(1, m_login);

		if (query.executeStep()) {
			std::println("User {} already exists.", m_login);
			ResponsePacket resp1(ResponseID::RegErrUserExists, "User already exists", m_requestID);
			client.sendData(resp1);
			return;
		}

		m_password = bcrypt::generateHash(m_password);
		
		auto query2 = SQLite::Statement(db, R"(
			INSERT INTO Users (email, password_hash, first_name, last_name, phone_number)
			VALUES (?, ?, ?, ?, ?)
		)");

		query2.bind(1, m_login);
		query2.bind(2, m_password);
		query2.bind(3, m_name);
		query2.bind(4, m_surname);
		query2.bind(5, m_phoneNumber);

		query2.exec();

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID);
		client.sendData(resp);

		std::println("User {} successfully registered.", m_login);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Server error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}