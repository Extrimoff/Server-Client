#include "DeleteDataPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"
#include "../GetDataPacket/GetDataPacket.hpp"

#include <print>

void DeleteDataPacket::handlePacket(class Server& server, class RemoteClient& client)
{
	try {
		if (client.clientData.role != UserRole::ADMIN) {
			ResponsePacket resp(ResponseID::AccessDenied, "Access Denied", m_requestID);
			client.sendData(resp);
			return;
		}

		auto& db = server.getDatabase();

		auto& tableName = s_tableIDmap.at(m_tableID);

		std::unique_ptr<SQLite::Statement> query = std::make_unique<SQLite::Statement>(db, std::format("SELECT * FROM {} WHERE id = ?", tableName));

		query->bind(1, m_recordID);

		if (!query->executeStep()) {
			std::string errStr = std::format("Can't find data id = {} from {}.", m_recordID, static_cast<int>(m_tableID));
			std::println("{}", errStr);
			ResponsePacket resp(ResponseID::DeletionError, errStr, m_requestID);
			client.sendData(resp);
			return;
		}
		
		if(m_tableID == TableID::USERS) {
			auto emailIndex = query->getColumnIndex("email");
			auto targetLogin = query->getColumn(emailIndex).getString();
			if (client.clientData.login == targetLogin) {
				ResponsePacket resp(ResponseID::AccessDenied, "Cannot delete yourself", m_requestID);
				client.sendData(resp);
				return;
			}
		}
		
		query = std::make_unique<SQLite::Statement>(db, std::format("DELETE FROM {} WHERE id = ?", tableName));

		query->bind(1, m_recordID);

		query->exec();

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID);
		client.sendData(resp);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Server error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}