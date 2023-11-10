/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 ****/
#pragma once

#include "MainPage.g.h"

namespace WindowsLiveAuth
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
    void Button_Click_1(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void Button_Click_2(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void LogOutButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void UploadButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void DownloadButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
};
} // namespace WindowsLiveAuth
