#pragma once

#include "../Core/KeepAliveConfig.hpp"
#include "../Core/ServerStatus.hpp"
#include "../Core/ClientComparator.hpp"
#include "../RemoteClient/RemoteClient.hpp"
#include "../../Utils/ThreadPool/ThreadPool.hpp"

#include <SQLiteCpp/SQLiteCpp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <set>

class Server
{
	using ClientIterator = std::set<std::unique_ptr<RemoteClient>, ClientComparator>::iterator;
private:
	SOCKET														m_serv_socket;
	WSAData														m_wData;
	uint16_t													m_port;
	ServerStatus												m_status;
	ThreadPool													m_thread_pool;
	KeepAliveConfig												m_ka_conf;
	SQLite::Database											m_db;
	std::set<std::unique_ptr<RemoteClient>, ClientComparator>	m_client_list;
	std::mutex													m_client_mutex;
	SSL_CTX*													m_ssl_ctx;

public:
	Server(
		const uint16_t port,
		KeepAliveConfig ka_conf = { },
		unsigned int thread_count = std::thread::hardware_concurrency()
	);

	~Server();

private:
	bool enableKeepAlive(SOCKET socket);
	void handlingAcceptLoop();
	void dataReceivingLoop();
	void initDatabase();

public:
	ThreadPool& getThreadPool() { return this->m_thread_pool; }
	SQLite::Database& getDatabase() { return this->m_db; }
	void joinLoop() { m_thread_pool.join(); }
	uint16_t getPort() const { return this->m_port; }
	uint16_t setPort(const uint16_t port) {
		this->m_port = port;
		this->start();
		return this->m_port;
	}
	ServerStatus getStatus() const { return this->m_status; }

	ServerStatus start();
	void stop();

	bool connectTo(uint32_t host, uint16_t port);
	void sendData(class Packet const& packet);
	bool sendDataBy(uint32_t host, uint16_t port, class Packet const& packet);
	bool disconnectBy(uint32_t host, uint16_t port);
	void disconnectAll();
};

