/***
* Copyright (C) Microsoft. All rights reserved.
* Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* MainPage.xaml.h - Declaration of the MainPage class.  Also includes
* the declaration for the FacebookAlbum class that the GridView databinds to.
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
#pragma once

#include "MainPage.g.h"

namespace FacebookDemo
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void LoginButton_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void AlbumButton_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};

	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class FacebookAlbum sealed
	{
	internal:
		FacebookAlbum(std::wstring title, int count, std::wstring id, std::wstring photo_id):
			title_(title), count_(count), id_(id), photo_id_(photo_id) {}

	public:
		property Platform::String^ Title {
			Platform::String^ get();
		}

		property int Count {
			int get();
		}

		property Windows::UI::Xaml::Media::ImageSource^ Preview {
			Windows::UI::Xaml::Media::ImageSource^ get();
		}

	private:
		std::wstring id_;
		std::wstring title_;
		std::wstring photo_id_;
		int count_;
		Windows::UI::Xaml::Media::ImageSource^ preview_;
	};
}
