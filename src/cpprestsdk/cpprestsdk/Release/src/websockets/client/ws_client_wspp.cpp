/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Websocket library: Client-side APIs.
 *
 * This file contains a cross platform implementation based on WebSocket++.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <thread>

#if !defined(CPPREST_EXCLUDE_WEBSOCKETS)

#include "../../http/common/x509_cert_utilities.h"
#include "pplx/threadpool.h"
#include "ws_client_impl.h"

// Force websocketpp to use C++ std::error_code instead of Boost.
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100 4127 4512 4996 4701 4267)
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CONSTEXPR_TOKEN_
#if _MSC_VER < 1900
#define _WEBSOCKETPP_NOEXCEPT_TOKEN_
#endif
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Winfinite-recursion"
#pragma clang diagnostic ignored "-Wtautological-constant-compare"
#pragma clang diagnostic ignored "-Wtautological-unsigned-enum-zero-compare"
#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#if defined(_WIN32)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4503)
#endif

// Workaround data-race on websocketpp's _htonll function, see
// https://github.com/Microsoft/cpprestsdk/pull/1082
auto avoidDataRaceOnHtonll = websocketpp::lib::net::_htonll(0);

// This is a hack to avoid memory leak reports from the debug MSVC CRT for all
// programs using the library: ASIO calls SSL_library_init() which calls
// SSL_COMP_get_compression_methods(), which allocates some heap memory and the
// only way to free it later is to call SSL_COMP_free_compression_methods(),
// but this function is unaccessible from the application code as OpenSSL is
// statically linked into the C++ REST SDK DLL. So, just to be nice, call it
// here ourselves -- even if the real problem is in ASIO (up to v1.60.0).
#if defined(_WIN32) && !defined(NDEBUG) && !defined(CPPREST_NO_SSL_LEAK_SUPPRESS)

#include <openssl/ssl.h>
static struct ASIO_SSL_memory_leak_suppress
{
    ~ASIO_SSL_memory_leak_suppress()
    {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
        ::SSL_COMP_free_compression_methods();
#endif
    }
} ASIO_SSL_memory_leak_suppressor;

#endif /* _WIN32 && !NDEBUG */

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

namespace web
{
namespace websockets
{
namespace client
{
namespace details
{
// Utility function to build up error string based on error code and location.
static std::string build_error_msg(const std::error_code& ec, const std::string& location)
{
    std::string result = location;
    result += ": ";
    result += std::to_string(ec.value());
    result += ": ";
    result += ec.message();
    return result;
}

static utility::string_t g_subProtocolHeader(_XPLATSTR("Sec-WebSocket-Protocol"));

class wspp_callback_client : public websocket_client_callback_impl,
                             public std::enable_shared_from_this<wspp_callback_client>
{
private:
    enum State
    {
        CREATED,
        CONNECTING,
        CONNECTED,
        CLOSING,
        CLOSED,
        DESTROYED
    };

public:
    wspp_callback_client(websocket_client_config config)
        : websocket_client_callback_impl(std::move(config))
        , m_state(CREATED)
#ifdef CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
        , m_openssl_failed(false)
#endif
    {
    }

    ~wspp_callback_client() CPPREST_NOEXCEPT
    {
        _ASSERTE(m_state < DESTROYED);
        State localState;
        {
            std::lock_guard<std::mutex> lock(m_wspp_client_lock);
            localState = m_state;
        } // Unlock the mutex so connect/close can use it.

        // Now, what states could we be in?
        switch (localState)
        {
            case DESTROYED:
                // This should be impossible
                std::abort();
            case CREATED: break;
            case CLOSED:
            case CONNECTING:
            case CONNECTED:
            case CLOSING:
                try
                {
                    // This will do nothing in the already-connected case
                    pplx::task<void>(m_connect_tce).get();
                }
                catch (...)
                {
                }
                try
                {
                    // This will do nothing in the already-closing case
                    close().wait();
                }
                catch (...)
                {
                }
                break;
        }

        // At this point, there should be no more references to me.
        m_state = DESTROYED;
    }

    pplx::task<void> connect()
    {
        if (m_uri.scheme() == U("wss"))
        {
            m_client = std::unique_ptr<websocketpp_client_base>(new websocketpp_tls_client());

            // Options specific to TLS client.
            auto& client = m_client->client<websocketpp::config::asio_tls_client>();
            client.set_tls_init_handler([this](websocketpp::connection_hdl) {
                auto sslContext = websocketpp::lib::shared_ptr<boost::asio::ssl::context>(
                    new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
                sslContext->set_default_verify_paths();
                sslContext->set_options(boost::asio::ssl::context::default_workarounds);
                if (m_config.get_ssl_context_callback())
                {
                    m_config.get_ssl_context_callback()(*sslContext);
                }
                if (m_config.validate_certificates())
                {
                    sslContext->set_verify_mode(boost::asio::ssl::context::verify_peer);
                }
                else
                {
                    sslContext->set_verify_mode(boost::asio::ssl::context::verify_none);
                }

#ifdef CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
                m_openssl_failed = false;
#endif
                sslContext->set_verify_callback([this](bool preverified, boost::asio::ssl::verify_context& verifyCtx) {
#ifdef CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
                    // Attempt to use platform certificate validation when it is available:
                    // If OpenSSL fails we will doing verification at the end using the whole certificate chain,
                    // so wait until the 'leaf' cert. For now return true so OpenSSL continues down the certificate
                    // chain.
                    if (!preverified)
                    {
                        m_openssl_failed = true;
                    }
                    if (m_openssl_failed)
                    {
                        return http::client::details::verify_cert_chain_platform_specific(
                            verifyCtx, utility::conversions::to_utf8string(m_uri.host()));
                    }
#endif
                    boost::asio::ssl::rfc2818_verification rfc2818(utility::conversions::to_utf8string(m_uri.host()));
                    return rfc2818(preverified, verifyCtx);
                });

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
                // OpenSSL stores some per thread state that never will be cleaned up until
                // the dll is unloaded. If static linking, like we do, the state isn't cleaned up
                // at all and will be reported as leaks.
                // See http://www.openssl.org/support/faq.html#PROG13
                // This is necessary here because it is called on the user's thread calling connect(...)
                // eventually through websocketpp::client::get_connection(...)
                ERR_remove_thread_state(nullptr);
#endif

                return sslContext;
            });

            // Options specific to underlying socket.
            client.set_socket_init_handler([this](websocketpp::connection_hdl,
                                                  boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& ssl_stream) {
                // Support for SNI.
                if (m_config.is_sni_enabled())
                {
                    // If user specified server name is empty default to use URI host name.
                    if (!m_config.server_name().empty())
                    {
                        // OpenSSL runs the string parameter through a macro casting away const with a C style cast.
                        // Do a C++ cast ourselves to avoid warnings.
                        SSL_set_tlsext_host_name(ssl_stream.native_handle(),
                                                 const_cast<char*>(m_config.server_name().c_str()));
                    }
                    else
                    {
                        const auto& server_name = utility::conversions::to_utf8string(m_uri.host());
                        SSL_set_tlsext_host_name(ssl_stream.native_handle(), const_cast<char*>(server_name.c_str()));
                    }
                }
            });

            return connect_impl<websocketpp::config::asio_tls_client>();
        }
        else
        {
            m_client = std::unique_ptr<websocketpp_client_base>(new websocketpp_client());
            return connect_impl<websocketpp::config::asio_client>();
        }
    }

    template<typename WebsocketConfigType>
    pplx::task<void> connect_impl()
    {
        auto& client = m_client->client<WebsocketConfigType>();

        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::alevel::all);
        client.init_asio();
        client.start_perpetual();

        _ASSERTE(m_state == CREATED);
        client.set_open_handler([this](websocketpp::connection_hdl) {
            _ASSERTE(m_state == CONNECTING);
            m_state = CONNECTED;
            m_connect_tce.set();
        });

        client.set_fail_handler([this](websocketpp::connection_hdl con_hdl) {
            _ASSERTE(m_state == CONNECTING);
            this->shutdown_wspp_impl<WebsocketConfigType>(con_hdl, true);
        });

        client.set_message_handler(
            [this](websocketpp::connection_hdl, const websocketpp::config::asio_client::message_type::ptr& msg) {
                if (m_external_message_handler)
                {
                    _ASSERTE(m_state >= CONNECTED && m_state < CLOSED);
                    websocket_incoming_message incoming_msg;

                    switch (msg->get_opcode())
                    {
                        case websocketpp::frame::opcode::binary:
                            incoming_msg.m_msg_type = websocket_message_type::binary_message;
                            break;
                        case websocketpp::frame::opcode::text:
                            incoming_msg.m_msg_type = websocket_message_type::text_message;
                            break;
                        default:
                            // Unknown message type. Since both websocketpp and our code use the RFC codes, we'll just
                            // pass it on to the user.
                            incoming_msg.m_msg_type = static_cast<websocket_message_type>(msg->get_opcode());
                            break;
                    }

                    // 'move' the payload into a container buffer to avoid any copies.
                    auto& payload = msg->get_raw_payload();
                    incoming_msg.m_body = concurrency::streams::container_buffer<std::string>(std::move(payload));

                    m_external_message_handler(incoming_msg);
                }
            });

        client.set_ping_handler([this](websocketpp::connection_hdl, const std::string& msg) {
            if (m_external_message_handler)
            {
                _ASSERTE(m_state >= CONNECTED && m_state < CLOSED);
                websocket_incoming_message incoming_msg;

                incoming_msg.m_msg_type = websocket_message_type::ping;
                incoming_msg.m_body = concurrency::streams::container_buffer<std::string>(msg);

                m_external_message_handler(incoming_msg);
            }
            return true;
        });

        client.set_pong_handler([this](websocketpp::connection_hdl, const std::string& msg) {
            if (m_external_message_handler)
            {
                _ASSERTE(m_state >= CONNECTED && m_state < CLOSED);
                websocket_incoming_message incoming_msg;

                incoming_msg.m_msg_type = websocket_message_type::pong;
                incoming_msg.m_body = concurrency::streams::container_buffer<std::string>(msg);

                m_external_message_handler(incoming_msg);
            }
        });

        client.set_close_handler([this](websocketpp::connection_hdl con_hdl) {
            _ASSERTE(m_state != CLOSED);
            this->shutdown_wspp_impl<WebsocketConfigType>(con_hdl, false);
        });

        // Set User Agent specified by the user. This needs to happen before any connection is created
        const auto& headers = m_config.headers();

        auto user_agent_it = headers.find(web::http::header_names::user_agent);
        if (user_agent_it != headers.end())
        {
            client.set_user_agent(utility::conversions::to_utf8string(user_agent_it->second));
        }

        // Get the connection handle to save for later, have to create temporary
        // because type erasure occurs with connection_hdl.
        websocketpp::lib::error_code ec;
        auto con = client.get_connection(utility::conversions::to_utf8string(m_uri.to_string()), ec);
        m_con = con;
        if (ec.value() != 0)
        {
            return pplx::task_from_exception<void>(websocket_exception(ec, build_error_msg(ec, "get_connection")));
        }

        // Add any request headers specified by the user.
        for (const auto& header : headers)
        {
            if (!utility::details::str_iequal(header.first, g_subProtocolHeader))
            {
                con->append_header(utility::conversions::to_utf8string(header.first),
                                   utility::conversions::to_utf8string(header.second));
            }
        }

        // Add any specified subprotocols.
        if (headers.has(g_subProtocolHeader))
        {
            const std::vector<utility::string_t> protocols = m_config.subprotocols();
            for (const auto& value : protocols)
            {
                con->add_subprotocol(utility::conversions::to_utf8string(value), ec);
                if (ec.value())
                {
                    return pplx::task_from_exception<void>(
                        websocket_exception(ec, build_error_msg(ec, "add_subprotocol")));
                }
            }
        }

        // Setup proxy options.
        const auto& proxy = m_config.proxy();
        if (proxy.is_specified())
        {
            con->set_proxy(utility::conversions::to_utf8string(proxy.address().to_string()), ec);
            if (ec)
            {
                return pplx::task_from_exception<void>(websocket_exception(ec, build_error_msg(ec, "set_proxy")));
            }

            const auto& cred = proxy.credentials();
            if (cred.is_set())
            {
                con->set_proxy_basic_auth(utility::conversions::to_utf8string(cred.username()),
                                          utility::conversions::to_utf8string(*cred._internal_decrypt()),
                                          ec);
                if (ec)
                {
                    return pplx::task_from_exception<void>(
                        websocket_exception(ec, build_error_msg(ec, "set_proxy_basic_auth")));
                }
            }
        }

        m_state = CONNECTING;
        client.connect(con);
        {
            std::lock_guard<std::mutex> lock(m_wspp_client_lock);
            m_thread = std::thread([&client]() {
#if defined(__ANDROID__)
                crossplat::get_jvm_env();
#endif
                client.run();
#if defined(__ANDROID__)
                crossplat::JVM.load()->DetachCurrentThread();
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)
                // OpenSSL stores some per thread state that never will be cleaned up until
                // the dll is unloaded. If static linking, like we do, the state isn't cleaned up
                // at all and will be reported as leaks.
                // See http://www.openssl.org/support/faq.html#PROG13
                ERR_remove_thread_state(nullptr);
#endif
            });
        } // unlock
        return pplx::create_task(m_connect_tce);
    }

    pplx::task<void> send(websocket_outgoing_message& msg)
    {
        if (!m_connect_tce._IsTriggered())
        {
            return pplx::task_from_exception<void>(websocket_exception("Client not connected."));
        }

        switch (msg.m_msg_type)
        {
            case websocket_message_type::text_message:
            case websocket_message_type::binary_message:
            case websocket_message_type::ping:
            case websocket_message_type::pong: break;
            default: return pplx::task_from_exception<void>(websocket_exception("Message Type not supported."));
        }

        const auto length = msg.m_length;
        if (length == 0 && msg.m_msg_type != websocket_message_type::ping &&
            msg.m_msg_type != websocket_message_type::pong)
        {
            return pplx::task_from_exception<void>(websocket_exception("Cannot send empty message."));
        }
        if (length >= UINT_MAX && length != SIZE_MAX)
        {
            return pplx::task_from_exception<void>(
                websocket_exception("Message size too large. Ensure message length is less than UINT_MAX."));
        }

        auto msg_pending = m_out_queue.push(msg);

        // No sends in progress
        if (msg_pending == outgoing_msg_queue::state::was_empty)
        {
            // Start sending the message
            send_msg(msg);
        }

        return pplx::create_task(msg.body_sent());
    }

    void send_msg(websocket_outgoing_message& msg)
    {
        auto this_client = this->shared_from_this();
        auto& is_buf = msg.m_body;
        auto length = msg.m_length;

        if (length == SIZE_MAX)
        {
            // This indicates we should determine the length automatically.
            if (is_buf.has_size())
            {
                // The user's stream knows how large it is -- there's no need to buffer.
                auto buf_sz = is_buf.size();
                if (buf_sz >= SIZE_MAX)
                {
                    msg.signal_body_sent(
                        std::make_exception_ptr(websocket_exception("Cannot send messages larger than SIZE_MAX.")));
                    return;
                }
                length = static_cast<size_t>(buf_sz);
                // We have determined the length and can proceed normally.
            }
            else
            {
                // The stream needs to be buffered.
                auto is_buf_istream = is_buf.create_istream();
                msg.m_body = concurrency::streams::container_buffer<std::vector<uint8_t>>();
                is_buf_istream.read_to_end(msg.m_body).then([this_client, msg](pplx::task<size_t> t) mutable {
                    try
                    {
                        msg.m_length = t.get();
                        this_client->send_msg(msg);
                    }
                    catch (...)
                    {
                        msg.signal_body_sent(std::current_exception());
                    }
                });
                // We have postponed the call to send_msg() until after the data is buffered.
                return;
            }
        }

        // First try to acquire the data (Get a pointer to the next already allocated contiguous block of data)
        // If acquire succeeds, send the data over the socket connection, there is no copy of data from stream to
        // temporary buffer. If acquire fails, copy the data to a temporary buffer managed by sp_allocated and send it
        // over the socket connection.
        std::shared_ptr<uint8_t> sp_allocated;
        size_t acquired_size = 0;
        uint8_t* ptr;
        auto read_task = pplx::task_from_result();
        bool acquired = is_buf.acquire(ptr, acquired_size);

        if (!acquired ||
            acquired_size < length) // Stream does not support acquire or failed to acquire specified number of bytes
        {
            // If acquire did not return the required number of bytes, do not rely on its return value.
            if (acquired_size < length)
            {
                acquired = false;
                is_buf.release(ptr, 0);
            }

            // Allocate buffer to hold the data to be read from the stream.
            sp_allocated.reset(new uint8_t[length], [=](uint8_t* p) { delete[] p; });

            read_task = is_buf.getn(sp_allocated.get(), length).then([length](size_t bytes_read) {
                if (bytes_read != length)
                {
                    throw websocket_exception("Failed to read required length of data from the stream.");
                }
            });
        }
        else
        {
            // Acquire succeeded, assign the acquired pointer to sp_allocated. Use an empty custom destructor
            // so that the data is not released when sp_allocated goes out of scope. The streambuf will manage its
            // memory.
            sp_allocated.reset(ptr, [](uint8_t*) {});
        }

        read_task
            .then([this_client, msg, sp_allocated, length]() {
                std::lock_guard<std::mutex> lock(this_client->m_wspp_client_lock);
                if (this_client->m_state > CONNECTED)
                {
                    // The client has already been closed.
                    throw websocket_exception("Websocket connection is closed.");
                }

                websocketpp::lib::error_code ec;
                if (this_client->m_client->is_tls_client())
                {
                    this_client->send_msg_impl<websocketpp::config::asio_tls_client>(
                        this_client, msg, sp_allocated, length, ec);
                }
                else
                {
                    this_client->send_msg_impl<websocketpp::config::asio_client>(
                        this_client, msg, sp_allocated, length, ec);
                }
                return ec;
            })
            .then([this_client, msg, is_buf, acquired, sp_allocated, length](
                      pplx::task<websocketpp::lib::error_code> previousTask) mutable {
                std::exception_ptr eptr;
                try
                {
                    // Catch exceptions from previous tasks, if any and convert it to websocket exception.
                    const auto& ec = previousTask.get();
                    if (ec.value() != 0)
                    {
                        eptr = std::make_exception_ptr(websocket_exception(ec, build_error_msg(ec, "sending message")));
                    }
                }
                catch (...)
                {
                    eptr = std::current_exception();
                }

                if (acquired)
                {
                    is_buf.release(sp_allocated.get(), length);
                }

                // Set the send_task_completion_event after calling release.
                if (eptr)
                {
                    msg.signal_body_sent(eptr);
                }
                else
                {
                    msg.signal_body_sent();
                }

                websocket_outgoing_message next_msg;
                bool msg_pending = this_client->m_out_queue.pop_and_peek(next_msg);

                if (msg_pending)
                {
                    this_client->send_msg(next_msg);
                }
            });
    }

    pplx::task<void> close()
    {
        return close(static_cast<websocket_close_status>(websocketpp::close::status::normal), U("Normal"));
    }

    pplx::task<void> close(websocket_close_status status, const utility::string_t& reason)
    {
        websocketpp::lib::error_code ec;
        {
            std::lock_guard<std::mutex> lock(m_wspp_client_lock);
            if (m_state == CONNECTED)
            {
                m_state = CLOSING;
                if (m_client->is_tls_client())
                {
                    close_impl<websocketpp::config::asio_tls_client>(status, reason, ec);
                }
                else
                {
                    close_impl<websocketpp::config::asio_client>(status, reason, ec);
                }
            }
        }
        return pplx::task<void>(m_close_tce);
    }

private:
    template<typename WebsocketConfigType>
    void shutdown_wspp_impl(const websocketpp::connection_hdl& con_hdl, bool connecting)
    {
        // Only need to hold the lock when setting the state to closed.
        {
            std::lock_guard<std::mutex> lock(m_wspp_client_lock);
            m_state = CLOSED;
        }

        auto& client = m_client->client<WebsocketConfigType>();
        const auto& connection = client.get_con_from_hdl(con_hdl);
        const auto& closeCode = connection->get_local_close_code();
        const auto& reason = connection->get_local_close_reason();
        const auto& ec = connection->get_ec();
        client.stop_perpetual();

        // Can't join thread directly since it is the current thread.
        pplx::create_task([] {}).then([this, connecting, ec, closeCode, reason]() mutable {
            {
                std::lock_guard<std::mutex> lock(m_wspp_client_lock);
                if (m_thread.joinable())
                {
                    m_thread.join();
                }
            } // unlock

            if (connecting)
            {
                websocket_exception exc(ec, build_error_msg(ec, "set_fail_handler"));
                m_connect_tce.set_exception(exc);
            }
            if (m_external_close_handler)
            {
                m_external_close_handler(
                    static_cast<websocket_close_status>(closeCode), utility::conversions::to_string_t(reason), ec);
            }
            // Making a local copy of the TCE prevents it from being destroyed along with "this"
            auto tceref = m_close_tce;
            tceref.set();
        });
    }

    template<typename WebsocketClientType>
    static void send_msg_impl(const std::shared_ptr<wspp_callback_client>& this_client,
                              const websocket_outgoing_message& msg,
                              const std::shared_ptr<uint8_t>& sp_allocated,
                              size_t length,
                              websocketpp::lib::error_code& ec)
    {
        auto& client = this_client->m_client->client<WebsocketClientType>();
        switch (msg.m_msg_type)
        {
            case websocket_message_type::text_message:
                client.send(this_client->m_con, sp_allocated.get(), length, websocketpp::frame::opcode::text, ec);
                break;
            case websocket_message_type::binary_message:
                client.send(this_client->m_con, sp_allocated.get(), length, websocketpp::frame::opcode::binary, ec);
                break;
            case websocket_message_type::ping:
            {
                std::string s(reinterpret_cast<char*>(sp_allocated.get()), length);
                client.ping(this_client->m_con, s, ec);
                break;
            }
            case websocket_message_type::pong:
            {
                std::string s(reinterpret_cast<char*>(sp_allocated.get()), length);
                client.pong(this_client->m_con, s, ec);
                break;
            }
            default:
                // This case should have already been filtered above.
                std::abort();
        }
    }

    template<typename WebsocketConfig>
    void close_impl(websocket_close_status status, const utility::string_t& reason, websocketpp::lib::error_code& ec)
    {
        auto& client = m_client->client<WebsocketConfig>();
        client.close(m_con,
                     static_cast<websocketpp::close::status::value>(status),
                     utility::conversions::to_utf8string(reason),
                     ec);
    }

    void set_message_handler(const std::function<void(const websocket_incoming_message&)>& handler)
    {
        m_external_message_handler = handler;
    }

    void set_close_handler(
        const std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)>& handler)
    {
        m_external_close_handler = handler;
    }

    std::thread m_thread;

    // Perform type erasure to set the websocketpp client in use at runtime
    // after construction based on the URI.
    struct websocketpp_client_base
    {
        virtual ~websocketpp_client_base() CPPREST_NOEXCEPT {}
        template<typename WebsocketConfig>
        websocketpp::client<WebsocketConfig>& client()
        {
            if (is_tls_client())
            {
                return reinterpret_cast<websocketpp::client<WebsocketConfig>&>(tls_client());
            }
            else
            {
                return reinterpret_cast<websocketpp::client<WebsocketConfig>&>(non_tls_client());
            }
        }
        virtual websocketpp::client<websocketpp::config::asio_client>& non_tls_client() { throw std::bad_cast(); }
        virtual websocketpp::client<websocketpp::config::asio_tls_client>& tls_client() { throw std::bad_cast(); }
        virtual bool is_tls_client() const = 0;
    };
    struct websocketpp_client : websocketpp_client_base
    {
        ~websocketpp_client() CPPREST_NOEXCEPT {}
        websocketpp::client<websocketpp::config::asio_client>& non_tls_client() override { return m_client; }
        bool is_tls_client() const override { return false; }
        websocketpp::client<websocketpp::config::asio_client> m_client;
    };
    struct websocketpp_tls_client : websocketpp_client_base
    {
        ~websocketpp_tls_client() CPPREST_NOEXCEPT {}
        websocketpp::client<websocketpp::config::asio_tls_client>& tls_client() override { return m_client; }
        bool is_tls_client() const override { return true; }
        websocketpp::client<websocketpp::config::asio_tls_client> m_client;
    };

    pplx::task_completion_event<void> m_connect_tce;
    pplx::task_completion_event<void> m_close_tce;

    // Used to safe guard the wspp client.
    std::mutex m_wspp_client_lock;
    State m_state;
    std::unique_ptr<websocketpp_client_base> m_client;
    websocketpp::connection_hdl m_con;

    // Queue to track pending sends
    outgoing_msg_queue m_out_queue;

    // External callback for handling received and close event
    std::function<void(websocket_incoming_message)> m_external_message_handler;
    std::function<void(websocket_close_status, const utility::string_t&, const std::error_code&)>
        m_external_close_handler;

    // Used to track if any of the OpenSSL server certificate verifications
    // failed. This can safely be tracked at the client level since connections
    // only happen once for each client.
#ifdef CPPREST_PLATFORM_ASIO_CERT_VERIFICATION_AVAILABLE
    bool m_openssl_failed;
#endif
};

websocket_client_task_impl::websocket_client_task_impl(websocket_client_config config)
    : m_client_closed(false), m_callback_client(std::make_shared<details::wspp_callback_client>(std::move(config)))
{
    set_handler();
}
} // namespace details

websocket_callback_client::websocket_callback_client()
    : m_client(std::make_shared<details::wspp_callback_client>(websocket_client_config()))
{
}

websocket_callback_client::websocket_callback_client(websocket_client_config config)
    : m_client(std::make_shared<details::wspp_callback_client>(std::move(config)))
{
}

} // namespace client
} // namespace websockets
} // namespace web

#endif
