/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * CardShape.xaml.h - Declaration of the CardShape class
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include "pch.h"

#include "CardShape.g.h"

#define CardWidth 71
#define CardHeight 96
#define CardWidthRect 72
#define CardHeightRect 97

#define SCALE_FACTOR 1.80

namespace BlackjackClient
{
public
ref class CardShape sealed
{
public:
    CardShape();
    virtual ~CardShape();

private:
    friend ref class PlayingTable;
    friend ref class PlayerSpace;

    void adjust();

    int _suit;
    int _value;
    Platform::Boolean _visible;
};
} // namespace BlackjackClient
