#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

namespace SQLite {
	class Database;
}


class AddDataPacket : public Packet
{
private:
	TableID			m_table;
	nlohmann::json	m_data;
public:
	AddDataPacket() = default;
	AddDataPacket(nlohmann::json& data) { this->parse(data); }

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override { return PacketID::AddData; }
	std::string getName() const override { return "AddDataPacket"; }

	void parse(nlohmann::json& data) override {
		m_table = data["table"];
		m_requestID = data["request_id"];
		try {
			m_data = nlohmann::json::parse(data.value("data", "{}"));
		}
		catch (...) {
			m_data = nlohmann::json::object();
		}
	}

	std::string toString() const override {
		return this->toJSON().dump();
	}

	nlohmann::json toJSON() const override {
		nlohmann::json json;
		json["type"] = this->getID();
		json["request_id"] = m_requestID;
		json["table"] = m_table;
		json["data"] = m_data.dump();
		return json;
	}

private:
	bool handleRoomAdd(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName);
	bool handleBookingAdd(class Server& server, class RemoteClient& client, SQLite::Database& db, std::string const& tableName);
};

