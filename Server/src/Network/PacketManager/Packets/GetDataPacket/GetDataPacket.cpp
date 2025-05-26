#include "GetDataPacket.hpp"
#include "../../../Server/Server.hpp"
#include "../ResponsePacket/ResponsePacket.hpp"

#include <print>

void GetDataPacket::handlePacket(class Server& server, class RemoteClient& client) 
{
	try {
		if (client.clientData.role != UserRole::ADMIN) {
			ResponsePacket resp(ResponseID::AccessDenied, "Access Denied", m_requestID);
			client.sendData(resp);
			return;
		}

		auto& db = server.getDatabase();

		auto& tableName = s_tableIDmap.at(m_table);

		std::string sql = std::format("SELECT * FROM {}", tableName);
		SQLite::Statement query(db, sql);

		nlohmann::json result = nlohmann::json::array();

		bool hasData = false;
		while (query.executeStep()) {
			hasData = true;

			int colCount = query.getColumnCount();
			nlohmann::json row;

			for (int i = 0; i < colCount; ++i) {
				auto col = query.getColumn(i);
				std::string colName = col.getName();

				if (col.isNull()) {
					row[colName] = nullptr;
				}
				else if (col.getType() == SQLite::INTEGER) {
					row[colName] = col.getInt64();
				}
				else if (col.getType() == SQLite::FLOAT) {
					row[colName] = col.getDouble();
				}
				else if (col.getType() == SQLite::TEXT) {
					row[colName] = col.getString();
				}
				else if (col.getType() == SQLite::BLOB) {
					std::string blob(reinterpret_cast<const char*>(col.getBlob()), col.getBytes());
					row[colName] = blob;
				}
			}
			result.push_back(row);
		}

		if (!hasData) {
			std::println("Invalid table or no data: {}.", tableName);
			ResponsePacket resp(ResponseID::InvalidTable, "Invalid table or no data", m_requestID);
			client.sendData(resp);
			return;
		}

		ResponsePacket resp(ResponseID::Sucess, "", m_requestID, result);
		client.sendData(resp);
	}
	catch (const std::exception& e) {
		std::println(stderr, "Server error: {}", e.what());
		ResponsePacket resp(ResponseID::InternalError, "Internal server error", m_requestID);
		client.sendData(resp);
	}
}