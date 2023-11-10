//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

using web::http::client::http_client;
using web::http::oauth2::experimental::oauth2_config;

namespace OAuth2Live
{
/// <summary>
/// An empty page that can be used on its own or navigated to within a Frame.
/// </summary>
public
ref class MainPage sealed
{
public:
    MainPage();

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e) override;

private:
    oauth2_config m_live_oauth2_config;
    std::unique_ptr<http_client> m_live_client;

    void _UpdateButtonState();
    void _GetToken();

    void GetTokenButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e);
    void RefreshTokenButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void ImplicitGrantUnchecked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void ImplicitGrantChecked(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);

    void GetInfoButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e);
    void GetContactsButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e);
    void GetEventsButtonClick(Platform::Object ^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e);

    void AccessTokenTextChanged(Platform::Object ^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs ^ e);
};
} // namespace OAuth2Live
