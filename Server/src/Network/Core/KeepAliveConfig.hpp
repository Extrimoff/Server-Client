#pragma once
#include <WinSock2.h>

struct KeepAliveConfig {
    u_long ka_idle = 120;
    u_long ka_intvl = 3;
    u_long ka_cnt = 5;
};
