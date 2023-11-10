/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * live_connect.h
 *
 * Simple API for connecting to Windows Live services, such as OneDrive and Hotmail.
 * Only supported for App Store apps.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "cpprest/filestream.h"
#include "cpprest/http_client.h"
#include "cpprest/streams.h"
#include <collection.h>

namespace web
{
namespace live
{
#if defined(__cplusplus_winrt)

/// <summary>
/// This namespace contains symbols for the standard Windows Live permission scopes, which
/// determine what privileges the application has to access data. What operations are allowed
/// and what details are returned will depend on privileges requested and granted.
///
/// See MSDN documentation for Live Connect at http://msdn.microsoft.com/en-us/live/ for details
/// on the precise meaning of these scopes.
/// </summary>
namespace scopes
{
static const utility::string_t wl_birthday = U("wl.birthday");
static const utility::string_t wl_basic = U("wl.basic");
static const utility::string_t wl_calendars = U("wl.calendars");
static const utility::string_t wl_calendars_update = U("wl.calendars_update");
static const utility::string_t wl_contacts_birthday = U("wl.contacts_birthday");
static const utility::string_t wl_contacts_create = U("wl.contacts_create");
static const utility::string_t wl_contacts_calendars = U("wl.contacts_calendars");
static const utility::string_t wl_contacts_photos = U("wl.contacts_photos");
static const utility::string_t wl_contacts_skydrive = U("wl.contacts_skydrive");
static const utility::string_t wl_emails = U("wl.emails");
static const utility::string_t wl_events_create = U("wl.events_create");
static const utility::string_t wl_messenger = U("wl.messenger");
static const utility::string_t wl_offline_access = U("wl.offline_access");
static const utility::string_t wl_phone_numbers = U("wl.phone_numbers");
static const utility::string_t wl_photos = U("wl.photos");
static const utility::string_t wl_postal_addresses = U("wl.postal_addresses");
static const utility::string_t wl_share = U("wl.share");
static const utility::string_t wl_signin = U("wl.signin");
static const utility::string_t wl_skydrive = U("wl.skydrive");
static const utility::string_t wl_skydrive_update = U("wl.skydrive_update");
static const utility::string_t wl_work_profile = U("wl.work_profile");
} // namespace scopes

/// <summary>
/// Represents a session connected to Windows Live services like Calendar, Contacts, OneDrive, and the
/// user profile, by using the Live Connect REST API. It is a thin layer on top of the Casablanca HTTP
/// library, tailored to the usage scenarios for Windows Live clients.
/// </summary>
/// <remarks>
///   See http://msdn.microsoft.com/onedrive/ for details on using the OneDrive REST API.
///
///   Note: when passing resource paths into the functions in this class, it is not necessary to add the
///   leading '/' path character. It will be added automatically when constructing the HTTP request.
///
///   Most Windows Live requests will return data in the form of plain text or JSON. The APIs in this
///   class will typically return data as JSON. The most significant exception to this is download(),
///   which will not return any data to be extracted, it will place the result in the stream or file
///   passed into it. Also, remove() will return data only to provide error information: for non-error
///   cases, the JSON value will be <c>null</c>.
///
///   This class relies on PPL tasks to represent asynchrony: the examples contained within the comments
///   show synchronous forms of invoking these APIs. In real use, applications should be relying on
///   .then() instead of .get() or .wait().
/// </remarks>
class live_client
{
public:
    /// <summary>
    /// Constructs a new Windows Live client
    /// </summary>
    live_client()
        : m_client(L"https://apis.live.net/v5.0/")
        , m_authenticator(ref new Windows::Security::Authentication::OnlineId::OnlineIdAuthenticator)
    {
    }

    /// <summary>
    /// Authenticates the current user and requests permission to access a listed Live services.
    /// </summary>
    /// <param name="services">A string containing a space-separated list of access scopes to request</param>
    /// <returns><c>true</c> if authentication succeeded, <c>false</c> otherwise.</returns>
    pplx::task<bool> login(utility::string_t scopes)
    {
        this->m_token.clear();

        auto request = ref new Windows::Security::Authentication::OnlineId::OnlineIdServiceTicketRequest(
            ref new Platform::String(scopes.c_str()), "DELEGATION");

        return pplx::create_task(m_authenticator->AuthenticateUserAsync(request))
            .then([this](Windows::Security::Authentication::OnlineId::UserIdentity ^ ident) {
                if (ident->Tickets->Size > 0)
                {
                    auto ticket = ident->Tickets->GetAt(0);

                    m_token = std::wstring(ticket->Value->Data());
                    return true;
                }
                return false;
            });
    }

    /// <summary>
    /// Authenticates the current user and requests permission to access a listed Live services.
    /// </summary>
    /// <param name="begin">
    ///   The starting point of an iterator over a std::string collection, (for example a vector of
    ///   strings), holding the names of scopes to get permission for.
    /// </param>
    /// <param name="end">The end point for the same collection.</param>
    /// <returns><c>true</c> if authentication succeeded, <c>false</c> otherwise.</returns>
    template<typename Iter>
    pplx::task<bool> login(Iter begin, Iter end)
    {
        utility::string_t services;

        for (Iter iter = begin; iter != end; ++iter)
        {
            services.append(*iter);
            services.append(utility::string_t(L" "));
        }

        return login(services);
    }

    /// <summary>
    /// Dismisses any and all permissions that have been granted to the user.
    /// </summary>
    /// <returns><c>true</c> if logout succeeded, <c>false</c> otherwise.</returns>
    /// <remarks>
    ///   Whether the logout attempt was successful or not, the application will
    ///   be required to log in again, since the access token will be cleared.
    /// </remarks>
    pplx::task<bool> logout()
    {
        this->m_token.clear();

        if (!m_authenticator->CanSignOut) return pplx::task_from_result(false);

        return pplx::create_task(m_authenticator->SignOutUserAsync()).then([this] { return true; });
    }

    /// <summary>
    /// Retrieves the access token in use by this client instance.
    /// </summary>
    /// <returns>The current token: a string. An invalid token is indicated by an empty string.</returns>
    const utility::string_t& access_token() const { return m_token; }

    /// <summary>
    /// Retrieves data from Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource to retrieve.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <example>To get the current user profile: <code>json::value profile =
    /// live_clnt.get(L"me").get().extract_json().get();</code></example>
    pplx::task<web::json::value> get(const utility::string_t& resource)
    {
        return _make_request(web::http::methods::GET, resource).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Deletes data from Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource to delete.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <example>To delete a file: <code>live_clnt.remove(L"file.NNNNNNNNNNNN").wait();</code>, where file.NNNNNNNNNNNN
    /// is a file identifier.</example>
    pplx::task<web::json::value> remove(const utility::string_t& resource)
    {
        return _make_request(web::http::methods::DEL, resource).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Modifies data in Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource to modify.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <remarks>In most cases, the response will contain data about the modified resource.</remarks>
    pplx::task<web::json::value> put(const utility::string_t& resource, const web::json::value& data)
    {
        return _make_request(web::http::methods::PUT, resource, data).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Adds data to Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource (such as a folder) where data should be added.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <remarks>In most cases, the response will contain data about the added resource.</remarks>
    /// <example>To add a contact: <code>json::value contact = ...; live_clnt.post(L"me/contacts",
    /// contact).wait();</code></example>
    pplx::task<web::json::value> post(const utility::string_t& resource, const web::json::value& data)
    {
        return _make_request(web::http::methods::POST, resource, data).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Copies data in Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource to copy.</param>
    /// <param name="destination">The location where the resource copy should be placed.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <remarks>In most cases, the response will contain data about the added resource.</remarks>
    /// <example>To copy a file: <code>live_clnt.copy(L"file.NNNNNNNNNNNN",
    /// L"folder.MMMMMMMMMMMM").wait();</code></example>
    pplx::task<web::json::value> copy(const utility::string_t& resource, const utility::string_t& destination)
    {
        return _make_request(U("COPY"), resource, destination).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Moves data in Windows Live.
    /// </summary>
    /// <param name="resource">The Windows Live resource to move.</param>
    /// <param name="destination">The location where the resource should be placed.</param>
    /// <returns>The JSON value resulting from the request.</returns>
    /// <remarks>In most cases, the response will contain data about the resource in its new location.</remarks>
    /// <example>To copy a file: <code>live_clnt.copy(L"file.NNNNNNNNNNNN",
    /// L"folder.MMMMMMMMMMMM").wait();</code></example>
    pplx::task<web::json::value> move(const utility::string_t& resource, const utility::string_t& destination)
    {
        return _make_request(U("MOVE"), resource, destination).then([](web::http::http_response response) {
            return _json_extract(response);
        });
    }

    /// <summary>
    /// Download a file from OneDrive.
    /// </summary>
    /// <param name="file_id">The OneDrive file id to download.</param>
    /// <param name="stream">A stream into which the contents of the file should be placed.</param>
    /// <returns>The size of the downloaded resource.</returns>
    /// <remarks></remarks>
    /// <example>To download a file: <code>ostream stream = ...; live_clnt.download(L"file.NNNNNNNNNNNN",
    /// ostream).get().content_ready().wait();</code></example>
    pplx::task<size_t> download(const utility::string_t& file_id, concurrency::streams::ostream stream)
    {
        web::http::uri_builder bldr;
        bldr.append(file_id);
        bldr.append_path(U("content"));

        web::http::http_request req(web::http::methods::GET);
        req.set_request_uri(bldr.to_string());

        return _make_request(req).then([stream](web::http::http_response response) -> pplx::task<size_t> {
            if (response.status_code() >= 400)
            {
                return response.extract_string().then([](utility::string_t message) -> pplx::task<size_t> {
                    return pplx::task_from_exception<size_t>(
                        std::exception(utility::conversions::to_utf8string(message).c_str()));
                });
            }
            return response.body().read_to_end(stream.streambuf());
        });
    }

    /// <summary>
    /// Download a file from OneDrive.
    /// </summary>
    /// <param name="file_id">The OneDrive file id to download.</param>
    /// <param name="file">A StorageFile reference identifying the target for the downloaded data.</param>
    /// <returns>The size of the downloaded resource.</returns>
    /// <remarks></remarks>
    pplx::task<size_t> download(const utility::string_t& file_id, Windows::Storage::StorageFile ^ file)
    {
        if (file == nullptr) throw std::invalid_argument("file reference cannot be null");

        return concurrency::streams::file_stream<uint8_t>::open_ostream(file).then(
            [file_id, this](concurrency::streams::ostream stream) {
                web::http::uri_builder bldr;
                bldr.append(file_id);
                bldr.append_path(U("content"));

                web::http::http_request req(web::http::methods::GET);
                req.set_request_uri(bldr.to_string());

                return _make_request(req)
                    .then([stream](web::http::http_response response) -> pplx::task<size_t> {
                        if (response.status_code() >= 400)
                        {
                            return response.extract_string().then([](utility::string_t message) -> pplx::task<size_t> {
                                return pplx::task_from_exception<size_t>(
                                    std::exception(utility::conversions::to_utf8string(message).c_str()));
                            });
                        }
                        return response.body().read_to_end(stream.streambuf());
                    })
                    .then([stream](pplx::task<size_t> ret_task) {
                        return stream.flush().then([stream, ret_task]() { return stream.close(); }).then([ret_task]() {
                            return ret_task;
                        });
                    });
            });
    }

    /// <summary>
    /// Upload a file to OneDrive.
    /// </summary>
    /// <param name="path">The path of the file location in OneDrive. It should be of the form
    /// "folder.NNNNNNNNNN/files/file_name.ext"</param> <param name="stream">A stream from which data for the file will
    /// be read.</param> <param name="content_length">The size of the data to upload.</param> <returns>The JSON value
    /// resulting from the request, containing metadata about the uploaded file.</returns> <remarks>
    ///   The stream must contain at least as many bytes as indicated by 'content_length', starting from its current
    ///   read position.
    /// </remarks>
    pplx::task<web::json::value> upload(const utility::string_t& path,
                                        concurrency::streams::istream stream,
                                        size_t content_length)
    {
        web::http::uri_builder bldr;
        bldr.append(path);

        web::http::http_request req(web::http::methods::PUT);
        req.set_request_uri(bldr.to_string());
        req.set_body(stream, content_length, utility::string_t{});

        return _make_request(req).then([](web::http::http_response response) { return _json_extract(response); });
    }

    /// <summary>
    /// Upload a file to OneDrive.
    /// </summary>
    /// <param name="path">The path of the file location in OneDrive. It should be of the form
    /// "folder.NNNNNNNNNN/files/file_name.ext"</param> <param name="file">A StorageFile reference identifying the
    /// source of the uploaded data.</param> <param name="content_length">The size of the data to upload.</param>
    /// <returns>The JSON value resulting from the request, containing metadata about the uploaded file.</returns>
    /// <remarks>The entire file will be uploaded.</remarks>
    pplx::task<web::json::value> upload(const utility::string_t& path, Windows::Storage::StorageFile ^ file)
    {
        if (file == nullptr) throw std::invalid_argument("file reference cannot be null");

        return pplx::create_task(file->GetBasicPropertiesAsync())
            .then([path, this, file](Windows::Storage::FileProperties::BasicProperties ^ props) {
                if (props == nullptr)
                    throw std::exception("failed to retrieve file properties; cannot determine its size");
                size_t size = (size_t)props->Size;

                return concurrency::streams::file_stream<uint8_t>::open_istream(file).then(
                    [path, this, size](concurrency::streams::istream stream) {
                        web::http::uri_builder bldr;
                        bldr.append(path);

                        web::http::http_request req(web::http::methods::PUT);
                        req.set_request_uri(bldr.to_string());
                        req.set_body(stream, size, utility::string_t{});

                        return _make_request(req)
                            .then([](web::http::http_response response) { return response.content_ready(); })
                            .then([stream](pplx::task<web::http::http_response> response) {
                                return stream.close().then([response]() { return _json_extract(response.get()); });
                            });
                    });
            });
    }

private:
    static pplx::task<web::json::value> _json_extract(web::http::http_response response)
    {
        switch (response.status_code())
        {
            case web::http::status_codes::NoContent: return pplx::task_from_result(web::json::value::null());
            default:
                if (response.status_code() >= 400)
                {
                    return response.extract_string().then(
                        [](utility::string_t message) -> pplx::task<web::json::value> {
                            return pplx::task_from_exception<web::json::value>(
                                std::exception(utility::conversions::to_utf8string(message).c_str()));
                        });
                }
                return response.extract_json();
        }
    }

    pplx::task<web::http::http_response> _make_request(web::http::method method, const utility::string_t& path)
    {
        web::http::uri_builder bldr;
        bldr.append(path);

        web::http::http_request req(method);
        req.set_request_uri(bldr.to_string());
        return _make_request(req);
    }

    pplx::task<web::http::http_response> _make_request(web::http::method method,
                                                       const utility::string_t& path,
                                                       const web::json::value& data)
    {
        web::http::uri_builder bldr;
        bldr.append(path);

        web::http::http_request req(method);
        req.set_request_uri(bldr.to_string());
        req.set_body(data);
        return _make_request(req);
    }

    pplx::task<web::http::http_response> _make_request(web::http::method method,
                                                       const utility::string_t& path,
                                                       const utility::string_t& destination)
    {
        web::http::uri_builder bldr;
        bldr.append(path);

        web::json::value data;
        data[U("destination")] = web::json::value::string(destination);

        web::http::http_request req(method);
        req.set_request_uri(bldr.to_string());
        req.set_body(data);
        return _make_request(req);
    }

    pplx::task<web::http::http_response> _make_request(web::http::http_request req)
    {
        if (!m_token.empty()) req.headers().add(U("Authorization"), U("Bearer ") + m_token);
        return m_client.request(req);
    }

    Windows::Security::Authentication::OnlineId::OnlineIdAuthenticator ^ m_authenticator;
    web::http::client::http_client m_client;
    utility::string_t m_token;
};
#endif
} // namespace live
} // namespace web
