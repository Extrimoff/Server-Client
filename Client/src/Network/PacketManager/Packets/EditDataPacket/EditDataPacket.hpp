#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

class EditDataPacket : public Packet
{
private:
	TableID			m_tableID;
	int64_t		m_recordID;
	nlohmann::json	m_newData;
public:
	EditDataPacket();
	EditDataPacket(TableID tableID, int64_t recordID, nlohmann::json newData);
	EditDataPacket(nlohmann::json& data);

	void handlePacket() override;
	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override;
	nlohmann::json toJSON() const override;
};
