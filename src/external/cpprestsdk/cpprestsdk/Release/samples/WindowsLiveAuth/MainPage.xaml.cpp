/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 ****/

#include "pch.h"

#include "MainPage.xaml.h"

#include "cpprest/filestream.h"

using namespace WindowsLiveAuth;

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

using namespace Platform::Collections;
using namespace Windows::Security::Authentication::OnlineId;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

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

static web::live::live_client lv_client;

void MainPage::Button_Click_1(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    try
    {
        auto ui_ctx = pplx::task_continuation_context::use_current();

        std::vector<utility::string_t> scopes;
        scopes.push_back(web::live::scopes::wl_basic);
        scopes.push_back(web::live::scopes::wl_skydrive);
        scopes.push_back(web::live::scopes::wl_skydrive_update);
        lv_client.login(std::begin(scopes), std::end(scopes))
            .then(
                [this](bool ok) {
                    if (ok)
                    {
                        this->LogOutButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
                        this->LogInButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

                        this->Block1->Text =
                            ref new Platform::String((L"access_token = \n" + lv_client.access_token()).c_str());
                    }
                },
                ui_ctx);
    }
    catch (...)
    {
    }
}

void MainPage::LogOutButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    auto ui_ctx = pplx::task_continuation_context::use_current();

    lv_client.logout().then(
        [this](bool) {
            this->LogOutButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            this->LogInButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
        },
        ui_ctx);
}

// The following functions let you get information for an arbitrary WL resource, upload a file, or download a file.
// Use the Live Connect Interactive SDK on MSDN to explore your WL data and then try the same here.
//
// Some other things to try:
//
//  delete a file using lv_client.remove()
//  copy or move a file using lv_client.copy() and lv_client.move().
//  create a contact using lv_client.post()
//  modify a calendar event using lv_client.put()
//
void MainPage::Button_Click_2(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    auto ui_ctx = pplx::task_continuation_context::use_current();

    lv_client.get(this->Box1->Text->Data())
        .then(
            [this](pplx::task<web::json::value> value) {
                try
                {
                    auto str = value.get().serialize();
                    this->Block1->Text = ref new Platform::String(str.c_str());
                }
                catch (std::exception& exc)
                {
                    this->Block1->Text =
                        ref new Platform::String(utility::conversions::to_string_t(exc.what()).c_str());
                }
            },
            ui_ctx);
}

void MainPage::UploadButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    this->Block1->Text = ref new Platform::String(L"Processing request...");

    auto ui_ctx = pplx::task_continuation_context::use_current();

    auto filePicker = ref new Windows::Storage::Pickers::FileOpenPicker();
    filePicker->ViewMode = Windows::Storage::Pickers::PickerViewMode::List;
    filePicker->FileTypeFilter->Append(ref new Platform::String(L".txt"));
    filePicker->FileTypeFilter->Append(ref new Platform::String(L".jpg"));
    filePicker->FileTypeFilter->Append(ref new Platform::String(L".pdf"));
    filePicker->FileTypeFilter->Append(ref new Platform::String(L".docx"));
    filePicker->FileTypeFilter->Append(ref new Platform::String(L".doc"));

    auto file = filePicker->PickSingleFileAsync();

    utility::string_t path = this->Box1->Text->Data();

    pplx::create_task(file)
        .then([path](Windows::Storage::StorageFile ^ file) {
            if (file == nullptr)
            {
                throw std::exception("No file was picked");
            }

            auto full_path = path + L"/" + file->Name->Data();

            return lv_client.upload(full_path, file);
        })
        .then(
            [this](pplx::task<web::json::value> response) {
                try
                {
                    auto message = response.get().serialize();
                    this->Block1->Text = ref new Platform::String(message.c_str());
                }
                catch (std::exception& exc)
                {
                    this->Block1->Text =
                        ref new Platform::String(utility::conversions::to_string_t(exc.what()).c_str());
                }
            },
            ui_ctx);
}

void MainPage::DownloadButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    this->Block1->Text = ref new Platform::String(L"Processing request...");

    auto ui_ctx = pplx::task_continuation_context::use_current();
    utility::string_t path = this->Box1->Text->Data();

    // Start by getting the file metadata from OneDrive. We need the file name.
    lv_client.get(path)
        .then([this](web::json::value file_info) {
            if (!file_info.is_object()) throw std::exception("unexpected file info response format");

            auto name = file_info[L"name"].as_string();

            // Once we have the name, we can create a storage file in the downloads folder.

            return pplx::create_task(Windows::Storage::DownloadsFolder::CreateFileAsync(
                ref new Platform::String(name.c_str()), Windows::Storage::CreationCollisionOption::GenerateUniqueName));
        })
        .then([path, ui_ctx, this](Windows::Storage::StorageFile ^ file) {
            if (file == nullptr) throw std::exception("unexpected file info response format");
            auto name = file->Name;
            // With a file reference in hand, we download the file.
            return lv_client.download(path, file);
        })
        .then(
            [this](pplx::task<size_t> response) {
                try
                {
                    response.wait();
                    this->Block1->Text = ref new Platform::String(L"Download complete.");
                }
                catch (std::exception& exc)
                {
                    this->Block1->Text =
                        ref new Platform::String(utility::conversions::to_string_t(exc.what()).c_str());
                }
            },
            ui_ctx);
}
