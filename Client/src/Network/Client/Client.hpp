#pragma once
#include "../../Utils/ThreadPool/ThreadPool.hpp"
#include "../Core/SocketStatus.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <WinSock2.h>
#include <vector>
#include <string>

class Client
{
private:
	SOCKET						m_socket;
	WSAData						m_wData;
	SOCKADDR_IN					m_address;
	SocketStatus				m_status;
	ThreadPool					m_thread_pool;
	class SDLContainer*			m_container;
	SSL_CTX*					m_ctx;
	SSL*						m_ssl;

public:
	Client(class SDLContainer* con);
	~Client() { 
		if (m_status == SocketStatus::connected)
			this->disconnect(false);
		m_thread_pool.stop();
		WSACleanup(); 
	}
	ThreadPool const& getThreadPool() const noexcept { return this->m_thread_pool; }
	SocketStatus getStatus() const noexcept { return this->m_status; }
	SocketStatus connectTo(uint32_t host, uint16_t port) noexcept;
	SocketStatus connectTo(std::string const& host, uint16_t port) noexcept;
	SocketStatus disconnect(bool forced = true) noexcept;

	void dataReceivingLoop();
	bool sendData(class Packet const& packet) const;
private:
	bool sendData(const void* buffer, const size_t size) const;
	void handleSSLError(int result);
	std::vector<uint8_t> receiveData();


};

