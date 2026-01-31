# Quick Reference: Secure Connection Code Locations

This document provides a quick reference to answer: "Where is the code in WinGet to enable a secure connection to download an installer?"

## Main Installer Download (Primary)

**File:** `src/AppInstallerCommonCore/Downloader.cpp`

**Key Lines:**
- Lines 146-151: `InternetOpen()` - Initialize WinINet session with proxy (includes automatic HTTPS/TLS support)
- Lines 155-160: `InternetOpen()` - Initialize WinINet session without proxy (includes automatic HTTPS/TLS support)
- Lines 178-184: `InternetOpenUrl()` - Open secure HTTPS connection with security flags
- Line 247: `InternetReadFile()` - Read encrypted data over secure connection

**How it works:** Uses Windows WinINet API which automatically:
- Negotiates TLS connection
- Validates server certificates via Windows Certificate Store
- Establishes encrypted channel

## Certificate Pinning (Additional Security)

**Files:**
- **Header:** `src/AppInstallerSharedLib/Public/winget/Certificates.h`
- **Implementation:** `src/AppInstallerSharedLib/Certificates.cpp`
- **Usage:** `src/AppInstallerCommonCore/HttpClientHelper.cpp` (lines 28-44: `NativeHandleServerCertificateValidation` function)

**How it works:** 
- Extracts server certificate using `WinHttpQueryOption()`
- Validates against pre-configured pinning rules
- Rejects connections if certificate doesn't match expected values

## Certificate Resources

**Location:** `src/CertificateResources/`

Contains Microsoft root certificates used for pinning validation:
- `Microsoft_TLS_ECC_Root_G2.crt`
- `Microsoft_TLS_RSA_Root_G2.crt`

## Alternative Download Method

**File:** `src/AppInstallerCommonCore/DODownloader.cpp`

Uses Windows Delivery Optimization service (also HTTPS-based)

## Summary

**Primary secure connection code:** `src/AppInstallerCommonCore/Downloader.cpp` using WinINet API

**Enhanced security (certificate pinning):** `src/AppInstallerSharedLib/Certificates.cpp` and `src/AppInstallerCommonCore/HttpClientHelper.cpp`

For detailed architecture information, see [SecureConnectionArchitecture.md](SecureConnectionArchitecture.md).
