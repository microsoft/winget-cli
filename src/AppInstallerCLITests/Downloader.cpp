// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "AppInstallerDownloader.h"
#include "HttpStream/HttpLocalCache.h"
#include "TestHooks.h"
#include "winget/Hash.h"
#include "winget/NetworkSettings.h"

#include <deliveryoptimizationerrors.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winrt/Windows.Foundation.h>

#include <mutex>
#include <stdexcept>
#include <thread>

using namespace AppInstaller;
using namespace AppInstaller::Utility;
using namespace std::string_literals;

namespace
{
    class LoopbackHttpServer
    {
    public:
        struct Request
        {
            std::string Path;
            std::string Headers;
        };

        explicit LoopbackHttpServer(std::function<std::string(const Request&)> responder)
            : m_responder(std::move(responder))
        {
            WSADATA data{};
            if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            {
                throw std::runtime_error("WSAStartup failed");
            }

            m_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_listener == INVALID_SOCKET)
            {
                WSACleanup();
                throw std::runtime_error("socket failed");
            }

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = 0;
            if (bind(m_listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
                listen(m_listener, SOMAXCONN) == SOCKET_ERROR)
            {
                closesocket(m_listener);
                WSACleanup();
                throw std::runtime_error("loopback server bind/listen failed");
            }

            int addressLength = sizeof(address);
            if (getsockname(m_listener, reinterpret_cast<sockaddr*>(&address), &addressLength) == SOCKET_ERROR)
            {
                closesocket(m_listener);
                WSACleanup();
                throw std::runtime_error("getsockname failed");
            }

            m_port = ntohs(address.sin_port);
            m_worker = std::thread([this] { Run(); });
        }

        LoopbackHttpServer(const LoopbackHttpServer&) = delete;
        LoopbackHttpServer& operator=(const LoopbackHttpServer&) = delete;

        ~LoopbackHttpServer()
        {
            shutdown(m_listener, SD_BOTH);
            closesocket(m_listener);
            m_worker.join();
            WSACleanup();
        }

        std::vector<Request> Requests() const
        {
            std::lock_guard lock(m_mutex);
            return m_requests;
        }

        std::string Url(std::string_view host, std::string_view path) const
        {
            return "http://" + std::string(host) + ":" + std::to_string(m_port) + std::string(path);
        }

    private:
        void Run()
        {
            while (true)
            {
                SOCKET connection = accept(m_listener, nullptr, nullptr);
                if (connection == INVALID_SOCKET)
                {
                    return;
                }

                std::string requestText;
                char buffer[1024];
                while (requestText.find("\r\n\r\n") == std::string::npos)
                {
                    int received = recv(connection, buffer, sizeof(buffer), 0);
                    if (received <= 0)
                    {
                        break;
                    }
                    requestText.append(buffer, received);
                }

                Request request;
                auto requestLineEnd = requestText.find("\r\n");
                if (requestLineEnd != std::string::npos)
                {
                    auto pathStart = requestText.find(' ');
                    auto pathEnd = requestText.find(' ', pathStart + 1);
                    if (pathStart != std::string::npos && pathEnd != std::string::npos)
                    {
                        request.Path = requestText.substr(pathStart + 1, pathEnd - pathStart - 1);
                    }
                    request.Headers = requestText.substr(requestLineEnd + 2);
                }

                {
                    std::lock_guard lock(m_mutex);
                    m_requests.push_back(request);
                }

                std::string response = m_responder(request);
                size_t sent = 0;
                while (sent < response.size())
                {
                    int written = send(connection, response.data() + sent, static_cast<int>(response.size() - sent), 0);
                    if (written <= 0)
                    {
                        break;
                    }
                    sent += written;
                }

                shutdown(connection, SD_BOTH);
                closesocket(connection);
            }
        }

        SOCKET m_listener = INVALID_SOCKET;
        uint16_t m_port = 0;
        std::function<std::string(const Request&)> m_responder;
        std::thread m_worker;
        mutable std::mutex m_mutex;
        std::vector<Request> m_requests;
    };

    std::string RedirectResponse(int status, std::string_view location)
    {
        return "HTTP/1.1 " + std::to_string(status) + " Redirect\r\nLocation: " +
            std::string(location) + "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    }

    std::string SuccessResponse()
    {
        return "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n"
            "ok";
    }

    bool HasHeader(const LoopbackHttpServer::Request& request, std::string_view header)
    {
        std::string foldedHeaders = Utility::FoldCase(std::string_view{ request.Headers });
        std::string foldedHeader = Utility::FoldCase(header);
        return foldedHeaders.find(foldedHeader) != std::string::npos;
    }

    DownloadInfo CreateDummyAuthDownloadInfo()
    {
        DownloadInfo info;
        info.RequestHeaders = {
            { "Authorization", "Bearer dummy-test-token", true },
            { "x-test-auth", "dummy-sensitive-value", true },
            { "x-test-header", "retained", false },
        };
        return info;
    }

    std::string HttpsRedirectUrl(std::string_view target, int status = 302, std::string_view host = "localhost")
    {
        auto escapedTarget = winrt::Windows::Foundation::Uri::EscapeComponent(Utility::ConvertToUTF16(target));
        return "https://" + std::string(host) + ":5001/DownloadRedirect/" + std::to_string(status) +
            "?target=" + Utility::ConvertToUTF8(escapedTarget);
    }
}

TEST_CASE("DownloadValidFileAndVerifyHash", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Todo: point to files from our repo when the repo goes public
    ProgressCallback callback;
    auto result = Download("https://raw.githubusercontent.com/microsoft/msix-packaging/master/LICENSE", tempFile.GetPath(), DownloadType::Manifest, callback);

    REQUIRE(!result.Sha256Hash.empty());
    auto resultHash = result.Sha256Hash;

    auto expectedHash = SHA256::ConvertToBytes("d2a45116709136462ee7a1c42f0e75f0efa258fe959b1504dc8ea4573451b759");
    REQUIRE(std::equal(
        expectedHash.begin(),
        expectedHash.end(),
        resultHash.begin()));

    uint64_t expectedFileSize = 1119;
    REQUIRE(result.SizeInBytes == expectedFileSize);
    REQUIRE(std::filesystem::file_size(tempFile.GetPath()) == expectedFileSize);

    REQUIRE(result.ContentType);
    REQUIRE(!result.ContentType.value().empty());

    // Verify motw content
    std::filesystem::path motwFile(tempFile);
    motwFile += ":Zone.Identifier:$data";
    std::ifstream motwStream(motwFile);
    std::stringstream motwContent;
    motwContent << motwStream.rdbuf();
    std::string motwContentStr = motwContent.str();
    REQUIRE(motwContentStr.find("ZoneId=3") != std::string::npos);
}

TEST_CASE("DownloadValidFileAndCancel", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    ProgressCallback callback;

    DownloadResult waitResult;
    std::thread waitThread([&]
        {
            waitResult = Download("https://aka.ms/win32-x64-user-stable", tempFile.GetPath(), DownloadType::Installer, callback);
        });

    callback.Cancel();

    waitThread.join();

    REQUIRE(waitResult.Sha256Hash.empty());
}

TEST_CASE("DownloadInvalidUrl", "[Downloader]")
{
    TestCommon::TempFile tempFile("downloader_test"s, ".test"s);
    INFO("Using temporary file named: " << tempFile.GetPath());

    ProgressCallback callback;

    REQUIRE_THROWS(Download("blargle-flargle-fluff", tempFile.GetPath(), DownloadType::Installer, callback));
}

TEST_CASE("AuthenticatedDownloadRedirectPolicy", "[Downloader]")
{
    using AppInstaller::Utility::TestHooks::ApplyAuthenticatedRedirectPolicyForTest;

    SECTION("Same origin retains authentication headers and resolves relative locations")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        auto redirectUrl = ApplyAuthenticatedRedirectPolicyForTest(
            "https://example.test:443/path/file", "../next", headers);

        REQUIRE(redirectUrl == "https://example.test/next");
        REQUIRE(headers.size() == 3);
        REQUIRE(headers[0].IsAuth);
        REQUIRE(headers[1].IsAuth);
    }

    SECTION("Origin comparison normalizes case and default ports")
    {
        auto urls = GENERATE(
            std::make_pair("HTTP://EXAMPLE.TEST:80/file", "http://example.test/next"),
            std::make_pair("HTTPS://EXAMPLE.TEST:443/file", "https://example.test/next"));
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;

        REQUIRE(ApplyAuthenticatedRedirectPolicyForTest(urls.first, urls.second, headers) == urls.second);
        REQUIRE(headers.size() == 3);
    }

    SECTION("Cross-origin redirect strips only sensitive headers")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        auto redirectUrl = ApplyAuthenticatedRedirectPolicyForTest(
            "https://example.test/path/file", "https://other.test/next", headers);

        REQUIRE(redirectUrl == "https://other.test/next");
        REQUIRE(headers.size() == 1);
        REQUIRE(headers[0].Name == "x-test-header");
    }

    SECTION("Port changes are cross-origin")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        ApplyAuthenticatedRedirectPolicyForTest(
            "https://example.test:8443/file", "https://example.test:8444/next", headers);

        REQUIRE(headers.size() == 1);
        REQUIRE_FALSE(headers[0].IsAuth);
    }

    SECTION("HTTP-to-HTTPS is cross-origin")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        ApplyAuthenticatedRedirectPolicyForTest(
            "http://example.test/file", "https://example.test/next", headers);

        REQUIRE(headers.size() == 1);
        REQUIRE_FALSE(headers[0].IsAuth);
    }

    SECTION("HTTPS downgrade is rejected")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        REQUIRE_THROWS_HR(
            ApplyAuthenticatedRedirectPolicyForTest(
                "https://example.test/file", "http://example.test/next", headers),
            HRESULT_FROM_WIN32(ERROR_HTTP_REDIRECT_FAILED));
        REQUIRE(headers.size() == 3);
    }

    SECTION("Non-HTTP redirect targets are rejected")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        REQUIRE_THROWS_HR(
            ApplyAuthenticatedRedirectPolicyForTest(
                "https://example.test/file", "ftp://example.test/next", headers),
            E_INVALIDARG);
    }

    SECTION("Authentication remains stripped across later redirects")
    {
        auto headers = CreateDummyAuthDownloadInfo().RequestHeaders;
        auto firstRedirect = ApplyAuthenticatedRedirectPolicyForTest(
            "https://example.test/file", "https://other.test/next", headers);

        ApplyAuthenticatedRedirectPolicyForTest(firstRedirect, "https://example.test/final", headers);
        REQUIRE(headers.size() == 1);
        REQUIRE_FALSE(headers[0].IsAuth);
    }
}

TEST_CASE("AuthenticatedDownloadRedirectTransport", "[Downloader]")
{
    for (int status : { 301, 302, 303, 307, 308 })
    {
        DYNAMIC_SECTION("Same-origin " << status << " redirect retains authentication")
        {
            LoopbackHttpServer server([status](const auto& request)
                {
                    return request.Path == "/start" ? RedirectResponse(status, "/final") : SuccessResponse();
                });
            ProgressCallback callback;
            std::ostringstream content;

            DownloadToStream(server.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

            REQUIRE(content.str() == "ok");
            auto requests = server.Requests();
            REQUIRE(requests.size() == 2);
            REQUIRE(HasHeader(requests[0], "Authorization: Bearer dummy-test-token"));
            REQUIRE(HasHeader(requests[1], "Authorization: Bearer dummy-test-token"));
            REQUIRE(HasHeader(requests[1], "x-test-auth: dummy-sensitive-value"));
            REQUIRE(HasHeader(requests[1], "x-test-header: retained"));
        }
    }

    SECTION("Cross-port redirect strips only authentication headers")
    {
        LoopbackHttpServer destination([](const auto&) { return SuccessResponse(); });
        LoopbackHttpServer source([&](const auto&)
            {
                return RedirectResponse(302, destination.Url("127.0.0.1", "/final"));
            });
        ProgressCallback callback;
        std::ostringstream content;

        DownloadToStream(source.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE(content.str() == "ok");
        REQUIRE(source.Requests().size() == 1);
        REQUIRE(destination.Requests().size() == 1);
        REQUIRE(HasHeader(source.Requests()[0], "Authorization: Bearer dummy-test-token"));
        auto destinationRequest = destination.Requests()[0];
        REQUIRE_FALSE(HasHeader(destinationRequest, "Authorization:"));
        REQUIRE_FALSE(HasHeader(destinationRequest, "x-test-auth:"));
        REQUIRE(HasHeader(destinationRequest, "x-test-header: retained"));
    }

    SECTION("Cross-host redirect strips authentication headers")
    {
        std::string responseLocation;
        LoopbackHttpServer server([&](const auto& request)
            {
                return request.Path == "/start" ? RedirectResponse(302, responseLocation) : SuccessResponse();
            });
        ProgressCallback callback;
        std::ostringstream content;

        responseLocation = server.Url("localhost", "/final");
        DownloadToStream(server.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE(content.str() == "ok");
        auto requests = server.Requests();
        REQUIRE(requests.size() == 2);
        REQUIRE(HasHeader(requests[0], "Authorization: Bearer dummy-test-token"));
        REQUIRE_FALSE(HasHeader(requests[1], "Authorization:"));
        REQUIRE_FALSE(HasHeader(requests[1], "x-test-auth:"));
        REQUIRE(HasHeader(requests[1], "x-test-header: retained"));
    }

    SECTION("Cross-origin multi-hop redirect keeps authentication stripped")
    {
        std::string otherOriginUrl;
        LoopbackHttpServer origin([&](const auto& request)
            {
                return request.Path == "/start" ? RedirectResponse(302, otherOriginUrl) : SuccessResponse();
            });
        LoopbackHttpServer otherOrigin([&](const auto&)
            {
                return RedirectResponse(302, origin.Url("127.0.0.1", "/final"));
            });
        otherOriginUrl = otherOrigin.Url("127.0.0.1", "/next");
        ProgressCallback callback;
        std::ostringstream content;

        DownloadToStream(origin.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE(content.str() == "ok");
        REQUIRE(otherOrigin.Requests().size() == 1);
        auto originRequests = origin.Requests();
        REQUIRE(originRequests.size() == 2);
        REQUIRE(HasHeader(originRequests[0], "Authorization: Bearer dummy-test-token"));
        REQUIRE_FALSE(HasHeader(otherOrigin.Requests()[0], "Authorization:"));
        REQUIRE_FALSE(HasHeader(originRequests[1], "Authorization:"));
        REQUIRE_FALSE(HasHeader(originRequests[1], "x-test-auth:"));
        REQUIRE(HasHeader(originRequests[1], "x-test-header: retained"));
    }

    SECTION("Redirect loops fail after the bounded redirect chain")
    {
        LoopbackHttpServer server([](const auto&) { return RedirectResponse(302, "/loop"); });
        ProgressCallback callback;
        std::ostringstream content;

        REQUIRE_THROWS_HR(
            DownloadToStream(server.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo()),
            HRESULT_FROM_WIN32(ERROR_HTTP_REDIRECT_FAILED));

        REQUIRE(server.Requests().size() == 11);
        REQUIRE(content.str().empty());
    }

    SECTION("Redirects without a location fail without retrying")
    {
        LoopbackHttpServer server([](const auto&)
            {
                return "HTTP/1.1 302 Redirect\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"s;
            });
        ProgressCallback callback;
        std::ostringstream content;

        REQUIRE_THROWS_HR(
            DownloadToStream(server.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo()),
            HRESULT_FROM_WIN32(ERROR_HTTP_REDIRECT_FAILED));

        REQUIRE(server.Requests().size() == 1);
        REQUIRE(content.str().empty());
    }

    SECTION("Unauthenticated automatic redirects still complete")
    {
        LoopbackHttpServer server([](const auto& request)
            {
                return request.Path == "/start" ? RedirectResponse(302, "/final") : SuccessResponse();
            });
        ProgressCallback callback;
        std::ostringstream content;

        DownloadToStream(server.Url("127.0.0.1", "/start"), content, DownloadType::Installer, callback);

        REQUIRE(content.str() == "ok");
        REQUIRE(server.Requests().size() == 2);
    }
}

TEST_CASE("AuthenticatedDownloadRedirectInstallerRouting", "[Downloader]")
{
    TestCommon::TestUserSettings settings;
    settings.Set<Settings::Setting::NetworkDownloader>(Settings::InstallerDownloader::DeliveryOptimization);
    auto originalProxy = Settings::Network().GetProxyUri();
    Settings::Network().SetProxyUri(std::nullopt);
    auto restoreProxy = wil::scope_exit([&] { Settings::Network().SetProxyUri(originalProxy); });

    TestCommon::TempFile tempFile("downloader_routing"s, ".test"s);
    ProgressCallback callback;
    bool doCalled = false;
    std::string expectedContent = "ok";

    SECTION("Authenticated installer downloads bypass DO and apply the redirect policy")
    {
        TestHook::SetDODownloadResult_Function_Override doOverride([&](const auto&, const auto&, auto&, auto) -> DownloadResult
            {
                doCalled = true;
                THROW_HR(E_FAIL);
            });
        LoopbackHttpServer destination([](const auto&) { return SuccessResponse(); });
        LoopbackHttpServer source([&](const auto&)
            {
                return RedirectResponse(302, destination.Url("127.0.0.1", "/final"));
            });

        Download(source.Url("127.0.0.1", "/start"), tempFile.GetPath(), DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE_FALSE(doCalled);
        REQUIRE(source.Requests().size() == 1);
        REQUIRE(destination.Requests().size() == 1);
        REQUIRE(HasHeader(source.Requests()[0], "Authorization: Bearer dummy-test-token"));
        REQUIRE_FALSE(HasHeader(destination.Requests()[0], "Authorization:"));
        REQUIRE_FALSE(HasHeader(destination.Requests()[0], "x-test-auth:"));
        REQUIRE(HasHeader(destination.Requests()[0], "x-test-header: retained"));
    }

    SECTION("Unauthenticated installer downloads still use DO")
    {
        expectedContent = "do";
        LoopbackHttpServer server([](const auto&) { return SuccessResponse(); });
        TestHook::SetDODownloadResult_Function_Override doOverride([&](const auto&, const auto& dest, auto&, auto info) -> DownloadResult
            {
                doCalled = true;
                REQUIRE(info);
                REQUIRE(info->RequestHeaders.size() == 1);
                REQUIRE(info->RequestHeaders[0].Name == "x-test-header");
                REQUIRE_FALSE(info->RequestHeaders[0].IsAuth);
                std::ofstream file(dest);
                file << "do";
                return {};
            });
        DownloadInfo info;
        info.RequestHeaders = { { "x-test-header", "retained", false } };

        Download(server.Url("127.0.0.1", "/start"), tempFile.GetPath(), DownloadType::Installer, callback, info);

        REQUIRE(doCalled);
        REQUIRE(server.Requests().empty());
    }

    SECTION("Unauthenticated DO failures still fall back and discard partial files")
    {
        TestHook::SetDODownloadResult_Function_Override doOverride([&](const auto&, const auto& dest, auto&, auto) -> DownloadResult
            {
                doCalled = true;
                std::ofstream file(dest);
                file << "partial";
                file.close();
                THROW_HR(E_FAIL);
            });
        LoopbackHttpServer server([](const auto& request)
            {
                return request.Path == "/start" ? RedirectResponse(302, "/final") : SuccessResponse();
            });
        DownloadInfo info;
        info.RequestHeaders = { { "x-test-header", "retained", false } };

        Download(server.Url("127.0.0.1", "/start"), tempFile.GetPath(), DownloadType::Installer, callback, info);

        REQUIRE(doCalled);
        REQUIRE(server.Requests().size() == 2);
        REQUIRE(HasHeader(server.Requests()[1], "x-test-header: retained"));
    }

    SECTION("Fatal DO policy errors do not fall back")
    {
        TestHook::SetDODownloadResult_Function_Override doOverride([&](const auto&, const auto&, auto&, auto) -> DownloadResult
            {
                doCalled = true;
                THROW_HR(DO_E_BLOCKED_BY_NO_NETWORK);
            });
        LoopbackHttpServer server([](const auto&) { return SuccessResponse(); });

        REQUIRE_THROWS_HR(
            Download(server.Url("127.0.0.1", "/start"), tempFile.GetPath(), DownloadType::Installer, callback),
            DO_E_BLOCKED_BY_NO_NETWORK);

        REQUIRE(doCalled);
        REQUIRE(server.Requests().empty());
        return;
    }

    std::ifstream file(tempFile.GetPath());
    std::ostringstream content;
    content << file.rdbuf();
    REQUIRE(content.str() == expectedContent);
}

// Requires LocalhostWebServer on port 5001 with a trusted certificate for localhost and 127.0.0.1.
TEST_CASE("AuthenticatedDownloadRedirectHttpsTransport", "[Downloader][.LocalhostWebServer]")
{
    const std::string headersUrl = "https://localhost:5001/DownloadHeaders";
    ProgressCallback callback;
    std::ostringstream content;

    for (int status : { 301, 302, 303, 307, 308 })
    {
        DYNAMIC_SECTION("Same-origin HTTPS " << status << " retains sensitive headers")
        {
            DownloadToStream(HttpsRedirectUrl(headersUrl, status), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

            REQUIRE(content.str() == "Authorization=True;x-test-auth=True;x-test-header=True");
        }

        DYNAMIC_SECTION("HTTPS-to-HTTP " << status << " is rejected before reaching the destination")
        {
            LoopbackHttpServer destination([](const auto&) { return SuccessResponse(); });

            REQUIRE_THROWS_HR(
                DownloadToStream(HttpsRedirectUrl(destination.Url("127.0.0.1", "/final"), status),
                    content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo()),
                HRESULT_FROM_WIN32(ERROR_HTTP_REDIRECT_FAILED));

            REQUIRE(destination.Requests().empty());
            REQUIRE(content.str().empty());
        }
    }

    SECTION("Cross-host HTTPS strips sensitive headers")
    {
        DownloadToStream(HttpsRedirectUrl("https://127.0.0.1:5001/DownloadHeaders"),
            content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE(content.str() == "Authorization=False;x-test-auth=False;x-test-header=True");
    }

    SECTION("Returning to the original HTTPS origin does not restore sensitive headers")
    {
        auto otherOriginRedirect = HttpsRedirectUrl(headersUrl, 302, "127.0.0.1");
        DownloadToStream(HttpsRedirectUrl(otherOriginRedirect), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo());

        REQUIRE(content.str() == "Authorization=False;x-test-auth=False;x-test-header=True");
    }

    SECTION("HTTPS downgrade remains blocked after sensitive headers have been stripped")
    {
        LoopbackHttpServer destination([](const auto&) { return SuccessResponse(); });
        auto otherOriginRedirect = HttpsRedirectUrl(destination.Url("127.0.0.1", "/final"), 302, "127.0.0.1");

        REQUIRE_THROWS_HR(
            DownloadToStream(HttpsRedirectUrl(otherOriginRedirect), content, DownloadType::Installer, callback, CreateDummyAuthDownloadInfo()),
            HRESULT_FROM_WIN32(ERROR_HTTP_REDIRECT_FAILED));

        REQUIRE(destination.Requests().empty());
        REQUIRE(content.str().empty());
    }

    SECTION("Unauthenticated HTTPS automatic redirects preserve ordinary headers")
    {
        DownloadInfo info;
        info.RequestHeaders = { { "x-test-header", "retained", false } };
        DownloadToStream(HttpsRedirectUrl(headersUrl), content, DownloadType::Installer, callback, info);

        REQUIRE(content.str() == "Authorization=False;x-test-auth=False;x-test-header=True");
    }
}

TEST_CASE("HttpStream_ReadLastFullPage", "[HttpStream]")
{
    Microsoft::WRL::ComPtr<IStream> stream;
    STATSTG stat = { 0 };

    for (size_t i = 0; i < 10; ++i)
    {
        stream = GetReadOnlyStreamFromURI("https://aka.ms/win32-x64-user-stable");

        stat = { 0 };
        REQUIRE(stream->Stat(&stat, STATFLAG_NONAME) == S_OK);

        if (stat.cbSize.QuadPart > 0)
        {
            break;
        }

        Sleep(500);
    }

    {
        INFO("https://aka.ms/win32-x64-user-stable gave back a 0 byte file");
        REQUIRE(stream);
    }

    LARGE_INTEGER seek;
    seek.QuadPart = (stat.cbSize.QuadPart / HttpStream::HttpLocalCache::PAGE_SIZE) * HttpStream::HttpLocalCache::PAGE_SIZE;
    REQUIRE(stream->Seek(seek, STREAM_SEEK_SET, nullptr) == S_OK);

    std::unique_ptr<BYTE[]> buffer = std::make_unique<BYTE[]>(HttpStream::HttpLocalCache::PAGE_SIZE);
    ULONG read = 0;
    REQUIRE(stream->Read(buffer.get(), static_cast<ULONG>(HttpStream::HttpLocalCache::PAGE_SIZE), &read) >= S_OK);
    REQUIRE(read == (stat.cbSize.QuadPart % HttpStream::HttpLocalCache::PAGE_SIZE));
}

TEST_CASE("CacheControl", "[Downloader]")
{
    SECTION("Empty")
    {
        CacheControlPolicy test{ L"" };
        REQUIRE(!test.Present);
    }
    SECTION("Space")
    {
        CacheControlPolicy test{ L" " };
        REQUIRE(!test.Present);
    }
    SECTION("Standard")
    {
        CacheControlPolicy test{ L"public, max-age=77287" };
        REQUIRE(test.Present);
        REQUIRE(test.Public);
        REQUIRE(!test.NoCache);
        REQUIRE(!test.NoStore);
        REQUIRE(test.MaxAge == 77287);
    }
    SECTION("All")
    {
        CacheControlPolicy test{ L"public, no-cache, no-store, max-age = 77" };
        REQUIRE(test.Present);
        REQUIRE(test.Public);
        REQUIRE(test.NoCache);
        REQUIRE(test.NoStore);
        REQUIRE(test.MaxAge == 77);
    }
    SECTION("Casing")
    {
        CacheControlPolicy test{ L"Public, Max-Age=42" };
        REQUIRE(test.Present);
        REQUIRE(test.Public);
        REQUIRE(!test.NoCache);
        REQUIRE(!test.NoStore);
        REQUIRE(test.MaxAge == 42);
    }
    SECTION("Unknown")
    {
        CacheControlPolicy test{ L"public, max-age=77287, not-real" };
        REQUIRE(test.Present);
        REQUIRE(test.Public);
        REQUIRE(!test.NoCache);
        REQUIRE(!test.NoStore);
        REQUIRE(test.MaxAge == 77287);
    }
    SECTION("MaxAge Negative")
    {
        CacheControlPolicy test{ L"max-age=-1" };
        REQUIRE(test.MaxAge == CacheControlPolicy::MaximumMaxAge);
    }
    SECTION("MaxAge not a number")
    {
        CacheControlPolicy test{ L"max-age=FOO" };
        REQUIRE(test.MaxAge == 0);
    }
}
