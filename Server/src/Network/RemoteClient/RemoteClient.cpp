#include "RemoteClient.hpp"
#include "../../Utils/base64.hpp"
#include "../PacketManager/PacketManager.hpp"
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <print>

std::string RemoteClient::getFullIP() const {
    char buffer[256];
    auto host = this->getHost();
    inet_ntop(AF_INET, &host, buffer, sizeof(buffer) - 1);
    std::string str(buffer);
    str += ":" + std::to_string(ntohs(this->getPort()));
    return str;
}

void RemoteClient::handleSSLError(int result)
{
    int err = SSL_get_error(m_ssl, result);
    switch (err) {
    case SSL_ERROR_ZERO_RETURN:
        this->disconnect();

    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_SYSCALL:
        break;
    case SSL_ERROR_SSL:
        ERR_print_errors_fp(stderr);
        this->disconnect();
        break;
    default:
        std::println(stderr, "Unknown SSL error: {}", err);
        this->disconnect();
    }
}

std::vector<uint8_t> RemoteClient::receiveData() {
    std::vector<uint8_t> buffer;
    if (!m_ssl) return buffer;

    if (u_long t = true; SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &t)) return {};

    uint32_t size = 0;
    int read_bytes = SSL_read(m_ssl, &size, sizeof(size));

    if (u_long t = false; SOCKET_ERROR == ioctlsocket(m_socket, FIONBIO, &t)) return {};

    if (read_bytes <= 0) {
        handleSSLError(read_bytes);
        return buffer;
    }

    if (size == 0 || size > 10 * 1024 * 1024)
        return buffer;

    buffer.resize(size);
    size_t received = 0;

    while (received < size) {
        int ret = SSL_read(m_ssl, buffer.data() + received, static_cast<int>(size - received));
        if (ret <= 0) {
            handleSSLError(read_bytes);
            return {};
        }
        received += ret;
    }

    return buffer;
}

bool RemoteClient::sendData(class Packet const& packet) const
{
    auto rawData = base64::to_base64(packet.toString());
    return this->sendData(rawData.c_str(), rawData.size());
}

bool RemoteClient::sendData(const void* buffer, size_t size) const {
    if (!m_ssl || size == 0) return false;

    uint32_t len = static_cast<uint32_t>(size);
    uint8_t header[sizeof(len)];
    memcpy(header, &len, sizeof(len));

    int ret = SSL_write(m_ssl, header, sizeof(len));
    if (ret <= 0) {
        ERR_print_errors_fp(stderr);
        const_cast<RemoteClient*>(this)->disconnect();
        return false;
    }

    ret = SSL_write(m_ssl, buffer, static_cast<int>(size));
    if (ret <= 0) {
        ERR_print_errors_fp(stderr);
        const_cast<RemoteClient*>(this)->disconnect();
        return false;
    }

    return true;
}

SocketStatus RemoteClient::disconnect() noexcept {
    if (this->m_status != SocketStatus::connected)
        return this->m_status;

    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_socket != INVALID_SOCKET) {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    return this->m_status = SocketStatus::disconnected;
}

void RemoteClient::onConnect()
{
    std::println("Client {} connected", this->getFullIP());
}

void RemoteClient::onDisconnect()
{
    std::println("Client {} disconnected", this->getFullIP());
}