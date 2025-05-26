#pragma once
#include <string>

enum class UserRole {
	GUEST,
	ADMIN
};

struct ClientData {
	bool		isLoggedIn;
	std::string login;
	UserRole	role;
};