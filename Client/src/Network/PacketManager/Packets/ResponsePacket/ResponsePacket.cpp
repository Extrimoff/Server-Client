#include "ResponsePacket.hpp"

ResponsePacket::ResponsePacket() = default;

ResponsePacket::ResponsePacket(nlohmann::json& data) { 
	this->parse(data); 
}

PacketID ResponsePacket::getID() const  {
	return PacketID::Response; 
}

std::string ResponsePacket::getName() const  {
	return "ResponsePacket"; 
}

void ResponsePacket::parse(nlohmann::json& data)  {
	m_errorCode = data["error_code"];
	m_errorMessage = data["error_message"];
	m_requestID = data["request_id"];
	try {
		m_additionalData = nlohmann::json::parse(data.value("additional_data", "{}"));
	}
	catch (...) {
		m_additionalData = nlohmann::json::object();
	}
}

std::string ResponsePacket::toString() const  {
	return this->toJSON().dump();
}

nlohmann::json ResponsePacket::toJSON() const  {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["error_code"] = m_errorCode;
	json["error_message"] = m_errorMessage;
	json["additional_data"] = m_additionalData.dump();
	return json;
}

