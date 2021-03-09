/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * BlackJackClient.cpp : Defines the entry point for the console application
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#ifdef _WIN32
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <objbase.h>
#include <winsock2.h>

#include <windows.h>

// ws2tcpip.h - isn't warning clean.
#pragma warning(push)
#pragma warning(disable : 6386)
#include <ws2tcpip.h>
#pragma warning(pop)

#include <iphlpapi.h>
#endif

#include "../BlackJack_Server/messagetypes.h"
#include "cpprest/http_client.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace std;
using namespace web;
using namespace utility;
using namespace http;
using namespace http::client;

http_response CheckResponse(const std::string& url, const http_response& response)
{
    ucout << response.to_string() << endl;
    return response;
}

http_response CheckResponse(const std::string& url, const http_response& response, bool& refresh)
{
    ucout << response.to_string() << endl;
    BJPutResponse answer = BJPutResponse::FromJSON(response.extract_json().get());
    refresh = answer.Status == ST_Refresh;
    return response;
}

void PrintResult(BJHandResult result)
{
    switch (result)
    {
        case HR_PlayerBlackJack: ucout << "Black Jack"; break;
        case HR_PlayerWin: ucout << "Player wins"; break;
        case HR_ComputerWin: ucout << "Computer Wins"; break;
        case HR_Push: ucout << "Push"; break;
    }
}

void PrintCard(const Card& card)
{
    switch (card.value)
    {
        case CV_King: ucout << "K"; break;
        case CV_Queen: ucout << "Q"; break;
        case CV_Jack: ucout << "J"; break;
        case CV_Ace: ucout << "A"; break;
        default: ucout << (int)card.value; break;
    }
    switch (card.suit)
    {
        case CS_Club: ucout << "C"; break;
        case CS_Spade: ucout << "S"; break;
        case CS_Heart: ucout << "H"; break;
        case CS_Diamond: ucout << "D"; break;
    }
}

void PrintHand(bool suppress_bet, const BJHand& hand)
{
    if (!suppress_bet)
    {
        if (hand.insurance > 0)
            ucout << "Bet: " << hand.bet << "Insurance: " << hand.insurance << " Hand: ";
        else
            ucout << "Bet: " << hand.bet << " Hand: ";
    }
    for (auto iter = hand.cards.begin(); iter != hand.cards.end(); iter++)
    {
        PrintCard(*iter);
        ucout << " ";
    }
    PrintResult(hand.result);
}

void PrintTable(const http_response& response, bool& refresh)
{
    BJHand hand;

    refresh = false;

    if (response.status_code() == status_codes::OK)
    {
        if (response.headers().content_type() == U("application/json"))
        {
            BJPutResponse answer = BJPutResponse::FromJSON(response.extract_json().get());
            json::value players = answer.Data[PLAYERS];

            refresh = answer.Status == ST_Refresh;

            for (auto iter = players.as_array().begin(); iter != players.as_array().end(); ++iter)
            {
                auto& player = *iter;

                json::value name = player[NAME];
                json::value bet = player[BALANCE];

                bool suppressMoney = iter == players.as_array().begin();

                if (suppressMoney)
                    ucout << "'" << name.as_string() << "'";
                else
                    ucout << "'" << name.as_string() << "' Balance = $" << bet.as_double() << " ";

                PrintHand(suppressMoney, BJHand::FromJSON(player[HAND].as_object()));
                ucout << std::endl;
            }

            switch (answer.Status)
            {
                case ST_PlaceBet: ucout << "Place your bet!\n"; break;
                case ST_YourTurn: ucout << "Your turn!\n"; break;
            }
        }
    }
}

//
// Entry point for the blackjack client.
// Arguments: BlackJack_Client.exe <port>
// If port is not specified, client will assume that the server is listening on port 34568
//
#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    utility::string_t port = U("34568");
    if (argc == 2)
    {
        port = argv[1];
    }

    utility::string_t address = U("http://localhost:");
    address.append(port);

    http::uri uri = http::uri(address);

    http_client bjDealer(http::uri_builder(uri).append_path(U("/blackjack/dealer")).to_uri());

    utility::string_t userName;
    utility::string_t table;

    json::value availableTables = json::value::array();

    bool was_refresh = false;

    while (true)
    {
        while (was_refresh)
        {
            was_refresh = false;
            utility::ostringstream_t buf;
            buf << table << U("?request=refresh&name=") << userName;
            PrintTable(CheckResponse("blackjack/dealer", bjDealer.request(methods::PUT, buf.str()).get()), was_refresh);
        }

        std::string method;
        ucout << "Enter method:";
        cin >> method;

        const auto methodFirst = &method[0];
        const auto methodLast = methodFirst + method.size();
        std::use_facet<std::ctype<char>>(std::locale::classic()).tolower(methodFirst, methodLast);

        if (method == "quit")
        {
            if (!userName.empty() && !table.empty())
            {
                utility::ostringstream_t buf;
                buf << table << U("?name=") << userName;
                CheckResponse("blackjack/dealer", bjDealer.request(methods::DEL, buf.str()).get());
            }
            break;
        }

        if (method == "name")
        {
            ucout << "Enter user name:";
            ucin >> userName;
        }
        else if (method == "join")
        {
            ucout << "Enter table name:";
            ucin >> table;

            if (userName.empty())
            {
                ucout << "Must have a name first!\n";
                continue;
            }

            utility::ostringstream_t buf;
            buf << table << U("?name=") << userName;
            CheckResponse("blackjack/dealer", bjDealer.request(methods::POST, buf.str()).get(), was_refresh);
        }
        else if (method == "hit" || method == "stay" || method == "double")
        {
            utility::ostringstream_t buf;
            buf << table << U("?request=") << utility::conversions::to_string_t(method) << U("&name=") << userName;
            PrintTable(CheckResponse("blackjack/dealer", bjDealer.request(methods::PUT, buf.str()).get()), was_refresh);
        }
        else if (method == "bet" || method == "insure")
        {
            utility::string_t bet;
            ucout << "Enter bet:";
            ucin >> bet;

            if (userName.empty())
            {
                ucout << "Must have a name first!\n";
                continue;
            }

            utility::ostringstream_t buf;
            buf << table << U("?request=") << utility::conversions::to_string_t(method) << U("&name=") << userName
                << U("&amount=") << bet;
            PrintTable(CheckResponse("blackjack/dealer", bjDealer.request(methods::PUT, buf.str()).get()), was_refresh);
        }
        else if (method == "newtbl")
        {
            CheckResponse("blackjack/dealer", bjDealer.request(methods::POST).get(), was_refresh);
        }
        else if (method == "leave")
        {
            ucout << "Enter table:";
            ucin >> table;

            if (userName.empty())
            {
                ucout << "Must have a name first!\n";
                continue;
            }

            utility::ostringstream_t buf;
            buf << table << U("?name=") << userName;
            CheckResponse("blackjack/dealer", bjDealer.request(methods::DEL, buf.str()).get(), was_refresh);
        }
        else if (method == "list")
        {
            was_refresh = false;
            http_response response = CheckResponse("blackjack/dealer", bjDealer.request(methods::GET).get());

            if (response.status_code() == status_codes::OK)
            {
                availableTables = response.extract_json().get();
                for (auto iter = availableTables.as_array().begin(); iter != availableTables.as_array().end(); ++iter)
                {
                    BJTable bj_table = BJTable::FromJSON(iter->as_object());
                    json::value id = json::value::number(bj_table.Id);

                    ucout << "table " << bj_table.Id << ": {capacity: " << (long unsigned int)bj_table.Capacity
                          << " no. players: " << (long unsigned int)bj_table.Players.size() << " }\n";
                }
                ucout << std::endl;
            }
        }
        else
        {
            ucout << utility::conversions::to_string_t(method) << ": not understood\n";
        }
    }

    return 0;
}
