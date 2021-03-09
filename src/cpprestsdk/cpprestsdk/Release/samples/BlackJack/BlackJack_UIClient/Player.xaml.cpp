/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Player.xaml.cpp: Implementation of the Player class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "pch.h"

#include "Player.xaml.h"

#include "CardShape.xaml.h"
#include "cpprest/uri.h"

using namespace BlackjackClient;

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

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

void PlayerSpace::_init()
{
    Thickness margin;
    margin.Left = 10;
    margin.Top = 0;
    margin.Bottom = 0;
    margin.Right = 0;

    this->Margin = margin;
    this->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
    this->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
    this->Width = 250;
    this->Height = 300;

    InitializeComponent();
    Clear();
}

void PlayerSpace::Update(Player player)
{
    playerName->Text = ref new Platform::String(web::uri::decode(player.Name).c_str());
    playerBalance->Text = "$" + player.Balance.ToString();
    playerBet->Text = "Bet: $" + player.Hand.bet.ToString();
    if (player.Hand.insurance > 0) {
        auto& text = playerInsurance->Text;
        text.assign("Ins: $");
        text.append(std::to_string(player.Hand.insurance));
    } else {
        text.clear();
    }
}

void PlayerSpace::AddCard(Card card)
{
    Thickness margin;
    margin.Left = 5 + m_cards.size() * (CardWidthRect * SCALE_FACTOR * .25 + 0);
    margin.Top = 12;
    margin.Bottom = 0;
    margin.Right = 0;

    auto crd = ref new CardShape();
    crd->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
    crd->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
    crd->Margin = margin;
    crd->_suit = (int)card.suit;
    crd->_value = (int)card.value;
    crd->_visible = (Platform::Boolean)card.visible;

    crd->adjust();

    m_cards.push_back(card);
    playerCardGrid->Children->Append(crd);

    UpdateLayout();
}

void PlayerSpace::ShowResult(BJHandResult result)
{
    switch (result)
    {
        case BJHandResult::HR_ComputerWin:
            playerInsurance->Text = L"Dealer Wins";
            playerBet->Text.clear();
            break;
        case BJHandResult::HR_PlayerWin:
            playerInsurance->Text = L"Player Wins";
            playerBet->Text.clear();
            break;
        case BJHandResult::HR_Push:
            playerInsurance->Text = L"Push";
            playerBet->Text.clear();
            break;
        case BJHandResult::HR_PlayerBlackJack:
            playerInsurance->Text = L"Blackjack!";
            playerBet->Text.clear();
            break;
        default: break;
    }
    UpdateLayout();
}
