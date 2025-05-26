#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

class DeleteDataPacket : public Packet
{
private:
	TableID  m_tableID;
	int64_t m_recordID;
public:
	DeleteDataPacket() = default;
	DeleteDataPacket(TableID tableID, int64_t recordID) : m_tableID(tableID), m_recordID(recordID) { }
	DeleteDataPacket(nlohmann::json& data) {
		this->parse(data);
	}

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override
	{
		return PacketID::DeleteData;
	}

	std::string getName() const override
	{
		return "DeleteDataPacket";
	}

	void parse(nlohmann::json& data) override {
		m_tableID = data["table_id"];
		m_recordID = data["record_id"];
		m_requestID = data["request_id"];
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
		return json;
	}
};

