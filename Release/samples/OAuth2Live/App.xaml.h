//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"

namespace OAuth2Live
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
} // namespace OAuth2Live
