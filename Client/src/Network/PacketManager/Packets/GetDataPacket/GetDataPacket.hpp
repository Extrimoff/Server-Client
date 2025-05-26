#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

class GetDataPacket : public Packet
{
private:
	TableID m_table;
public:
	GetDataPacket();
	GetDataPacket(TableID table);
	GetDataPacket(nlohmann::json& data);

	void handlePacket() override;
	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override ;
	nlohmann::json toJSON() const override;
};

