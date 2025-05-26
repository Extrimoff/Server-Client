#pragma once
#include <memory>
#include "ClientKey.hpp"
#include "../RemoteClient/RemoteClient.hpp"

struct ClientComparator {
    using is_transparent = std::true_type;

    bool operator()(const std::unique_ptr<RemoteClient>& lhs, const std::unique_ptr<RemoteClient>& rhs) const {
        return (uint64_t(lhs->getHost()) | uint64_t(lhs->getPort()) << 32) < (uint64_t(rhs->getHost()) | uint64_t(rhs->getPort()) << 32);
    }

    bool operator()(const std::unique_ptr<RemoteClient>& lhs, const ClientKey& rhs) const {
        return (uint64_t(lhs->getHost()) | uint64_t(lhs->getPort()) << 32) < (uint64_t(rhs.host) | uint64_t(rhs.port) << 32);
    }

    bool operator()(const ClientKey& lhs, const std::unique_ptr<RemoteClient>& rhs) const {
        return (uint64_t(lhs.host) | uint64_t(lhs.port) << 32) < (uint64_t(rhs->getHost()) | uint64_t(rhs->getPort()) << 32);
    }
};