#pragma once
#include <map>

enum class TableID {
	USERS,
	ROOMS,
	BOOKINGS
};
static const std::unordered_map<TableID, std::string> s_tableIDmap = {
	{ TableID::USERS,		"Users" },
	{ TableID::ROOMS,		"Rooms" },
	{ TableID::BOOKINGS,	"Bookings" },
};