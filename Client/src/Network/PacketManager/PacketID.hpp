#pragma once
#include <stdint.h>

enum class PacketID : uint8_t
{
	Login,
	Register,
	Response,
	GetData,
	Logout,
	DeleteData,
	EditData,
	AddData,
	Unknown = 0xFF
};
