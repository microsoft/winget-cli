// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <memory>
#include <type_traits>
#include <winrt/base.h>

// TODO: This code is expected to eventually be placed into it's own DLL to support the CLI
//       and OOP COM server for use by OS integration points.
//       For now we will just maintain the ABI with helper C++ wrappers for client use.

// "C" ABI

using AICLIString = wchar_t*;
using AICLICString = wchar_t const*;
using RepositoryHandle = void*;
using ApplicationHandle = void*;
using ManifestHandle = void*;

winrt::hresult aicliGetApplicationById(RepositoryHandle repo, AICLICString id, ApplicationHandle* app);

winrt::hresult aicliGetManifestByVersion(ApplicationHandle app, AICLICString version, ManifestHandle* man);

winrt::hresult aicliGetManifestContents(ManifestHandle man, AICLICString* contents);

void aicliFreeRepository(RepositoryHandle repo);
void aicliFreeApplication(ApplicationHandle app);
void aicliFreeManifest(ManifestHandle manifest);

// C++ wrapper

namespace AppInstaller::CLI
{
    namespace details
    {
        struct RepositoryDeleter
        {
            void operator()(RepositoryHandle repo) { aicliFreeRepository(repo); }
        };

        struct ApplicationDeleter
        {
            void operator()(ApplicationHandle app) { aicliFreeApplication(app); }
        };

        struct ManifestDeleter
        {
            void operator()(ManifestHandle manifest) { aicliFreeManifest(manifest); }
        };

        using unique_repository = std::unique_ptr<std::remove_pointer_t<RepositoryHandle>, RepositoryDeleter>;
        using unique_application = std::unique_ptr<std::remove_pointer_t<ApplicationHandle>, ApplicationDeleter>;
        using unique_manifest = std::unique_ptr<std::remove_pointer_t<ManifestHandle>, ManifestDeleter>;

        template <typename Result, typename Func, typename... Args>
        Result CallABIFunc(Func&& f, Args&&... args)
        {
            Result result{};
            winrt::check_hresult(f(std::forward(args)..., &result));
            return result;
        }
    }

    struct Manifest
    {
        Manifest() = default;
        Manifest(ManifestHandle mh) : m_man(mh) {}
        ~Manifest() = default;

        Manifest(const Manifest&) = delete;
        Manifest& operator=(const Manifest&) = delete;

        Manifest(Manifest&&) = default;
        Manifest& operator=(Manifest&&) = default;

        AICLICString GetManifestContents() const { return details::CallABIFunc<AICLICString>(aicliGetManifestContents, m_man.get()); }

    private:
        details::unique_manifest m_man;
    };

    struct Application
    {
        Application() = default;
        Application(ApplicationHandle ah) : m_app(ah) {}
        ~Application() = default;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&&) = default;
        Application& operator=(Application&&) = default;

        static Application GetById(AICLICString id) { return details::CallABIFunc<ApplicationHandle>(aicliGetApplicationById, nullptr, id); }

        Manifest GetManifestByVersion(AICLICString version) const { return details::CallABIFunc<ManifestHandle>(aicliGetManifestByVersion, m_app.get(), version); }

    private:
        details::unique_application m_app;
    };
}
