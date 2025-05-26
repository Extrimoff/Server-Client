#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

class DeleteDataPacket : public Packet
{
private:
	TableID  m_tableID;
	int64_t m_recordID;
public:
	DeleteDataPacket();
	DeleteDataPacket(TableID tableID, int64_t recordID);
	DeleteDataPacket(nlohmann::json& data);

	void handlePacket() override;
	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override;
	nlohmann::json toJSON() const override;
};

