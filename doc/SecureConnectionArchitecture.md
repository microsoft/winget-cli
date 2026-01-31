# WinGet Secure Connection Architecture for Installer Downloads

This document describes where and how WinGet implements secure connections (HTTPS/TLS) when downloading installers.

## Overview

WinGet uses a multi-layered approach to ensure secure downloads of installers:
1. **WinINet API** - Primary download method using Windows built-in networking
2. **Delivery Optimization (DO)** - Fallback/alternative download method
3. **Certificate Pinning** - Additional security layer for validating server certificates
4. **HTTP Client (WinHTTP)** - Used for REST API interactions

## Core Download Implementation

### Primary Download Path: WinINet

**Location:** `src/AppInstallerCommonCore/Downloader.cpp`

The main download functionality uses the WinINet API which provides:
- Automatic HTTPS/TLS support through Windows
- Built-in certificate validation via Windows Certificate Store
- Support for HTTP to HTTPS redirects

**Key Functions:**
- `InternetOpen()` - Initializes WinINet session with HTTPS/TLS support (lines 146-151 for proxy, 155-160 for no proxy)
- `InternetOpenUrl()` - Opens URL connection with security flags (lines 178-184)
  - Uses `INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS` to allow HTTP→HTTPS redirects
- `InternetReadFile()` - Reads data from secure connection (line 247)

**Security Features:**
- Inherits Windows OS TLS/SSL configuration
- Validates server certificates against Windows Certificate Store
- Supports proxy configurations with secure connections
- Automatic handling of HTTPS protocol

### Alternative Download: Delivery Optimization

**Location:** `src/AppInstallerCommonCore/DODownloader.cpp`

Provides an alternative download path using Windows Delivery Optimization service:
- Optimizes bandwidth usage
- Supports peer-to-peer delivery
- Uses HTTPS for secure connections
- Built on top of Windows DO service security features

## Certificate Pinning System

### Certificate Validation Framework

**Location:** `src/AppInstallerSharedLib/Public/winget/Certificates.h`
**Implementation:** `src/AppInstallerSharedLib/Certificates.cpp`

Provides advanced certificate validation beyond standard Windows certificate validation:

**Key Components:**

1. **PinningDetails** - Defines specific certificate properties to validate:
   - Public key pinning (`PinningVerificationType::PublicKey`)
   - Subject validation (`PinningVerificationType::Subject`)
   - Issuer validation (`PinningVerificationType::Issuer`)
   - Support for partial certificate chains

2. **PinningChain** - Validates full certificate chain:
   - Root certificate validation
   - Intermediate certificate validation
   - Leaf certificate validation
   - Partial chain support for flexible validation

3. **PinningConfiguration** - Main configuration interface:
   - Manages multiple acceptable certificate chains
   - Validates certificates against pinning rules
   - Caches validated certificates for performance

### Certificate Resources

**Location:** `src/CertificateResources/`

Contains embedded certificate files:
- `Microsoft_TLS_ECC_Root_G2.crt` - Microsoft TLS ECC Root Certificate
- `Microsoft_TLS_RSA_Root_G2.crt` - Microsoft TLS RSA Root Certificate
- Additional intermediate and root certificates

These certificates are used for pinning validation to ensure connections are to trusted Microsoft services.

## HTTP Client for REST APIs

### WinHTTP-based HTTP Client

**Location:** `src/AppInstallerCommonCore/HttpClientHelper.cpp`

Used for REST API interactions (repository metadata, etc.):

**Key Security Functions:**

1. **Certificate Validation Callback** (lines 28-44):
   ```cpp
   void NativeHandleServerCertificateValidation(
       web::http::client::native_handle handle,
       const Certificates::PinningConfiguration& pinningConfiguration,
       ThreadLocalStorage::ThreadGlobals* threadGlobals)
   ```
   - Retrieves server certificate using `WinHttpQueryOption()`
   - Validates against pinning configuration
   - Throws `APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH` on failure

2. **Setting Pinning Configuration** (lines 188-194):
   ```cpp
   void HttpClientHelper::SetPinningConfiguration(
       const Certificates::PinningConfiguration& configuration,
       std::shared_ptr<ThreadLocalStorage::ThreadGlobals> threadGlobals)
   ```
   - Configures custom certificate validation
   - Integrates pinning configuration with HTTP client

**Security Features:**
- Uses WinHTTP native API for HTTPS connections
- Custom certificate validation via `set_nativehandle_servercertificate_validation`
- Built-in TLS/SSL support through Windows
- Proxy support with secure connections

### HTTP Streaming Components

**Location:** `src/AppInstallerCommonCore/HttpStream/`

Additional HTTP functionality for streaming downloads:
- `HttpClientWrapper.cpp/h` - Wrapper around Windows.Web.Http client
- `HttpRandomAccessStream.cpp/h` - Random access streaming for HTTP downloads
- `HttpLocalCache.cpp/h` - Local caching for HTTP content

All components leverage Windows.Web.Http which provides:
- Automatic HTTPS/TLS handling
- Certificate validation
- Secure connection management

## Network Configuration

### Network Settings

**Location:** `src/AppInstallerCommonCore/Public/winget/NetworkSettings.h`
**Implementation:** Network settings management

Provides configuration for:
- Proxy settings (applies to secure connections)
- Network timeout configurations
- Connection retry logic

### Authentication

**Location:** `src/AppInstallerCommonCore/Authentication/`

Contains authentication mechanisms that work over secure connections:
- `Authentication.cpp` - Core authentication framework
- `WebAccountManagerAuthenticator.cpp` - Web account integration
- All authentication data transmitted over HTTPS

## Security Flow for Installer Downloads

### Step-by-Step Process:

1. **Connection Initialization:**
   - `InternetOpen()` creates session with Windows networking
   - Inherits Windows TLS/SSL configuration
   - Configures proxy if specified

2. **URL Opening:**
   - `InternetOpenUrl()` opens HTTPS URL
   - WinINet automatically:
     - Negotiates TLS connection
     - Validates server certificate against Windows Certificate Store
     - Establishes encrypted connection

3. **Certificate Validation (if configured):**
   - For REST APIs: Custom certificate pinning via `HttpClientHelper`
   - Validates certificate against pinning configuration
   - Ensures connection is to expected server

4. **Secure Data Transfer:**
   - `InternetReadFile()` reads encrypted data
   - WinINet automatically decrypts data
   - Data integrity verified via TLS

5. **Hash Verification:**
   - SHA256 hash computed during download (Downloader.cpp, line 230)
   - Validates installer integrity after download
   - Prevents tampering even if TLS compromised

## Summary of Secure Connection Locations

| Component | Primary Location | Purpose |
|-----------|-----------------|---------|
| Main Download (WinINet) | `src/AppInstallerCommonCore/Downloader.cpp` | Primary installer download with HTTPS |
| DO Download | `src/AppInstallerCommonCore/DODownloader.cpp` | Alternative download with HTTPS |
| Certificate Pinning | `src/AppInstallerSharedLib/Certificates.cpp` | Advanced certificate validation |
| HTTP Client | `src/AppInstallerCommonCore/HttpClientHelper.cpp` | REST API with certificate pinning |
| Certificate Resources | `src/CertificateResources/` | Embedded trusted certificates |
| Network Settings | `src/AppInstallerCommonCore/NetworkSettings.cpp` | Network configuration |

## Security Guarantees

WinGet's secure connection implementation provides:

1. **Transport Security:** All connections use HTTPS with TLS encryption
2. **Certificate Validation:** Server certificates validated against Windows Certificate Store
3. **Certificate Pinning:** Additional validation for critical connections (REST APIs)
4. **Integrity Verification:** SHA256 hashing of downloaded files
5. **Windows Integration:** Leverages OS-level security features and certificate management

## Related Error Codes

- `APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH` - Certificate pinning validation failed
- `HTTP_STATUS_*` - Various HTTP status codes handled in Downloader.cpp
- WinINet/WinHTTP error codes - Passed through from underlying APIs
