#include "DeleteDataPacket.hpp"

DeleteDataPacket::DeleteDataPacket() = default;

DeleteDataPacket::DeleteDataPacket(TableID tableID, int64_t recordID) : m_tableID(tableID), m_recordID(recordID) {
}

DeleteDataPacket::DeleteDataPacket(nlohmann::json& data) {
	this->parse(data);
}

void DeleteDataPacket::handlePacket() {
}

PacketID DeleteDataPacket::getID() const
{
	return PacketID::DeleteData;
}

std::string DeleteDataPacket::getName() const
{
	return "DeleteDataPacket";
}

void DeleteDataPacket::parse(nlohmann::json& data) {
	m_tableID = data["table_id"];
	m_recordID = data["record_id"];
	m_requestID = data["request_id"];
}

std::string DeleteDataPacket::toString() const {
	return this->toJSON().dump();
}

nlohmann::json DeleteDataPacket::toJSON() const {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["table_id"] = m_tableID;
	json["record_id"] = m_recordID;
	return json;
}
