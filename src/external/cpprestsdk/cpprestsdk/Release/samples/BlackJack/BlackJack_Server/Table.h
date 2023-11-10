/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Table.h
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once
#include "stdafx.h"

#ifdef _WIN32
#include <concrt.h>
#endif
#include "messagetypes.h"
#include <queue>
#include <string>
#include <vector>

class DealerTable : public BJTable
{
public:
    DealerTable() : m_currentPlayer(0), m_betsMade(0), m_betting(true)
    {
        _init();
        FillShoe(6);
    }
    DealerTable(int id, size_t capacity, int decks)
        : BJTable(id, capacity), m_currentPlayer(0), m_betsMade(0), m_betting(true)
    {
        _init();
        FillShoe(decks);
    }

    void Deal();
    void Hit(web::http::http_request);
    void Stay(web::http::http_request); // True if it's now your turn again, such as when you're the only player...

    void DoubleDown(web::http::http_request);
    void Bet(web::http::http_request);
    void Insure(web::http::http_request);

    void Wait(web::http::http_request);

    bool AddPlayer(const Player& player);
    bool RemovePlayer(const utility::string_t& name);

private:
    void FillShoe(size_t decks);
    void DealerDeal();
    void PayUp(size_t playerId);
    void NextPlayer(web::http::http_request);

    int FindPlayer(const utility::string_t& name);

    void _init()
    {
        m_responses.push_back(message_wrapper());
        m_pendingrefresh.push_back(ST_None);
    }

    int m_stopAt;
    std::queue<Card> m_shoe;

    bool m_betting;

    pplx::extensibility::critical_section_t m_resplock;

    typedef std::shared_ptr<web::http::http_request> message_wrapper;

    std::vector<BJStatus> m_pendingrefresh;
    std::vector<message_wrapper> m_responses;

    int m_currentPlayer;
    int m_betsMade;
};
