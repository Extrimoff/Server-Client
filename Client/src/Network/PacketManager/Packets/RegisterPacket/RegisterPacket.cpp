#include "RegisterPacket.hpp"

RegisterPacket::RegisterPacket() = default;

RegisterPacket::RegisterPacket(const std::string& _login, const std::string& password, const std::string& _name,
		const std::string& _surname, const std::string& _phoneNumber) :
		m_login(_login), m_password(password), m_name(_name),
		m_surname(_surname), m_phoneNumber(_phoneNumber) {
}

RegisterPacket::RegisterPacket(nlohmann::json& data) { 
	this->parse(data); 
}

void RegisterPacket::handlePacket() {
	
};

PacketID RegisterPacket::getID() const  {
	return PacketID::Register;
}

std::string RegisterPacket::getName() const { 
	return "RegisterPacket"; 
}

void RegisterPacket::parse(nlohmann::json& data)  {
	m_login = data["login"];
	m_password = data["password"];
	m_name = data["name"];
	m_surname = data["surname"];
	m_phoneNumber = data["phone_number"];
	m_requestID = data["request_id"];
}

std::string RegisterPacket::toString() const {
	return this->toJSON().dump();
}

nlohmann::json RegisterPacket::toJSON() const {
	nlohmann::json json;
	json["type"] = this->getID();
	json["request_id"] = m_requestID;
	json["login"] = m_login;
	json["password"] = m_password;
	json["name"] = m_name;
	json["surname"] = m_surname;
	json["phone_number"] = m_phoneNumber;
	return json;
}