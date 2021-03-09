/***
* Copyright (C) Microsoft. All rights reserved.
* Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* HTTP Library: HTTP listener (server-side) APIs

* This file contains a cross platform implementation based on Boost.ASIO.
*
* For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/
#include "stdafx.h"

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/read_until.hpp>
#include <set>
#include <sstream>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "../common/internal_http_helpers.h"
#include "cpprest/asyncrt_utils.h"
#include "http_server_impl.h"
#include "pplx/threadpool.h"

#ifdef __ANDROID__
using utility::conversions::details::to_string;
#else
using std::to_string;
#endif

using namespace boost::asio;
using namespace boost::asio::ip;

#define CRLF std::string("\r\n")
#define CRLFCRLF std::string("\r\n\r\n")

namespace listener = web::http::experimental::listener;
namespace chunked_encoding = web::http::details::chunked_encoding;

using web::uri;
using web::http::header_names;
using web::http::http_exception;
using web::http::http_request;
using web::http::http_response;
using web::http::methods;
using web::http::status_codes;
using web::http::experimental::listener::http_listener_config;
using web::http::experimental::listener::details::http_listener_impl;

using utility::details::make_unique;

namespace
{
class hostport_listener;
class http_linux_server;
class asio_server_connection;
} // namespace

namespace
{
struct iequal_to
{
    bool operator()(const std::string& left, const std::string& right) const
    {
        return boost::ilexicographical_compare(left, right);
    }
};

class http_linux_server : public web::http::experimental::details::http_server
{
private:
    friend class asio_server_connection;

    pplx::extensibility::reader_writer_lock_t m_listeners_lock;
    std::map<std::string, std::unique_ptr<hostport_listener>, iequal_to> m_listeners;
    std::unordered_map<http_listener_impl*, std::unique_ptr<pplx::extensibility::reader_writer_lock_t>>
        m_registered_listeners;
    bool m_started;

public:
    http_linux_server() : m_listeners_lock(), m_listeners(), m_started(false) {}

    ~http_linux_server() { stop(); }

    virtual pplx::task<void> start();
    virtual pplx::task<void> stop();

    virtual pplx::task<void> register_listener(http_listener_impl* listener);
    virtual pplx::task<void> unregister_listener(http_listener_impl* listener);

    pplx::task<void> respond(http_response response);
};

struct linux_request_context : web::http::details::_http_server_context
{
    linux_request_context() {}

    pplx::task_completion_event<void> m_response_completed;

private:
    linux_request_context(const linux_request_context&) = delete;
    linux_request_context& operator=(const linux_request_context&) = delete;
};

class hostport_listener
{
private:
    int m_backlog;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor;
    std::map<std::string, http_listener_impl*> m_listeners;
    pplx::extensibility::reader_writer_lock_t m_listeners_lock;

    std::mutex m_connections_lock;
    pplx::extensibility::event_t m_all_connections_complete;
    std::set<asio_server_connection*> m_connections;

    http_linux_server* m_p_server;

    std::string m_host;
    std::string m_port;

    bool m_is_https;
    const std::function<void(boost::asio::ssl::context&)>& m_ssl_context_callback;

public:
    hostport_listener(http_linux_server* server,
                      const std::string& hostport,
                      bool is_https,
                      const http_listener_config& config)
        : m_backlog(config.backlog())
        , m_acceptor()
        , m_listeners()
        , m_listeners_lock()
        , m_connections_lock()
        , m_connections()
        , m_p_server(server)
        , m_is_https(is_https)
        , m_ssl_context_callback(config.get_ssl_context_callback())
    {
        m_all_connections_complete.set();

        std::istringstream hostport_in(hostport);
        hostport_in.imbue(std::locale::classic());

        std::getline(hostport_in, m_host, ':');
        std::getline(hostport_in, m_port);
    }

    ~hostport_listener() { stop(); }

    void start();
    void stop();

    void add_listener(const std::string& path, http_listener_impl* listener);
    void remove_listener(const std::string& path, http_listener_impl* listener);

    void internal_erase_connection(asio_server_connection*);

    http_listener_impl* find_listener(uri const& u)
    {
        auto path_segments = uri::split_path(uri::decode(u.path()));
        for (auto i = static_cast<long>(path_segments.size()); i >= 0; --i)
        {
            std::string path;
            for (size_t j = 0; j < static_cast<size_t>(i); ++j)
            {
                path += "/" + utility::conversions::to_utf8string(path_segments[j]);
            }
            path += "/";

            pplx::extensibility::scoped_read_lock_t lock(m_listeners_lock);
            auto it = m_listeners.find(path);
            if (it != m_listeners.end())
            {
                return it->second;
            }
        }
        return nullptr;
    }

private:
    void on_accept(std::unique_ptr<boost::asio::ip::tcp::socket> socket, const boost::system::error_code& ec);
};

} // namespace

namespace
{
/// This class replaces the regex "\r\n\r\n|[\x00-\x1F]|[\x80-\xFF]"
// It was added due to issues with regex on Android, however since
// regex was rather overkill for such a simple parse it makes sense
// to use it on all *nix platforms.
//
// This is used as part of the async_read_until call below; see the
// following for more details:
// http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/async_read_until/overload4.html
struct crlfcrlf_nonascii_searcher_t
{
    enum class State
    {
        none = 0,  // ".\r\n\r\n$"
        cr = 1,    // "\r.\n\r\n$"
                   // "  .\r\n\r\n$"
        crlf = 2,  // "\r\n.\r\n$"
                   // "    .\r\n\r\n$"
        crlfcr = 3 // "\r\n\r.\n$"
                   // "    \r.\n\r\n$"
                   // "      .\r\n\r\n$"
    };

    // This function implements the searcher which "consumes" a certain amount of the input
    // and returns whether or not there was a match (see above).

    // From the Boost documentation:
    // "The first member of the return value is an iterator marking one-past-the-end of the
    //  bytes that have been consumed by the match function. This iterator is used to
    //  calculate the begin parameter for any subsequent invocation of the match condition.
    //  The second member of the return value is true if a match has been found, false
    //  otherwise."
    template<typename Iter>
    std::pair<Iter, bool> operator()(const Iter begin, const Iter end) const
    {
        // In the case that we end inside a partially parsed match (like abcd\r\n\r),
        // we need to signal the matcher to give us the partial match back again (\r\n\r).
        // We use the excluded variable to keep track of this sequence point (abcd.\r\n\r
        // in the previous example).
        Iter excluded = begin;
        Iter cur = begin;
        State state = State::none;
        while (cur != end)
        {
            const auto c = static_cast<unsigned char>(*cur);
            if (c == '\r')
            {
                if (state == State::crlf)
                {
                    state = State::crlfcr;
                }
                else
                {
                    // In the case of State::cr or State::crlfcr, setting the state here
                    // "skips" a none state and therefore fails to move up the excluded
                    // counter.
                    excluded = cur;
                    state = State::cr;
                }
            }
            else if (c == '\n')
            {
                if (state == State::cr)
                {
                    state = State::crlf;
                }
                else if (state == State::crlfcr)
                {
                    ++cur;
                    return std::make_pair(cur, true);
                }
                else
                {
                    state = State::none;
                }
            }
            else if (c <= 0x1Fu || c >= 0x80)
            {
                ++cur;
                return std::make_pair(cur, true);
            }
            else
            {
                state = State::none;
            }
            ++cur;
            if (state == State::none) excluded = cur;
        }
        return std::make_pair(excluded, false);
    }
} crlfcrlf_nonascii_searcher;

// These structures serve as proof witnesses
struct will_erase_from_parent_t
{
};
struct will_deref_and_erase_t
{
};
struct will_deref_t
{
};

class asio_server_connection
{
private:
    typedef void (asio_server_connection::*ResponseFuncPtr)(const http_response& response,
                                                            const boost::system::error_code& ec);

    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket;
    boost::asio::streambuf m_request_buf;
    boost::asio::streambuf m_response_buf;
    http_linux_server* m_p_server;
    hostport_listener* m_p_parent;
    mutable std::mutex m_request_mtx;
    http_request m_request_tmp;

    void set_request(http_request req)
    {
        std::lock_guard<std::mutex> lck(m_request_mtx);
        m_request_tmp = std::move(req);
    }

    http_request get_request() const
    {
        std::lock_guard<std::mutex> lck(m_request_mtx);
        return m_request_tmp;
    }

    size_t m_read, m_write;
    size_t m_read_size, m_write_size;
    bool m_close;
    bool m_chunked;
    std::atomic<int> m_refs; // track how many threads are still referring to this

    using ssl_stream = boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>;

    std::unique_ptr<boost::asio::ssl::context> m_ssl_context;
    std::unique_ptr<ssl_stream> m_ssl_stream;

    asio_server_connection(std::unique_ptr<boost::asio::ip::tcp::socket> socket,
                           http_linux_server* server,
                           hostport_listener* parent)
        : m_socket(std::move(socket))
        , m_request_buf()
        , m_response_buf()
        , m_p_server(server)
        , m_p_parent(parent)
        , m_close(false)
        , m_chunked(false)
        , m_refs(1)
    {
    }

    struct Dereferencer
    {
        void operator()(asio_server_connection* conn) const { conn->deref(); }
    };

public:
    using refcount_ptr = std::unique_ptr<asio_server_connection, Dereferencer>;

    static refcount_ptr create(std::unique_ptr<boost::asio::ip::tcp::socket> socket,
                               http_linux_server* server,
                               hostport_listener* parent)
    {
        return refcount_ptr(new asio_server_connection(std::move(socket), server, parent));
    }

    refcount_ptr get_reference()
    {
        ++m_refs;
        return refcount_ptr(this);
    }

    will_erase_from_parent_t start_connection(
        bool is_https, const std::function<void(boost::asio::ssl::context&)>& ssl_context_callback)
    {
        auto unique_reference = this->get_reference();

        if (is_https)
        {
            m_ssl_context = make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
            if (ssl_context_callback)
            {
                ssl_context_callback(*m_ssl_context);
            }
            m_ssl_stream = make_unique<ssl_stream>(*m_socket, *m_ssl_context);

            m_ssl_stream->async_handshake(
                boost::asio::ssl::stream_base::server,
                [this](const boost::system::error_code&) { (will_deref_and_erase_t) this->start_request_response(); });
            unique_reference.release();
            return will_erase_from_parent_t {};
        }
        else
        {
            (will_deref_and_erase_t) start_request_response();
            unique_reference.release();
            return will_erase_from_parent_t {};
        }
    }

    void close();

    asio_server_connection(const asio_server_connection&) = delete;
    asio_server_connection& operator=(const asio_server_connection&) = delete;

private:
    ~asio_server_connection() = default;

    will_deref_and_erase_t start_request_response();
    will_deref_and_erase_t handle_http_line(const boost::system::error_code& ec);
    will_deref_and_erase_t handle_headers();
    will_deref_t handle_body(const boost::system::error_code& ec);
    will_deref_t handle_chunked_header(const boost::system::error_code& ec);
    will_deref_t handle_chunked_body(const boost::system::error_code& ec, int toWrite);
    will_deref_and_erase_t dispatch_request_to_listener();
    will_erase_from_parent_t do_response()
    {
        auto unique_reference = this->get_reference();
        get_request().get_response().then([=](pplx::task<http_response> r_task) {
            http_response response;
            try
            {
                response = r_task.get();
            }
            catch (...)
            {
                response = http_response(status_codes::InternalError);
            }

            serialize_headers(response);

            // before sending response, the full incoming message need to be processed.
            return get_request().content_ready().then([=](pplx::task<http_request>) {
                (will_deref_and_erase_t) this->async_write(&asio_server_connection::handle_headers_written, response);
            });
        });
        unique_reference.release();
        return will_erase_from_parent_t {};
    }
    will_erase_from_parent_t do_bad_response()
    {
        auto unique_reference = this->get_reference();
        get_request().get_response().then([=](pplx::task<http_response> r_task) {
            http_response response;
            try
            {
                response = r_task.get();
            }
            catch (...)
            {
                response = http_response(status_codes::InternalError);
            }

            // before sending response, the full incoming message need to be processed.
            serialize_headers(response);

            (will_deref_and_erase_t) async_write(&asio_server_connection::handle_headers_written, response);
        });
        unique_reference.release();
        return will_erase_from_parent_t {};
    }

    will_deref_t async_handle_chunked_header();
    template<typename ReadHandler>
    void async_read_until_buffersize(size_t size, const ReadHandler& handler);
    void serialize_headers(http_response response);
    will_deref_and_erase_t cancel_sending_response_with_error(const http_response& response, const std::exception_ptr&);
    will_deref_and_erase_t handle_headers_written(const http_response& response, const boost::system::error_code& ec);
    will_deref_and_erase_t handle_write_large_response(const http_response& response,
                                                       const boost::system::error_code& ec);
    will_deref_and_erase_t handle_write_chunked_response(const http_response& response,
                                                         const boost::system::error_code& ec);
    will_deref_and_erase_t handle_response_written(const http_response& response, const boost::system::error_code& ec);
    will_deref_and_erase_t finish_request_response();

    using WriteFunc = decltype(&asio_server_connection::handle_headers_written);
    will_deref_and_erase_t async_write(WriteFunc response_func_ptr, const http_response& response);

    inline will_deref_t deref()
    {
        if (--m_refs == 0) delete this;
        return will_deref_t {};
    }
};

} // namespace

namespace boost
{
namespace asio
{
template<>
struct is_match_condition<crlfcrlf_nonascii_searcher_t> : public boost::true_type
{
};
} // namespace asio
} // namespace boost

namespace
{
const size_t ChunkSize = 4 * 1024;

void hostport_listener::internal_erase_connection(asio_server_connection* conn)
{
    std::lock_guard<std::mutex> lock(m_connections_lock);
    m_connections.erase(conn);
    if (m_connections.empty())
    {
        m_all_connections_complete.set();
    }
}

void hostport_listener::start()
{
    // resolve the endpoint address
    auto& service = crossplat::threadpool::shared_instance().service();
    tcp::resolver resolver(service);
    // #446: boost resolver does not recognize "+" as a host wildchar
    tcp::resolver::query query =
        ("+" == m_host) ? tcp::resolver::query(m_port, boost::asio::ip::resolver_query_base::flags())
                        : tcp::resolver::query(m_host, m_port, boost::asio::ip::resolver_query_base::flags());

    tcp::endpoint endpoint = *resolver.resolve(query);

    m_acceptor.reset(new tcp::acceptor(service));
    m_acceptor->open(endpoint.protocol());
    m_acceptor->set_option(socket_base::reuse_address(true));
    m_acceptor->bind(endpoint);
    m_acceptor->listen(0 != m_backlog ? m_backlog : socket_base::max_connections);

    auto socket = new ip::tcp::socket(service);
    std::unique_ptr<ip::tcp::socket> usocket(socket);
    m_acceptor->async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        std::unique_ptr<ip::tcp::socket> usocket(socket);
        this->on_accept(std::move(usocket), ec);
    });
    usocket.release();
}

void asio_server_connection::close()
{
    m_close = true;
    auto sock = m_socket.get();
    if (sock != nullptr)
    {
        boost::system::error_code ec;
        sock->cancel(ec);
        sock->shutdown(tcp::socket::shutdown_both, ec);
        sock->close(ec);
    }
    get_request()._reply_if_not_already(status_codes::InternalError);
}

will_deref_and_erase_t asio_server_connection::start_request_response()
{
    m_read_size = 0;
    m_read = 0;
    m_request_buf.consume(m_request_buf.size()); // clear the buffer

    if (m_ssl_stream)
    {
        boost::asio::async_read_until(
            *m_ssl_stream, m_request_buf, CRLFCRLF, [this](const boost::system::error_code& ec, std::size_t) {
                (will_deref_and_erase_t) this->handle_http_line(ec);
            });
    }
    else
    {
        boost::asio::async_read_until(*m_socket,
                                      m_request_buf,
                                      crlfcrlf_nonascii_searcher,
                                      [this](const boost::system::error_code& ec, std::size_t) {
                                          (will_deref_and_erase_t) this->handle_http_line(ec);
                                      });
    }
    return will_deref_and_erase_t {};
}

void hostport_listener::on_accept(std::unique_ptr<ip::tcp::socket> socket, const boost::system::error_code& ec)
{
    // Listener closed
    if (ec == boost::asio::error::operation_aborted)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_connections_lock);

    // Handle successful accept
    if (!ec)
    {
        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code error_ignored;
        socket->set_option(option, error_ignored);

        auto conn = asio_server_connection::create(std::move(socket), m_p_server, this);

        m_connections.insert(conn.get());
        try
        {
            (will_erase_from_parent_t) conn->start_connection(m_is_https, m_ssl_context_callback);
            // at this point an asynchronous task has been launched which will call
            // m_connections.erase(conn.get()) eventually

            // the following cannot throw
            if (m_connections.size() == 1) m_all_connections_complete.reset();
        }
        catch (boost::system::system_error&)
        {
            // boost ssl apis throw boost::system::system_error.
            // Exception indicates something went wrong setting ssl context.
            // Drop connection and continue handling other connections.
            m_connections.erase(conn.get());
        }
    }

    if (m_acceptor)
    {
        // spin off another async accept
        auto newSocket = new ip::tcp::socket(crossplat::threadpool::shared_instance().service());
        std::unique_ptr<ip::tcp::socket> usocket(newSocket);
        m_acceptor->async_accept(*newSocket, [this, newSocket](const boost::system::error_code& ec) {
            std::unique_ptr<ip::tcp::socket> usocket(newSocket);
            this->on_accept(std::move(usocket), ec);
        });
        usocket.release();
    }
}

will_deref_and_erase_t asio_server_connection::handle_http_line(const boost::system::error_code& ec)
{
    auto thisRequest = http_request::_create_request(make_unique<linux_request_context>());
    set_request(thisRequest);
    if (ec)
    {
        // client closed connection
        if (ec == boost::asio::error::eof || // peer has performed an orderly shutdown
            ec ==
                boost::asio::error::operation_aborted ||  // this can be removed. ECONNABORTED happens only for accept()
            ec == boost::asio::error::connection_reset || // connection reset by peer
            ec == boost::asio::error::timed_out           // connection timed out
        )
        {
            return finish_request_response();
        }
        else
        {
            thisRequest._reply_if_not_already(status_codes::BadRequest);
            m_close = true;
            (will_erase_from_parent_t) do_bad_response();
            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }
    }
    else
    {
        // read http status line
        std::istream request_stream(&m_request_buf);
        request_stream.imbue(std::locale::classic());
        std::skipws(request_stream);

        web::http::method http_verb;
#ifndef _UTF16_STRINGS
        request_stream >> http_verb;
#else
        {
            std::string tmp;
            request_stream >> tmp;
            http_verb = utility::conversions::latin1_to_utf16(tmp);
        }
#endif

        if (boost::iequals(http_verb, methods::GET))
            http_verb = methods::GET;
        else if (boost::iequals(http_verb, methods::POST))
            http_verb = methods::POST;
        else if (boost::iequals(http_verb, methods::PUT))
            http_verb = methods::PUT;
        else if (boost::iequals(http_verb, methods::DEL))
            http_verb = methods::DEL;
        else if (boost::iequals(http_verb, methods::HEAD))
            http_verb = methods::HEAD;
        else if (boost::iequals(http_verb, methods::TRCE))
            http_verb = methods::TRCE;
        else if (boost::iequals(http_verb, methods::CONNECT))
            http_verb = methods::CONNECT;
        else if (boost::iequals(http_verb, methods::OPTIONS))
            http_verb = methods::OPTIONS;

        // Check to see if there is not allowed character on the input
        if (!web::http::details::validate_method(http_verb))
        {
            thisRequest.reply(status_codes::BadRequest);
            m_close = true;
            (will_erase_from_parent_t) do_bad_response();
            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }

        thisRequest.set_method(http_verb);

        std::string http_path_and_version;
        std::getline(request_stream, http_path_and_version);
        const size_t VersionPortionSize = sizeof(" HTTP/1.1\r") - 1;

        // Make sure path and version is long enough to contain the HTTP version
        if (http_path_and_version.size() < VersionPortionSize + 2)
        {
            thisRequest.reply(status_codes::BadRequest);
            m_close = true;
            (will_erase_from_parent_t) do_bad_response();
            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }

        // Get the path - remove the version portion and prefix space
        try
        {
            thisRequest.set_request_uri(utility::conversions::to_string_t(
                http_path_and_version.substr(1, http_path_and_version.size() - VersionPortionSize - 1)));
        }
        catch (const std::exception& e) // may be std::range_error indicating invalid Unicode, or web::uri_exception
        {
            thisRequest.reply(status_codes::BadRequest, e.what());
            m_close = true;
            (will_erase_from_parent_t) do_bad_response();
            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }

        // Get the version
        std::string http_version =
            http_path_and_version.substr(http_path_and_version.size() - VersionPortionSize + 1, VersionPortionSize - 2);

        auto requestImpl = thisRequest._get_impl().get();
        web::http::http_version parsed_version = web::http::http_version::from_string(http_version);
        requestImpl->_set_http_version(parsed_version);

        // if HTTP version is 1.0 then disable pipelining
        if (parsed_version == web::http::http_versions::HTTP_1_0)
        {
            m_close = true;
        }

        // Get the remote IP address
        boost::system::error_code socket_ec;
        auto endpoint = m_socket->remote_endpoint(socket_ec);
        if (!socket_ec)
        {
            requestImpl->_set_remote_address(utility::conversions::to_string_t(endpoint.address().to_string()));
        }

        return handle_headers();
    }
}

will_deref_and_erase_t asio_server_connection::handle_headers()
{
    std::istream request_stream(&m_request_buf);
    request_stream.imbue(std::locale::classic());
    std::string header;

    auto currentRequest = get_request();
    auto& headers = currentRequest.headers();

    while (std::getline(request_stream, header) && header != "\r")
    {
        auto colon = header.find(':');
        if (colon != std::string::npos && colon != 0)
        {
            auto name = utility::conversions::to_string_t(header.substr(0, colon));
            auto value = utility::conversions::to_string_t(
                header.substr(colon + 1, header.length() - (colon + 1))); // also exclude '\r'
            web::http::details::trim_whitespace(name);
            web::http::details::trim_whitespace(value);

            if (boost::iequals(name, header_names::content_length))
            {
                headers[header_names::content_length] = value;
            }
            else
            {
                headers.add(name, value);
            }
        }
        else
        {
            currentRequest.reply(status_codes::BadRequest);
            m_close = true;
            (will_erase_from_parent_t) do_bad_response();
            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }
    }

    m_chunked = false;
    utility::string_t name;
    // check if the client has requested we close the connection
    if (currentRequest.headers().match(header_names::connection, name))
    {
        m_close = boost::iequals(name, U("close"));
    }

    if (currentRequest.headers().match(header_names::transfer_encoding, name))
    {
        m_chunked = boost::ifind_first(name, U("chunked"));
    }

    currentRequest._get_impl()->_prepare_to_receive_data();
    if (m_chunked)
    {
        ++m_refs;
        (will_deref_t) async_handle_chunked_header();
        return dispatch_request_to_listener();
    }

    if (!currentRequest.headers().match(header_names::content_length, m_read_size))
    {
        m_read_size = 0;
    }

    if (m_read_size == 0)
    {
        currentRequest._get_impl()->_complete(0);
    }
    else // need to read the sent data
    {
        m_read = 0;
        ++m_refs;
        async_read_until_buffersize(
            (std::min)(ChunkSize, m_read_size),
            [this](const boost::system::error_code& ec, size_t) { (will_deref_t) this->handle_body(ec); });
    }

    return dispatch_request_to_listener();
}

will_deref_t asio_server_connection::handle_chunked_header(const boost::system::error_code& ec)
{
    auto requestImpl = get_request()._get_impl();
    if (ec)
    {
        requestImpl->_complete(0, std::make_exception_ptr(http_exception(ec.value())));
        return deref();
    }
    else
    {
        std::istream is(&m_request_buf);
        is.imbue(std::locale::classic());
        int len;
        is >> std::hex >> len;
        m_request_buf.consume(CRLF.size());
        m_read += len;
        if (len == 0)
        {
            requestImpl->_complete(m_read);
            return deref();
        }
        else
        {
            async_read_until_buffersize(len + 2, [this, len](const boost::system::error_code& ec, size_t) {
                (will_deref_t) this->handle_chunked_body(ec, len);
            });
            return will_deref_t {};
        }
    }
}

will_deref_t asio_server_connection::handle_chunked_body(const boost::system::error_code& ec, int toWrite)
{
    auto requestImpl = get_request()._get_impl();
    if (ec)
    {
        requestImpl->_complete(0, std::make_exception_ptr(http_exception(ec.value())));
        return deref();
    }
    else
    {
        auto writebuf = requestImpl->outstream().streambuf();
        writebuf.putn_nocopy(buffer_cast<const uint8_t*>(m_request_buf.data()), toWrite)
            .then([=](pplx::task<size_t> writeChunkTask) -> will_deref_t {
                try
                {
                    writeChunkTask.get();
                }
                catch (...)
                {
                    requestImpl->_complete(0, std::current_exception());
                    return deref();
                }

                m_request_buf.consume(2 + toWrite);
                return async_handle_chunked_header();
            });
        return will_deref_t {};
    }
}

will_deref_t asio_server_connection::handle_body(const boost::system::error_code& ec)
{
    auto requestImpl = get_request()._get_impl();
    // read body
    if (ec)
    {
        requestImpl->_complete(0, std::make_exception_ptr(http_exception(ec.value())));
        return deref();
    }
    else if (m_read < m_read_size) // there is more to read
    {
        auto writebuf = requestImpl->outstream().streambuf();
        writebuf
            .putn_nocopy(boost::asio::buffer_cast<const uint8_t*>(m_request_buf.data()),
                         (std::min)(m_request_buf.size(), m_read_size - m_read))
            .then([this](pplx::task<size_t> writtenSizeTask) -> will_deref_t {
                size_t writtenSize = 0;
                try
                {
                    writtenSize = writtenSizeTask.get();
                }
                catch (...)
                {
                    get_request()._get_impl()->_complete(0, std::current_exception());
                    return deref();
                }
                m_read += writtenSize;
                m_request_buf.consume(writtenSize);

                async_read_until_buffersize(
                    (std::min)(ChunkSize, m_read_size - m_read),
                    [this](const boost::system::error_code& ec, size_t) { (will_deref_t) this->handle_body(ec); });
                return will_deref_t {};
            });
        return will_deref_t {};
    }
    else // have read request body
    {
        requestImpl->_complete(m_read);
        return deref();
    }
}

will_deref_and_erase_t asio_server_connection::async_write(WriteFunc response_func_ptr, const http_response& response)
{
    if (m_ssl_stream)
    {
        boost::asio::async_write(*m_ssl_stream, m_response_buf, [=](const boost::system::error_code& ec, std::size_t) {
            (this->*response_func_ptr)(response, ec);
        });
    }
    else
    {
        boost::asio::async_write(*m_socket, m_response_buf, [=](const boost::system::error_code& ec, std::size_t) {
            (this->*response_func_ptr)(response, ec);
        });
    }
    return will_deref_and_erase_t {};
}

will_deref_t asio_server_connection::async_handle_chunked_header()
{
    if (m_ssl_stream)
    {
        boost::asio::async_read_until(
            *m_ssl_stream, m_request_buf, CRLF, [this](const boost::system::error_code& ec, size_t) {
                (will_deref_t) this->handle_chunked_header(ec);
            });
    }
    else
    {
        boost::asio::async_read_until(
            *m_socket, m_request_buf, CRLF, [this](const boost::system::error_code& ec, size_t) {
                (will_deref_t) this->handle_chunked_header(ec);
            });
    }
    return will_deref_t {};
}

template<typename ReadHandler>
void asio_server_connection::async_read_until_buffersize(size_t size, const ReadHandler& handler)
{
    // The condition is such that after completing the async_read below, m_request_buf will contain at least `size`
    // bytes.
    auto condition = transfer_at_least(0);

    auto bufsize = m_request_buf.size();
    if (size > bufsize)
    {
        condition = transfer_at_least(size - bufsize);
    }

    if (m_ssl_stream)
    {
        boost::asio::async_read(*m_ssl_stream, m_request_buf, condition, handler);
    }
    else
    {
        boost::asio::async_read(*m_socket, m_request_buf, condition, handler);
    }
}

will_deref_and_erase_t asio_server_connection::dispatch_request_to_listener()
{
    // locate the listener:
    http_listener_impl* pListener = nullptr;
    auto currentRequest = get_request();
    try
    {
        pListener = m_p_parent->find_listener(currentRequest.relative_uri());
    }
    catch (const std::exception&) // may be web::uri_exception, or std::range_error indicating invalid Unicode
    {
        currentRequest.reply(status_codes::BadRequest);
        (will_erase_from_parent_t) do_response();
        (will_deref_t) deref();
        return will_deref_and_erase_t {};
    }

    if (pListener == nullptr)
    {
        currentRequest.reply(status_codes::NotFound);
        (will_erase_from_parent_t) do_response();
        (will_deref_t) deref();
        return will_deref_and_erase_t {};
    }

    currentRequest._set_listener_path(pListener->uri().path());
    (will_erase_from_parent_t) do_response();

    // Look up the lock for the http_listener.
    pplx::extensibility::reader_writer_lock_t* pListenerLock;
    {
        pplx::extensibility::reader_writer_lock_t::scoped_lock_read lock(m_p_server->m_listeners_lock);

        // It is possible the listener could have unregistered.
        if (m_p_server->m_registered_listeners.find(pListener) == m_p_server->m_registered_listeners.end())
        {
            currentRequest.reply(status_codes::NotFound);

            (will_deref_t) deref();
            return will_deref_and_erase_t {};
        }
        pListenerLock = m_p_server->m_registered_listeners[pListener].get();

        // We need to acquire the listener's lock before releasing the registered listeners lock.
        // But we don't need to hold the registered listeners lock when calling into the user's code.
        pListenerLock->lock_read();
    }

    try
    {
        pListener->handle_request(currentRequest);
        pListenerLock->unlock();
    }
    catch (...)
    {
        pListenerLock->unlock();
        currentRequest._reply_if_not_already(status_codes::InternalError);
    }

    (will_deref_t) deref();
    return will_deref_and_erase_t {};
}

void asio_server_connection::serialize_headers(http_response response)
{
    m_response_buf.consume(m_response_buf.size()); // clear the buffer
    std::ostream os(&m_response_buf);
    os.imbue(std::locale::classic());

    os << "HTTP/1.1 " << response.status_code() << " " << utility::conversions::to_utf8string(response.reason_phrase())
       << CRLF;

    m_chunked = false;
    m_write = m_write_size = 0;

    std::string transferencoding;
    if (response.headers().match(header_names::transfer_encoding, transferencoding) && transferencoding == "chunked")
    {
        m_chunked = true;
    }
    if (!response.headers().match(header_names::content_length, m_write_size) && response.body())
    {
        m_chunked = true;
        response.headers()[header_names::transfer_encoding] = U("chunked");
    }
    if (!response.body())
    {
        response.headers().add(header_names::content_length, 0);
    }

    for (const auto& header : response.headers())
    {
        // check if the responder has requested we close the connection
        if (boost::iequals(header.first, U("connection")))
        {
            if (boost::iequals(header.second, U("close")))
            {
                m_close = true;
            }
        }
        os << utility::conversions::to_utf8string(header.first) << ": "
           << utility::conversions::to_utf8string(header.second) << CRLF;
    }
    os << CRLF;
}

will_deref_and_erase_t asio_server_connection::cancel_sending_response_with_error(const http_response& response,
                                                                                  const std::exception_ptr& eptr)
{
    auto* context = static_cast<linux_request_context*>(response._get_server_context());
    context->m_response_completed.set_exception(eptr);

    // always terminate the connection since error happens
    return finish_request_response();
}

will_deref_and_erase_t asio_server_connection::handle_write_chunked_response(const http_response& response,
                                                                             const boost::system::error_code& ec)
{
    if (ec)
    {
        return handle_response_written(response, ec);
    }

    auto readbuf = response._get_impl()->instream().streambuf();
    if (readbuf.is_eof())
    {
        return cancel_sending_response_with_error(
            response, std::make_exception_ptr(http_exception("Response stream close early!")));
    }
    auto membuf = m_response_buf.prepare(ChunkSize + chunked_encoding::additional_encoding_space);

    readbuf.getn(buffer_cast<uint8_t*>(membuf) + chunked_encoding::data_offset, ChunkSize)
        .then([=](pplx::task<size_t> actualSizeTask) -> will_deref_and_erase_t {
            size_t actualSize = 0;
            try
            {
                actualSize = actualSizeTask.get();
            }
            catch (...)
            {
                return cancel_sending_response_with_error(response, std::current_exception());
            }
            size_t offset = chunked_encoding::add_chunked_delimiters(
                buffer_cast<uint8_t*>(membuf), ChunkSize + chunked_encoding::additional_encoding_space, actualSize);
            m_response_buf.commit(actualSize + chunked_encoding::additional_encoding_space);
            m_response_buf.consume(offset);
            if (actualSize == 0)
                return async_write(&asio_server_connection::handle_response_written, response);
            else
                return async_write(&asio_server_connection::handle_write_chunked_response, response);
        });
    return will_deref_and_erase_t {};
}

will_deref_and_erase_t asio_server_connection::handle_write_large_response(const http_response& response,
                                                                           const boost::system::error_code& ec)
{
    if (ec || m_write == m_write_size) return handle_response_written(response, ec);

    auto readbuf = response._get_impl()->instream().streambuf();
    if (readbuf.is_eof())
        return cancel_sending_response_with_error(
            response, std::make_exception_ptr(http_exception("Response stream close early!")));
    size_t readBytes = (std::min)(ChunkSize, m_write_size - m_write);
    readbuf.getn(buffer_cast<uint8_t*>(m_response_buf.prepare(readBytes)), readBytes)
        .then([=](pplx::task<size_t> actualSizeTask) -> will_deref_and_erase_t {
            size_t actualSize = 0;
            try
            {
                actualSize = actualSizeTask.get();
            }
            catch (...)
            {
                return cancel_sending_response_with_error(response, std::current_exception());
            }
            m_write += actualSize;
            m_response_buf.commit(actualSize);
            return async_write(&asio_server_connection::handle_write_large_response, response);
        });
    return will_deref_and_erase_t {};
}

will_deref_and_erase_t asio_server_connection::handle_headers_written(const http_response& response,
                                                                      const boost::system::error_code& ec)
{
    if (ec)
    {
        return cancel_sending_response_with_error(
            response, std::make_exception_ptr(http_exception(ec.value(), "error writing headers")));
    }
    else
    {
        if (m_chunked)
            return handle_write_chunked_response(response, ec);
        else
            return handle_write_large_response(response, ec);
    }
}

will_deref_and_erase_t asio_server_connection::handle_response_written(const http_response& response,
                                                                       const boost::system::error_code& ec)
{
    auto* context = static_cast<linux_request_context*>(response._get_server_context());
    if (ec)
    {
        return cancel_sending_response_with_error(
            response, std::make_exception_ptr(http_exception(ec.value(), "error writing response")));
    }
    else
    {
        context->m_response_completed.set();
        if (!m_close)
        {
            return start_request_response();
        }
        else
        {
            return finish_request_response();
        }
    }
}

will_deref_and_erase_t asio_server_connection::finish_request_response()
{
    // kill the connection
    m_p_parent->internal_erase_connection(this);

    close();
    (will_deref_t) deref();

    // internal_erase_connection has been called above.
    return will_deref_and_erase_t {};
}

void hostport_listener::stop()
{
    // halt existing connections
    {
        std::lock_guard<std::mutex> lock(m_connections_lock);
        m_acceptor.reset();
        for (auto connection : m_connections)
        {
            connection->close();
        }
    }

    m_all_connections_complete.wait();
}

void hostport_listener::add_listener(const std::string& path, http_listener_impl* listener)
{
    pplx::extensibility::scoped_rw_lock_t lock(m_listeners_lock);

    if (m_is_https != (listener->uri().scheme() == U("https")))
        throw std::invalid_argument(
            "Error: http_listener can not simultaneously listen both http and https paths of one host");
    else if (!m_listeners.insert(std::map<std::string, http_listener_impl*>::value_type(path, listener)).second)
        throw std::invalid_argument("Error: http_listener is already registered for this path");
}

void hostport_listener::remove_listener(const std::string& path, http_listener_impl*)
{
    pplx::extensibility::scoped_rw_lock_t lock(m_listeners_lock);

    if (m_listeners.erase(path) != 1) throw std::invalid_argument("Error: no http_listener found for this path");
}

pplx::task<void> http_linux_server::start()
{
    pplx::extensibility::reader_writer_lock_t::scoped_lock_read lock(m_listeners_lock);

    auto it = m_listeners.begin();
    try
    {
        for (; it != m_listeners.end(); ++it)
        {
            it->second->start();
        }
    }
    catch (...)
    {
        while (it != m_listeners.begin())
        {
            --it;
            it->second->stop();
        }
        return pplx::task_from_exception<void>(std::current_exception());
    }

    m_started = true;
    return pplx::task_from_result();
}

pplx::task<void> http_linux_server::stop()
{
    pplx::extensibility::reader_writer_lock_t::scoped_lock_read lock(m_listeners_lock);

    m_started = false;

    for (auto& listener : m_listeners)
    {
        listener.second->stop();
    }

    return pplx::task_from_result();
}

std::pair<std::string, std::string> canonical_parts(const uri& uri)
{
    std::string endpoint;
    endpoint += utility::conversions::to_utf8string(uri::decode(uri.host()));
    endpoint += ":";
    endpoint += to_string(uri.port());

    auto path = utility::conversions::to_utf8string(uri::decode(uri.path()));

    if (path.size() > 1 && path[path.size() - 1] != '/')
    {
        path += "/"; // ensure the end slash is present
    }

    return std::make_pair(std::move(endpoint), std::move(path));
}

pplx::task<void> http_linux_server::register_listener(http_listener_impl* listener)
{
    auto parts = canonical_parts(listener->uri());
    auto hostport = parts.first;
    auto path = parts.second;
    bool is_https = listener->uri().scheme() == U("https");

    {
        pplx::extensibility::scoped_rw_lock_t lock(m_listeners_lock);
        if (m_registered_listeners.find(listener) != m_registered_listeners.end())
        {
            throw std::invalid_argument("listener already registered");
        }

        try
        {
            m_registered_listeners[listener] = make_unique<pplx::extensibility::reader_writer_lock_t>();

            auto found_hostport_listener = m_listeners.find(hostport);
            if (found_hostport_listener == m_listeners.end())
            {
                found_hostport_listener =
                    m_listeners
                        .insert(std::make_pair(
                            hostport,
                            make_unique<hostport_listener>(this, hostport, is_https, listener->configuration())))
                        .first;

                if (m_started)
                {
                    found_hostport_listener->second->start();
                }
            }

            found_hostport_listener->second->add_listener(path, listener);
        }
        catch (...)
        {
            // Future improvement - really this API should entirely be asynchronously.
            // the hostport_listener::start() method should be made to return a task
            // throwing the exception.
            m_registered_listeners.erase(listener);
            m_listeners.erase(hostport);
            throw;
        }
    }

    return pplx::task_from_result();
}

pplx::task<void> http_linux_server::unregister_listener(http_listener_impl* listener)
{
    auto parts = canonical_parts(listener->uri());
    auto hostport = parts.first;
    auto path = parts.second;
    // First remove the listener from hostport listener
    {
        pplx::extensibility::scoped_read_lock_t lock(m_listeners_lock);
        auto itr = m_listeners.find(hostport);
        if (itr == m_listeners.end())
        {
            throw std::invalid_argument("Error: no listener registered for that host");
        }

        itr->second->remove_listener(path, listener);
    }

    // Second remove the listener form listener collection
    std::unique_ptr<pplx::extensibility::reader_writer_lock_t> pListenerLock = nullptr;
    {
        pplx::extensibility::scoped_rw_lock_t lock(m_listeners_lock);
        pListenerLock = std::move(m_registered_listeners[listener]);
        m_registered_listeners[listener] = nullptr;
        m_registered_listeners.erase(listener);
    }

    // Then take the listener write lock to make sure there are no calls into the listener's
    // request handler.
    if (pListenerLock != nullptr)
    {
        pplx::extensibility::scoped_rw_lock_t lock(*pListenerLock);
    }

    return pplx::task_from_result();
}

pplx::task<void> http_linux_server::respond(http_response response)
{
    linux_request_context* p_context = static_cast<linux_request_context*>(response._get_server_context());
    return pplx::create_task(p_context->m_response_completed);
}

} // namespace

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
std::unique_ptr<http_server> make_http_asio_server() { return make_unique<http_linux_server>(); }

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web
