/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Table.xaml.h: Declaration of the Table.xaml class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "pch.h"

#include "Player.xaml.h"
#include "PlayingTable.g.h"
#include "cpprest/http_client.h"

using namespace Platform;
using namespace Windows::UI::Xaml::Controls;
using namespace web;
using namespace utility;
using namespace http;
using namespace http::client;

namespace BlackjackClient
{
public
ref class PlayingTable sealed
{
public:
    PlayingTable();
    virtual ~PlayingTable();

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e) override;

private:
    void JoinButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
    void LeaveButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);

    void InsuranceButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
    void DoubleButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
    void StayButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
    void HitButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
    void BetButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e);
    void ExitButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);

    void Refresh();

    http_client m_client;

    std::wstring _tableId;

    bool m_alreadyInsured;

    std::wstring m_dealerResource;

    std::wstring m_name;

    pplx::cancellation_token_source _cancellationTokenSource;

    std::vector<Card> _dealerCards;

    std::vector<PlayerSpace ^> _playerSpaces;

    void AddPlayerCard(int idx, Card card);
    void AddDealerCard(Card card);

    void ShowResult(int playerIdx, Player player);

    bool InterpretError(HRESULT hr);

    void InterpretResponse(http_response& response);
    void InterpretResponse(json::value response);

    void ClearPlayerCards()
    {
        for (size_t i = 0; i < _playerSpaces.size(); i++)
        {
            _playerSpaces[i]->Clear();
        }
    }

    void ClearDealerCards()
    {
        _dealerCards.clear();
        dealerGrid->Children->Clear();
    }

    void ClearTable()
    {
        ClearDealerCards();
        ClearPlayerCards();
        resultLabel->Text.clear();
    }

    void EnableBetting()
    {
        button1->Visibility = Windows::UI::Xaml::Visibility::Visible;
        betBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        label1->Visibility = Windows::UI::Xaml::Visibility::Visible;
        betBox->IsEnabled = true;
    }
    void DisableBetting()
    {
        button1->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        betBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        label1->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    void EnableInsurance()
    {
        buyInsuranceLabel->Visibility = Windows::UI::Xaml::Visibility::Visible;
        buyInsuranceTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
        buyInsuranceButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
        m_alreadyInsured = true;
    }
    void DisableInsurance()
    {
        buyInsuranceLabel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        buyInsuranceTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        buyInsuranceButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    void EnableHit()
    {
        hitButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
        stayButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }
    void DisableHit()
    {
        hitButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        stayButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        doubleButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    void EnableConnecting()
    {
        joinButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
        TopBar->IsEnabled = true;
    }

    void DisableConnecting()
    {
        joinButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        TopBar->IsEnabled = false;
    }

    void EnableDisconnecting() { leaveButton->Visibility = Windows::UI::Xaml::Visibility::Visible; }

    void DisableDisconnecting() { leaveButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed; }

    void EnableExit() { exitButton->Visibility = Windows::UI::Xaml::Visibility::Visible; }

    void DisableExit() { exitButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed; }
};
} // namespace BlackjackClient
