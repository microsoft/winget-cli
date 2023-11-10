/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Player.xaml.h:  Declaration of the Player class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "pch.h"

#include "Player.g.h"

namespace BlackjackClient
{
public
ref class PlayerSpace sealed
{
public:
    PlayerSpace() { _init(); }

    void Clear()
    {
        playerBalance->Text.clear();
        playerBet->Text.clear();
        playerName->Text.clear();
        playerInsurance->Text.clear();
        m_cards.clear();
        playerCardGrid->Children->Clear();
    }

    int CardsHeld() { return int(m_cards.size()); }

private:
    friend ref class PlayingTable;

    void AddCard(Card card);
    void Update(Player player);

    void ShowResult(BJHandResult result);

    void _init();

    std::vector<Card> m_cards;
};
} // namespace BlackjackClient
