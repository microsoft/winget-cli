#include <activationregistration.h>
#include <string>
#include <string_view>
#include <cor.h>
#include <xmllite.h>
#include <Shlwapi.h>

#include "wil/result.h"
#include "wil/resource.h"

HRESULT LoadManifestFromPath(std::wstring path);

HRESULT LoadFromSxSManifest(PCWSTR path);

HRESULT LoadFromEmbeddedManifest(PCWSTR path);

HRESULT WinRTLoadComponentFromFilePath(PCWSTR manifestPath);

HRESULT WinRTLoadComponentFromString(std::string_view xmlStringValue);

HRESULT ParseRootManifestFromXmlReaderInput(IUnknown* pInput);

HRESULT ParseFileTag(Microsoft::WRL::ComPtr<IXmlReader> xmlReader);

HRESULT ParseActivatableClassTag(Microsoft::WRL::ComPtr<IXmlReader> xmlReader, PCWSTR fileName);

HRESULT WinRTGetThreadingModel(
    HSTRING activatableClassId,
    ABI::Windows::Foundation::ThreadingType* threading_model);

HRESULT WinRTGetActivationFactory(
    HSTRING activatableClassId,
    REFIID  iid,
    void** factory);

HRESULT WinRTGetMetadataFile(
    const HSTRING        name,
    IMetaDataDispenserEx* metaDataDispenser,
    HSTRING* metaDataFilePath,
    IMetaDataImport2** metaDataImport,
    mdTypeDef* typeDefToken);