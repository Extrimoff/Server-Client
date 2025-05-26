#pragma once
#include "../Packet.hpp"

class LoginPacket : public Packet
{
private:
	std::string m_login;
	std::string m_password;
public:
	LoginPacket();
	LoginPacket(std::string _login, std::string password);
	LoginPacket(nlohmann::json& data);

	void handlePacket() override;
	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override;
	nlohmann::json toJSON() const override;
};

