#include "LogoutPacket.hpp"

LogoutPacket::LogoutPacket() = default;
LogoutPacket::LogoutPacket(nlohmann::json& data) {
	this->parse(data);
}

PacketID LogoutPacket::getID() const {
	return PacketID::Logout;
}

std::string LogoutPacket::getName() const {
	return "LogoutPacket";
}
