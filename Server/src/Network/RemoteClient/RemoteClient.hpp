#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdint.h>
#include <mutex>
#include <WinSock2.h>
#include <vector>

#include "../Core/SocketStatus.hpp"
#include "../Core/ClientData.hpp"

class RemoteClient
{
	friend class Server;
private:
	std::mutex			m_access_mtx;
	SOCKADDR_IN			m_address;
	SOCKET				m_socket;
	SocketStatus		m_status;
	SSL*				m_ssl;		

public:
	std::atomic_bool	isDisconnecting;
	ClientData			clientData;

public:
	RemoteClient() = default;
	RemoteClient(SOCKET socket, SOCKADDR_IN address, SSL* ssl) : m_address(address), m_socket(socket), 
		m_status(SocketStatus::connected), m_ssl(ssl), clientData(ClientData(false, "Anonymous", UserRole::GUEST)) {
	}

	~RemoteClient() {
		if (this->m_socket == INVALID_SOCKET) return;
		shutdown(this->m_socket, SD_BOTH);
		closesocket(this->m_socket);
		if (!m_ssl) return;
		SSL_shutdown(m_ssl);
		SSL_free(m_ssl);
		m_ssl = nullptr;
	}

	uint32_t getHost() const { return m_address.sin_addr.S_un.S_addr; }
	uint16_t getPort() const { return m_address.sin_port; }
	SocketStatus getStatus() const { return m_status; }
	auto lock() { return std::lock_guard(m_access_mtx); }

	std::string getFullIP() const;
	std::vector<uint8_t> receiveData();
	bool sendData(class Packet const& packet) const;
	SocketStatus disconnect() noexcept;
	void onConnect();
	void onDisconnect();
private:
	bool sendData(const void* buffer, const size_t size) const;
	void handleSSLError(int result);
};

