/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Parallel Patterns Library : cancellation_token
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#ifndef _PPLX_H
#error This header must not be included directly
#endif

#ifndef _PPLXCANCELLATION_TOKEN_H
#define _PPLXCANCELLATION_TOKEN_H

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
#error This file must not be included for Visual Studio 12 or later
#endif

#include "pplx/pplxinterface.h"
#include <cstdlib>
#include <string>

#pragma pack(push, _CRT_PACKING)
// All header files are required to be protected from the macro new
#pragma push_macro("new")
#undef new

namespace pplx
{
/// <summary>
///     This class describes an exception thrown by the PPL tasks layer in order to force the current task
///     to cancel. It is also thrown by the <c>get()</c> method on <see cref="task Class">task</see>, for a
///     canceled task.
/// </summary>
/// <seealso cref="task::get Method"/>
/// <seealso cref="cancel_current_task Method"/>
/**/
class task_canceled : public std::exception
{
private:
    std::string _message;

public:
    /// <summary>
    ///     Constructs a <c>task_canceled</c> object.
    /// </summary>
    /// <param name="_Message">
    ///     A descriptive message of the error.
    /// </param>
    /**/
    explicit task_canceled(_In_z_ const char* _Message) throw() : _message(_Message) {}

    /// <summary>
    ///     Constructs a <c>task_canceled</c> object.
    /// </summary>
    /**/
    task_canceled() throw() : exception() {}

    ~task_canceled() throw() {}

    const char* what() const CPPREST_NOEXCEPT { return _message.c_str(); }
};

/// <summary>
///     This class describes an exception thrown when an invalid operation is performed that is not more accurately
///     described by another exception type thrown by the Concurrency Runtime.
/// </summary>
/// <remarks>
///     The various methods which throw this exception will generally document under what circumstances they will throw
///     it.
/// </remarks>
/**/
class invalid_operation : public std::exception
{
private:
    std::string _message;

public:
    /// <summary>
    ///     Constructs an <c>invalid_operation</c> object.
    /// </summary>
    /// <param name="_Message">
    ///     A descriptive message of the error.
    /// </param>
    /**/
    invalid_operation(_In_z_ const char* _Message) throw() : _message(_Message) {}

    /// <summary>
    ///     Constructs an <c>invalid_operation</c> object.
    /// </summary>
    /**/
    invalid_operation() throw() : exception() {}

    ~invalid_operation() throw() {}

    const char* what() const CPPREST_NOEXCEPT { return _message.c_str(); }
};

namespace details
{
// Base class for all reference counted objects
class _RefCounter
{
public:
    virtual ~_RefCounter() { _ASSERTE(_M_refCount == 0); }

    // Acquires a reference
    // Returns the new reference count.
    long _Reference()
    {
        long _Refcount = atomic_increment(_M_refCount);

        // 0 - 1 transition is illegal
        _ASSERTE(_Refcount > 1);
        return _Refcount;
    }

    // Releases the reference
    // Returns the new reference count
    long _Release()
    {
        long _Refcount = atomic_decrement(_M_refCount);
        _ASSERTE(_Refcount >= 0);

        if (_Refcount == 0)
        {
            _Destroy();
        }

        return _Refcount;
    }

protected:
    // Allow derived classes to provide their own deleter
    virtual void _Destroy() { delete this; }

    // Only allow instantiation through derived class
    _RefCounter(long _InitialCount = 1) : _M_refCount(_InitialCount) { _ASSERTE(_M_refCount > 0); }

    // Reference count
    atomic_long _M_refCount;
};

class _CancellationTokenState;

class _CancellationTokenRegistration : public _RefCounter
{
private:
    static const long _STATE_CLEAR = 0;
    static const long _STATE_DEFER_DELETE = 1;
    static const long _STATE_SYNCHRONIZE = 2;
    static const long _STATE_CALLED = 3;

public:
    _CancellationTokenRegistration(long _InitialRefs = 1)
        : _RefCounter(_InitialRefs), _M_state(_STATE_CALLED), _M_pTokenState(NULL)
    {
    }

    _CancellationTokenState* _GetToken() const { return _M_pTokenState; }

protected:
    virtual ~_CancellationTokenRegistration() { _ASSERTE(_M_state != _STATE_CLEAR); }

    virtual void _Exec() = 0;

private:
    friend class _CancellationTokenState;

    void _Invoke()
    {
        long tid = ::pplx::details::platform::GetCurrentThreadId();
        _ASSERTE((tid & 0x3) == 0); // If this ever fires, we need a different encoding for this.

        long result = atomic_compare_exchange(_M_state, tid, _STATE_CLEAR);

        if (result == _STATE_CLEAR)
        {
            _Exec();

            result = atomic_compare_exchange(_M_state, _STATE_CALLED, tid);

            if (result == _STATE_SYNCHRONIZE)
            {
                _M_pSyncBlock->set();
            }
        }
        _Release();
    }

    atomic_long _M_state;
    extensibility::event_t* _M_pSyncBlock;
    _CancellationTokenState* _M_pTokenState;
};

template<typename _Function>
class _CancellationTokenCallback : public _CancellationTokenRegistration
{
public:
    _CancellationTokenCallback(const _Function& _Func) : _M_function(_Func) {}

protected:
    virtual void _Exec() { _M_function(); }

private:
    _Function _M_function;
};

class CancellationTokenRegistration_TaskProc : public _CancellationTokenRegistration
{
public:
    CancellationTokenRegistration_TaskProc(TaskProc_t proc, _In_ void* pData, int initialRefs)
        : _CancellationTokenRegistration(initialRefs), m_proc(proc), m_pData(pData)
    {
    }

protected:
    virtual void _Exec() { m_proc(m_pData); }

private:
    TaskProc_t m_proc;
    void* m_pData;
};

// The base implementation of a cancellation token.
class _CancellationTokenState : public _RefCounter
{
protected:
    class TokenRegistrationContainer
    {
    private:
        typedef struct _Node
        {
            _CancellationTokenRegistration* _M_token;
            _Node* _M_next;
        } Node;

    public:
        TokenRegistrationContainer() : _M_begin(nullptr), _M_last(nullptr) {}

        ~TokenRegistrationContainer()
        {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6001)
#endif
            auto node = _M_begin;
            while (node != nullptr)
            {
                Node* tmp = node;
                node = node->_M_next;
                ::free(tmp);
            }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
        }

        void swap(TokenRegistrationContainer& list)
        {
            std::swap(list._M_begin, _M_begin);
            std::swap(list._M_last, _M_last);
        }

        bool empty() { return _M_begin == nullptr; }

        template<typename T>
        void for_each(T lambda)
        {
            Node* node = _M_begin;

            while (node != nullptr)
            {
                lambda(node->_M_token);
                node = node->_M_next;
            }
        }

        void push_back(_CancellationTokenRegistration* token)
        {
            Node* node = reinterpret_cast<Node*>(::malloc(sizeof(Node)));
            if (node == nullptr)
            {
                throw ::std::bad_alloc();
            }

            node->_M_token = token;
            node->_M_next = nullptr;

            if (_M_begin == nullptr)
            {
                _M_begin = node;
            }
            else
            {
                _M_last->_M_next = node;
            }

            _M_last = node;
        }

        void remove(_CancellationTokenRegistration* token)
        {
            Node* node = _M_begin;
            Node* prev = nullptr;

            while (node != nullptr)
            {
                if (node->_M_token == token)
                {
                    if (prev == nullptr)
                    {
                        _M_begin = node->_M_next;
                    }
                    else
                    {
                        prev->_M_next = node->_M_next;
                    }

                    if (node->_M_next == nullptr)
                    {
                        _M_last = prev;
                    }

                    ::free(node);
                    break;
                }

                prev = node;
                node = node->_M_next;
            }
        }

    private:
        Node* _M_begin;
        Node* _M_last;
    };

public:
    static _CancellationTokenState* _NewTokenState() { return new _CancellationTokenState(); }

    static _CancellationTokenState* _None() { return reinterpret_cast<_CancellationTokenState*>(2); }

    static bool _IsValid(_In_opt_ _CancellationTokenState* _PToken) { return (_PToken != NULL && _PToken != _None()); }

    _CancellationTokenState() : _M_stateFlag(0) {}

    ~_CancellationTokenState()
    {
        TokenRegistrationContainer rundownList;
        {
            extensibility::scoped_critical_section_t _Lock(_M_listLock);
            _M_registrations.swap(rundownList);
        }

        rundownList.for_each([](_CancellationTokenRegistration* pRegistration) {
            pRegistration->_M_state = _CancellationTokenRegistration::_STATE_SYNCHRONIZE;
            pRegistration->_Release();
        });
    }

    bool _IsCanceled() const { return (_M_stateFlag != 0); }

    void _Cancel()
    {
        if (atomic_compare_exchange(_M_stateFlag, 1l, 0l) == 0)
        {
            TokenRegistrationContainer rundownList;
            {
                extensibility::scoped_critical_section_t _Lock(_M_listLock);
                _M_registrations.swap(rundownList);
            }

            rundownList.for_each([](_CancellationTokenRegistration* pRegistration) { pRegistration->_Invoke(); });

            _M_stateFlag = 2;
            _M_cancelComplete.set();
        }
    }

    _CancellationTokenRegistration* _RegisterCallback(TaskProc_t _PCallback, _In_ void* _PData, int _InitialRefs = 1)
    {
        _CancellationTokenRegistration* pRegistration =
            new CancellationTokenRegistration_TaskProc(_PCallback, _PData, _InitialRefs);
        _RegisterCallback(pRegistration);
        return pRegistration;
    }

    void _RegisterCallback(_In_ _CancellationTokenRegistration* _PRegistration)
    {
        _PRegistration->_M_state = _CancellationTokenRegistration::_STATE_CLEAR;
        _PRegistration->_Reference();
        _PRegistration->_M_pTokenState = this;

        bool invoke = true;

        if (!_IsCanceled())
        {
            extensibility::scoped_critical_section_t _Lock(_M_listLock);

            if (!_IsCanceled())
            {
                invoke = false;
                _M_registrations.push_back(_PRegistration);
            }
        }

        if (invoke)
        {
            _PRegistration->_Invoke();
        }
    }

    void _DeregisterCallback(_In_ _CancellationTokenRegistration* _PRegistration)
    {
        bool synchronize = false;

        {
            extensibility::scoped_critical_section_t _Lock(_M_listLock);

            //
            // If a cancellation has occurred, the registration list is guaranteed to be empty if we've observed it
            // under the auspices of the lock.  In this case, we must synchronize with the canceling thread to guarantee
            // that the cancellation is finished by the time we return from this method.
            //
            if (!_M_registrations.empty())
            {
                _M_registrations.remove(_PRegistration);
                _PRegistration->_M_state = _CancellationTokenRegistration::_STATE_SYNCHRONIZE;
                _PRegistration->_Release();
            }
            else
            {
                synchronize = true;
            }
        }

        //
        // If the list is empty, we are in one of several situations:
        //
        // - The callback has already been made         --> do nothing
        // - The callback is about to be made           --> flag it so it doesn't happen and return
        // - The callback is in progress elsewhere      --> synchronize with it
        // - The callback is in progress on this thread --> do nothing
        //
        if (synchronize)
        {
            long result = atomic_compare_exchange(_PRegistration->_M_state,
                                                  _CancellationTokenRegistration::_STATE_DEFER_DELETE,
                                                  _CancellationTokenRegistration::_STATE_CLEAR);

            switch (result)
            {
                case _CancellationTokenRegistration::_STATE_CLEAR:
                case _CancellationTokenRegistration::_STATE_CALLED: break;
                case _CancellationTokenRegistration::_STATE_DEFER_DELETE:
                case _CancellationTokenRegistration::_STATE_SYNCHRONIZE: _ASSERTE(false); break;
                default:
                {
                    long tid = result;
                    if (tid == ::pplx::details::platform::GetCurrentThreadId())
                    {
                        //
                        // It is entirely legal for a caller to Deregister during a callback instead of having to
                        // provide their own synchronization mechanism between the two.  In this case, we do *NOT* need
                        // to explicitly synchronize with the callback as doing so would deadlock.  If the call happens
                        // during, skip any extra synchronization.
                        //
                        break;
                    }

                    extensibility::event_t ev;
                    _PRegistration->_M_pSyncBlock = &ev;

                    long result_1 =
                        atomic_exchange(_PRegistration->_M_state, _CancellationTokenRegistration::_STATE_SYNCHRONIZE);

                    if (result_1 != _CancellationTokenRegistration::_STATE_CALLED)
                    {
                        _PRegistration->_M_pSyncBlock->wait(::pplx::extensibility::event_t::timeout_infinite);
                    }

                    break;
                }
            }
        }
    }

private:
    // The flag for the token state (whether it is canceled or not)
    atomic_long _M_stateFlag;

    // Notification of completion of cancellation of this token.
    extensibility::event_t _M_cancelComplete; // Hmm.. where do we wait for it??

    // Lock to protect the registrations list
    extensibility::critical_section_t _M_listLock;

    // The protected list of registrations
    TokenRegistrationContainer _M_registrations;
};

} // namespace details

class cancellation_token_source;
class cancellation_token;

/// <summary>
///     The <c>cancellation_token_registration</c> class represents a callback notification from a
///     <c>cancellation_token</c>.  When the <c>register</c> method on a <c>cancellation_token</c> is used to receive
///     notification of when cancellation occurs, a <c>cancellation_token_registration</c> object is returned as a
///     handle to the callback so that the caller can request a specific callback no longer be made through use of the
///     <c>deregister</c> method.
/// </summary>
class cancellation_token_registration
{
public:
    cancellation_token_registration() : _M_pRegistration(NULL) {}

    ~cancellation_token_registration() { _Clear(); }

    cancellation_token_registration(const cancellation_token_registration& _Src) { _Assign(_Src._M_pRegistration); }

    cancellation_token_registration(cancellation_token_registration&& _Src) { _Move(_Src._M_pRegistration); }

    cancellation_token_registration& operator=(const cancellation_token_registration& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Assign(_Src._M_pRegistration);
        }
        return *this;
    }

    cancellation_token_registration& operator=(cancellation_token_registration&& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Move(_Src._M_pRegistration);
        }
        return *this;
    }

    bool operator==(const cancellation_token_registration& _Rhs) const
    {
        return _M_pRegistration == _Rhs._M_pRegistration;
    }

    bool operator!=(const cancellation_token_registration& _Rhs) const { return !(operator==(_Rhs)); }

private:
    friend class cancellation_token;

    cancellation_token_registration(_In_ details::_CancellationTokenRegistration* _PRegistration)
        : _M_pRegistration(_PRegistration)
    {
    }

    void _Clear()
    {
        if (_M_pRegistration != NULL)
        {
            _M_pRegistration->_Release();
        }
        _M_pRegistration = NULL;
    }

    void _Assign(_In_ details::_CancellationTokenRegistration* _PRegistration)
    {
        if (_PRegistration != NULL)
        {
            _PRegistration->_Reference();
        }
        _M_pRegistration = _PRegistration;
    }

    void _Move(_In_ details::_CancellationTokenRegistration*& _PRegistration)
    {
        _M_pRegistration = _PRegistration;
        _PRegistration = NULL;
    }

    details::_CancellationTokenRegistration* _M_pRegistration;
};

/// <summary>
///     The <c>cancellation_token</c> class represents the ability to determine whether some operation has been
///     requested to cancel.  A given token can be associated with a <c>task_group</c>, <c>structured_task_group</c>, or
///     <c>task</c> to provide implicit cancellation.  It can also be polled for cancellation or have a callback
///     registered for if and when the associated <c>cancellation_token_source</c> is canceled.
/// </summary>
class cancellation_token
{
public:
    typedef details::_CancellationTokenState* _ImplType;

    /// <summary>
    ///     Returns a cancellation token which can never be subject to cancellation.
    /// </summary>
    /// <returns>
    ///     A cancellation token that cannot be canceled.
    /// </returns>
    static cancellation_token none() { return cancellation_token(); }

    cancellation_token(const cancellation_token& _Src) { _Assign(_Src._M_Impl); }

    cancellation_token(cancellation_token&& _Src) { _Move(_Src._M_Impl); }

    cancellation_token& operator=(const cancellation_token& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Assign(_Src._M_Impl);
        }
        return *this;
    }

    cancellation_token& operator=(cancellation_token&& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Move(_Src._M_Impl);
        }
        return *this;
    }

    bool operator==(const cancellation_token& _Src) const { return _M_Impl == _Src._M_Impl; }

    bool operator!=(const cancellation_token& _Src) const { return !(operator==(_Src)); }

    ~cancellation_token() { _Clear(); }

    /// <summary>
    ///     Returns an indication of whether this token can be canceled or not.
    /// </summary>
    /// <returns>
    ///     An indication of whether this token can be canceled or not.
    /// </returns>
    bool is_cancelable() const { return (_M_Impl != NULL); }

    /// <summary>
    /// Returns <c>true</c> if the token has been canceled.
    /// </summary>
    /// <returns>
    /// The value <c>true</c> if the token has been canceled; otherwise, the value <c>false</c>.
    /// </returns>
    bool is_canceled() const { return (_M_Impl != NULL && _M_Impl->_IsCanceled()); }

    /// <summary>
    ///     Registers a callback function with the token.  If and when the token is canceled, the callback will be made.
    ///     Note that if the token is already canceled at the point where this method is called, the callback will be
    ///     made immediately and synchronously.
    /// </summary>
    /// <typeparam name="_Function">
    ///     The type of the function object that will be called back when this <c>cancellation_token</c> is canceled.
    /// </typeparam>
    /// <param name="_Func">
    ///     The function object that will be called back when this <c>cancellation_token</c> is canceled.
    /// </param>
    /// <returns>
    ///     A <c>cancellation_token_registration</c> object which can be utilized in the <c>deregister</c> method to
    ///     deregister a previously registered callback and prevent it from being made. The method will throw an <see
    ///     cref="invalid_operation Class">invalid_operation </see> exception if it is called on a
    ///     <c>cancellation_token</c> object that was created using the <see cref="cancellation_token::none
    ///     Method">cancellation_token::none </see> method.
    /// </returns>
    template<typename _Function>
    ::pplx::cancellation_token_registration register_callback(const _Function& _Func) const
    {
        if (_M_Impl == NULL)
        {
            // A callback cannot be registered if the token does not have an associated source.
            throw invalid_operation();
        }
#if defined(_MSC_VER)
#pragma warning(suppress : 28197)
#endif
        details::_CancellationTokenCallback<_Function>* _PCallback =
            new details::_CancellationTokenCallback<_Function>(_Func);
        _M_Impl->_RegisterCallback(_PCallback);
        return cancellation_token_registration(_PCallback);
    }

    /// <summary>
    ///     Removes a callback previously registered via the <c>register</c> method based on the
    ///     <c>cancellation_token_registration</c> object returned at the time of registration.
    /// </summary>
    /// <param name="_Registration">
    ///     The <c>cancellation_token_registration</c> object corresponding to the callback to be deregistered.  This
    ///     token must have been previously returned from a call to the <c>register</c> method.
    /// </param>
    void deregister_callback(const cancellation_token_registration& _Registration) const
    {
        _M_Impl->_DeregisterCallback(_Registration._M_pRegistration);
    }

    _ImplType _GetImpl() const { return _M_Impl; }

    _ImplType _GetImplValue() const
    {
        return (_M_Impl == NULL) ? ::pplx::details::_CancellationTokenState::_None() : _M_Impl;
    }

    static cancellation_token _FromImpl(_ImplType _Impl) { return cancellation_token(_Impl); }

private:
    friend class cancellation_token_source;

    _ImplType _M_Impl;

    void _Clear()
    {
        if (_M_Impl != NULL)
        {
            _M_Impl->_Release();
        }
        _M_Impl = NULL;
    }

    void _Assign(_ImplType _Impl)
    {
        if (_Impl != NULL)
        {
            _Impl->_Reference();
        }
        _M_Impl = _Impl;
    }

    void _Move(_ImplType& _Impl)
    {
        _M_Impl = _Impl;
        _Impl = NULL;
    }

    cancellation_token() : _M_Impl(NULL) {}

    cancellation_token(_ImplType _Impl) : _M_Impl(_Impl)
    {
        if (_M_Impl == ::pplx::details::_CancellationTokenState::_None())
        {
            _M_Impl = NULL;
        }

        if (_M_Impl != NULL)
        {
            _M_Impl->_Reference();
        }
    }
};

/// <summary>
///     The <c>cancellation_token_source</c> class represents the ability to cancel some cancelable operation.
/// </summary>
class cancellation_token_source
{
public:
    typedef ::pplx::details::_CancellationTokenState* _ImplType;

    /// <summary>
    ///     Constructs a new <c>cancellation_token_source</c>.  The source can be used to flag cancellation of some
    ///     cancelable operation.
    /// </summary>
    cancellation_token_source() { _M_Impl = new ::pplx::details::_CancellationTokenState; }

    cancellation_token_source(const cancellation_token_source& _Src) { _Assign(_Src._M_Impl); }

    cancellation_token_source(cancellation_token_source&& _Src) { _Move(_Src._M_Impl); }

    cancellation_token_source& operator=(const cancellation_token_source& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Assign(_Src._M_Impl);
        }
        return *this;
    }

    cancellation_token_source& operator=(cancellation_token_source&& _Src)
    {
        if (this != &_Src)
        {
            _Clear();
            _Move(_Src._M_Impl);
        }
        return *this;
    }

    bool operator==(const cancellation_token_source& _Src) const { return _M_Impl == _Src._M_Impl; }

    bool operator!=(const cancellation_token_source& _Src) const { return !(operator==(_Src)); }

    ~cancellation_token_source()
    {
        if (_M_Impl != NULL)
        {
            _M_Impl->_Release();
        }
    }

    /// <summary>
    ///     Returns a cancellation token associated with this source.  The returned token can be polled for cancellation
    ///     or provide a callback if and when cancellation occurs.
    /// </summary>
    /// <returns>
    ///     A cancellation token associated with this source.
    /// </returns>
    cancellation_token get_token() const { return cancellation_token(_M_Impl); }

    /// <summary>
    ///     Creates a <c>cancellation_token_source</c> which is canceled when the provided token is canceled.
    /// </summary>
    /// <param name="_Src">
    ///     A token whose cancellation will cause cancellation of the returned token source.  Note that the returned
    ///     token source can also be canceled independently of the source contained in this parameter.
    /// </param>
    /// <returns>
    ///     A <c>cancellation_token_source</c> which is canceled when the token provided by the <paramref name="_Src"/>
    ///     parameter is canceled.
    /// </returns>
    static cancellation_token_source create_linked_source(cancellation_token& _Src)
    {
        cancellation_token_source newSource;
        _Src.register_callback([newSource]() { newSource.cancel(); });
        return newSource;
    }

    /// <summary>
    ///     Creates a <c>cancellation_token_source</c> which is canceled when one of a series of tokens represented by
    ///     an STL iterator pair is canceled.
    /// </summary>
    /// <param name="_Begin">
    ///     The STL iterator corresponding to the beginning of the range of tokens to listen for cancellation of.
    /// </param>
    /// <param name="_End">
    ///     The STL iterator corresponding to the ending of the range of tokens to listen for cancellation of.
    /// </param>
    /// <returns>
    ///     A <c>cancellation_token_source</c> which is canceled when any of the tokens provided by the range described
    ///     by the STL iterators contained in the <paramref name="_Begin"/> and <paramref name="_End"/> parameters is
    ///     canceled.
    /// </returns>
    template<typename _Iter>
    static cancellation_token_source create_linked_source(_Iter _Begin, _Iter _End)
    {
        cancellation_token_source newSource;
        for (_Iter _It = _Begin; _It != _End; ++_It)
        {
            _It->register_callback([newSource]() { newSource.cancel(); });
        }
        return newSource;
    }

    /// <summary>
    ///     Cancels the token.  Any <c>task_group</c>, <c>structured_task_group</c>, or <c>task</c> which utilizes the
    ///     token will be canceled upon this call and throw an exception at the next interruption point.
    /// </summary>
    void cancel() const { _M_Impl->_Cancel(); }

    _ImplType _GetImpl() const { return _M_Impl; }

    static cancellation_token_source _FromImpl(_ImplType _Impl) { return cancellation_token_source(_Impl); }

private:
    _ImplType _M_Impl;

    void _Clear()
    {
        if (_M_Impl != NULL)
        {
            _M_Impl->_Release();
        }
        _M_Impl = NULL;
    }

    void _Assign(_ImplType _Impl)
    {
        if (_Impl != NULL)
        {
            _Impl->_Reference();
        }
        _M_Impl = _Impl;
    }

    void _Move(_ImplType& _Impl)
    {
        _M_Impl = _Impl;
        _Impl = NULL;
    }

    cancellation_token_source(_ImplType _Impl) : _M_Impl(_Impl)
    {
        if (_M_Impl == ::pplx::details::_CancellationTokenState::_None())
        {
            _M_Impl = NULL;
        }

        if (_M_Impl != NULL)
        {
            _M_Impl->_Reference();
        }
    }
};

} // namespace pplx

#pragma pop_macro("new")
#pragma pack(pop)

#endif // _PPLXCANCELLATION_TOKEN_H
