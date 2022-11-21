---
id: XDN10
title: Platform Support for Activation Factories
author: ryansh@microsoft.com
status: Draft
---

# Title: XDN10 - Platform Support for Activation factories
* Author: Ryan Shepherd (ryansh@microsoft.com)
* Status: Draft

## Abstract

This document describes how the Platform Abstraction Layer (PAL) supports and integrates with xlang activation factories. 

## Overview

As described in the Type System Specification, a class that is activatable, composable, and/or has static methods requires an activation factory.

When consuming code needs an instance of a factory, it should not require knowledge of which component implements that factory, nor should it be responsible for dynamically loading that module and searching for exports.

For that reason, consuming code will request factories through the PAL.
The PAL will consult its mapping of class names to component libraries, load (if necessary) the library, and then call the exported method on the library, returning the result to the caller.

This implies three pieces of functionality that work together to support activation:
* A function exported by the PAL for client code to request factories.
* A function exported by components to retrieve instances of its factories.
* A scheme for the PAL to build a mapping of class names to component libraries.

## Exporting factories from components

For the PAL to retrieve factories, xlang components must implement and export a function, **xlang_lib_get_activation_factory**, which accepts a class name in string form and the GUID of the requested interface, and returns the newly created class factory.
Components providing factories are expected to correctly handle calls to this function from multiple threads concurrently.

For components providing class factories, it is risky to have factories with state that depends on other components.
Because factories are requested and initialized in response to requests to construct a runtime class, a dependency cycle between components' factories could easily lead to their initialization methods being called reentrantly.
This problem is easily avoided, and correct multithreaded behavior is simpler, if class factories are stateless.

## Retrieving factory interfaces

The PAL provides the function **xlang_get_activation_factory**, which accepts as input the class name in string form, and the GUID of the requested interface.
Upon success, the output is the requested interface.

This function may fail if:
* The PAL can not map the class name to a component.
* The PAL can not load the mapped component.
* The PAL loads the component, but can not locate the exported **xlang_lib_get_activation_factory** function to retrieve the component's factories.
* The component itself fails to return the requested factory.

## Mapping classes to components

This functionality is expected to evolve as xlang's application model matures, with new behaviors and abstractions being added as appropriate.

Upon a request for an activation factory, or in other words, when a class name is passed to **xlang_get_activation_factory**, the PAL will attempt to load the owning component in the following manner:
* Obtain the innermost namespace scope from the class name
* Look for a library in the application's current directory with a matching file name (minus extension).
* If it exists:
  * Load that library and the entry point function **xlang_lib_get_activation_factory**.
  * Call that function with the supplied class name and GUID, and return the result if valid.
* If the library returns a null factory - the library is not responsible for implementing that factory - or a library matching that namespace does not exist, move out one nested namespace and repeat.
* If an attempt at the top level namespace fails to produce a factory, fail and return null.

As an illustrative example, imagine an app requests a factory for the class *MyComponent.Feature.Widget*. A possible sequence of events would be:
* The PAL searches for the library *MyComponent.Feature.Widget.(dll/so)* (on Windows/Linux, respectively).
* This library is not found in the application's directory, so it moves up one namespace level and:
* The PAL then searches for the library *MyComponent.Feature.(dll/so)*. This library exists, and so the PAL loads it, calls **xlang_lib_get_activation_factory**, passing the string *"MyComponent.Feature.Widget"*.
* This call succeeds, and the PAL returns the resulting factory.

## Built in class names

All class names in the "xlang" namespace or one of its nested namespaces are reserved for the xlang runtime.
Attempting to map a reserved class name to a different component is a failure.
Projects wanting to override or "mock" built in classes for testing purposes should instead intercept calls to **xlang_get_activation_factory**.

PAL implementations running on platforms that support the Windows Runtime (i.e. Windows 8 and later) will forward requests for all class names under the "Windows" namespace directly to the OS function, **RoGetActivationFactory**.
All requests for factories that fail, either due to not locating a matching library, or due to no loaded libraries implementing the factory, will also forward those requests to **RoGetActivationFactory**.
