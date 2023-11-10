/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * MainPage.xaml.cpp - Implementation of the MainPage and
 *  FacebookAlbum classes.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "pch.h"

#include "MainPage.xaml.h"

#include "Facebook.h"
#include <collection.h>

using namespace FacebookDemo;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage::MainPage() { InitializeComponent(); }

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    (void)e; // Unused parameter
}

void MainPage::LoginButton_Click_1(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    LoginButton->IsEnabled = false; // Disable button to prevent double-login

    facebook_client::instance()
        .login(L"user_photos")
        .then([=]() { AlbumButton->IsEnabled = true; }, pplx::task_continuation_context::use_current());
}

void MainPage::AlbumButton_Click_1(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    using namespace pplx;
    AlbumButton->IsEnabled = false;

    facebook_client::instance()
        .get(L"/me/albums")
        .then([](web::json::value v) {
            web::json::object& obj = v.as_object();
            std::vector<FacebookAlbum ^> albums;

            for (auto& elem : obj[L"data"].as_array())
            {
                albums.push_back(ref new FacebookAlbum(elem[L"name"].as_string(),
                                                       elem[L"count"].as_integer(),
                                                       elem[L"id"].as_string(),
                                                       elem[L"cover_photo"].as_string()));
            }

            return task_from_result(std::move(albums));
        })
        .then(
            [=](std::vector<FacebookAlbum ^> albums) {
                AlbumGrid->ItemsSource = ref new Vector<FacebookAlbum ^>(std::move(albums));
            },
            task_continuation_context::use_current());
}

String ^ FacebookAlbum::Title::get() { return ref new String(title_.c_str()); }

int FacebookAlbum::Count::get() { return count_; }

ImageSource ^ FacebookAlbum::Preview::get()
{
    if (preview_ == nullptr)
    {
        auto preview_uri = facebook_client::instance().base_uri(true);

        preview_uri.append_path(photo_id_);
        preview_uri.append_path(L"/picture");

        preview_ = ref new Imaging::BitmapImage(ref new Uri(StringReference(preview_uri.to_string().c_str())));
    }

    return preview_;
}
