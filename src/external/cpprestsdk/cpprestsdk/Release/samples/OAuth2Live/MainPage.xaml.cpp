//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"

#include "MainPage.xaml.h"

using namespace OAuth2Live;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Security::Authentication::Web;

using namespace web::http;
using namespace web::http::client;
using namespace web::http::oauth2::experimental;

//
// NOTE: You must set this Live key and secret for app to work.
//
static const utility::string_t s_live_key;
static const utility::string_t s_live_secret;

MainPage::MainPage()
    : m_live_oauth2_config(s_live_key,
                           s_live_secret,
                           L"https://login.live.com/oauth20_authorize.srf",
                           L"https://login.live.com/oauth20_token.srf",
                           L"https://login.live.com/oauth20_desktop.srf")
{
    InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void)e; // Unused parameter
}

void OAuth2Live::MainPage::_UpdateButtonState()
{
    const bool has_access_token = !m_live_oauth2_config.token().access_token().empty();
    GetInfoButton->IsEnabled = has_access_token;
    GetContactsButton->IsEnabled = has_access_token;
    GetEventsButton->IsEnabled = has_access_token;

    const bool has_refresh_token = !m_live_oauth2_config.token().refresh_token().empty();
    RefreshTokenButton->IsEnabled = has_refresh_token;
}

void OAuth2Live::MainPage::_GetToken()
{
    m_live_oauth2_config.set_scope(L"wl.basic wl.calendars");

    // Start over, clear tokens and button state.
    m_live_oauth2_config.set_token(oauth2_token());
    AccessToken->Text.clear();
    _UpdateButtonState();

    String ^ authURI = ref new String(m_live_oauth2_config.build_authorization_uri(true).c_str());
    auto startURI = ref new Uri(authURI);
    String ^ redirectURI = ref new String(m_live_oauth2_config.redirect_uri().c_str());
    auto endURI = ref new Uri(redirectURI);

    try
    {
        DebugArea->Text += "> Navigating WebAuthenticationBroker to " + authURI + "\n";

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
        WebAuthenticationBroker::AuthenticateAndContinue(startURI, endURI, nullptr, WebAuthenticationOptions::None);
#else
        concurrency::create_task(
            WebAuthenticationBroker::AuthenticateAsync(WebAuthenticationOptions::None, startURI, endURI))
            .then([this](WebAuthenticationResult ^ result) {
                String ^ statusString;

                DebugArea->Text += "< WebAuthenticationBroker returned: ";
                switch (result->ResponseStatus)
                {
                    case WebAuthenticationStatus::ErrorHttp:
                    {
                        DebugArea->Text += "ErrorHttp: " + result->ResponseErrorDetail + "\n";
                        break;
                    }
                    case WebAuthenticationStatus::Success:
                    {
                        DebugArea->Text += "Success\n";
                        utility::string_t data = result->ResponseData->Data();
                        DebugArea->Text += "Redirected URI:\n" + result->ResponseData + "\n";
                        DebugArea->Text += "> Obtaining token using the redirected URI\n";
                        m_live_oauth2_config.token_from_redirected_uri(data).then(
                            [this](pplx::task<void> token_task) {
                                try
                                {
                                    token_task.wait();
                                    DebugArea->Text += "< Got token\n";
                                    AccessToken->Text =
                                        ref new String(m_live_oauth2_config.token().access_token().c_str());
                                }
                                catch (const oauth2_exception& e)
                                {
                                    DebugArea->Text += "< Failed to get token\n";
                                    String ^ error =
                                        ref new String(utility::conversions::to_string_t(e.what()).c_str());
                                    DebugArea->Text += "Error: " + error + "\n";
                                }
                            },
                            pplx::task_continuation_context::use_current());
                        break;
                    }
                    default:
                    case WebAuthenticationStatus::UserCancel:
                    {
                        DebugArea->Text += "UserCancel\n";
                        break;
                    }
                }
            });
#endif
    }
    catch (Exception ^ ex)
    {
        DebugArea->Text += "< Error launching WebAuthenticationBroker: " + ex->Message + "\n";
    }
}

void OAuth2Live::MainPage::GetTokenButtonClick(Platform::Object ^ sender,
                                               Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    if (m_live_oauth2_config.client_key().empty() || m_live_oauth2_config.client_secret().empty())
    {
        DebugArea->Text +=
            "Error: Cannot get token because Live app key or secret is empty. Please see instructions.\n";
    }
    else
    {
        _GetToken();
    }
}

void OAuth2Live::MainPage::GetInfoButtonClick(Platform::Object ^ sender,
                                              Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    DebugArea->Text += "> Get user info\n";
    m_live_client->request(methods::GET, U("me"))
        .then([](http_response resp) { return resp.extract_json(); })
        .then(
            [this](web::json::value j) -> void {
                String ^ json_code = ref new String(j.serialize().c_str());
                DebugArea->Text += "< User info (JSON): " + json_code + "\n";
            },
            pplx::task_continuation_context::use_current());
}

void OAuth2Live::MainPage::GetContactsButtonClick(Platform::Object ^ sender,
                                                  Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    DebugArea->Text += "> Get user contacts\n";
    m_live_client->request(methods::GET, U("me/contacts"))
        .then([](http_response resp) { return resp.extract_json(); })
        .then(
            [this](web::json::value j) -> void {
                String ^ json_code = ref new String(j.serialize().c_str());
                DebugArea->Text += "< User contacts (JSON): " + json_code + "\n";
            },
            pplx::task_continuation_context::use_current());
}

void OAuth2Live::MainPage::GetEventsButtonClick(Platform::Object ^ sender,
                                                Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    DebugArea->Text += "> Get user events\n";
    m_live_client->request(methods::GET, U("me/events"))
        .then([](http_response resp) { return resp.extract_json(); })
        .then(
            [this](web::json::value j) -> void {
                String ^ json_code = ref new String(j.serialize().c_str());
                DebugArea->Text += "< User calendar events (JSON): " + json_code + "\n";
            },
            pplx::task_continuation_context::use_current());
}

void OAuth2Live::MainPage::AccessTokenTextChanged(Platform::Object ^ sender,
                                                  Windows::UI::Xaml::Controls::TextChangedEventArgs ^ e)
{
    http_client_config http_config;
    http_config.set_oauth2(m_live_oauth2_config);
    m_live_client.reset(new http_client(U("https://apis.live.net/v5.0/"), http_config));
    _UpdateButtonState();
}

void OAuth2Live::MainPage::ImplicitGrantUnchecked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    m_live_oauth2_config.set_implicit_grant(false);
}

void OAuth2Live::MainPage::ImplicitGrantChecked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    m_live_oauth2_config.set_implicit_grant(true);
}

void OAuth2Live::MainPage::RefreshTokenButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    DebugArea->Text += "> Refreshing token\n";

    m_live_oauth2_config.token_from_refresh().then(
        [this](pplx::task<void> refresh_task) {
            try
            {
                refresh_task.wait();
                DebugArea->Text += "< Got token\n";
                AccessToken->Text = ref new String(m_live_oauth2_config.token().access_token().c_str());
            }
            catch (const oauth2_exception& e)
            {
                DebugArea->Text += "< Failed to get token\n";
                String ^ error = ref new String(utility::conversions::to_string_t(e.what()).c_str());
                DebugArea->Text += "Error: " + error + "\n";
            }
        },
        pplx::task_continuation_context::use_current());
}
