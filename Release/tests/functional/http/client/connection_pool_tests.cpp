#include "stdafx.h"

#include "../../../src/http/common/connection_pool_helpers.h"
#include <memory>

using namespace web::http::client::details;

SUITE(connection_pooling)
{
    TEST(empty_returns_nullptr)
    {
        connection_pool_stack<int> connectionStack;
        VERIFY_ARE_EQUAL(connectionStack.try_acquire(), std::shared_ptr<int> {});
    }

    static int noisyCount = 0;
    struct noisy
    {
        noisy() = delete;
        noisy(int) { ++noisyCount; }
        noisy(const noisy&) = delete;
        noisy(noisy&&) { ++noisyCount; }
        noisy& operator=(const noisy&) = delete;
        noisy& operator=(noisy&&) = delete;
        ~noisy() { --noisyCount; }
    };

    TEST(cycled_connections_survive)
    {
        connection_pool_stack<noisy> connectionStack;
        VERIFY_ARE_EQUAL(0, noisyCount);
        connectionStack.release(std::make_shared<noisy>(42));
        connectionStack.release(std::make_shared<noisy>(42));
        connectionStack.release(std::make_shared<noisy>(42));
        VERIFY_ARE_EQUAL(3, noisyCount);
        VERIFY_IS_TRUE(connectionStack.free_stale_connections());
        auto tmp = connectionStack.try_acquire();
        VERIFY_ARE_NOT_EQUAL(tmp, std::shared_ptr<noisy> {});
        connectionStack.release(std::move(tmp));
        VERIFY_ARE_EQUAL(tmp, std::shared_ptr<noisy> {});
        tmp = connectionStack.try_acquire();
        VERIFY_ARE_NOT_EQUAL(tmp, std::shared_ptr<noisy> {});
        connectionStack.release(std::move(tmp));
        VERIFY_IS_TRUE(connectionStack.free_stale_connections());
        VERIFY_ARE_EQUAL(1, noisyCount);
        VERIFY_IS_FALSE(connectionStack.free_stale_connections());
        VERIFY_ARE_EQUAL(0, noisyCount);
        VERIFY_IS_FALSE(connectionStack.free_stale_connections());
    }
};
