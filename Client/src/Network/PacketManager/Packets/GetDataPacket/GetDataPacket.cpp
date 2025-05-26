#include "GetDataPacket.hpp"

GetDataPacket::GetDataPacket() = default;

GetDataPacket::GetDataPacket(nlohmann::json& data) { 
	this->parse(data); 
}

GetDataPacket::GetDataPacket(TableID table) : m_table(table) {

}

void GetDataPacket::handlePacket() {

}

PacketID GetDataPacket::getID() const { 
	return PacketID::GetData; 
}

std::string GetDataPacket::getName() const {
	return "GetDataPacket"; 
}

void GetDataPacket::parse(nlohmann::json& data) {
	m_table = data["table"];
	m_requestID = data["request_id"];
}

std::string GetDataPacket::toString() const {
	return this->toJSON().dump();
}

nlohmann::json GetDataPacket::toJSON() const {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["table"] = m_table;
	return json;
}
