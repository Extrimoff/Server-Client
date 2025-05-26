#include "AddDataPacket.hpp"

AddDataPacket::AddDataPacket() = default;

AddDataPacket::AddDataPacket(TableID table, nlohmann::json&& data) : m_table(table), m_data(std::move(data)) {
}

AddDataPacket::AddDataPacket(nlohmann::json& data) {
	this->parse(data); 
}

PacketID AddDataPacket::getID() const  {
	return PacketID::AddData; 
}

std::string AddDataPacket::getName() const  {
	return "AddDataPacket"; 
}

void AddDataPacket::parse(nlohmann::json& data)  {
	m_table = data["table"];
	m_requestID = data["request_id"];
	try {
		m_data = nlohmann::json::parse(data.value("data", "{}"));
	}
	catch (...) {
		m_data = nlohmann::json::object();
	}
}

std::string AddDataPacket::toString() const  {
	return this->toJSON().dump();
}

nlohmann::json AddDataPacket::toJSON() const  {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["table"] = m_table;
	json["data"] = m_data.dump();
	return json;
}