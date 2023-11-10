/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * messagetypes.h
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once
#include "cpprest/json.h"

enum BJHandResult
{
    HR_None,
    HR_PlayerBlackJack,
    HR_PlayerWin,
    HR_ComputerWin,
    HR_Push
};

enum BJHandState
{
    HR_Empty,
    HR_BlackJack,
    HR_Active,
    HR_Held,
    HR_Busted
};

enum BJPossibleMoves
{
    PM_None = 0x0,
    PM_Hit = 0x1,
    PM_DoubleDown = 0x2,
    PM_Split = 0x4,
    PM_All = 0x7
};

enum CardSuit
{
    CS_Heart,
    CS_Diamond,
    CS_Club,
    CS_Spade
};

enum CardValue
{
    CV_None,
    CV_Ace,
    CV_Two,
    CV_Three,
    CV_Four,
    CV_Five,
    CV_Six,
    CV_Seven,
    CV_Eight,
    CV_Nine,
    CV_Ten,
    CV_Jack,
    CV_Queen,
    CV_King
};

enum BJStatus
{
    ST_PlaceBet,
    ST_Refresh,
    ST_YourTurn,
    ST_None
};

#define STATE L"state"
#define BET L"bet"
#define DOUBLE L"double"
#define INSURE L"insure"
#define HIT L"hit"
#define STAY L"stay"
#define REFRESH L"refresh"
#define INSURANCE L"insurance"
#define RESULT L"result"
#define NAME L"Name"
#define BALANCE L"Balance"
#define HAND L"Hand"
#define SUIT L"suit"
#define VALUE L"value"
#define CARDS L"cards"
#define CAPACITY L"Capacity"
#define ID L"Id"
#define PLAYERS L"Players"
#define DEALER L"DEALER"
#define DATA L"Data"
#define STATUS L"Status"
#define REQUEST L"request"
#define AMOUNT L"amount"
#define QUERY_NAME L"name"

struct Card
{
    CardSuit suit;
    CardValue value;
    bool visible;

    Card(CardSuit suit, CardValue value, bool show = true) : suit(suit), value(value), visible(show) {}
    Card(const Card& other) : suit(other.suit), value(other.value), visible(other.visible) {}

    Card(web::json::value object) : visible(true)
    {
        suit = (CardSuit)(int)object[SUIT].as_double();
        value = (CardValue)(int)object[VALUE].as_double();
    }
};

struct NumericHandValues
{
    int low;
    int high;

    int Best() { return (high < 22) ? high : low; }
};

struct BJHand
{
    bool revealBoth;

    std::vector<Card> cards;
    double bet;
    double insurance;
    BJHandState state;
    BJHandResult result;

    BJHand() : state(HR_Empty), result(HR_None), bet(0.0), insurance(0), revealBoth(true) {}

    void Clear()
    {
        cards.clear();
        state = HR_Empty;
        result = HR_None;
        insurance = 0.0;
    }

    void AddCard(Card card)
    {
        cards.push_back(card);
        NumericHandValues value = GetNumericValues();

        if (cards.size() == 2 && value.high == 21)
        {
            state = HR_BlackJack;
        }
        else if (value.low > 21)
        {
            state = HR_Busted;
        }
        else
        {
            state = HR_Active;
        }
    }

    NumericHandValues GetNumericValues()
    {
        NumericHandValues r;
        r.low = 0;
        r.low = 0;

        bool hasAces = false;

        for (auto iter = cards.begin(); iter != cards.end(); ++iter)
        {
            if (iter->value == CV_Ace) hasAces = true;

            r.low += (iter->value < 10) ? iter->value : 10;
        }
        r.high = hasAces ? r.low + 10 : r.low;
        return r;
    }

    BJHand(web::json::value object)
    {
        web::json::value l_cards = object[CARDS];

        for (auto iter = l_cards.as_array().begin(); iter != l_cards.as_array().end(); ++iter)
        {
            if (!iter->is_null())
            {
                Card card(*iter);
                this->cards.push_back(card);
            }
        }

        state = (BJHandState)(int)object[STATE].as_double();
        bet = object[BET].as_double();
        insurance = object[INSURANCE].as_double();
        result = (BJHandResult)(int)object[RESULT].as_double();
    }
};

struct Player
{
    std::wstring Name;
    BJHand Hand;
    double Balance;

    Player() {}
    Player(const std::wstring& name) : Name(name), Balance(1000.0) {}

    static Player FromJSON(web::json::value object)
    {
        Player result;

        const web::json::value& name = object[NAME];
        const web::json::value& balance = object[BALANCE];
        const web::json::value& hand = object[HAND];

        result.Name = name.as_string();
        result.Balance = balance.as_double();
        result.Hand = BJHand(hand);

        return result;
    }
};

struct BJTable
{
    int Id;
    size_t Capacity;
    std::vector<Player> Players;

    BJTable() : Capacity(0) {}
    BJTable(int id, size_t capacity) : Id(id), Capacity(capacity)
    {
        Player tmp(DEALER);
        Players.push_back(tmp);
    }

    BJTable(web::json::value object)
    {
        Id = (int)object[ID].as_double();
        Capacity = (size_t)object[CAPACITY].as_double();

        web::json::value players = object[PLAYERS];

        for (auto iter = players.as_object().begin(); iter != players.as_object().end(); ++iter)
        {
            Players.push_back(Player::FromJSON(iter->second));
        }
    }
};

struct BJPutResponse
{
    BJStatus Status;
    web::json::value Data;

    BJPutResponse() {}
    BJPutResponse(BJStatus status, web::json::value data) : Status(status), Data(data) {}

    static BJPutResponse FromJSON(web::json::value object)
    {
        BJPutResponse result((BJStatus)(int)object[STATUS].as_double(), object[DATA]);
        return result;
    }

    web::json::value AsJSON() const
    {
        web::json::value result = web::json::value::object();
        result[STATUS] = web::json::value::number((double)Status);
        result[DATA] = Data;
        return result;
    }
};
