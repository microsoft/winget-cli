#pragma once

#include "cpprest/details/cpprest_compat.h"
#include <memory>
#include <stddef.h>
#include <vector>

namespace web
{
namespace http
{
namespace client
{
namespace details
{
template<class ConnectionIsh>
class connection_pool_stack
{
public:
    // attempts to acquire a connection from the deque; returns nullptr if no connection is
    // available
    std::shared_ptr<ConnectionIsh> try_acquire() CPPREST_NOEXCEPT
    {
        const size_t oldConnectionsSize = m_connections.size();
        if (oldConnectionsSize == 0)
        {
            m_staleBefore = 0;
            return nullptr;
        }

        auto result = std::move(m_connections.back());
        m_connections.pop_back();
        const size_t newConnectionsSize = m_connections.size();
        if (m_staleBefore > newConnectionsSize)
        {
            m_staleBefore = newConnectionsSize;
        }

        return result;
    }

    // releases `released` back to the connection pool
    void release(std::shared_ptr<ConnectionIsh>&& released) { m_connections.push_back(std::move(released)); }

    bool free_stale_connections() CPPREST_NOEXCEPT
    {
        assert(m_staleBefore <= m_connections.size());
        m_connections.erase(m_connections.begin(), m_connections.begin() + m_staleBefore);
        const size_t connectionsSize = m_connections.size();
        m_staleBefore = connectionsSize;
        return (connectionsSize != 0);
    }

private:
    std::vector<std::shared_ptr<ConnectionIsh>> m_connections;
    size_t m_staleBefore = 0;
};

} // namespace details
} // namespace client
} // namespace http
} // namespace web
