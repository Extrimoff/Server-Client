#include "EditDataPacket.hpp"


EditDataPacket::EditDataPacket() = default;

EditDataPacket::EditDataPacket(TableID tableID, int64_t recordID, nlohmann::json newData) : m_tableID(tableID), m_recordID(recordID), m_newData(newData) {
}

EditDataPacket::EditDataPacket(nlohmann::json& data) {
	this->parse(data);
}

void EditDataPacket::handlePacket() {
}

PacketID EditDataPacket::getID() const
{
	return PacketID::EditData;
}

std::string EditDataPacket::getName() const
{
	return "EditDataPacket";
}

void EditDataPacket::parse(nlohmann::json& data) {
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

std::string EditDataPacket::toString() const {
	return this->toJSON().dump();
}

nlohmann::json EditDataPacket::toJSON() const {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["table_id"] = m_tableID;
	json["record_id"] = m_recordID;
	json["new_data"] = m_newData.dump();
	return json;
}
