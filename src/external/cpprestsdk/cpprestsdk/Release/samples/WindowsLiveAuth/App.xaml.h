/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 ****/

#pragma once

#include "App.g.h"

namespace WindowsLiveAuth
{
/// <summary>
/// Provides application-specific behavior to supplement the default Application class.
/// </summary>
ref class App sealed
{
public:
    App();
    virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args) override;

private:
    void OnSuspending(Platform::Object ^ sender, Windows::ApplicationModel::SuspendingEventArgs ^ e);
};
} // namespace WindowsLiveAuth
