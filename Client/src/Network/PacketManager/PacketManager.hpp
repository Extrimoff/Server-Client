#pragma once
#include "Packets/Packet.hpp"
#include "Packets/LoginPacket/LoginPacket.hpp"
#include "Packets/RegisterPacket/RegisterPacket.hpp"
#include "Packets/ResponsePacket/ResponsePacket.hpp"
#include "Packets/GetDataPacket/GetDataPacket.hpp"
#include "Packets/LogoutPacket/LogoutPacket.hpp"
#include "Packets/DeleteDataPacket/DeleteDataPacket.hpp"
#include "Packets/EditDataPacket/EditDataPacket.hpp"
#include "Packets/AddDataPacket/AddDataPacket.hpp"

class PacketManager
{
public:
	static std::unique_ptr<Packet> CreatePacket(PacketID id) {
		switch (id)
		{
		case PacketID::Login:
			return std::make_unique<LoginPacket>();
			break;
		case PacketID::Register:
			return std::make_unique<RegisterPacket>();
			break;
		case PacketID::Response:
			return std::make_unique<ResponsePacket>();
			break;
		case PacketID::GetData:
			return std::make_unique<GetDataPacket>();
			break;
		case PacketID::Logout:
			return std::make_unique<LogoutPacket>();
			break;
		case PacketID::DeleteData:
			return std::make_unique<DeleteDataPacket>();
			break;
		case PacketID::EditData:
			return std::make_unique<EditDataPacket>();
			break;
		case PacketID::AddData:
			return std::make_unique<AddDataPacket>();
			break;
		default:
			return std::make_unique<Packet>();
			break;
		}
	}
	static std::unique_ptr<Packet> CreatePacket(PacketID id, nlohmann::json& data) {
		switch (id)
		{
		case PacketID::Login:
			return std::make_unique<LoginPacket>(data);
			break;
		case PacketID::Register:
			return std::make_unique<RegisterPacket>(data);
			break;
		case PacketID::Response:
			return std::make_unique<ResponsePacket>(data);
			break;
		case PacketID::GetData:
			return std::make_unique<GetDataPacket>(data);
			break;
		case PacketID::Logout:
			return std::make_unique<LogoutPacket>(data);
			break;
		case PacketID::DeleteData:
			return std::make_unique<DeleteDataPacket>(data);
			break;
		case PacketID::EditData:
			return std::make_unique<EditDataPacket>(data);
			break;
		case PacketID::AddData:
			return std::make_unique<AddDataPacket>(data);
			break;
		default:
			return std::make_unique<Packet>();
			break;
		}
	}
	static std::unique_ptr<Packet> CreatePacket(nlohmann::json& data) {
		PacketID id = data["type"];
		return PacketManager::CreatePacket(id, data);
	}
};

