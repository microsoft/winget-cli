/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Table.xaml.cpp: Implementation of the Table.xaml class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "pch.h"

#include "PlayingTable.xaml.h"

#include "cpprest/asyncrt_utils.h"

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

using namespace Windows::Networking;

#include "CardShape.xaml.h"
#include "cpprest/json.h"
#include <string>

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace BlackjackClient;

PlayingTable::PlayingTable()
    : m_dealerResource(L"dealer"), m_alreadyInsured(false), m_client(L"http://localhost:34568/blackjack/")
{
    InitializeComponent();

    this->serverAddress->Text = ref new Platform::String(L"http://localhost:34568/blackjack/");

    this->playerName->Text = L"Player";

    size_t players = 5;

    for (size_t i = 0; i < players; i++)
    {
        auto player = ref new PlayerSpace();
        playerPanel->Children->Append(player);
        _playerSpaces.push_back(player);
    }

    EnableExit();
    EnableConnecting();

    DisableDisconnecting();
    DisableHit();
    DisableBetting();
    DisableInsurance();
}

PlayingTable::~PlayingTable() {}

void BlackjackClient::PlayingTable::AddDealerCard(Card card)
{
    Thickness margin;
    margin.Left = 5 + _dealerCards.size() * (CardWidthRect * SCALE_FACTOR * .25 + 0);
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

    _dealerCards.push_back(card);
    dealerGrid->Children->Append(crd);
}

bool BlackjackClient::PlayingTable::InterpretError(HRESULT hr)
{
    if (hr == S_OK) return false;

    switch (hr)
    {
        case 0x800C0005: this->resultLabel->Text = L"Error communicating with server"; break;
        default: this->resultLabel->Text = L"Internal error"; break;
    }

    return true;
}

void BlackjackClient::PlayingTable::Refresh()
{
    this->resultLabel->Text = L"Waiting for other players";

    std::wostringstream buf;
    buf << _tableId << L"?request=refresh&name=" << uri::encode_uri(m_name, uri::components::path);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                this->resultLabel->Text = L"Waiting for other players";
                Refresh();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::InterpretResponse(http_response& response)
{
    if (response.headers().content_type() != L"application/json") return;

    this->resultLabel->Text.clear();

    response.extract_json().then(
        [this, response](json::value jsonResponse) {
            BlackjackClient::PlayingTable ^ _this = this;
            http_response r = response;
            _this->InterpretResponse(jsonResponse);
        },
        pplx::task_continuation_context::use_current());
}

void BlackjackClient::PlayingTable::InterpretResponse(json::value jsonResponse)
{
    BJPutResponse answer = BJPutResponse::FromJSON(jsonResponse);
    json::value players = answer.Data[PLAYERS];

    bool allow_double = false;
    bool allow_insurance = false;

    size_t i = 0;

    for (auto iter = players.as_array().begin(); iter != players.as_array().end() && i < _playerSpaces.size();
         ++iter, i++)
    {
        Player player = Player::FromJSON(*iter);

        if (player.Name == DEALER)
        {
            if (player.Hand.cards.size() == 1 && _dealerCards.size() == 2)
            {
                // The cards are already shown.
                continue;
            }

            ClearDealerCards();

            for (size_t j = 0; j < player.Hand.cards.size(); j++)
            {
                AddDealerCard(player.Hand.cards[j]);
            }

            if (player.Hand.cards.size() == 1)
            {
                // Add a dummy card that isn't shown.
                AddDealerCard(Card(CardSuit::CS_Club, CardValue::CV_Ace, false));
                if (player.Hand.cards[0].value == CardValue::CV_Ace && !m_alreadyInsured) allow_insurance = true;
            }
            continue;
        }

        if (uri::decode(player.Name) == m_name)
        {
            allow_double = (player.Hand.cards.size() == 2);
        }

        _playerSpaces[i - 1]->Update(player);

        for (size_t j = _playerSpaces[i - 1]->CardsHeld(); j < player.Hand.cards.size(); j++)
        {
            _playerSpaces[i - 1]->AddCard(player.Hand.cards[j]);
        }

        if (player.Hand.result != BJHandResult::HR_None)
        {
            _playerSpaces[i - 1]->ShowResult(player.Hand.result);
        }
    }

    switch (answer.Status)
    {
        case ST_PlaceBet:
            this->resultLabel->Text = L"Place your bet!";
            EnableBetting();
            DisableHit();
            DisableInsurance();
            EnableExit();
            EnableDisconnecting();
            break;
        case ST_YourTurn:
            this->resultLabel->Text = L"Your turn!";
            DisableExit();
            DisableDisconnecting();
            DisableBetting();
            EnableHit();
            doubleButton->Visibility =
                allow_double ? Windows::UI::Xaml::Visibility::Visible : Windows::UI::Xaml::Visibility::Collapsed;
            if (allow_insurance)
                EnableInsurance();
            else
                DisableInsurance();
            break;
        case ST_Refresh:
            this->resultLabel->Text = L"Waiting for other players";
            DisableExit();
            DisableDisconnecting();
            DisableHit();
            DisableBetting();
            Refresh();
            break;
    }
}

void BlackjackClient::PlayingTable::BetButton_Click(Platform::Object ^ sender,
                                                    Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty()) return;

    ClearPlayerCards();
    ClearDealerCards();

    DisableExit();
    DisableDisconnecting();
    DisableBetting();

    std::wstring betText(betBox->Text->Data());

    this->resultLabel->Text = L"Waiting for a response";

    std::wostringstream buf;
    buf << _tableId << L"?request=bet&amount=" << betText
        << "&name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableExit();
                EnableDisconnecting();
                EnableBetting();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::InsuranceButton_Click(Platform::Object ^ sender,
                                                          Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty()) return;

    ClearTable();
    DisableHit();
    DisableInsurance();
    m_alreadyInsured = true;

    std::wstring betText(buyInsuranceTextBox->Text->Data());

    this->resultLabel->Text = L"Waiting for a response";

    std::wostringstream buf;
    buf << _tableId << L"?request=insure&amount=" << betText
        << "&name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableHit();
                EnableInsurance();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::DoubleButton_Click(Platform::Object ^ sender,
                                                       Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty()) return;

    DisableHit();
    DisableInsurance();

    this->resultLabel->Text = L"Waiting for a response";

    std::wostringstream buf;
    buf << _tableId << L"?request=double&name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableHit();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::StayButton_Click(Platform::Object ^ sender,
                                                     Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty()) return;

    DisableHit();
    DisableInsurance();

    this->resultLabel->Text = L"Waiting for a response";

    std::wostringstream buf;
    buf << _tableId << L"?request=stay&name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableHit();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::HitButton_Click(Platform::Object ^ sender,
                                                    Windows::UI::Xaml::Input::TappedRoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty()) return;

    DisableInsurance();

    this->resultLabel->Text = L"Waiting for a response";

    std::wostringstream buf;
    buf << _tableId << L"?request=hit&name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::PUT, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();
                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::ExitButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    if (_tableId.empty() || m_name.empty())
    {
        Windows::ApplicationModel::Core::CoreApplication::Exit();
    }
    else
    {
        DisableExit();
        DisableConnecting();

        auto ctx = pplx::task_continuation_context::use_current();

        std::wostringstream buf;
        buf << _tableId << L"?name=" << uri::encode_uri(m_name, uri::components::query);

        auto request = m_client.request(methods::DEL, buf.str());

        request.then(
            [this](pplx::task<http_response> tsk) {
                EnableExit();

                this->resultLabel->Text.clear();

                try
                {
                    auto response = tsk.get();
                }
                catch (const http_exception& exc)
                {
                    InterpretError(exc.error_code().value());
                    EnableExit();
                }

                Windows::ApplicationModel::Core::CoreApplication::Exit();
            },
            ctx);
    }
}

void BlackjackClient::PlayingTable::JoinButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    m_name = this->playerName->Text->Data();

    if (m_name.size() == 0)
    {
        this->resultLabel->Text = L"Please state your name!";
        return;
    }

    _tableId = L"1";

    DisableExit();
    DisableConnecting();

    this->resultLabel->Text = L"Joining table";

    uri_builder bldr(this->serverAddress->Text->Data());
    bldr.append_path(m_dealerResource);
    m_client = http_client(bldr.to_string());

    std::wostringstream buf;
    buf << _tableId << L"?name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::POST, buf.str());

    auto ctx = pplx::task_continuation_context::use_current();

    request.then(
        [this](pplx::task<http_response> tsk) {
            EnableExit();

            this->resultLabel->Text.clear();

            try
            {
                auto response = tsk.get();

                this->resultLabel->Text = L"Place your bet!";

                EnableDisconnecting();
                EnableBetting();

                InterpretResponse(response);
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableConnecting();
                _tableId.clear();
            }
        },
        ctx);
}

void BlackjackClient::PlayingTable::LeaveButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
    this->resultLabel->Text = L"Leaving table";

    DisableExit();
    DisableBetting();
    DisableDisconnecting();

    auto ctx = pplx::task_continuation_context::use_current();

    std::wostringstream buf;
    buf << _tableId << L"?name=" << uri::encode_uri(m_name, uri::components::query);

    auto request = m_client.request(methods::DEL, buf.str());

    request.then(
        [this](pplx::task<http_response> tsk) {
            EnableExit();

            try
            {
                auto response = tsk.get();

                EnableConnecting();
                ClearDealerCards();
                ClearTable();

                this->resultLabel->Text = L"Thanks for playing!";

                _tableId.clear();
            }
            catch (const http_exception& exc)
            {
                InterpretError(exc.error_code().value());
                EnableBetting();
                EnableDisconnecting();
            }
        },
        ctx);
}

void PlayingTable::OnNavigatedTo(NavigationEventArgs ^ e) {}
