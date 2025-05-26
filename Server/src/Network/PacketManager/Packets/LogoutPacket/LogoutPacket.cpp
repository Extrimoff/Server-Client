#include "LogoutPacket.hpp"
#include "../../../RemoteClient/RemoteClient.hpp"

void LogoutPacket::handlePacket(class Server& server, class RemoteClient& client)
{
	client.clientData = ClientData(false, "Anonymous", UserRole::GUEST);
}