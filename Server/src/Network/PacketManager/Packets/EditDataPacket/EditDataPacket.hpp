#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

namespace SQLite {
	class Database;
}

class EditDataPacket : public Packet
{
private:
	TableID			m_tableID;
	int64_t			m_recordID;
	nlohmann::json	m_newData;

public:
	EditDataPacket() = default;

	EditDataPacket(TableID tableID, int64_t recordID, nlohmann::json newData) : m_tableID(tableID), m_recordID(recordID), m_newData(newData) { }

	EditDataPacket(nlohmann::json& data) {
		this->parse(data);
	}


	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override
	{
		return PacketID::EditData;
	}

	std::string getName() const override
	{
		return "EditDataPacket";
	}

	void parse(nlohmann::json& data) override {
		m_tableID = data["table_id"];
		m_recordID = data["record_id"];
		m_requestID = data["request_id"];
		try {
			m_newData = nlohmann::json::parse(data.value("new_data", "{}"));
		}
		catch (...) {
			m_newData = nlohmann::json::object();
		}
	}

	std::string toString() const override {
		return this->toJSON().dump();
	}

	nlohmann::json toJSON() const override {
		nlohmann::json json;
		json["type"] = this->getID();
		json["request_id"] = m_requestID;
		json["table_id"] = m_tableID;
		json["record_id"] = m_recordID;
		json["new_data"] = m_newData.dump();
		return json;
	}
private:
	bool handleUserEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName);
	bool handleRoomEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName);
	bool handleBookingEdit(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName);
};
