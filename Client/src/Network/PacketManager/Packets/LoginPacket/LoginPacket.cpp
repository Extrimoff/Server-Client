#include "LoginPacket.hpp"

LoginPacket::LoginPacket() = default;

LoginPacket::LoginPacket(std::string _login, std::string password) : m_login(_login), m_password(password) {
	
}

LoginPacket::LoginPacket(nlohmann::json& data) { 
	this->parse(data); 
}

void LoginPacket::handlePacket() {

};

PacketID LoginPacket::getID() const { 
	return PacketID::Login; 
}

std::string LoginPacket::getName() const { 
	return "LoginPacket"; 
}

void LoginPacket::parse(nlohmann::json& data) {
	m_login = data["login"];
	m_password = data["password"];
	m_requestID = data["request_id"];
}

std::string LoginPacket::toString() const {
	return this->toJSON().dump();
}

nlohmann::json LoginPacket::toJSON() const {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["login"] = m_login;
	json["password"] = m_password;
	return json;
}