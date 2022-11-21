---
id: XDN06
title: xlang Platform Abstraction Layer
author: hpierson@microsoft.com
status: draft
---

# Title: XDN06 - xlang Platform Abstraction Layer

- Author: Harry Pierson (hpierson@microsoft.com)
- Status: draft

## Abstract

This design note describes xlang's [platform abstraction layer](https://en.wikipedia.org/wiki/Abstraction_layer)
(or PAL). The PAL contains the core functionality needed by xlang components and language
projections, regardless of the underlying platform. Examples of this functionality include
activating instances of [xlang types](XDN03%20-%20xlang%20Type%20System.md), propagating errors
across binary or language boundaries and managing memory across binary modules.

## Activation

### Factory Caching

## String Manipulation

## Memory Management

## Error Origination and Propagation

## Threading

## Numerics