#pragma once

#include "cpprest/ws_client.h"
#include "cpprest/ws_msg.h"
#include <mutex>
#include <queue>

namespace web
{
namespace websockets
{
namespace client
{
namespace details
{
struct outgoing_msg_queue
{
    enum class state
    {
        was_empty,
        was_not_empty,
    };

    state push(websocket_outgoing_message& msg)
    {
        state ret = state::was_not_empty;
        std::lock_guard<std::mutex> lock(m_lock);
        if (m_queue.empty())
        {
            ret = state::was_empty;
        }

        m_queue.push(msg);
        return ret;
    }

    bool pop_and_peek(websocket_outgoing_message& msg)
    {
        std::lock_guard<std::mutex> lock(m_lock);

        m_queue.pop();

        if (m_queue.empty())
        {
            return false;
        }
        msg = m_queue.front();
        return true;
    }

private:
    std::mutex m_lock;
    std::queue<websocket_outgoing_message> m_queue;
};

} // namespace details
} // namespace client
} // namespace websockets
} // namespace web