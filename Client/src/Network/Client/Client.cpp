#include "Client.hpp"
#include "../../Utils/Json.hpp"
#include "../../Utils/base64.hpp"
#include "../PacketManager/PacketManager.hpp"
#include "../../GUI/SDLContainer.hpp"

#include <mstcpip.h>
#include <WS2tcpip.h>
#include <print>

Client::Client(SDLContainer* con) : m_thread_pool(ThreadPool()), m_status(SocketStatus::disconnected), m_address(NULL), m_socket(NULL), m_container(con), m_ssl(nullptr), m_ctx(nullptr)
{
	if (auto err = WSAStartup(MAKEWORD(2, 2), &m_wData); err != 0) {
		char buffer[256];
		strerror_s(buffer, sizeof(buffer), err);
        std::println(stderr, 
            "WSAStartup error!\n"
            "Code: {} Err: {}", err, buffer
        );
		exit(1);
	}

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        ERR_print_errors_fp(stderr);
        return;
    }

    SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION);
    SSL_CTX_set_cipher_list(m_ctx, "HIGH:!aNULL:!MD5");
}

SocketStatus Client::connectTo(std::string const& host, uint16_t port) noexcept 
{
    uint32_t uhost;
    InetPtonA(AF_INET, host.c_str(), &uhost);
    return this->connectTo(uhost, port);
}

SocketStatus Client::connectTo(uint32_t host, uint16_t port) noexcept {
    if ((this->m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET)
        return m_status = SocketStatus::err_socket_init;

    new(&this->m_address) SOCKADDR_IN;
    this->m_address.sin_family = AF_INET;
    this->m_address.sin_addr.s_addr = host;
    this->m_address.sin_port = htons(port);

    if (connect(this->m_socket, reinterpret_cast<sockaddr*>(&m_address), sizeof(m_address)) == SOCKET_ERROR) {
        closesocket(m_socket);
        return this->m_status = SocketStatus::err_socket_connect;
    }

    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        std::println(stderr, "Failed to create SSL structure.");
        ERR_print_errors_fp(stderr);
        closesocket(m_socket);
        return this->m_status = SocketStatus::err_ssl_init;
    }

    SSL_set_fd(m_ssl, static_cast<int>(m_socket));
    SSL_set_verify(m_ssl, SSL_VERIFY_NONE, nullptr);

    if (SSL_connect(m_ssl) <= 0) {
        std::println(stderr, "SSL_connect failed.");
        ERR_print_errors_fp(stderr);
        SSL_free(m_ssl);
        m_ssl = nullptr;
        closesocket(m_socket);
        return this->m_status = SocketStatus::err_ssl_connect;
    }

    this->m_thread_pool.addJob(std::bind(&Client::dataReceivingLoop, this));
    return this->m_status = SocketStatus::connected;
}

SocketStatus Client::disconnect(bool forced) noexcept
{  
    if (this->m_status != SocketStatus::connected)
        return this->m_status;

    this->m_status = SocketStatus::disconnected;

    if (m_ssl) {
        int shutdown_status = SSL_shutdown(m_ssl);
        if (shutdown_status == 0) {
            SSL_shutdown(m_ssl);
        }
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    if (forced) {
        MessageBoxA(NULL, "Connection to server lost!", "Fatal Error", MB_ICONERROR | MB_OK);
        exit(1);
    }

    return this->m_status;
}

void Client::dataReceivingLoop()
{
    if (const auto& data = this->receiveData(); !data.empty()) {
        auto badPacket_func = [this] {
            std::println(stderr, "Bad Packet Error!");
            this->disconnect();
        };
        try {
            std::string rawData(data.begin(), data.end());
            rawData = base64::from_base64(rawData);

            nlohmann::json json = nlohmann::json::parse(rawData);

            auto packet = PacketManager::CreatePacket(json);
            if (packet->getID() == PacketID::Unknown) { badPacket_func(); }

            std::println("Received {} from Server", packet->getName());

            this->m_thread_pool.addJob(std::bind(&Client::dataReceivingLoop, this));
            m_container->on_packet_receive(std::move(packet));
            m_container->render();
        }
        catch (...) {
            badPacket_func();
        }
    }
}

bool Client::sendData(class Packet const& packet) const
{
    auto rawData = base64::to_base64(packet.toString());
    return this->sendData(rawData.c_str(), rawData.size());
}

bool Client::sendData(const void* buffer, const size_t size) const 
{
    if (!m_ssl || size == 0) return false;

    const size_t total_size = sizeof(uint32_t) + size;
    std::vector<uint8_t> send_buffer(total_size);

    uint32_t sz = static_cast<uint32_t>(size);
    memcpy(send_buffer.data(), &sz, sizeof(sz));
    memcpy(send_buffer.data() + sizeof(uint32_t), buffer, size);

    size_t total_sent = 0;
    while (total_sent < total_size) {
        int ret = SSL_write(m_ssl, send_buffer.data() + total_sent, static_cast<int>(total_size - total_sent));
        if (ret <= 0) {
            int err = SSL_get_error(m_ssl, ret);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
                continue;
            const_cast<Client*>(this)->disconnect();
            return false;
        }
        total_sent += ret;
    }

    return true;
}
//{ 
//    if (!m_ssl) return false;
//
//    const size_t total_size = size + sizeof(uint32_t);
//    std::vector<uint8_t> send_buffer(total_size);
//
//    *reinterpret_cast<uint32_t*>(send_buffer.data()) = static_cast<uint32_t>(size);
//    memcpy(send_buffer.data() + sizeof(uint32_t), buffer, size);
//
//    int bytes_sent = SSL_write(m_ssl, send_buffer.data(), static_cast<int>(send_buffer.size()));
//    return bytes_sent == static_cast<int>(send_buffer.size());
//}

void Client::handleSSLError(int result)
{
    int err = SSL_get_error(m_ssl, result);
    switch (err) {
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_SSL:
    {
        auto text = ERR_reason_error_string(ERR_get_error());
        std::println(stderr, "SSL connection error {}: {}", err, text == nullptr ? "" : text);
        disconnect();
        break;
    }
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        break;
    default:
        std::println(stderr, "Unknown SSL error: {}", err);
        disconnect();
        break;
    }
}

std::vector<uint8_t> Client::receiveData()
{
    std::vector<uint8_t> buffer;
    if (!m_ssl || m_status != SocketStatus::connected) return {};

    uint32_t size = 0;
    int bytes = SSL_read(m_ssl, reinterpret_cast<char*>(&size), sizeof(size));
    if (bytes <= 0) {
        handleSSLError(bytes);
        return {};
    }

    if (size == 0) return {};
    buffer.resize(size);

    bytes = SSL_read(m_ssl, buffer.data(), static_cast<int>(size));
    if (bytes <= 0) {
        handleSSLError(bytes);
        return {};
    }

    return buffer;
}