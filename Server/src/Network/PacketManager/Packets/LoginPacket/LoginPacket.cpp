#include "LoginPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"

#include <bcrypt_.h>
#include <print>

void LoginPacket::handlePacket(Server &const server, class RemoteClient& client)
{
	try {
		auto& db = server.getDatabase();

		SQLite::Statement query(db, "SELECT * FROM Users WHERE email = ?");

		query.bind(1, m_login);
		

		if (!query.executeStep()) {
			std::println("Invalid credentials for {}.", m_login);
			ResponsePacket resp(ResponseID::LogErrInvalidData, "Invalid credentials", m_requestID);
			client.sendData(resp);
			return;
		}
		
		int hashIndex = query.getColumnIndex("password_hash");
		std::string hash = query.getColumn(hashIndex).getString();

		bool validate = bcrypt::validatePassword(m_password, hash);

		if (!validate) {
			std::println("Invalid credentials for {}.", m_login);
			ResponsePacket resp(ResponseID::LogErrInvalidData, "Invalid credentials", m_requestID);
			client.sendData(resp);
			return;
		}

		int roleIndex = query.getColumnIndex("role");
		auto str_role = query.getColumn(roleIndex).getString();

		UserRole role = UserRole::GUEST;

		if (str_role == "admin") {
			role = UserRole::ADMIN;
		}

		if (role != UserRole::ADMIN) {
			std::println("Acess denied for: {} {}.", m_login, str_role);
			ResponsePacket resp(ResponseID::AccessDenied, "Access denied", m_requestID);
			client.sendData(resp);
			return;
		}

		client.clientData.isLoggedIn = true;
		client.clientData.login = m_login;
		client.clientData.role = role;

		int idIndex = query.getColumnIndex("id");
		auto str_id = query.getColumn(idIndex).getInt64();
		int nameIndex = query.getColumnIndex("first_name");
		auto str_name = query.getColumn(nameIndex).getString();
		int surnameIndex = query.getColumnIndex("last_name");
		auto str_surname = query.getColumn(surnameIndex).getString();

		nlohmann::json additionalData = {};
		additionalData["id"] = str_id;
		additionalData["name"] = str_name;
		additionalData["surname"] = str_surname;
		additionalData["role"] = str_role;

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID, additionalData);
		client.sendData(resp);
		std::println("Login success for: {} {}.", m_login, str_role);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Server error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}