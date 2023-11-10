/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * casalens.cpp: Listener code: Given a location/postal code, the listener queries different services
 * for weather, things to do: events, movie and pictures and returns it to the client.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "casalens.h"

using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

const utility::string_t casalens_creds::events_url = U("http://api.eventful.com/json/events/search?...&location=");
const utility::string_t casalens_creds::movies_url = U("http://data.tmsapi.com/v1/movies/showings?");
const utility::string_t casalens_creds::images_url =
    U("https://api.datamarket.azure.com/Bing/Search/Image?$format=json");
const utility::string_t casalens_creds::bmaps_url = U("http://dev.virtualearth.net/REST/v1/Locations");
const utility::string_t casalens_creds::gmaps_url = U("http://maps.googleapis.com/maps/api/geocode/json");
const utility::string_t casalens_creds::weather_url = U("http://api.openweathermap.org/data/2.1/find/name?q=");

// FILL IN THE API KEYS FOR THE DIFFERENT SERVICES HERE.
// Refer Readme.txt for details on how to obtain the key for the services.
const utility::string_t casalens_creds::events_keyname = U("app_key");
const utility::string_t casalens_creds::events_key;
const utility::string_t casalens_creds::movies_keyname = U("api_key");
const utility::string_t casalens_creds::movies_key;
const utility::string_t casalens_creds::images_keyname = U("username");
const utility::string_t casalens_creds::images_key;
const utility::string_t casalens_creds::bmaps_keyname = U("key");
const utility::string_t casalens_creds::bmaps_key;

const utility::string_t CasaLens::events_json_key = U("events");
const utility::string_t CasaLens::movies_json_key = U("movies");
const utility::string_t CasaLens::weather_json_key = U("weather");
const utility::string_t CasaLens::images_json_key = U("images");
const utility::string_t CasaLens::error_json_key = U("error");

CasaLens::CasaLens(utility::string_t url) : m_listener(url)
{
    m_listener.support(methods::GET, std::bind(&CasaLens::handle_get, this, std::placeholders::_1));
    m_listener.support(methods::POST, std::bind(&CasaLens::handle_post, this, std::placeholders::_1));

    m_htmlcontentmap[U("/")] = std::make_tuple(U("AppCode.html"), U("text/html"));
    m_htmlcontentmap[U("/js/default.js")] = std::make_tuple(U("js/default.js"), U("application/javascript"));
    m_htmlcontentmap[U("/css/default.css")] = std::make_tuple(U("css/default.css"), U("text/css"));
    m_htmlcontentmap[U("/image/logo.png")] = std::make_tuple(U("image/logo.png"), U("application/octet-stream"));
    m_htmlcontentmap[U("/image/bing-logo.jpg")] =
        std::make_tuple(U("image/bing-logo.jpg"), U("application/octet-stream"));
    m_htmlcontentmap[U("/image/wall.jpg")] = std::make_tuple(U("image/wall.jpg"), U("application/octet-stream"));
}

void CasaLens::handle_error(pplx::task<void>& t)
{
    try
    {
        t.get();
    }
    catch (...)
    {
        // Ignore the error, Log it if a logger is available
    }
}

pplx::task<void> CasaLens::open()
{
    return m_listener.open().then([](pplx::task<void> t) { handle_error(t); });
}

pplx::task<void> CasaLens::close()
{
    return m_listener.close().then([](pplx::task<void> t) { handle_error(t); });
}

// Handler to process HTTP::GET requests.
// Replies to the request with data.
void CasaLens::handle_get(http_request message)
{
    auto path = message.relative_uri().path();
    auto content_data = m_htmlcontentmap.find(path);
    if (content_data == m_htmlcontentmap.end())
    {
        message.reply(status_codes::NotFound, U("Path not found")).then([](pplx::task<void> t) { handle_error(t); });
        return;
    }

    auto file_name = std::get<0>(content_data->second);
    auto content_type = std::get<1>(content_data->second);
    concurrency::streams::fstream::open_istream(file_name, std::ios::in)
        .then([=](concurrency::streams::istream is) {
            message.reply(status_codes::OK, is, content_type).then([](pplx::task<void> t) { handle_error(t); });
        })
        .then([=](pplx::task<void> t) {
            try
            {
                t.get();
            }
            catch (...)
            {
                // opening the file (open_istream) failed.
                // Reply with an error.
                message.reply(status_codes::InternalError).then([](pplx::task<void> t) { handle_error(t); });
            }
        });
}

// Respond to HTTP::POST messages
// Post data will contain the postal code or location string.
// Aggregate location data from different services and reply to the POST request.
void CasaLens::handle_post(http_request message)
{
    auto path = message.relative_uri().path();
    if (0 == path.compare(U("/")))
    {
        message.extract_string()
            .then([=](const utility::string_t& location) { get_data(message, location); })
            .then([](pplx::task<void> t) { handle_error(t); });
    }
    else
    {
        message.reply(status_codes::NotFound, U("Path not found")).then([](pplx::task<void> t) { handle_error(t); });
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t* args[])
#else
int main(int argc, char* args[])
#endif
{
    if (argc != 2)
    {
        wprintf(U("Usage: casalens.exe port\n"));
        return -1;
    }

    std::wstring address = U("http://localhost:");
    address.append(args[1]);

    CasaLens listener(address);
    listener.open().wait();

    std::wcout << utility::string_t(U("Listening for requests at: ")) << address << std::endl;

    std::string line;
    std::wcout << U("Hit Enter to close the listener.");
    std::getline(std::cin, line);

    listener.close().wait();

    return 0;
}
