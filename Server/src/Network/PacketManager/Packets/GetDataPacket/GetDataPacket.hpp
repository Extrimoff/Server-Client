#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

class GetDataPacket : public Packet
{
private:
	TableID m_table;
public:
	GetDataPacket() = default;
	GetDataPacket(TableID table) : m_table(table) { }
	GetDataPacket(nlohmann::json& data) { this->parse(data); }

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override { return PacketID::GetData; }
	std::string getName() const override { return "GetDataPacket"; }

	void parse(nlohmann::json& data) override {
		m_table = data["table"];
		m_requestID = data["request_id"];
	}

	std::string toString() const override {
		return this->toJSON().dump();
	}

	nlohmann::json toJSON() const override {
		nlohmann::json json;
		json["type"] = this->getID();
		json["request_id"] = m_requestID;
		json["table"] = m_table;
		return json;
	}
};

