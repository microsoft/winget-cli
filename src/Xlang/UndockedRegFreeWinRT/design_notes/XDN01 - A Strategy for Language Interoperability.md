---
id: XDN01
title: A Strategy for Language Interoperability
author: hpierson@microsoft.com
status: draft
---

# XDN01 - A Strategy for Language Interoperability

- Harry Pierson (hpierson@microsoft.com)
- Status: Draft

## Abstract

This design note describes the goals and guiding principles of the xlang project. This document is
aspirational: It describes the project and its features as it has been envisioned. Some of the
features or characteristics described have not yet been implemented yet.

## Overview

xlang is a system that enables a component written in any supported language to be callable from any
other supported language using object oriented programming constructs. Components are written using
natural and familiar code in the implementation language and clients invoke those components using
constructs familiar to the language that they are consuming the component from. xlang projections
use metadata that describes the component's interface to translate from one language to another
using a common binary interface.

This project is based on an approach originally developed for Windows 8 called the Windows Runtime.
The project adopts many concepts from that work, enabling language interoperability on numerous
operating systems beyond the Windows family.

This is an open source project that welcomes community contributions. See the license & readme files
in the root of this project for details on how to contribute.

## Conceptual Model

In xlang, language interoperability is achieved via a combination of language independent metadata,
binary interface standards, language-specific projections, and a shared runtime. The
interoperability enables bidirectional control & data flow among any number of programming languages
within a process.

### Common Type System

Components are written using the type system of the implementation language and accessed using the
type system of the consuming language. As such, the xlang type system is designed to interoperate
with modern object-oriented programming languages. This includes both static (such as C#) and
dynamic (such as Python) object-oriented languages as well as strongly typed (such as C++) and
weakly typed (such as JavaScript) languages.

xlang's type system is _not_ designed to be accessed directly, except by language projections. The
underlying type system includes concepts such as resource ownership semantics, error propagation
semantics, and binary-stable interfaces that component authors and consumers do not need to be aware
of.

More details in [XDN03 - xlang Type System](XDN03%20-%20xlang%20Type%20System.md).

### Application Programming Interface (API) Metadata

All xlang APIs are described in machine-readable metadata stored in the industry standard [ECMA-335
format](https://www.ecma-international.org/publications/standards/Ecma-335.htm). This metadata is
used by both the consuming & producing projections. The data may be used at build time, run time or
both depending on the nature of the projection. The metadata is the definitive description of the
available APIs and how to call them. Metadata also provides information about dependencies and versioning.

More details in [XDN04 - xlang Metadata](XDN04%20-%20xlang%20Metadata.md).

### Application Binary Interface (ABI)

The application binary interface is a specification of how the machine state is set to transfer
control from a caller to a callee, irrespective of programming language. This includes register and
stack layout of parameters, calling conventions, and ownership transfer semantics for pointers to
memory off the stack. For convenience, these semantics are expressed in terms of carefully crafted C
or C++ function specifications that avoid ambiguities that might arise from different compiler or
processor architecture factors.

While the ABI is critical to enable interoperability among languages, most developers creating
components do not need to be aware of the ABI. Projections provide an adaptation layer that map
natural and familiar concepts from each programming language to the underlying ABI.

More Details in [XDN05 - xlang Binary Interface](XDN05%20-%20xlang%20Binary%20Interface.md).

### Language Projections

A projection converts calls made using familiar concepts in one programming language into a calls
that, on the machine's call stack and in machine registers, conforms to the application binary
interface. On the other side of the call, another projection converts those calls into a call
tailored to the language that the library was implemented in. As a simple example, this means that
you could call an API in C# using a System.String, and the C++ implementation would receive a std::string_view.

In some compiled languages, this translation may involve a few simple type conversions, or
potentially no work at all. In interpreted languages, this can sometimes involve more complex
transformations. For example, JavaScript has a very different notion of numbers than most other
languages. However, the ABI is specifically tailored to minimize the cost and data loss in these conversions.

### Platform Adaptation Layer

The ABI is defined in terms of specific machine layout of call data, including the binary formats of
data types. However, a few specific operations and types are intentionally opaque and implemented in
the platform adaptation layer. This enables contextual optimizations of the behavior of those types,
hides details that might be system-dependent and provides canonical implementations of behavior that
might otherwise be implementation dependent for a specific language or tool chain. The runtime
handles ownership and primitive operations such as cross-language string representation, thread
management, object activation, error origination, and shared memory lifetime management. The runtime
itself is exposed as flat C APIs. All components that interoperate with each other must share the
same platform adaptation layer.

More Details in [XDN06 - xlang Platform Abstraction Layer](XDN06%20-%20xlang%20Platform%20Abstraction%20Layer.md).

### Developer Tools

For most language projections, code generation or preprocessing is required to create a binding from
the language-specific code to the ABI. The tooling developed for this project supports a variety of
operating systems and languages. Key components, such as metadata reading, have been factored out to
enable reuse and rapid development of new language projections.
