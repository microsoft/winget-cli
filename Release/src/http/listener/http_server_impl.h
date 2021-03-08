#pragma once

#include "cpprest/details/http_server.h"
#include <memory>

namespace web
{
namespace http
{
namespace experimental
{
namespace details
{
std::unique_ptr<http_server> make_http_httpsys_server();
std::unique_ptr<http_server> make_http_asio_server();

} // namespace details
} // namespace experimental
} // namespace http
} // namespace web
