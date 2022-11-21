---
title: Application Binary Interface
author: ryansh@microsoft.com
---

# Title: XSPEC04 - Application Binary Interface

- Author: Ryan Shepherd (ryansh\@microsoft.com)

- Status: Draft

## Abstract

This document describes the Application Binary Interface (or ABI) for Xlang.

## Overview

Xlang language projections depend on a stable binary interface for both interacting with the Platform Abstraction Layer (PAL) and other components.

The C programming language is the lingua franca for describing this interface. This document describes how the Xlang type system maps to C types.

## Types and conventions

The following fundamental Xlang types map to C types as follows. These correspond to the fundamental types specified in [XSPEC01](XSPEC01%20-%20Type%20System%20Specification.md).

Xlang Type | C type
---------- | ---------------------------------------------------
Int16      | int16_t
Int32      | int32_t
Int64      | int64_t
UInt8      | uint8_t
UInt16     | uint16_t
UInt32     | uint32_t
UInt64     | uint64_t
Single     | float
Double     | double
Char16     | char16_t
Boolean    | uint8_t (C99 does not guarantee _Bool to be 8 bits)

- void* - A native pointer type to "unknown type". This implies that the pointer size is sufficient to address arbitrary memory on the target architecture - 32 bits in 32-bit code, 64 bits in 64-bit code.

- size_t - An unsigned integer type, capable of storing the size of any object. Generally, this implies it is the same size as a pointer, and will be uint32_t or uint64_t on 32-bit or 64-bit platforms, respectively.

### XlangString

Strings are represented by an opaque handle type. This type is the size of a pointer, but distinct from other pointer types, as if defined as:

```c
typedef struct XlangString__
{
    int unused;
} XlangString__;
typedef XlangString__* XlangString;
```

### Calling conventions

For non-member (i.e. free) functions, the 32-bit x86 calling convention will be specified in the function syntax declaration. This will be either **_stdcall_ or ***cdecl*. These calling conventions shall have no effect on other architectures and can be safely ignored in those cases.

### XlangResult

To represent success or failure, functions will generally return an XlangResult. This data type is a 32-bit signed integer, or **uint32_t** where the most significant bit will be set to 0 on success, or 1 on failure. The other 31 bits have a platform-dependent meaning.

In general, when an XlangResult indicating error is returned from a function, language projections are expected to translate this value into an appropriate exception type.

Some commonly used XlangResult error codes, including those that can be returned by the [PAL](XSPEC03%20-%20Platform%20Abstraction%20Layer.md) include:

XLangResult                      | Value      | Description
-------------------------------- | ---------- | -------------------------------------------
XLANG_OK                         | 0          | Success
XLANG_INVALID_ARG                | 0x80070057 | One or more arguments are not valid
XLANG_OUT_OF_MEMORY              | 0x8007000E | Failed to allocate memory
XLANG_POINTER                    | 0x80004003 | A pointer is not valid
XLANG_STRING_NOT_NULL_TERMINATED | 0x80000017 | A string was not null-terminated
XLANG_MEM_INVALID_SIZE           | 0x80080011 | The requested allocation size was too large

On Windows platforms, this type is identical to **HRESULT**, with the same values.
