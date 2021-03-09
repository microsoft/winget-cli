/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Oauth1Client.cpp : Defines the entry point for the console application
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

/*

INSTRUCTIONS

This sample performs authorization on various OAuth 1.0 services and
then requests basic user information.

This sample is for Windows Desktop, OS X and Linux.
Execute with administrator privileges.

Set the app key & secret strings below (i.e. s_linkedin_key, s_linkedin_secret, etc.)
To get key & secret, register an app in the corresponding service.

Set following entry in the hosts file:
127.0.0.1    testhost.local

*/
#include "cpprest/http_client.h"
#include <mutex>

#if defined(_WIN32) && !defined(__cplusplus_winrt)
// Extra includes for Windows desktop.
#include <windows.h>

#include <Shellapi.h>
#endif

#include "cpprest/http_client.h"
#include "cpprest/http_listener.h"

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace web::http::oauth1::experimental;
using namespace web::http::experimental::listener;

//
// Set key & secret pair to enable session for that service.
//
static const utility::string_t s_linkedin_key;
static const utility::string_t s_linkedin_secret;

static const utility::string_t s_twitter_key;
static const utility::string_t s_twitter_secret;

//
// Utility method to open browser on Windows, OS X and Linux systems.
//
static void open_browser(utility::string_t auth_uri)
{
#if defined(_WIN32) && !defined(__cplusplus_winrt)
    // NOTE: Windows desktop only.
    auto r = ShellExecuteA(NULL, "open", conversions::utf16_to_utf8(auth_uri).c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // NOTE: OS X only.
    string_t browser_cmd(U("open \"") + auth_uri + U("\""));
    (void)system(browser_cmd.c_str());
#else
    // NOTE: Linux/X11 only.
    string_t browser_cmd(U("xdg-open \"") + auth_uri + U("\""));
    (void)system(browser_cmd.c_str());
#endif
}

//
// A simple listener class to capture OAuth 1.0 HTTP redirect to localhost.
// The listener captures redirected URI and obtains the token.
// This type of listener can be implemented in the back-end to capture and store tokens.
//
class oauth1_code_listener
{
public:
    oauth1_code_listener(uri listen_uri, oauth1_config& config)
        : m_listener(new http_listener(listen_uri)), m_config(config)
    {
        m_listener->support([this](http::http_request request) -> void {
            if (request.request_uri().path() == U("/") && request.request_uri().query() != U(""))
            {
                m_resplock.lock();

                m_config.token_from_redirected_uri(request.request_uri())
                    .then([this, request](pplx::task<void> token_task) -> void {
                        try
                        {
                            token_task.wait();
                            m_tce.set(true);
                        }
                        catch (const oauth1_exception& e)
                        {
                            ucout << "Error: " << e.what() << std::endl;
                            m_tce.set(false);
                        }
                    });

                request.reply(status_codes::OK, U("Ok."));

                m_resplock.unlock();
            }
            else
            {
                request.reply(status_codes::NotFound, U("Not found."));
            }
        });

        m_listener->open().wait();
    }

    ~oauth1_code_listener() { m_listener->close().wait(); }

    pplx::task<bool> listen_for_code() { return pplx::create_task(m_tce); }

private:
    std::unique_ptr<http_listener> m_listener;
    pplx::task_completion_event<bool> m_tce;
    oauth1_config& m_config;
    std::mutex m_resplock;
};

//
// Base class for OAuth 1.0 sessions of this sample.
//
class oauth1_session_sample
{
public:
    oauth1_session_sample(utility::string_t name,
                          utility::string_t consumer_key,
                          utility::string_t consumer_secret,
                          utility::string_t temp_endpoint,
                          utility::string_t auth_endpoint,
                          utility::string_t token_endpoint,
                          utility::string_t callback_uri)
        : m_oauth1_config(consumer_key,
                          consumer_secret,
                          temp_endpoint,
                          auth_endpoint,
                          token_endpoint,
                          callback_uri,
                          oauth1_methods::hmac_sha1)
        , m_name(name)
        , m_listener(new oauth1_code_listener(callback_uri, m_oauth1_config))
    {
    }

    void run()
    {
        if (is_enabled())
        {
            ucout << "Running " << m_name.c_str() << " session..." << std::endl;

            if (!m_oauth1_config.token().is_valid_access_token())
            {
                if (do_authorization().get())
                {
                    m_http_config.set_oauth1(m_oauth1_config);
                }
                else
                {
                    ucout << "Authorization failed for " << m_name.c_str() << "." << std::endl;
                }
            }

            run_internal();
        }
        else
        {
            ucout << "Skipped " << m_name.c_str()
                  << " session sample because app key or secret is empty. Please see instructions." << std::endl;
        }
    }

protected:
    virtual void run_internal() = 0;

    pplx::task<bool> do_authorization()
    {
        if (open_browser_auth())
        {
            return m_listener->listen_for_code();
        }
        else
        {
            return pplx::create_task([]() { return false; });
        }
    }

    http_client_config m_http_config;
    oauth1_config m_oauth1_config;

private:
    bool is_enabled() const
    {
        return !m_oauth1_config.consumer_key().empty() && !m_oauth1_config.consumer_secret().empty();
    }

    bool open_browser_auth()
    {
        auto auth_uri_task(m_oauth1_config.build_authorization_uri());
        try
        {
            auto auth_uri(auth_uri_task.get());
            ucout << "Opening browser in URI:" << std::endl;
            ucout << auth_uri << std::endl;
            open_browser(auth_uri);
            return true;
        }
        catch (const oauth1_exception& e)
        {
            ucout << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    utility::string_t m_name;
    std::unique_ptr<oauth1_code_listener> m_listener;
};

//
// Specialized class for LinkedIn OAuth 1.0 session.
//
class linkedin_session_sample : public oauth1_session_sample
{
public:
    linkedin_session_sample()
        : oauth1_session_sample(U("LinkedIn"),
                                s_linkedin_key,
                                s_linkedin_secret,
                                U("https://api.linkedin.com/uas/oauth/requestToken"),
                                U("https://www.linkedin.com/uas/oauth/authenticate"),
                                U("https://api.linkedin.com/uas/oauth/accessToken"),
                                U("http://localhost:8888/"))
    {
    }

protected:
    void run_internal() override
    {
        http_client api(U("https://api.linkedin.com/v1/people/"), m_http_config);
        ucout << "Requesting user information:" << std::endl;
        ucout << "Information: " << api.request(methods::GET, U("~?format=json")).get().extract_json().get()
              << std::endl;
    }
};

//
// Specialized class for Twitter OAuth 1.0 session.
//
class twitter_session_sample : public oauth1_session_sample
{
public:
    twitter_session_sample()
        : oauth1_session_sample(U("Twitter"),
                                s_twitter_key,
                                s_twitter_secret,
                                U("https://api.twitter.com/oauth/request_token"),
                                U("https://api.twitter.com/oauth/authorize"),
                                U("https://api.twitter.com/oauth/access_token"),
                                U("http://testhost.local:8890/"))
    {
    }

protected:
    void run_internal() override
    {
        http_client api(U("https://api.twitter.com/1.1/"), m_http_config);
        ucout << "Requesting account information:" << std::endl;
        ucout << api.request(methods::GET, U("account/settings.json")).get().extract_json().get() << std::endl;
    }
};

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    ucout << "Running OAuth 1.0 client sample..." << std::endl;

    linkedin_session_sample linkedin;
    twitter_session_sample twitter;

    linkedin.run();
    twitter.run();

    ucout << "Done." << std::endl;
    return 0;
}
