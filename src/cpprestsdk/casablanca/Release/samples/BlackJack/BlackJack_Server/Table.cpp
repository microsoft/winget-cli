/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Table.cpp : Contains the main logic of game
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "Table.h"

#include "messagetypes.h"

using namespace std;
using namespace web;
using namespace utility;
using namespace web::http;
using namespace web::http::experimental::listener;

void DealerTable::Deal()
{
    //
    // Give everyone two cards.
    //
    Players[0].Hand.revealBoth = false;
    m_currentPlayer = 0;
    m_betting = false;

    for (size_t i = 0; i < Players.size(); i++)
    {
        Players[i].Hand.Clear();
        if (i == 0 || Players[i].Hand.bet > 0)
        {
            Card card = m_shoe.front();
            m_shoe.pop();
            Players[i].Hand.cards.push_back(card);
        }
    }
    for (size_t i = 0; i < Players.size(); i++)
    {
        if (i == 0 || Players[i].Hand.bet > 0)
        {
            Card card = m_shoe.front();
            m_shoe.pop();
            Players[i].Hand.AddCard(card);
        }
    }

    pplx::extensibility::scoped_critical_section_t lck(m_resplock);

    for (size_t player = m_currentPlayer + 1; player < Players.size(); player++)
    {
        m_currentPlayer = (int)player;

        if (Players[player].Hand.bet == 0) continue;

        if (Players[player].Hand.state == HR_Active)
        {
            m_responses[player]->reply(status_codes::OK, BJPutResponse(ST_YourTurn, this->AsJSON()).AsJSON());
            m_responses[player].reset();
            break;
        }
    }

    for (size_t i = 1; i < Players.size(); i++)
    {
        if (i != m_currentPlayer)
        {
            if (m_responses[i])
            {
                m_responses[i]->reply(status_codes::OK, BJPutResponse(ST_Refresh, this->AsJSON()).AsJSON());
                m_responses[i].reset();
            }
            else
            {
                m_pendingrefresh[i] = ST_Refresh;
            }
        }
    }
}

void DealerTable::Hit(http_request message)
{
    Card card = m_shoe.front();
    m_shoe.pop();
    Players[m_currentPlayer].Hand.AddCard(card);

    if (Players[m_currentPlayer].Hand.state == HR_BlackJack || Players[m_currentPlayer].Hand.state == HR_Busted)
    {
        Stay(message);
    }
    else
    {
        message.reply(status_codes::OK, BJPutResponse(ST_YourTurn, this->AsJSON()).AsJSON());

        pplx::extensibility::scoped_critical_section_t lck(m_resplock);

        for (size_t i = 1; i < Players.size(); i++)
        {
            if (i != m_currentPlayer)
            {
                if (m_responses[i])
                {
                    m_responses[i]->reply(status_codes::OK, BJPutResponse(ST_Refresh, this->AsJSON()).AsJSON());
                    m_responses[i].reset();
                }
                else
                {
                    m_pendingrefresh[i] = ST_Refresh;
                }
            }
        }
    }
}

void DealerTable::DoubleDown(http_request message)
{
    if (m_currentPlayer == 0 || Players[m_currentPlayer].Hand.state != HR_Active)
    {
        message.reply(status_codes::Forbidden, U("Not your turn"));
        return;
    }

    Player& current = Players[m_currentPlayer];

    if (current.Balance < current.Hand.bet)
    {
        message.reply(status_codes::Forbidden, U("Not enough money"));
        return;
    }
    if (current.Hand.cards.size() > 2)
    {
        message.reply(status_codes::Forbidden, U("Too many cards"));
        return;
    }

    // Double the bet

    current.Balance -= current.Hand.bet;
    current.Hand.bet *= 2;

    // Take one card and then stay

    Card card = m_shoe.front();
    m_shoe.pop();
    Players[m_currentPlayer].Hand.AddCard(card);

    Stay(message);
}

void DealerTable::Wait(http_request message)
{
    utility::string_t name;

    auto query = http::uri::split_query(http::uri::decode(message.relative_uri().query()));
    auto itr = query.find(QUERY_NAME);
    if (itr == query.end())
    {
        message.reply(status_codes::Forbidden, U("name and amount are required in query"));
        return;
    }
    else
        name = itr->second;

    int playerIdx = FindPlayer(name);

    if (playerIdx > 0)
    {
        pplx::extensibility::scoped_critical_section_t lck(m_resplock);

        if (m_pendingrefresh[playerIdx] != ST_None)
        {
            message.reply(status_codes::OK, BJPutResponse(m_pendingrefresh[playerIdx], this->AsJSON()).AsJSON());
            m_pendingrefresh[playerIdx] = ST_None;
        }
        else
        {
            m_responses[playerIdx] = message_wrapper(new http_request(message));
        }
    }
}

void DealerTable::Bet(http_request message)
{
    int amount;
    utility::string_t name;

    auto query = http::uri::split_query(http::uri::decode(message.relative_uri().query()));

    auto itrAmount = query.find(AMOUNT), itrName = query.find(QUERY_NAME);
    if (itrAmount == query.end() || itrName == query.end())
    {
        message.reply(status_codes::Forbidden, U("name and amount are required in query"));
        return;
    }
    utility::istringstream_t ss(itrAmount->second);
    ss >> amount;
    name = itrName->second;

    int playerIdx = FindPlayer(name);

    if (playerIdx > 0)
    {
        Players[playerIdx].Balance -= amount;
        Players[playerIdx].Hand.bet += amount;
    }

    m_betsMade += 1;

    m_responses[playerIdx] = message_wrapper(new http_request(message));

    if (m_betsMade == Players.size() - 1)
    {
        Deal();
    }

    // It is **possible** that all players have Blackjack, in which case
    // the round is over. It's especially likely when there's only one
    // player at the table.

    if (m_currentPlayer == Players.size())
    {
        m_currentPlayer = 0;
        DealerDeal();
    }
}

void DealerTable::Insure(http_request message)
{
    int amount;
    utility::string_t name;

    auto query = http::uri::split_query(http::uri::decode(message.relative_uri().query()));

    auto itrAmount = query.find(AMOUNT), itrName = query.find(QUERY_NAME);
    if (itrAmount == query.end() || itrName == query.end())
    {
        message.reply(status_codes::Forbidden, U("name and amount are required in query"));
        return;
    }
    utility::istringstream_t ss(itrAmount->second);
    ss >> amount;
    name = itrName->second;
    int playerIdx = FindPlayer(name);

    if (playerIdx > 0)
    {
        const BJHand& dealer = Players[0].Hand;
        if (Players[playerIdx].Hand.insurance > 0.0)
        {
            message.reply(status_codes::Forbidden, U("Already insured"));
            return;
        }

        if (dealer.cards.size() < 1 || dealer.revealBoth || dealer.cards[0].value != CV_Ace)
        {
            message.reply(status_codes::Forbidden, U("Dealer is not showing an Ace"));
            return;
        }

        Players[playerIdx].Balance -= amount;
        Players[playerIdx].Hand.insurance += amount;
    }

    message.reply(status_codes::OK, BJPutResponse(ST_YourTurn, this->AsJSON()).AsJSON());
}

void DealerTable::Stay(http_request message)
{
    if (m_currentPlayer == 0) return;

    if (Players[m_currentPlayer].Hand.state == HR_Active) Players[m_currentPlayer].Hand.state = HR_Held;

    // int idx = m_currentPlayer;

    message.reply(status_codes::OK, BJPutResponse(ST_Refresh, this->AsJSON()).AsJSON());

    NextPlayer(message);
}

void DealerTable::NextPlayer(http_request message)
{
    size_t player = m_currentPlayer + 1;

    for (; player < Players.size(); player++)
    {
        pplx::extensibility::scoped_critical_section_t lck(m_resplock);

        if (Players[player].Hand.bet > 0 && Players[player].Hand.state == HR_Active)
        {
            m_responses[player]->reply(status_codes::OK, BJPutResponse(ST_YourTurn, this->AsJSON()).AsJSON());
            m_responses[player].reset();
            break;
        }
    }

    m_currentPlayer = (int)player;

    if (m_currentPlayer == Players.size()) m_currentPlayer = 0;

    if (m_currentPlayer == 0) DealerDeal();
}

void DealerTable::PayUp(size_t idx)
{
    Player& player = Players[idx];
    if (player.Hand.result == HR_PlayerWin)
    {
        player.Balance += player.Hand.bet * 2;
        player.Hand.bet = 0.0;
    }
    else if (player.Hand.result == HR_ComputerWin)
    {
        player.Hand.bet = 0.0;
    }
    else if (player.Hand.result == HR_PlayerBlackJack)
    {
        player.Balance += player.Hand.bet * 2.5;
        player.Hand.bet = 0.0;
    }
    else if (player.Hand.result == HR_Push)
    {
        player.Balance += player.Hand.bet;
        player.Hand.bet = 0.0;
    }

    // Handle insurance

    if (player.Hand.insurance > 0 && Players[0].Hand.state == HR_PlayerBlackJack)
    {
        player.Balance += player.Hand.insurance * 3;
    }
    player.Hand.insurance = 0;

    pplx::extensibility::scoped_critical_section_t lck(m_resplock);

    if (m_responses[idx])
    {
        m_responses[idx]->reply(status_codes::OK, BJPutResponse(ST_PlaceBet, this->AsJSON()).AsJSON());
        m_responses[idx].reset();
    }
    else
    {
        m_pendingrefresh[idx] = ST_PlaceBet;
    }
}

void DealerTable::DealerDeal()
{
    BJHand& dealersHand = Players[0].Hand;

    dealersHand.revealBoth = true;

    NumericHandValues handValue = dealersHand.GetNumericValues();
    while (handValue.high < 17 || (handValue.high > 21 && handValue.low < 17))
    {
        Card card = m_shoe.front();
        m_shoe.pop();
        dealersHand.AddCard(card);

        if (dealersHand.state == HR_BlackJack || Players[m_currentPlayer].Hand.state == HR_Busted) break;

        handValue = dealersHand.GetNumericValues();
    }

    for (size_t i = 1; i < Players.size(); i++)
    {
        Player& player = Players[i];

        if (player.Hand.bet == 0.0)
        {
            player.Hand.result = HR_None;
            continue;
        }

        if (player.Hand.state == HR_Busted)
        {
            player.Hand.result = HR_ComputerWin;
            continue;
        }

        if (player.Hand.state == HR_BlackJack)
        {
            player.Hand.result = (dealersHand.state == HR_BlackJack) ? HR_Push : HR_PlayerBlackJack;
            continue;
        }

        if (dealersHand.state == HR_Busted)
        {
            player.Hand.result = HR_PlayerWin;
            continue;
        }

        if (dealersHand.state == HR_BlackJack)
        {
            player.Hand.result = HR_ComputerWin;
            continue;
        }

        NumericHandValues value = player.Hand.GetNumericValues();
        player.Hand.result = (value.Best() < handValue.Best())
                                 ? HR_ComputerWin
                                 : ((value.Best() == handValue.Best()) ? HR_Push : HR_PlayerWin);
    }

    m_betting = true;

    for (size_t i = 1; i < Players.size(); i++)
    {
        PayUp(i);
    }

    m_betsMade = 0;
}

// TODO: Right now, players must make sure to get in when the dealer is taking bets.
//       Change the logic to anticipate that a new player may come to the table at
//       any point, and wait until the next round.
bool DealerTable::AddPlayer(const Player& player)
{
    pplx::extensibility::scoped_critical_section_t lck(m_resplock);

    int idx = FindPlayer(player.Name);

    if (idx > 0) return false;

    Players.push_back(player);
    m_responses.push_back(message_wrapper());
    m_pendingrefresh.push_back(ST_None);

    return true;
}

bool DealerTable::RemovePlayer(const utility::string_t& name)
{
    pplx::extensibility::scoped_critical_section_t lck(m_resplock);

    auto evnts = m_responses.begin();
    auto pends = m_pendingrefresh.begin();

    for (auto iter = Players.begin(); iter != Players.end(); iter++, evnts++, pends++)
    {
        if (iter->Name == name)
        {
            Players.erase(iter);
            m_responses.erase(evnts);
            m_pendingrefresh.erase(pends);
            return true;
        }
    }
    return false;
}

void DealerTable::FillShoe(size_t decks)
{
    //
    // Value == suit*16+(facevalue-1)
    //

    //
    // Stack the decks.
    //
    std::vector<int> shoe(decks * 52);

    for (size_t d = 0; d < decks; d++)
    {
        for (int suit = 0; suit < 4; suit++)
        {
            for (int fv = 0; fv < 13; fv++)
            {
                shoe[d * 52 + suit * 13 + fv] = suit * 16 + (fv + 1);
            }
        }
    }

    // Randomize the shoe

#ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    mt19937 eng(li.LowPart);
#else
    struct timeval val;
    gettimeofday(&val, nullptr);
    mt19937 eng(val.tv_usec);
#endif
    uniform_int_distribution<int> dist(0, (int)(decks * 52 - 1));

#define ITER 4

    for (size_t r = 0; r < ITER; r++)
    {
        for (size_t i = 0; i < decks * 52; i++)
        {
            int other = dist(eng);
            swap(shoe[i], shoe[other]);
        }
    }

    //
    // Convert to the other format
    //
    for (size_t i = 0; i < decks * 52; i++)
    {
        Card card;
        card.suit = (CardSuit)(shoe[i] / 16);
        card.value = (CardValue)(shoe[i] % 16);
        m_shoe.push(card);
    }

    m_stopAt = uniform_int_distribution<int>(26, 52)(eng);
}

int DealerTable::FindPlayer(const utility::string_t& name)
{
    int idx = 0;
    for (auto iter = Players.begin(); iter != Players.end(); iter++, idx++)
    {
        if (iter->Name == name)
        {
            return idx;
        }
    }
    return -1;
}
