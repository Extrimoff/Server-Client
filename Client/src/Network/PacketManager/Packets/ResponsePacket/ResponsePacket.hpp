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
	RegErrInvalidEmail,
	RegErrInvalidPassword,
	RegErrInvalidName,
	RegErrInvalidSurname,
	RegErrInvalidPhone
};

class ResponsePacket : public Packet
{
private:
	ResponseID		m_errorCode;
	std::string		m_errorMessage;
	nlohmann::json	m_additionalData;
public:
	ResponsePacket();
	ResponsePacket(nlohmann::json& data);

	virtual PacketID getID() const override;
	virtual std::string getName() const override;
	virtual void parse(nlohmann::json& data) override;
	virtual std::string toString() const override;
	virtual nlohmann::json toJSON() const override;

public:
	ResponseID	getErrorCode() const { return m_errorCode; }
	std::string const& getErrorMessage() const { return m_errorMessage; }
	nlohmann::json const& getAdditionalData() const { return m_additionalData; }

	__declspec(property(get = getErrorCode))		ResponseID		errorCode;
	__declspec(property(get = getErrorMessage))		std::string		errorMessage;
	__declspec(property(get = getAdditionalData))	nlohmann::json	additionalData;

};

