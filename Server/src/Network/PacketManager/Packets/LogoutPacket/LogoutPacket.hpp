#pragma once
#include "../Packet.hpp"

class LogoutPacket : public Packet
{
public:
	LogoutPacket() = default;
	LogoutPacket(nlohmann::json& data) { this->parse(data); }

	void handlePacket(class Server& server, class RemoteClient& client) override;

	PacketID getID() const override { return PacketID::Logout; }
	std::string getName() const override { return "LogoutPacket"; }
};

