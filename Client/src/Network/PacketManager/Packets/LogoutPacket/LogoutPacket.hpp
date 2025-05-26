#pragma once
#include "../Packet.hpp"

class LogoutPacket : public Packet
{
public:
	LogoutPacket();
	LogoutPacket(nlohmann::json& data);

	PacketID getID() const override;
	std::string getName() const override;
};

