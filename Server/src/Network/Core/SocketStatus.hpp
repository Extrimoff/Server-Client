#pragma once
#include <stdint.h>

enum class SocketStatus : uint8_t {
	connected = 0,
	err_socket_init = 1,
	err_socket_bind = 2,
	err_socket_connect = 3,
	disconnected = 4,
};