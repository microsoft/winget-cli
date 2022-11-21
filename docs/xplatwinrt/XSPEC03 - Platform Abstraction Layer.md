---
title: Platform Abstraction Layer
author: ryansh@microsoft.com
---

# Title: XSPEC03 - Platform Abstraction Layer

- Author: Ryan Shepherd (ryansh\@microsoft.com)

- Status: Draft

## Abstract

This document describes the platform abstraction layer (or PAL) for Xlang.

## Overview

Xlang language projections depend on language-agnostic functionality that is provided by the underlying platform. Activating types, allocating shared cross-module memory and strings all require some built in functionality that's provided for app code and/or language projections.

The C language binding will be the lingua franca for the ABI underpinning all other language projections and interop. In this document, therefore, function syntax will be declared in the C language.

## Types and conventions

For low-level type definitions, see the [ABI specification](XSPEC04%20-%20Application%20Binary%20Interface.md).

The ABI design notes also have XlangResult values used by the PAL, and ABI definitions of XlangString. Some types specific to the PAL are described below.

### XlangStringHeader

This is a structure used to track the lifetime of _fast-pass_ XlangStrings. See [XlangCreateStringReference](#Xlangcreatestringreference) for more details.

Its layout and size is platform-defined.

### XlangStringBuffer

This type holds a preallocated string buffer for subsequent promotion into a **XlangString**. This type is the size of a pointer, but distinct from other pointer types, as if defined as:

```c
typedef struct XlangStringBuffer__
{
    int unused;
} XlangStringBuffer__;
typedef XlangStringBuffer__* XlangStringBuffer;
```

## Shared memory

Xlang code will, at times, require some memory to be dynamically allocated and passed between in-process components. _ReceiveArray_ method parameters are a common example of this pattern.

Because these components may use different different allocators, runtime heaps, or even different language projections, Xlang provides a dedicated facility to guarantee these allocations and deallocations are performed in a consistent manner.

This also precludes the possibility of statically-linking the PAL, as each component carrying a statically-linked PAL would defeat the purpose of having a dedicated, single module managing this memory.

Note that at this time, no guarantees are being made regarding cross-process shared memory. This functionality exists solely for intra-process shared memory.

### XlangMemAlloc

Allocates a block of memory.

#### Syntax

```c
void* __stdcall XlangMemAlloc(size_t count);
```

#### Parameters

- count - The size of the memory block to be allocated, in bytes. If count is 0, XlangMemAlloc attempts to allocates a zero-length item and return a valid pointer to that item.

#### Return value

If the function succeeds, it returns the allocated memory block. Otherwise, it returns **NULL**.

#### Remarks

To free this memory, call [XlangMemFree](#Xlangmemfree).

This function is thread-safe: it behaves as though only accessing the memory locations visible through its argument, and not any static storage. In other words, the same thread safety guarantees as malloc in C11 and C++11.

### XlangMemFree

Frees a block of memory that was allocated by [XlangMemAlloc](#Xlangmemalloc)

#### Syntax

```c
void __stdcall XlangMemFree(void* ptr);
```

#### Parameters

- ptr - A pointer to the memory block to be freed. If this parameter is **NULL**, the function has no effect.

#### Return value

This function does not return a value.

#### Remarks

This function is thread-safe: it behaves as though only accessing the memory locations visible through its argument, and not any static storage. In other words, the same thread safety guarantees as free in C11 and C++11.

### XlangStringEncoding

This is an enum representing the possible character encodings in a given XlangString. Its underlying type is an unsigned 32-bit integer.

This is a "flags" enum, meaning that an instance of this type is meant to hold some combination of value bitwise or'ed together.

#### Syntax

```c
enum XlangStringEncoding
{
    ENCODING_UTF8 = 0x1,
    ENCODING_UTF16 = 0x2
};
```

## String functions

Passing and sharing strings across components presents some of same challenges as dynamically allocated memory, with respect to matching allocation and deallocation. Therefore, Xlang has support for allocating strings for cross-component interop.

### Encoding

Xlang strings can be encoded in either UTF-8 or UTF-16, using C's char or char16_t, respectively. Functions that accept and/or return character data will have _U8 or_ U16 suffixes to disambiguate the two "flavors" of function call.

If a string is created with one encoding, and then the underlying buffer data for a different encoding is requested, the PAL will automatically perform a conversion. Callers wishing to avoid the overhead of the conversion can query the encoding state of the string and retrieve data in an encoding that would not require a conversion.

### API overview

Xlang strings are immutable and hidden behind an opaque handle: **XlangString**. The underlying string data can only be accessed through Xlang string APIs via this handle.

Call [XlangCreateString](#Xlangcreatestring) to create a new **XlangString**, and call [XlangDeleteString](#Xlangdeletestring) to release the reference to the backing memory.

Copy an **XlangString** by calling [XlangDuplicateString](#Xlangduplicatestring).

The underlying character data can be accessed by calling [XlangGetStringRawBuffer](#Xlanggetstringrawbuffer).

Call [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer) to allocate a mutable string buffer that you can then promote into an immutable **XlangString**. When you have finished populating the buffer, you can call [XlangPromoteStringBuffer](#Xlangpromotestringbuffer) to convert that buffer into an immutable **XlangString**, or call [XlangDeleteStringBuffer](#Xlangdeletestringbuffer) to discard it prior to promotion. This two-phase construction has similar functionality to a "string builder" found in other libraries.

Xlang also supports creating "fast pass" strings by calling [XlangCreateStringReference](#Xlangcreatestringreference). In this case, the memory containing the backing string data is owned by the caller, and not allocated on the heap. Therefore, Xlang relies upon the caller to maintain the backing string data, unchanged, for the duration of the string's lifetime.

Semantically, a **XlangString** containing the value **NULL** represents the empty string, which consists of zero content characters and a terminating null character. Calling [XlangCreateString](#Xlangcreatestring) with zero characters will produce the value **NULL**. Calling [XlangGetStringRawBuffer](#Xlanggetstringrawbuffer) with **NULL** will return an empty string followed only by the null terminating character.

### XlangCreateString

Creates a new XlangString.

#### Syntax

```c
XlangResult XlangCreateStringU8(
    char const* sourceString,
    uint32_t length,
    XlangString* string
);

XlangResult XlangCreateStringU16(
    char16_t const* sourceString,
    uint32_t length,
    XlangString* string
);
```

#### Parameters

- sourceString - A string to use as the source for a new **XlangString**. To create a new empty, or **NULL** string, pass **NULL** for _sourceString_ and 0 for _length_.

- length - The length of the string, in code units. In other words, the number of elements pointed to by _sourceString_, not counting the null-terminator. Must be 0 if _sourceString_ is **NULL**.

- string - A pointer to the newly created **XlangString**, or **NULL** if an error occurs.

#### Return value

Return code            | Description
---------------------- | ------------------------------------------------------
Xlang_OK               | The XlangString was created successfully.
Xlang_INVALID_ARG      | _string_ was **NULL**.
Xlang_OUTOFMEMORY      | Failed to allocate memory for the new XlangString.
Xlang_POINTER          | _sourceString_ was **NULL** and _length_ was non-zero.
Xlang_MEM_INVALID_SIZE | The requested allocation size was too large.

#### Remarks

Xlang copies _length_ elements from _sourceString_ to the backing buffer of the new **XlangString**, plus a null-terminator.

Call [XlangDeleteString](#Xlangdeletestring) to deallocate the string. Each call to **XlangCreateString** must be matched by a call to **XlangDeleteString**.

To create a _fast pass_ string without a heap allocation or copy, call [XlangCreateStringReference](#Xlangcreatestringreference).

To create a new empty or **NULL** string, pass **NULL** for _sourceString_ and 0 for _length_.

The backing buffer of this string will be managed by a thread-safe reference count.

### XlangCreateStringReference

Create a _fast-pass_ string based on the supplied string data.

#### Syntax

```c
XlangResult XlangCreateStringReferenceU8(
    char const* sourceString,
    uint32_t length,
    XlangStringHeader* header,
    XlangString* string
);

XlangResult XlangCreateStringReferenceU16(
    char16_t const* sourceString,
    uint32_t length,
    XlangStringHeader* header,
    XlangString* string
);
```

#### Parameters

- sourceString - A null-terminated string to use as the source. **NULL** represents the empty string if _length_ is 0.

- length - The length of the string in code units. Must be 0 if _sourceString_ is **NULL**. Otherwise, _sourceString_ must have a terminating null character.

- header - A pointer to a [XlangStringHeader](#Xlangstringheader) structure that Xlang uses to identify _string_ as a _fast-pass_ string.

- string - A pointer to the newly created string, or **NULL** if an error occurs. This string will be a _fast-pass_ string.

#### Return value

Return code                      | Description
-------------------------------- | --------------------------------------------------------
Xlang_OK                         | The _fast-pass_ **XlangString** was created successfully.
Xlang_INVALID_ARG                | Either _string_ or _header_ was **NULL**.
Xlang_STRING_NOT_NULL_TERMINATED | _string_ was not null-terminated.
Xlang_POINTER                    | _sourceString_ was **NULL** and _length_ was non-zero.

#### Remarks

Use this function to create a _fast-pass_ string. Unlike a string created by [XlangCreateString](#Xlangcreatestring), the lifetime of the backing buffer is not managed by Xlang. The caller allocates _sourceString_ on the stack frame, together with an uninitialized [XlangStringHeader](#Xlangstringheader), to avoid a heap allocation. The caller must ensure that _sourceString_ and the contents of _header_ remain unchanged during the lifetime of the attached **XlangString**.

Strings created with this function need to be deleted with [XlangDeleteString](#Xlangdeletestring).

### XlangDeleteString

Deletes a XlangString.

#### Syntax

```c
void XlangDeleteString(XlangString string);
```

#### Parameters

- string - The **XlangString** to delete.

- If _string_ is a fast-pass string created by [XlangCreateStringReference](#Xlangcreatestringreference) or **NULL**, no action is taken.

#### Return value

This function does not return a value.

#### Remarks

This function decrements the reference count of the backing buffer. If the reference count reaches 0, the buffer will be deallocated.

### XlangDeleteStringBuffer

Discards a preallocated string buffer if it was not promoted to a **XlangString**.

#### Syntax

```c
XlangResult XlangDeleteStringBuffer(
    XlangStringBuffer bufferHandle
);
```

#### Parameters

- bufferHandle - The buffer to discard.

#### Return value

Return code       | Description
----------------- | ------------------------------------------------------------------------------------------------
Xlang_OK          | The buffer was discarded successfully.
Xlang_POINTER     | _bufferHandle_ is **NULL**.
Xlang_INVALID_ARG | _bufferHandle_ was not created by [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer).

#### Remarks

Call this function to discard a string buffer that was created by [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer), but has not been promoted to an **XlangString** by the [XlangPromoteStringBuffer](#Xlangpromotestringbuffer) function.

Calling **XlangPromoteStringBuffer** after calling **XlangDeleteStringBuffer** is undefined.

### XlangDuplicateString

Creates a copy of the specified string.

#### Syntax

```c
XlangResult XlangDuplicateString(
    XlangString string,
    XlangString* newString
);
```

#### Parameters

- string - The source string to be copied.

- newString - The copy of _string_.

#### Return value

Return code       | Description
----------------- | ------------------------------------------------------
Xlang_OK          | The **XlangString** was copied successfully.
Xlang_INVALID_ARG | _newString_ was **NULL**.
Xlang_OUTOFMEMORY | Failed to allocate memory for the new **XlangString**.

#### Remarks

Use this function to copy a **XlangString**.

If _string_ was created by calling [XlangCreateString](#Xlangcreatestring), the reference count of the backing buffer is incremented, and _newString_ uses the same backing buffer.

If _string_ was created by calling [XlangCreateStringReference](#Xlangcreatestringreference), (implying it is a _fast-pass_ string), Xlang copies the source string to a new buffer with a new reference count. The resulting copy has its own backing buffer and is not a _fast-pass_ string.

Each call to **XlangDuplicateString** must be matched with a corresponding call to [XlangDeleteString](#Xlangdeletestring).

### XlangGetStringEncoding

Get the encodings present in a **XlangString**.

#### Syntax

```c
XlangStringEncoding XlangGetStringEncoding(
    XlangString string
);
```

#### Parameters

- string - The string to be queried.

#### Return value

A combination of one or more of the values of the XlangStringEncoding enum.

### XlangGetStringRawBuffer

Get the backing buffer for the specified string.

#### Syntax

```c
XlangResult XlangGetStringRawBufferU8(
    XlangString string,
    char const* * buffer,
    uint32_t* length
);

XlangResult XlangGetStringRawBufferU16(
    XlangString string,
    char16_t const* * buffer,
    uint32_t* length
);
```

#### Parameters

- string - The string for which the backing buffer is to be received.

- buffer - Receives a pointer to the backing store. Receives **NULL** if _string_ is **NULL** or the empty string.

- length - Receives the number of code units in the string, excluding the null terminator, or 0 if _string_ is **NULL** or the empty string.

#### Return value

Return code       | Description
----------------- | ---------------------------------------------
Xlang_OK          | Success.
Xlang_OUTOFMEMORY | Failed to allocate memory for the new buffer.

#### Remarks

If the string doesn't currently have backing data in the requested encoding (for example, UTF-16 data requested from a string originally created from UTF-8 data), a conversion will be performed on demand at this time.

Do not change the contents of the buffer.

### XlangPreallocateStringBuffer

Allocates a mutable character buffer for use in string creation.

#### Syntax

```c
XlangResult XlangPreallocateStringBufferU8(
    uint32_t length,
    char** charBuffer,
    XlangStringBuffer* bufferHandle
);

XlangResult XlangPreallocateStringBufferU16(
    uint32_t length,
    char16_t** charBuffer,
    XlangStringBuffer* bufferHandle
);
```

#### Parameters

- length - The size of the buffer to allocate, in elements. A value of zero corresponds to the empty string.

- charBuffer - Receives the mutable buffer that holds the characters. The buffer already contains a terminating null character.

- bufferHandle - Receives the preallocated string buffer.

#### Return value

Return code            | Description
---------------------- | ---------------------------------------------
Xlang_OK               | Success.
Xlang_OUTOFMEMORY      | Failed to allocate memory for the new buffer.
Xlang_POINTER          | _charBuffer_ or _bufferHandle_ was **NULL**.
Xlang_MEM_INVALID_SIZE | The requested allocation size was too large.

#### Remarks

Use this function to create a mutable character buffer that you can manipulate prior to promoting it into an immutable **XlangString**. When you have finished populating the character buffer, call [XlangPromoteStringBuffer](#Xlangpromotestringbuffer) to create the **XlangString**.

Call [XlangDeleteStringBuffer](#Xlangdeletestringbuffer) to discard the buffer prior to promotion. If the buffer has already been promoted by a call to **XlangPromoteStringBuffer**, call [XlangDeleteString](#Xlangdeletestring) to discard the string. If **XlangPromoteStringBuffer** fails, you can call **XlangDeleteStringBuffer** to discard the buffer.

### XlangPromoteStringBuffer

Creates a **XlangString** from the specified **XlangStringBuffer**.

#### Syntax

```c
XlangResult XlangPromoteStringBuffer(
    XlangStringBuffer bufferHandle,
    XlangString* string,
    uint32_t length
);
```

#### Parameters

- bufferHandle - The buffer to use for the new string. You must call [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer) to create this.

- string - The newly created XlangString that contains the contents of _bufferHandle_.

- length - The length of the string in _bufferHandle_, not counting the null terminator. This value must be less than or equal to the length passed to [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer).

#### Return value

Return code       | Description
----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Xlang_OK          | Success.
Xlang_POINTER     | _string_ was **NULL**.
Xlang_INVALID_ARG | _bufferHandle_ was not created by calling [XlangPreallocateStringBuffer](#Xlangpreallocatestringbuffer), or the caller has overwritten the terminating null character in _bufferHandle_, or _length_ is greater than the _length_ the buffer was created with.

#### Remarks

Calling this function converts the mutable buffer to an immutable **XlangString**.

If this function fails, you can use [XlangDeleteStringBuffer](#Xlangstringbuffer) to discard the mutable buffer.

Each call to **XlangPromoteStringBuffer** must be matched with a corresponding call to [XlangDeleteString](#Xlangdeletestring).

## Activation

The PAL exports one function **xlang_get_activation_factory** for apps to request activation factories for classes.

Components implementing factories are expected to export a single function **xlang_lib_get_activation_factory** which the PAL will call to retrieve said factories, as described in the Activation Design Note.
This behavior and signature of this function is documented here as well, even though it's not actually a function exported by the PAL.

### xlang_get_activation_factory

Retrieves an activation factory from the platform or from another component.

#### Syntax
```c
xlang_result __stdcall xlang_get_activation_factory(
    xlang_string class_name,
    GUID const& iid,
    void** factory
);
```

#### Parameters
- class_name - The name of the class.
- iid - The unique identifier (GUID) of the factory interface being requested.
- factory - The out parameter receiving the factory

#### Return value
If the function succeeds, it returns **xlang_error_ok**.

#### Remarks
When this function is called, the PAL will attempt to find and load the library implementing the factory, and call **xlang_lib_get_activation_factory** on that library to retrieve the requested factory.

### xlang_lib_get_activation_factory

The PAL does not implement this function. This function is implemented in a library/component, and is called by the PAL.

Retrieves the activation factory from a library that implements the specified class.

#### Syntax
```c
xlang_result __stdcall xlang_lib_get_activation_factory(
    xlang_string class_name,
    GUID const& iid,
    void** factory
);
```

#### Parameters
- class_name - The name of the class.
- iid - The unique identifier (GUID) of the factory interface being requested.
- factory - The out parameter receiving the factory

#### Return value
If the function succeeds, it returns **xlang_error_ok**.
