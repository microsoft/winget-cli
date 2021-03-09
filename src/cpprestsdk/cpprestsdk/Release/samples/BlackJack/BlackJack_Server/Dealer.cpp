/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Dealer.cpp : Contains the main logic of the black jack dealer
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "Table.h"
#include "messagetypes.h"

using namespace std;
using namespace web;
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;

map<utility::string_t, std::shared_ptr<BJTable>> s_tables;
int nextId = 1;

class BlackJackDealer
{
public:
    BlackJackDealer() {}
    BlackJackDealer(utility::string_t url);

    pplx::task<void> open() { return m_listener.open(); }
    pplx::task<void> close() { return m_listener.close(); }

private:
    void handle_get(http_request message);
    void handle_put(http_request message);
    void handle_post(http_request message);
    void handle_delete(http_request message);

    http_listener m_listener;
};

BlackJackDealer::BlackJackDealer(utility::string_t url) : m_listener(url)
{
    m_listener.support(methods::GET, std::bind(&BlackJackDealer::handle_get, this, std::placeholders::_1));
    m_listener.support(methods::PUT, std::bind(&BlackJackDealer::handle_put, this, std::placeholders::_1));
    m_listener.support(methods::POST, std::bind(&BlackJackDealer::handle_post, this, std::placeholders::_1));
    m_listener.support(methods::DEL, std::bind(&BlackJackDealer::handle_delete, this, std::placeholders::_1));

    std::shared_ptr<DealerTable> tbl = std::make_shared<DealerTable>(nextId, 8, 6);
    s_tables[conversions::to_string_t(std::to_string(nextId))] = tbl;
    nextId += 1;
}

//
// A GET of the dealer resource produces a list of existing tables.
//
void BlackJackDealer::handle_get(http_request message)
{
    ucout << message.to_string() << endl;

    auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
    if (paths.empty())
    {
        message.reply(status_codes::OK, TablesAsJSON(U("Available Tables"), s_tables));
        return;
    }

    utility::string_t wtable_id = paths[0];
    const utility::string_t table_id = wtable_id;

    // Get information on a specific table.
    auto found = s_tables.find(table_id);
    if (found == s_tables.end())
    {
        message.reply(status_codes::NotFound);
    }
    else
    {
        message.reply(status_codes::OK, found->second->AsJSON());
    }
};

//
// A POST of the dealer resource creates a new table and returns a resource for
// that table.
//
void BlackJackDealer::handle_post(http_request message)
{
    ucout << message.to_string() << endl;

    auto paths = uri::split_path(uri::decode(message.relative_uri().path()));

    if (paths.empty())
    {
        utility::ostringstream_t nextIdString;
        nextIdString << nextId;

        std::shared_ptr<DealerTable> tbl = std::make_shared<DealerTable>(nextId, 8, 6);
        s_tables[nextIdString.str()] = tbl;
        nextId += 1;

        message.reply(status_codes::OK, BJPutResponse(ST_PlaceBet, tbl->AsJSON()).AsJSON());
        return;
    }
    utility::string_t wtable_id = paths[0];
    const utility::string_t table_id = wtable_id;

    // Join an existing table.
    auto found = s_tables.find(table_id);
    if (found == s_tables.end())
    {
        message.reply(status_codes::NotFound);
        return;
    }

    auto table = std::static_pointer_cast<DealerTable>(found->second);

    if (table->Players.size() < table->Capacity)
    {
        std::map<utility::string_t, utility::string_t> query =
            uri::split_query(uri::decode(message.request_uri().query()));

        auto cntEntry = query.find(QUERY_NAME);

        if (cntEntry != query.end() && !cntEntry->second.empty())
        {
            table->AddPlayer(Player(cntEntry->second));
            message.reply(status_codes::OK, BJPutResponse(ST_PlaceBet, table->AsJSON()).AsJSON());
        }
        else
        {
            message.reply(status_codes::Forbidden, U("Player name is required in query"));
        }
    }
    else
    {
        utility::ostringstream_t os;
        os << U("Table ") << table->Id << U(" is full");
        message.reply(status_codes::Forbidden, os.str());
    }
};

//
// A DELETE of the player resource leaves the table.
//
void BlackJackDealer::handle_delete(http_request message)
{
    ucout << message.to_string() << endl;

    auto paths = uri::split_path(uri::decode(message.relative_uri().path()));

    if (paths.empty())
    {
        message.reply(status_codes::Forbidden, U("TableId is required."));
        return;
    }
    utility::string_t wtable_id = paths[0];

    const utility::string_t table_id = wtable_id;

    // Get information on a specific table.
    auto found = s_tables.find(table_id);
    if (found == s_tables.end())
    {
        message.reply(status_codes::NotFound);
        return;
    }

    auto table = std::static_pointer_cast<DealerTable>(found->second);

    std::map<utility::string_t, utility::string_t> query = uri::split_query(uri::decode(message.request_uri().query()));

    auto cntEntry = query.find(QUERY_NAME);

    if (cntEntry != query.end())
    {
        if (table->RemovePlayer(cntEntry->second))
        {
            message.reply(status_codes::OK);
        }
        else
        {
            message.reply(status_codes::NotFound);
        }
    }
    else
    {
        message.reply(status_codes::Forbidden, U("Player name is required in query"));
    }
};

//
// A PUT to a table resource makes a card request (hit / stay).
//
void BlackJackDealer::handle_put(http_request message)
{
    ucout << message.to_string() << endl;

    auto paths = uri::split_path(uri::decode(message.relative_uri().path()));
    auto query = uri::split_query(uri::decode(message.relative_uri().query()));
    auto queryItr = query.find(REQUEST);
    if (paths.empty() || queryItr == query.end())
    {
        message.reply(status_codes::Forbidden, U("TableId and request are required."));
    }
    utility::string_t wtable_id = paths[0];
    utility::string_t request = queryItr->second;
    const utility::string_t table_id = wtable_id;

    // Get information on a specific table.
    auto found = s_tables.find(table_id);
    if (found == s_tables.end())
    {
        message.reply(status_codes::NotFound);
    }

    auto table = std::static_pointer_cast<DealerTable>(found->second);

    if (request == BET)
    {
        table->Bet(message);
    }
    else if (request == DOUBLE)
    {
        table->DoubleDown(message);
    }
    else if (request == INSURE)
    {
        table->Insure(message);
    }
    else if (request == HIT)
    {
        table->Hit(message);
    }
    else if (request == STAY)
    {
        table->Stay(message);
    }
    else if (request == REFRESH)
    {
        table->Wait(message);
    }
    else
    {
        message.reply(status_codes::Forbidden, U("Unrecognized request"));
    }
};
