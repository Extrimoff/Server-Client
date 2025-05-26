#pragma once
#include "../Packet.hpp"

class LoginPacket : public Packet
{
private:
	std::string m_login;
	std::string m_password;
public:
	LoginPacket() = default;
	LoginPacket(nlohmann::json& data) { this->parse(data); }

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override { return PacketID::Login; }
	std::string getName() const override { return "LoginPacket"; }

	void parse(nlohmann::json& data) override {
		m_login = data["login"];
		m_password = data["password"];
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
		return json;
	}
};

