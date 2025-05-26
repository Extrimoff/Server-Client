#pragma once
#include "../Packet.hpp"

enum class ResponseID
{
	Sucess = 0,
	UnknownError,
	RegErrUserExists,
	LogErrInvalidData,
	InternalError,
	AccessDenied,
	InvalidTable,
	DeletionError,
	EditionError,
	RegErrInvalidData,
};

class ResponsePacket : public Packet
{
private:
	ResponseID		m_errorCode;
	std::string		m_errorMessage;
	nlohmann::json	m_additionalData;
public:
	ResponsePacket() = default;
	ResponsePacket(ResponseID code, const std::string& msg, uint64_t requestID, nlohmann::json additionalData = nlohmann::json()) : m_errorCode(code), m_errorMessage(msg), m_additionalData(additionalData), Packet(requestID) {}
	ResponsePacket(nlohmann::json& data) { this->parse(data); }

	virtual void handlePacket(class Server& server, class RemoteClient& client) override { };

	virtual PacketID getID() const override { return PacketID::Response; }
	virtual std::string getName() const override { return "ResponsePacket"; }

	virtual void parse(nlohmann::json& data) override {
		m_errorCode = data["error_code"];
		m_errorMessage = data["error_message"];
		m_requestID = data["request_id"];
		try {
			m_additionalData = nlohmann::json::parse(data.value("additional_data", "{}"));
		}
		catch (...) {
			m_additionalData = nlohmann::json::object();
		}
	}

	virtual std::string toString() const override {
		return this->toJSON().dump();
	}

	virtual nlohmann::json toJSON() const override {
		nlohmann::json json;
		json["type"] = this->getID();
		json["request_id"] = m_requestID;
		json["error_code"] = m_errorCode;
		json["error_message"] = m_errorMessage;
		json["additional_data"] = m_additionalData.dump();
		return json;
	}

};

