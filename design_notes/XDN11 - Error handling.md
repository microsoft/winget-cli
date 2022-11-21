---
id: XDN11
title: Error handling
author: mawign@microsoft.com
status: Draft
---

# XDN11 - Error handling

- Author: Manodasan Wignarajah (mawign@microsoft.com)
- Status: Draft

## Abstract

This document describes the error handling story for xlang and what it will support.

## Overview

As described in XDN01, xlang is a system that allows a component written in a supported language
to be callable from any other supported language. Upon calling a component, errors can occur and
can play a different role based on the audience and the situation.

Errors are used by component implementers to indicate both expected and unexpected issues. Expected
issues are situations that a component implementer foresees happening in a normal scenario such as
not being able to open a file and wants to communicate it to the consumer. In these cases, xlang
recommends the use of the Try Pattern to communicate these issues. Unexpected issues are situations
that are not foreseen in a normal scenario such as a memory allocation error. In these cases, a
component implementer may use custom logging to log the issue to help them with diagnosing it.
They can also propagate the error back to the consumer and xlang will translate it from the
component's language to the consumer's language.

Errors are also used by projection implementers to indicate an issue while translating a call from
one language to the other. These errors are also translated by xlang to the consumer's language.

Errors from components and projections are received by the consumer and either handled, propagated,
or result in a failure. If it results in a failure, the failure might be debugged by a developer,
logged by the application, or captured in a dump. To help facilitate this in a way that the developer
has information available to them to diagnose these translated errors, xlang producing projections
retrieve the error context captured by the component's language and store it as part of the
xlang error info when it is originated. This xlang error info is then stored with the consumer's
language error by the consuming projection and is available to be logged and captured. If the consumer
propagates the error, the error may end up traveling to another xlang projected component before
reaching the failure point. To facilitate diagnosing these failures, xlang allows any producing
projections that the error propagates through to append additional error context captured by that
language to the existing xlang error info. The information together shows how the error propagated
and resulted in a failure.

![Error origination diagram](XDN11_origination_diagram.png)
Diagram highlighting the error handling of a C# app calling a Python API that calls a Java API that results in an error.

## Guidance for Projection Authors

- **DO** translate language errors to equivalent xlang errors and xlang errors to equivalent natural
language errors at the projection boundary.
- **DO NOT** let language errors (exceptions) propagate past the projection boundary.
- **DO** invoke `xlang_originate_error` when receiving a language error with no associated xlang error info.
- **DO** invoke `xlang_originate_error` when returning a new xlang error as part of the projection
implementation.
- **DO** invoke `PropagateError` when receiving a language error with an associated xlang error info.
- **DO** retrieve the diagnostic information captured by the language for the error and provide it
to `xlang_originate_error` and `PropagateError`.
- **DO** associate the xlang error info with the translated language error.
- **CONSIDER** providing projection-specific diagnostic information that would be useful to diagnose
failures.
- **DO** expose xlang's diagnostic information as part of the language specific error logging function
and upon crash.
- **DO** encourage component authors to follow the Try Pattern to communicate domain errors to the consumer.
- **DO** take ownership of the xlang error info using attach semantics.

## Translation of errors

xlang's goal is to allow a component to be written using natural and familiar code for the language
it is implemented in and for it to be consumed using natural and familiar code for the language which
the consumer is written in. To achieve this, xlang needs to translate errors from the natural form
of one language to a natural form of that error in another language.

But different languages have different errors which can occur and not all errors in one language
always directly map to errors from another language. In addition, maintaining a mapping of errors
between each supported language is not scalable. Lastly, there are a lot of errors which are domain
specific and may not make sense in other languages and thereby not have equivalent error codes natively
available. Because of this, xlang will only support a limited set of errors which can be used by the
projection of a language to map to or from its error. These errors are intended to be used to
indicate an error during the translation of the call and not to indicate errors from components.
Components are encouraged to avoid returning errors and use the Try Pattern as discussed later in
[Alternative to errors](#alternative-to-errors).

If the error being mapped has an equivalent in the set of errors and the projection of the consumer's
language provides an equivalent mapping from that mapped error, then that error will have a natural
mapping in the other language. In the case there isn't an equivalent error to map to in xlang, then a
generic error, `xlang_fail`, will be available to be used. Note that in this case, the natural mapping of
errors from one language to the other will be lost in the sense that it will not be an equivalent mapping.
In the case there isn't an equivalent error to map to in a consumer's language, it is left to the projection
of that language to decide what error from its language to map to based on what makes sense for that language.
This can end up being a generic error.

## xlang errors

xlang will define a limited set of errors that are scoped to ones that are likely to occur during the
translation of calls and ones that are considered fatal. The errors will be represented by an `UInt32`,
`xlang_result` and will be monotonically increasing.

`xlang_result`       | Value  | Description
-------------------- | ------ | ----------------------------------------------------------------------
xlang_success        | 0      | Operation succeeded (should not be originated)
xlang_access_denied  | 1      | Requested operation / call is not allowed
xlang_bounds         | 2      | Invalid index used such as when indexing arrays
xlang_fail           | 3      | Generic error when no equivalent or better error mapping is available
xlang_handle         | 4      | Invalid handle
xlang_invalid_arg    | 5      | Invalid argument in a call
xlang_invalid_state  | 6      | Call isn't allowed in the current state
xlang_no_interface   | 7      | Interface not found
xlang_not_impl       | 8      | Function / feature is not implemented
xlang_out_of_memory  | 9      | Allocation issue resulting from not enough memory
xlang_pointer        | 10     | Null pointer related errors
xlang_type_load      | 11     | Class not found or syntax / type error during runtime

These errors defined in xlang should get a natural mapping from projections if an equivalent error
exists in the language it projects.

## Alternative to errors

Even though xlang does provide a set of errors to assist with translating errors from one language
to another in an equivalent form, it is scoped to errors that occur in projections. This is because
xlang encourages components to use the Try Pattern instead of returning / throwing errors.

The main reason for this is when errors are translated from one language to another, there is a chance that
the accuracy of the error is lost due to an exact error mapping is not available in xlang or in the other
language. This means the consumer of an API will not always have the accurate and necessary information that
was available before translation to determine what the error represents and to decide what to do with it.
With the Try Pattern, custom domain specific errors can be passed back as parameters if necessary.
The parameter type and its possible values are declared in the producing component and would be projected
across languages as is. This means the consumer can rely on using it to make decisions without being
concerned of the translation accuracy.

But this isn't to say that all errors should be passed back via the Try Pattern. If a component intends
on returning / throwing an error which a consumer may use to control code flow, then the Try Pattern should
be used. For errors which are considered fatal like out of memory and invalid argument, it would be fine to
return / throw the error as that would allow for useful diagnostic information to be collected about it
which would be available upon failure.

## Diagnosing application failures

Errors coming from components written in other languages can be one of the causes of application
failures. Just like with any other failure, developers will want to diagnose where and why the
failure occurred. To assist with this, xlang will provide the ability for a projection to associate
diagnostic information with the error. The following information can be associated:

- Language specific error (i.e. error code, exception type)
- Error message
- Execution trace
- Projection identifier
- Any additional diagnostic information

Some of this information can be represented by different languages in different ways. For instance,
in some languages, execution traces can be an array of addresses while in others it can be a list of
function names. Due to this, xlang will store the information generically as strings and it will
be the responsibility of the projections to convert the representation it has to a string representation
that can be logged and is readable by a developer or tooling.

Errors may also end up traveling through several different components written in different languages
before becoming a failure. To assist with diagnosing these failures, xlang will allow a projection to
associate additional diagnostic information to the existing information as an error travels through it.
This will help a developer get a complete picture of the failure which wouldn't be available with just
the information collected from the language that initially returned the error. An example of this is
the execution trace. A language will typically only be aware of the function calls within its language
boundary and thereby the information collected at the initial error would only have a partial list of
function calls. The following additional information can be associated with the error as it travels
through projections:

- Execution trace
- Projection identifier
- Any additional diagnostic information

All of this diagnostic information along with the `xlang_result` will be stored in a xlang object
implementing the `IXlangErrorInfo` interface. This object will be known as the xlang error info.
As with any xlang object, it will be reference counted and will implement `IXlangObject`.
This will allow it to be easily accessed by projections written in different languages while still
being able to ensure the cleanup of it.

## How to indicate the result of a function

Functions declared at the xlang ABI (i.e. language projections, PAL) need a way to indicate whether
an error occurred. There are multiple ways to accomplish this, but the chosen way needs to allow for
the error info associated with the error to be retrieved later by the consuming projection when it
receives the error from the function call.

One approach considered is having the `xlang_result` as the return value and storing the error info
on a thread level storage with APIs available to set and retrieve it. This approach is similar to what
WinRT does and achieves what we want. But it has the problem where callers need to make sure they don't
unintentionally replace the error info stored on the thread level storage. For instance, lets say a
projection receives an error from a call it projected. It will then create the error info for the error
which gets stored on the thread level storage. It may then want to do some cleanup before returning the
error. Some of the cleanup might be implicitly done by destructors. Any of the APIs that get called
as part of the cleanup may result in an error and replace the error info stored on the thread level
storage by creating a new one for its error. But the projection, when it receives the error may decide to
ignore the error from the cleanup API as it was a best effort call and return the error received from the
component. Now we have a scenario where the error and the error info are mismatched and the useful error
info is lost. The only way a projection can prevent this is by retrieving the error info before doing
the cleanup and then setting it back when returning the error. But this is hard to implement in a way
that also solves the problem with destructors doing cleanup and will require effort by each
projection implementer to solve or avoid.

Another approach considered is having the error info itself as the return value. This avoids the problem
the previous approach had with the error and the error info being mismatched. But it does have the
problem of not being able to allocate an error info during out of memory situations and thereby not
being able to indicate there is an error. This problem can be avoided by having a statically allocated
error info for each xlang error with just the `xlang_result` stored in it. One of these error infos
would be used when a new error info can't be allocated. This allows us to still be able to indicate an
error occurred.

The last approach considered is a variant of the 2nd approach. Instead of returning the error info,
the error info is passed to the caller as an out parameter. This approach is very similar to the
2nd approach, but is harder to use with macros that make code easier to write such as the one that
propagates the error by returning automatically when there is an error (i.e. RETURN_IF_FAILED).

Given these advantages and disadvantages, the 2nd approach is chosen for xlang which is having the
error info as the return value.

```cpp
[[nodiscard]] IXlangErrorInfo* functionName(param, ...)
```

Returning a xlang error info indicates that the function resulted in an error. When returning the
xlang error info, the function should transfer its ownership of the object to the caller rather than
decrementing the reference count on it and the caller should assume ownership of it or propagate it.
To highlight this fact, the ABI functions are decorated with the `nodiscard` attribute to indicate the
return value shouldn't be ignored. If the caller would like to handle the error, then the caller should
take ownership of the error and release it using the conventions for reference counted xlang objects.
Returning `nullptr` or the literal value 0 indicates the function completed without errors.

## Handling language errors

Producing projections may encounter a language error when projecting an API call to a component.
The error will either be from the translation of the call to the component's language or be from
the component itself as a result of the call. Either way, upon encountering an error, the projection
should determine whether the error is new or is a propagation of a previous xlang error. Strategies
on doing this will be discussed in a later section.

If the error is new, the projection should translate the error to an equivalent xlang error or to
xlang_fail if there is no equivalent mapping. After translating the error, the projection should
call xlang's error origination API, `xlang_originate_error`, to create the xlang error info containing the
error and the diagnostic information collected by the language. The created error info will be returned
and the projection should take ownership of it. Note that in situations where memory allocations fail,
the origination API may choose to return a statically allocated error info with only the `xlang_result`.

xlang_originate_error will be defined in the PAL and will take the following parameters:
- a `xlang_result` with the translated xlang error for the language specific error that occurred
- a `xlang_string` with the error message
- a `xlang_string` with an identifier for the projection
- a `xlang_string` with the language specific error that occurred
- an `IUnknown` with the execution trace if available
- an `IUnknown` pointer with any additional language specific information (can be null)

The `xlang_result` parameter is the only required parameter for the creation of the error info, but the
other parameters are recommended, if available, to assist the developer with diagnosing the error.
 
```cpp
[[nodiscard]] IXlangErrorInfo* xlang_originate_error(
    xlang_result error,
    xlang_string message,
    xlang_string projectionIdentifier,
    xlang_string languageError,
    IUnknown* executionTrace,
    IUnknown* languageInformation)
```

If the language error is a propagation of a previous xlang error (i.e. the component called another xlang
projected component which returned an error), then the associated error info should be retrieved.
The retrieved xlang error info and the associated xlang error should be used as the error instead of
translating the language error and creating a new xlang error info. This will allow the identity
of the error to be independent of the translations it may go through as it propagates through
various languages and for it to be defined as what the initial projection which returned the error
determined it was equivalent to. Diagnostic information collected by the language should still be
added to the error info as it can provide useful information in the case the error becomes a failure.
This can be done by a call to `PropagateError` which will add the provided information to the
error info.

`PropagateError` will be defined in the `IXlangErrorInfo` interface and will take the following parameters:
- a `xlang_string` indicating an identifier for the language
- a `xlang_string` with the language specific error
- an `IUnknown` with the execution trace if available
- an `IUnknown` pointer with any additional language specific information (can be null)

```cpp
void PropagateError(
    xlang_string projectionIdentifier,
    xlang_string languageError,
    IUnknown* executionTrace,
    IUnknown* languageInformation)
```

After calling xlang's origination / propagation API, the projection should return the error info as the
result so that the consuming projection can receive it and propagate it to the language it projects.

## Projection identifiers

xlang recommends projections to choose a unique projection identifier and provide it as part of the
error origination and propagation APIs. This identifier will help developers know which language the
error came from as it may not be obvious from the language error and the execution trace. It will
also help tooling determine whether the diagnostic information is from a projection that it recognizes
and can light up additional features such as converting the execution trace to something developer friendly.

Given that projections may evolve over time and change what they store as part of the diagnostic
information, the identifier provided should also have a version number. Due to this, xlang recommends
the identifier be in the form of <unique_projection_identifer>_<version_number>.

## Execution traces (Early draft, incomplete)

xlang allows projections to provide the execution trace captured by the language from when the error
happened when calling the origination and propagation API. This execution trace can be the entire trace
of the function calls leading to the error if the language had captured one or can be a stack trace
from the point in time of the error. It can also be the name or module offset of the failing function
if that is only what the projection has. It basically should be something that helps a developer or
tooling determine where the error happened and how it propagated when looked at together with the
execution traces from the propagations.

## Providing additional language specific information

The `IUnknown` parameter in the xlang origination and propagation API can be used to provide
additional information from the language and the projection. This parameter is of type `IUnknown`
rather than `IXlangObject` because we don't want to restrict the information provided here to be of
projected types. We want to provide the flexibility to projections to provide existing language
specific objects that may not already implement `IXlangObject` and have them not need to create a
wrapper around it.

A projection can use this parameter to provide additional diagnostic information about the error. It can
do this by passing a xlang object with a string representation provided by the `GetObjectInfo` function
for the `StringRepresentation` category. This allows any projection that the error propagates through
to make this information available without it needing to know the specifics of what additional
information each projection provides.

A projection can also use this parameter to store other information that it may want to retrieve later
or expose to tooling. An example use of this is to store a language exception or a custom error info.
Lets say an exception thrown in a component propagates through several different components. One of
these components may be written in the same language as the component which threw the exception.
In that case, a projection may want to re-throw the same exception initially thrown. It can achieve
this by storing the information necessary to re-construct the exception in an object and passing
it to this parameter. The object passed to this parameter will be stored along with the other error
information and will propagate with the error. It can later be retrieved when the error propagates back
to the same language and used to re-construct the exception. Similarly, a projection which allows to call
WinRT APIs may want tooling to be able to access the custom error info (IRestrictedErrorInfo)
it has for the error. It can achieve this by passing the IRestrictedErrorInfo to this parameter and
then let tooling query for it from xlang's error info upon failure.

## Projecting the error and associating error info

Upon receiving an `IXLangErrorInfo` for a call, consuming projections should retrieve the `xlang_result`
and map it to the equivalent error for the language they project. This will allow for consumers to
be written using natural and familiar code for the language they are written in. But in some cases, there
might not be an equivalent error to map to. In these cases, it is left to the projection to decide
which error to map to because a projection implementer will be able to make a better decision on what
a consumer of the language they project would expect.

Consuming projections should also associate the xlang error info in some way with the language error.
It should be associated in a way that allows the logging of the language error to also log the
information in the error info. For instance, in certain languages, exceptions have a ToString function
which consumers of that language call upon failure to log the error for later analysis. This ToString
function should also return the information in xlang's error info to allow the developer to get a
better picture of the failure. In addition, it should be associated in a way that allows a
producing projection for the same language to determine if there is an error info associated
with the language error and to be able to retrieve it if there is. This will allow the
producing projection to propagate the error with the previously collected diagnostic information if
the error ends up being propagated by the component instead of being handled.

It is left to the projections to decide how to achieve this because it will be different for each language.
But this design note will suggest some approaches to consider based on the error model.

### Exception-based projections

For exception-based projections not affected by object slicing, an approach would be to use
inheritance. For each of the errors from xlang that the projection maps to, a new type inheriting
the natural exception type for that error can be created and be used to store the error info. This
new type would just be an implementation detail of the projection and would not be caught directly
by components written in that language. Instead components would use the natural exception types
which these types inherit from to catch exceptions. Projections would use these custom types to
determine whether it is a translated error and to get the associated error info.

### Error code based projections

For error code based projections, an approach would be to use a thread level storage to store the
error info. The consuming projection would store the error info there after translating the error,
while the producing projection would retrieve it from there when the same error propagates to the
producing side. If the error is handled in the consumer, the consumer should have an API provided
by the projection to clear the stored error info to ensure proper cleanup of it. The consumer should
also have an API provided by the projection to get/set the stored error info if the error gets passed
across threads or if the consumer wants to log it when a failure occurs.

## Getting the information stored in the error info

As mentioned earlier, projections should expose the details stored in the xlang error info as part of
the function used in the language they project to log errors (i.e. ToString on an exception).
This will allow consumers to get a full picture of the failure when they log an error using the natural and
familiar code they would typically use. To achieve this, xlang provides the `IXlangErrorInfo` interface.

```cpp
struct IXLangErrorInfo
{
    void GetError(xlang_result* error);
    void GetMessage(xlang_string* message);
    void GetLanguageError(xlang_string* error);
    void GetExecutionTrace(IUnknown** executionTrace);
    void GetProjectionIdentifier(xlang_string* identifier);
    void GetLanguageInformation(IUnknown** languageInformation);
    void GetPropagatedError(IXlangErrorInfo** propagatedError);
    void PropagateError(
        xlang_string projectionIdentifier,
        xlang_string languageError,
        IUnknown* executionTrace,
        IUnknown* languageInformation);
}
```

Most of the functions provided in this interface are self-explanatory. But there are a few that
should be called out.

The `GetLanguageInformation` function should be called by all consuming projections. If the projection
uses it to store custom language specific objects, it should check if it had by querying for the
custom interface it implements on it. If it determines that the stored object isn't one that it had
stored or if it is a projection that doesn't use it to store custom language specific objects, then
it should check if the stored object implements `IXlangObject`. If it does, it should call `GetObjectInfo`
and retrieve the string representation of it, if any, and store it as part of the language error
it constructs for the error and expose it when the language error is logged.

The `GetPropagatedError` function will allow to traverse the diagnostic information provided by the
projections that the error propagated through. For each error info retrieved from it, the `GetError`
function and the `GetMessage` function will provide the same information as the initial error info.
The remaining functions will provide the respective information provided the projection. All this
information should be stored and exposed as part of the language error. It was decided to keep all
these functions as part of the same interface rather than splitting them out to avoid having
projections query for multiple interfaces to get basic details from the error info.

## Error free functions

Some languages have a concept of error free functions (i.e. noexcept in C++) and can be marked as such
in MIDL. Marking a function as noexcept will not affect how the function is represented at the
xlang ABI because not all languages recognize noexcept and their projections will likely ignore the
marker making it harder for them to accommodate such functions if there are differences in the ABI.

But a projection that recognizes the noexcept marker may choose to optimize for it by assuming that
functions marked with it will always return `nullptr` (to indicate success). To accommodate for this,
producing projections should drop the `nodiscard` attribute on such functions. In addition, the runtime
of the language that executes a function with this marker will likely take actions if there is an
error in the function (i.e. fail fast).