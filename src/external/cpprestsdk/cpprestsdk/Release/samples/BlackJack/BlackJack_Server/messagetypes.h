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
#include "stdafx.h"

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

#define STATE U("state")
#define BET U("bet")
#define DOUBLE U("double")
#define INSURE U("insure")
#define HIT U("hit")
#define STAY U("stay")
#define REFRESH U("refresh")
#define INSURANCE U("insurance")
#define RESULT U("result")
#define NAME U("Name")
#define BALANCE U("Balance")
#define HAND U("Hand")
#define SUIT U("suit")
#define VALUE U("value")
#define CARDS U("cards")
#define CAPACITY U("Capacity")
#define ID U("Id")
#define PLAYERS U("Players")
#define DEALER U("DEALER")
#define DATA U("Data")
#define STATUS U("Status")
#define REQUEST U("request")
#define AMOUNT U("amount")
#define QUERY_NAME U("name")

struct Card
{
    CardSuit suit;
    CardValue value;

    static Card FromJSON(const web::json::object& object)
    {
        Card result;
        result.suit = (CardSuit)object.at(SUIT).as_integer();
        result.value = (CardValue)object.at(VALUE).as_integer();
        return result;
    }

    web::json::value AsJSON() const
    {
        web::json::value result = web::json::value::object();
        result[SUIT] = web::json::value::number(suit);
        result[VALUE] = web::json::value::number(value);
        return result;
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
        NumericHandValues res;
        res.low = 0;
        res.low = 0;

        bool hasAces = false;

        for (auto iter = cards.begin(); iter != cards.end(); ++iter)
        {
            if (iter->value == CV_Ace) hasAces = true;

            res.low += (std::min)((int)iter->value, 10);
        }
        res.high = hasAces ? res.low + 10 : res.low;
        return res;
    }

    static BJHand FromJSON(const web::json::object& object)
    {
        BJHand res;

        web::json::value cs = object.at(CARDS);

        for (auto iter = cs.as_array().begin(); iter != cs.as_array().end(); ++iter)
        {
            if (!iter->is_null())
            {
                Card card;
                card = Card::FromJSON(iter->as_object());
                res.cards.push_back(card);
            }
        }

        auto iState = object.find(STATE);
        if (iState == object.end())
        {
            throw web::json::json_exception(U("STATE key not found"));
        }
        res.state = (BJHandState)iState->second.as_integer();
        auto iBet = object.find(BET);
        if (iBet == object.end())
        {
            throw web::json::json_exception(U("BET key not found"));
        }
        res.bet = iBet->second.as_double();
        auto iInsurance = object.find(INSURANCE);
        if (iInsurance == object.end())
        {
            throw web::json::json_exception(U("INSURANCE key not found"));
        }
        res.insurance = iInsurance->second.as_double();
        auto iResult = object.find(RESULT);
        if (iResult == object.end())
        {
            throw web::json::json_exception(U("RESULT key not found"));
        }
        res.result = (BJHandResult)object.find(RESULT)->second.as_integer();
        return res;
    }

    web::json::value AsJSON() const
    {
        web::json::value res = web::json::value::object();
        res[STATE] = web::json::value::number(state);
        res[RESULT] = web::json::value::number(this->result);
        res[BET] = web::json::value::number(bet);
        res[INSURANCE] = web::json::value::number(insurance);

        web::json::value jCards = web::json::value::array(cards.size());

        if (revealBoth)
        {
            int idx = 0;
            for (auto iter = cards.begin(); iter != cards.end(); ++iter)
            {
                jCards[idx++] = iter->AsJSON();
            }
        }
        else
        {
            int idx = 0;
            for (auto iter = cards.begin(); iter != cards.end();)
            {
                jCards[idx++] = iter->AsJSON();
                break;
            }
        }
        res[CARDS] = jCards;
        return res;
    }
};

struct Player
{
    utility::string_t Name;
    BJHand Hand;
    double Balance;

    Player() {}
    Player(const utility::string_t& name) : Name(name), Balance(1000.0) {}

    static Player FromJSON(const web::json::object& object)
    {
        Player result(utility::string_t{});

        auto iName = object.find(NAME);
        if (iName == object.end())
        {
            throw web::json::json_exception(U("NAME key not found"));
        }
        const web::json::value& name = iName->second;
        auto iBalance = object.find(BALANCE);
        if (iBalance == object.end())
        {
            throw web::json::json_exception(U("BALANCE key not found"));
        }
        const web::json::value& balance = iBalance->second;
        auto iHand = object.find(HAND);
        if (iHand == object.end())
        {
            throw web::json::json_exception(U("HAND key not found"));
        }
        const web::json::value& hand = iHand->second;

        result.Name = name.as_string();
        result.Balance = balance.as_double();
        result.Hand = BJHand::FromJSON(hand.as_object());
        return result;
    }

    web::json::value AsJSON() const
    {
        web::json::value result = web::json::value::object();
        result[NAME] = web::json::value::string(Name);
        result[BALANCE] = web::json::value::number(Balance);
        result[HAND] = Hand.AsJSON();
        return result;
    }
};

struct BJTable
{
    int Id;
    size_t Capacity;
    std::vector<Player> Players;

    BJTable() : Capacity(0) {}
    BJTable(int id, size_t capacity) : Id(id), Capacity(capacity) { Players.push_back(Player(DEALER)); }

    static BJTable FromJSON(const web::json::object& object)
    {
        BJTable result;
        auto iID = object.find(ID);
        if (iID == object.end())
        {
            throw web::json::json_exception(U("ID key not found"));
        }
        result.Id = (int)iID->second.as_double();
        auto iCapacity = object.find(CAPACITY);
        if (iCapacity == object.end())
        {
            throw web::json::json_exception(U("CAPACITY key not found"));
        }
        result.Capacity = (size_t)iCapacity->second.as_double();

        auto iPlayers = object.find(PLAYERS);
        if (iPlayers == object.end())
        {
            throw web::json::json_exception(U("PLAYTERS key not found"));
        }
        web::json::value players = iPlayers->second;
        int i = 0;

        for (auto iter = players.as_array().begin(); iter != players.as_array().end(); ++iter, i++)
        {
            result.Players.push_back(Player::FromJSON(iter->as_object()));
        }

        return result;
    }

    web::json::value AsJSON() const
    {
        web::json::value result = web::json::value::object();
        result[ID] = web::json::value::number((double)Id);
        result[CAPACITY] = web::json::value::number((double)Capacity);

        web::json::value jPlayers = web::json::value::array(Players.size());

        size_t idx = 0;
        for (auto iter = Players.begin(); iter != Players.end(); ++iter)
        {
            jPlayers[idx++] = iter->AsJSON();
        }
        result[PLAYERS] = jPlayers;
        return result;
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
        return BJPutResponse((BJStatus)(int)object[STATUS].as_double(), object[DATA]);
    }

    web::json::value AsJSON() const
    {
        web::json::value result = web::json::value::object();
        result[STATUS] = web::json::value::number((double)Status);
        result[DATA] = Data;
        return result;
    }
};

inline web::json::value TablesAsJSON(const utility::string_t& name,
                                     const std::map<utility::string_t, std::shared_ptr<BJTable>>& tables)
{
    web::json::value result = web::json::value::array();

    size_t idx = 0;
    for (auto tbl = tables.begin(); tbl != tables.end(); tbl++)
    {
        result[idx++] = tbl->second->AsJSON();
    }
    return result;
}
