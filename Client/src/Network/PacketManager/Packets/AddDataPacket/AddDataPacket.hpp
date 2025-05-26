#pragma once
#include "../Packet.hpp"
#include "../../../Core/DatabaseSchema.hpp"

namespace SQLite {
	class Database;
}


class AddDataPacket : public Packet
{
private:
	TableID			m_table;
	nlohmann::json	m_data;
public:
	AddDataPacket();
	AddDataPacket(TableID table, nlohmann::json&& data);
	AddDataPacket(nlohmann::json& data);

	PacketID getID() const override;
	std::string getName() const override;
	void parse(nlohmann::json& data) override;
	std::string toString() const override;
	nlohmann::json toJSON() const override;
};

