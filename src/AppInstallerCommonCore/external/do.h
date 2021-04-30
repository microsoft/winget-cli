

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0626 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __do_h__
#define __do_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if _CONTROL_FLOW_GUARD_XFG
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IDODownload_FWD_DEFINED__
#define __IDODownload_FWD_DEFINED__
typedef interface IDODownload IDODownload;

#endif 	/* __IDODownload_FWD_DEFINED__ */


#ifndef __IDODownloadStatusCallback_FWD_DEFINED__
#define __IDODownloadStatusCallback_FWD_DEFINED__
typedef interface IDODownloadStatusCallback IDODownloadStatusCallback;

#endif 	/* __IDODownloadStatusCallback_FWD_DEFINED__ */


#ifndef __IDOManager_FWD_DEFINED__
#define __IDOManager_FWD_DEFINED__
typedef interface IDOManager IDOManager;

#endif 	/* __IDOManager_FWD_DEFINED__ */


#ifndef __DeliveryOptimization_FWD_DEFINED__
#define __DeliveryOptimization_FWD_DEFINED__

#ifdef __cplusplus
typedef class DeliveryOptimization DeliveryOptimization;
#else
typedef struct DeliveryOptimization DeliveryOptimization;
#endif /* __cplusplus */

#endif 	/* __DeliveryOptimization_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_do_0000_0000 */
/* [local] */ 

typedef struct _DO_DOWNLOAD_RANGE
    {
    UINT64 Offset;
    UINT64 Length;
    } 	DO_DOWNLOAD_RANGE;

typedef struct _DO_DOWNLOAD_RANGES_INFO
    {
    UINT RangeCount;
    /* [size_is] */ DO_DOWNLOAD_RANGE Ranges[ 1 ];
    } 	DO_DOWNLOAD_RANGES_INFO;

typedef 
enum _DODownloadState
    {
        DODownloadState_Created	= 0,
        DODownloadState_Transferring	= ( DODownloadState_Created + 1 ) ,
        DODownloadState_Transferred	= ( DODownloadState_Transferring + 1 ) ,
        DODownloadState_Finalized	= ( DODownloadState_Transferred + 1 ) ,
        DODownloadState_Aborted	= ( DODownloadState_Finalized + 1 ) ,
        DODownloadState_Paused	= ( DODownloadState_Aborted + 1 ) 
    } 	DODownloadState;

typedef struct _DO_DOWNLOAD_STATUS
    {
    UINT64 BytesTotal;
    UINT64 BytesTransferred;
    DODownloadState State;
    HRESULT Error;
    HRESULT ExtendedError;
    } 	DO_DOWNLOAD_STATUS;

typedef 
enum _DODownloadCostPolicy
    {
        DODownloadCostPolicy_Always	= 0,
        DODownloadCostPolicy_Unrestricted	= ( DODownloadCostPolicy_Always + 1 ) ,
        DODownloadCostPolicy_Standard	= ( DODownloadCostPolicy_Unrestricted + 1 ) ,
        DODownloadCostPolicy_NoRoaming	= ( DODownloadCostPolicy_Standard + 1 ) ,
        DODownloadCostPolicy_NoSurcharge	= ( DODownloadCostPolicy_NoRoaming + 1 ) ,
        DODownloadCostPolicy_NoCellular	= ( DODownloadCostPolicy_NoSurcharge + 1 ) 
    } 	DODownloadCostPolicy;

typedef 
enum _DODownloadProperty
    {
        DODownloadProperty_Id	= 0,
        DODownloadProperty_Uri	= ( DODownloadProperty_Id + 1 ) ,
        DODownloadProperty_ContentId	= ( DODownloadProperty_Uri + 1 ) ,
        DODownloadProperty_DisplayName	= ( DODownloadProperty_ContentId + 1 ) ,
        DODownloadProperty_LocalPath	= ( DODownloadProperty_DisplayName + 1 ) ,
        DODownloadProperty_HttpCustomHeaders	= ( DODownloadProperty_LocalPath + 1 ) ,
        DODownloadProperty_CostPolicy	= ( DODownloadProperty_HttpCustomHeaders + 1 ) ,
        DODownloadProperty_SecurityFlags	= ( DODownloadProperty_CostPolicy + 1 ) ,
        DODownloadProperty_CallbackFreqPercent	= ( DODownloadProperty_SecurityFlags + 1 ) ,
        DODownloadProperty_CallbackFreqSeconds	= ( DODownloadProperty_CallbackFreqPercent + 1 ) ,
        DODownloadProperty_NoProgressTimeoutSeconds	= ( DODownloadProperty_CallbackFreqSeconds + 1 ) ,
        DODownloadProperty_ForegroundPriority	= ( DODownloadProperty_NoProgressTimeoutSeconds + 1 ) ,
        DODownloadProperty_BlockingMode	= ( DODownloadProperty_ForegroundPriority + 1 ) ,
        DODownloadProperty_CallbackInterface	= ( DODownloadProperty_BlockingMode + 1 ) ,
        DODownloadProperty_StreamInterface	= ( DODownloadProperty_CallbackInterface + 1 ) ,
        DODownloadProperty_SecurityContext	= ( DODownloadProperty_StreamInterface + 1 ) ,
        DODownloadProperty_NetworkToken	= ( DODownloadProperty_SecurityContext + 1 ) ,
        DODownloadProperty_CorrelationVector	= ( DODownloadProperty_NetworkToken + 1 ) ,
        DODownloadProperty_DecryptionInfo	= ( DODownloadProperty_CorrelationVector + 1 ) ,
        DODownloadProperty_IntegrityCheckInfo	= ( DODownloadProperty_DecryptionInfo + 1 ) ,
        DODownloadProperty_IntegrityCheckMandatory	= ( DODownloadProperty_IntegrityCheckInfo + 1 ) ,
        DODownloadProperty_TotalSizeBytes	= ( DODownloadProperty_IntegrityCheckMandatory + 1 ) ,
        DODownloadProperty_DisallowOnCellular	= ( DODownloadProperty_TotalSizeBytes + 1 ) ,
        DODownloadProperty_HttpCustomAuthHeaders	= ( DODownloadProperty_DisallowOnCellular + 1 ) 
    } 	DODownloadProperty;

typedef struct _DO_DOWNLOAD_ENUM_CATEGORY
    {
    DODownloadProperty Property;
    LPCWSTR Value;
    } 	DO_DOWNLOAD_ENUM_CATEGORY;



extern RPC_IF_HANDLE __MIDL_itf_do_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_do_0000_0000_v0_0_s_ifspec;

#ifndef __IDODownload_INTERFACE_DEFINED__
#define __IDODownload_INTERFACE_DEFINED__

/* interface IDODownload */
/* [uuid][object] */ 


EXTERN_C const IID IID_IDODownload;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FBBD7FC0-C147-4727-A38D-827EF071EE77")
    IDODownload : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [unique][in] */ __RPC__in_opt DO_DOWNLOAD_RANGES_INFO *ranges) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finalize( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ __RPC__out DO_DOWNLOAD_STATUS *status) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ DODownloadProperty propId,
            /* [out] */ __RPC__out VARIANT *propVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ DODownloadProperty propId,
            /* [in] */ __RPC__in VARIANT *propVal) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDODownloadVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDODownload * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDODownload * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDODownload * This);
        
        DECLSPEC_XFGVIRT(IDODownload, Start)
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            __RPC__in IDODownload * This,
            /* [unique][in] */ __RPC__in_opt DO_DOWNLOAD_RANGES_INFO *ranges);
        
        DECLSPEC_XFGVIRT(IDODownload, Pause)
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            __RPC__in IDODownload * This);
        
        DECLSPEC_XFGVIRT(IDODownload, Abort)
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            __RPC__in IDODownload * This);
        
        DECLSPEC_XFGVIRT(IDODownload, Finalize)
        HRESULT ( STDMETHODCALLTYPE *Finalize )( 
            __RPC__in IDODownload * This);
        
        DECLSPEC_XFGVIRT(IDODownload, GetStatus)
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            __RPC__in IDODownload * This,
            /* [out] */ __RPC__out DO_DOWNLOAD_STATUS *status);
        
        DECLSPEC_XFGVIRT(IDODownload, GetProperty)
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            __RPC__in IDODownload * This,
            /* [in] */ DODownloadProperty propId,
            /* [out] */ __RPC__out VARIANT *propVal);
        
        DECLSPEC_XFGVIRT(IDODownload, SetProperty)
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            __RPC__in IDODownload * This,
            /* [in] */ DODownloadProperty propId,
            /* [in] */ __RPC__in VARIANT *propVal);
        
        END_INTERFACE
    } IDODownloadVtbl;

    interface IDODownload
    {
        CONST_VTBL struct IDODownloadVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDODownload_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDODownload_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDODownload_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDODownload_Start(This,ranges)	\
    ( (This)->lpVtbl -> Start(This,ranges) ) 

#define IDODownload_Pause(This)	\
    ( (This)->lpVtbl -> Pause(This) ) 

#define IDODownload_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IDODownload_Finalize(This)	\
    ( (This)->lpVtbl -> Finalize(This) ) 

#define IDODownload_GetStatus(This,status)	\
    ( (This)->lpVtbl -> GetStatus(This,status) ) 

#define IDODownload_GetProperty(This,propId,propVal)	\
    ( (This)->lpVtbl -> GetProperty(This,propId,propVal) ) 

#define IDODownload_SetProperty(This,propId,propVal)	\
    ( (This)->lpVtbl -> SetProperty(This,propId,propVal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDODownload_INTERFACE_DEFINED__ */


#ifndef __IDODownloadStatusCallback_INTERFACE_DEFINED__
#define __IDODownloadStatusCallback_INTERFACE_DEFINED__

/* interface IDODownloadStatusCallback */
/* [uuid][object] */ 


EXTERN_C const IID IID_IDODownloadStatusCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D166E8E3-A90E-4392-8E87-05E996D3747D")
    IDODownloadStatusCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStatusChange( 
            /* [in] */ __RPC__in_opt IDODownload *download,
            /* [in] */ __RPC__in DO_DOWNLOAD_STATUS *status) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDODownloadStatusCallbackVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDODownloadStatusCallback * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDODownloadStatusCallback * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDODownloadStatusCallback * This);
        
        DECLSPEC_XFGVIRT(IDODownloadStatusCallback, OnStatusChange)
        HRESULT ( STDMETHODCALLTYPE *OnStatusChange )( 
            __RPC__in IDODownloadStatusCallback * This,
            /* [in] */ __RPC__in_opt IDODownload *download,
            /* [in] */ __RPC__in DO_DOWNLOAD_STATUS *status);
        
        END_INTERFACE
    } IDODownloadStatusCallbackVtbl;

    interface IDODownloadStatusCallback
    {
        CONST_VTBL struct IDODownloadStatusCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDODownloadStatusCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDODownloadStatusCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDODownloadStatusCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDODownloadStatusCallback_OnStatusChange(This,download,status)	\
    ( (This)->lpVtbl -> OnStatusChange(This,download,status) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDODownloadStatusCallback_INTERFACE_DEFINED__ */


#ifndef __IDOManager_INTERFACE_DEFINED__
#define __IDOManager_INTERFACE_DEFINED__

/* interface IDOManager */
/* [uuid][object] */ 


EXTERN_C const IID IID_IDOManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("400E2D4A-1431-4C1A-A748-39CA472CFDB1")
    IDOManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateDownload( 
            /* [out] */ __RPC__deref_out_opt IDODownload **download) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDownloads( 
            /* [unique][in] */ __RPC__in_opt DO_DOWNLOAD_ENUM_CATEGORY *category,
            /* [out] */ __RPC__deref_out_opt IEnumUnknown **ppEnum) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDOManagerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDOManager * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDOManager * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDOManager * This);
        
        DECLSPEC_XFGVIRT(IDOManager, CreateDownload)
        HRESULT ( STDMETHODCALLTYPE *CreateDownload )( 
            __RPC__in IDOManager * This,
            /* [out] */ __RPC__deref_out_opt IDODownload **download);
        
        DECLSPEC_XFGVIRT(IDOManager, EnumDownloads)
        HRESULT ( STDMETHODCALLTYPE *EnumDownloads )( 
            __RPC__in IDOManager * This,
            /* [unique][in] */ __RPC__in_opt DO_DOWNLOAD_ENUM_CATEGORY *category,
            /* [out] */ __RPC__deref_out_opt IEnumUnknown **ppEnum);
        
        END_INTERFACE
    } IDOManagerVtbl;

    interface IDOManager
    {
        CONST_VTBL struct IDOManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDOManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDOManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDOManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDOManager_CreateDownload(This,download)	\
    ( (This)->lpVtbl -> CreateDownload(This,download) ) 

#define IDOManager_EnumDownloads(This,category,ppEnum)	\
    ( (This)->lpVtbl -> EnumDownloads(This,category,ppEnum) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDOManager_INTERFACE_DEFINED__ */



#ifndef __DeliveryOptimization_LIBRARY_DEFINED__
#define __DeliveryOptimization_LIBRARY_DEFINED__

/* library DeliveryOptimization */
/* [uuid] */ 


EXTERN_C const IID LIBID_DeliveryOptimization;

EXTERN_C const CLSID CLSID_DeliveryOptimization;

#ifdef __cplusplus

class DECLSPEC_UUID("5b99fa76-721c-423c-adac-56d03c8a8007")
DeliveryOptimization;
#endif
#endif /* __DeliveryOptimization_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_do_0000_0004 */
/* [local] */ 

#define DO_LENGTH_TO_EOF     (UINT64)(-1)

#define DecryptionInfo_KeyData               L"KeyData"
#define DecryptionInfo_EncryptionBufferSize  L"EncryptionBufferSize"
#define DecryptionInfo_AlgorithmName         L"AlgorithmName"
#define DecryptionInfo_ChainingMode          L"ChainingMode"

#define IntegrityCheckInfo_PiecesHashFileUrl             L"PiecesHashFileUrl"
#define IntegrityCheckInfo_PiecesHashFileDigest          L"PiecesHashFileDigest"
#define IntegrityCheckInfo_PiecesHashFileDigestAlgorithm L"PiecesHashFileDigestAlgorithm"
#define IntegrityCheckInfo_HashOfHashes                  L"HashOfHashes"


extern RPC_IF_HANDLE __MIDL_itf_do_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_do_0000_0004_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     __RPC__in unsigned long *, __RPC__in VARIANT * ); 

unsigned long             __RPC_USER  VARIANT_UserSize64(     __RPC__in unsigned long *, unsigned long            , __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal64(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal64(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree64(     __RPC__in unsigned long *, __RPC__in VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


