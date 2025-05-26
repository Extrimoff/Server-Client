#pragma once
#include "../Packet.hpp"

class RegisterPacket : public Packet
{
private:
	std::string m_login;
	std::string m_password;
	std::string m_name;
	std::string m_surname;
	std::string m_phoneNumber;

public:
	RegisterPacket() = default;
	RegisterPacket(nlohmann::json& data) { this->parse(data); }

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override { return PacketID::Register; }
	std::string getName() const override { return "RegisterPacket"; }

	void parse(nlohmann::json& data) override {
		m_login = data["login"];
		m_password = data["password"];
		m_name = data["name"];
		m_surname = data["surname"];
		m_phoneNumber = data["phone_number"];
		m_requestID = data["request_id"];
	}

	std::string toString() const override {
		return this->toJSON().dump();
	}

	nlohmann::json toJSON() const override {
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
};

