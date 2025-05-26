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
	RegisterPacket();
	RegisterPacket(const std::string& _login, const std::string& password, const std::string& _name,
		const std::string& _surname, const std::string& _phoneNumber);
	RegisterPacket(nlohmann::json& data);

	void handlePacket() override;
	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override;
	nlohmann::json toJSON() const override;
};

