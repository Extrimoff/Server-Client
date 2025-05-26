#pragma once
#include "../PacketID.hpp"
#include "../../../Utils/Json.hpp"
#include <string>
#include <chrono>

class Packet
{
protected:
	uint64_t m_requestID;
public:
	Packet() : m_requestID(std::chrono::high_resolution_clock::now().time_since_epoch().count()) { }

	virtual ~Packet() = default;
	virtual PacketID getID() const { return PacketID::Unknown; }
	virtual std::string getName() const { return "Unknown"; }
	virtual void handlePacket() { return; }
	virtual void parse(nlohmann::json& data) { 
		m_requestID = data["request_id"];
	}
	virtual std::string toString() const {
		return this->toJSON().dump();
	}
	virtual nlohmann::json toJSON() const {
		nlohmann::json json;
		json["type"] = this->getID();
		json["request_id"] = m_requestID;
		return json;
	}

	uint64_t getRequestID() const { return m_requestID; }
};

