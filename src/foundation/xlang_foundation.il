
// Metadata version: WindowsRuntime 1.4
.assembly extern mscorlib
{
  .publickeytoken = (B7 7A 5C 56 19 34 E0 89 )                         // .z\V.4..
  .ver 255:255:255:255
}

.assembly windowsruntime xlang_foundation
{
  .ver 1:0:0:0
  .hash algorithm 0x00008004
}

.module xlang_foundation.xmeta
.imagebase 0x00400000
.file alignment 0x00000200
.stackreserve 0x00100000
.subsystem 0x0003       // WINDOWS_CUI
.corflags 0x00000001    //  ILONLY

// interface/delegate GUID are RFC 4122 name-based UUIDs, version 5
// Namespace guid used was 0513fbd1-f316-4128-bdcb-b235159ff91d

.class public auto ansi windowsruntime sealed Foundation.AsyncStatus
       extends [mscorlib]System.Enum
{
  .field private specialname rtspecialname int32 value__
  .field public static literal valuetype Foundation.AsyncStatus Canceled = int32(0x00000002)
  .field public static literal valuetype Foundation.AsyncStatus Completed = int32(0x00000001)
  .field public static literal valuetype Foundation.AsyncStatus Error = int32(0x00000003)
  .field public static literal valuetype Foundation.AsyncStatus Started = int32(0x00000000)
} // end of class Foundation.AsyncStatus

.class public sequential ansi windowsruntime sealed Foundation.DateTime
       extends [mscorlib]System.ValueType
{
  .field public int64 UniversalTime
} // end of class Foundation.DateTime

.class public sequential ansi windowsruntime sealed Foundation.Guid
       extends [mscorlib]System.ValueType
{
  .field public uint32 TimeLow
  .field public uint16 TimeMid
  .field public uint16 TimeHiAndVersion
  .field public uint8 ClockSeqHiAndReserved
  .field public uint8 ClockSeqLow
  .field public uint8 Node1
  .field public uint8 Node2
  .field public uint8 Node3
  .field public uint8 Node4
  .field public uint8 Node5
  .field public uint8 Node6
} // end of class Foundation.Guid

.class public auto ansi windowsruntime sealed Foundation.AsyncActionCompletedHandler
       extends [mscorlib]System.MulticastDelegate
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 66 12 4E 38 78 36 E0 59 BF 8A 6C 80 3A DC 6D 78 00 00 )

  .method private hidebysig specialname rtspecialname 
          instance void  .ctor(object 'object', native int 'method') runtime managed
  {
  } // end of method AsyncActionCompletedHandler.ctor

  .method public hidebysig newslot specialname virtual instance void Invoke([in] class Foundation.IAsyncAction asyncInfo, [in] valuetype Foundation.AsyncStatus asyncStatus) runtime managed
  {
  } // end of method AsyncActionCompletedHandler.Invoke

} // end of class Foundation.AsyncActionCompletedHandler

.class interface public abstract auto ansi windowsruntime Foundation.IAsyncAction

{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 06 00 04 D9 49 53 33 5F AA E1 EE AD 74 6C B4 DD 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance int32 get_ErrorCode () runtime managed
  {
  } // end of method IAsyncAction::get_ErrorCode

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype Foundation.AsyncStatus get_Status () runtime managed
  {
  } // end of method IAsyncAction::get_Status

  .method public hidebysig newslot abstract virtual 
          instance void Cancel () runtime managed
  {
  } // end of method IAsyncAction::Cancel

  .method public hidebysig newslot abstract virtual 
          instance void Close () runtime managed
  {
  } // end of method IAsyncAction::Close

  .method public hidebysig newslot specialname abstract virtual 
          instance class Foundation.AsyncActionCompletedHandler get_Completed () runtime managed
  {
  } // end of method IAsyncAction::get_Completed

  .method public hidebysig newslot specialname abstract virtual 
          instance void set_Completed ([in] class Foundation.AsyncActionCompletedHandler 'value') runtime managed
  {
  } // end of method IAsyncAction::set_Completed

  .method public hidebysig newslot abstract virtual 
          instance void GetResults () runtime managed
  {
  } // end of method IAsyncAction::GetResults

  .property instance int32 ErrorCode()
  {
    .get instance int32 Foundation.IAsyncAction::get_ErrorCode()
  } // end of property IAsyncAction::ErrorCode

  .property instance valuetype Foundation.AsyncStatus Status()
  {
    .get instance valuetype Foundation.AsyncStatus Foundation.IAsyncAction::get_Status()
  } // end of property IAsyncAction::Status

  .property instance class Foundation.AsyncActionCompletedHandler Completed()
  {
    .get instance class Foundation.AsyncActionCompletedHandler Foundation.IAsyncAction::get_Completed()
    .set instance void Foundation.IAsyncAction::set_Completed([in] class Foundation.AsyncActionCompletedHandler 'value')
  } // end of property IAsyncAction::Completed
} // end of class Foundation.IAsyncAction

.class public auto ansi windowsruntime sealed Foundation.AsyncOperationCompletedHandler`1<TResult>
       extends [mscorlib]System.MulticastDelegate
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 A2 C9 C7 F6 FF 9F 60 5F 82 93 CC 3F 9B 0F B4 4B 00 00 )

  .method private hidebysig specialname rtspecialname 
          instance void  .ctor(object 'object', native int 'method') runtime managed
  {
  } // end of method AsyncOperationCompletedHandler`1.ctor

  .method public hidebysig newslot specialname virtual instance void Invoke([in] class Foundation.IAsyncOperation`1<!TResult> asyncInfo, [in] valuetype Foundation.AsyncStatus asyncStatus) runtime managed
  {
  } // end of method AsyncOperationCompletedHandler`1.Invoke

} // end of class Foundation.AsyncOperationCompletedHandler`1<TResult>

.class interface public abstract auto ansi windowsruntime Foundation.IAsyncOperation`1<TResult>

{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 FB 0B 11 69 2F 60 10 5B 9A B0 CA 9F 04 48 02 E6 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance int32 get_ErrorCode () runtime managed
  {
  } // end of method IAsyncOperation`1::get_ErrorCode

  .method public hidebysig newslot specialname abstract virtual 
          instance valuetype Foundation.AsyncStatus get_Status () runtime managed
  {
  } // end of method IAsyncOperation`1::get_Status

  .method public hidebysig newslot abstract virtual 
          instance void Cancel () runtime managed
  {
  } // end of method IAsyncOperation`1::Cancel

  .method public hidebysig newslot abstract virtual 
          instance void Close () runtime managed
  {
  } // end of method IAsyncOperation`1::Close

  .method public hidebysig newslot specialname abstract virtual 
          instance class Foundation.AsyncOperationCompletedHandler`1<!TResult> get_Completed () runtime managed
  {
  } // end of method IAsyncOperation`1::get_Completed

  .method public hidebysig newslot specialname abstract virtual 
          instance void set_Completed ([in] class Foundation.AsyncOperationCompletedHandler`1<!TResult> 'value') runtime managed
  {
  } // end of method IAsyncOperation`1::set_Completed

  .method public hidebysig newslot abstract virtual 
          instance !TResult GetResults () runtime managed
  {
  } // end of method IAsyncOperation`1::GetResults

  .property instance int32 ErrorCode()
  {
    .get instance int32 Foundation.IAsyncOperation`1::get_ErrorCode()
  } // end of property IAsyncOperation`1::ErrorCode

  .property instance valuetype Foundation.AsyncStatus Status()
  {
    .get instance valuetype Foundation.AsyncStatus Foundation.IAsyncOperation`1::get_Status()
  } // end of property IAsyncOperation`1::Status

  .property instance class Foundation.AsyncOperationCompletedHandler`1<!TResult> Completed()
  {
    .get instance class Foundation.AsyncOperationCompletedHandler`1<!TResult> Foundation.IAsyncOperation`1::get_Completed()
    .set instance void Foundation.IAsyncOperation`1::set_Completed([in] class Foundation.AsyncOperationCompletedHandler`1<!TResult> 'value')
  } // end of property IAsyncOperation`1::Completed
} // end of class Foundation.IAsyncOperation`1<TResult>

.class public sequential ansi windowsruntime sealed Foundation.TimeSpan
       extends [mscorlib]System.ValueType
{
  .field public int64 Duration
} // end of class Foundation.TimeSpan

.class public auto ansi windowsruntime sealed Foundation.TypedEventHandler`2<TSender, TResult>
       extends [mscorlib]System.MulticastDelegate
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 FD 78 32 6C B8 FB 2F 54 86 63 E6 6C 89 39 B0 C9 00 00 )

  .method private hidebysig specialname rtspecialname 
          instance void  .ctor(object 'object', native int 'method') runtime managed
  {
  } // end of method TypedEventHandler`2.ctor

  .method public hidebysig newslot specialname virtual instance void Invoke([in] !TSender sender, [in] !TResult args) runtime managed
  {
  } // end of method TypedEventHandler`2.Invoke

} // end of class Foundation.TypedEventHandler`2<TSender, TResult>

.class public auto ansi windowsruntime sealed Foundation.Metadata.GuidAttribute
       extends [mscorlib]System.Attribute
{
  .method public hidebysig specialname rtspecialname instance void  .ctor(
[in] uint32 a, [in] uint16 b, [in] uint16 c, [in] uint8 d, [in] uint8 e, [in] uint8 f, [in] uint8 g, [in] uint8 h, [in] uint8 i, [in] uint8 j, [in] uint8 k) cil runtime managed
  {
  } // end of method Foundation.Metadata.GuidAttribute..ctor

} // end of class Foundation.Metadata.GuidAttribute

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IIterable`1<T>

{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 EB DA 26 DB 98 57 32 5B B2 9C 3C B7 34 AA 3A 0A 00 00 )

  .method public hidebysig newslot abstract virtual 
          instance class Foundation.Collections.IIterator`1<!T> First () runtime managed
  {
  } // end of method IIterable`1::First
} // end of class Foundation.Collections.IIterable`1<T>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IIterator`1<T>

{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 A3 5D 0D EF 4E D8 79 57 8E AA AB F5 9E 3C B5 AB 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance !T get_Current () runtime managed
  {
  } // end of method IIterator`1::get_Current

  .method public hidebysig newslot specialname abstract virtual 
          instance bool get_HasCurrent () runtime managed
  {
  } // end of method IIterator`1::get_HasCurrent

  .method public hidebysig newslot abstract virtual 
          instance bool MoveNext () runtime managed
  {
  } // end of method IIterator`1::MoveNext

  .method public hidebysig newslot abstract virtual 
          instance uint32 GetMany ([out] !T[]& items) runtime managed
  {
  } // end of method IIterator`1::GetMany

  .property instance !T Current()
  {
    .get instance !T Foundation.Collections.IIterator`1::get_Current()
  } // end of property IIterator`1::Current

  .property instance bool HasCurrent()
  {
    .get instance bool Foundation.Collections.IIterator`1::get_HasCurrent()
  } // end of property IIterator`1::HasCurrent
} // end of class Foundation.Collections.IIterator`1<T>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IKeyValuePair`2<K, V>

{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 62 0E 91 7E 55 F4 7A 52 87 3F 49 8D 4B DD 3A B6 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance !K get_Key () runtime managed
  {
  } // end of method IKeyValuePair`2::get_Key

  .method public hidebysig newslot specialname abstract virtual 
          instance !V get_Value () runtime managed
  {
  } // end of method IKeyValuePair`2::get_Value

  .property instance !K Key()
  {
    .get instance !K Foundation.Collections.IKeyValuePair`2::get_Key()
  } // end of property IKeyValuePair`2::Key

  .property instance !V Value()
  {
    .get instance !V Foundation.Collections.IKeyValuePair`2::get_Value()
  } // end of property IKeyValuePair`2::Value
} // end of class Foundation.Collections.IKeyValuePair`2<K, V>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IMap`2<K, V>
    implements class Foundation.Collections.IMapView`2<!K, !V>
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 AE F1 2B 30 F3 17 DC 55 83 8B 41 EF 6F F5 62 A3 00 00 )

  .method public hidebysig newslot abstract virtual 
          instance class Foundation.Collections.IMapView`2<!K, !V> GetView () runtime managed
  {
  } // end of method IMap`2::GetView

  .method public hidebysig newslot abstract virtual 
          instance bool Insert ([in] !K key, [in] !V 'value') runtime managed
  {
  } // end of method IMap`2::Insert

  .method public hidebysig newslot abstract virtual 
          instance void Remove ([in] !K key) runtime managed
  {
  } // end of method IMap`2::Remove

  .method public hidebysig newslot abstract virtual 
          instance void Clear () runtime managed
  {
  } // end of method IMap`2::Clear
} // end of class Foundation.Collections.IMap`2<K, V>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IMapView`2<K, V>
    implements class Foundation.Collections.IIterable`1<class Foundation.Collections.IKeyValuePair`2<!K, !V>>
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 C9 57 E1 0E 6B DD 60 52 97 D1 65 5D BC 2C B5 16 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32 get_Size () runtime managed
  {
  } // end of method IMapView`2::get_Size

  .method public hidebysig newslot abstract virtual 
          instance !V Lookup ([in] !K key) runtime managed
  {
  } // end of method IMapView`2::Lookup

  .method public hidebysig newslot abstract virtual 
          instance bool HasKey ([in] !K key) runtime managed
  {
  } // end of method IMapView`2::HasKey

  .property instance uint32 Size()
  {
    .get instance uint32 Foundation.Collections.IMapView`2::get_Size()
  } // end of property IMapView`2::Size
} // end of class Foundation.Collections.IMapView`2<K, V>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IVector`1<T>
    implements class Foundation.Collections.IVectorView`1<!T>
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 21 19 6B 02 0D 4F C8 5E 9B 14 DF 5B DB C4 B6 A5 00 00 )

  .method public hidebysig newslot abstract virtual 
          instance class Foundation.Collections.IVectorView`1<!T> GetView () runtime managed
  {
  } // end of method IVector`1::GetView

  .method public hidebysig newslot abstract virtual 
          instance void SetAt ([in] uint32 index, [in] !T 'value') runtime managed
  {
  } // end of method IVector`1::SetAt

  .method public hidebysig newslot abstract virtual 
          instance void InsertAt ([in] uint32 index, [in] !T 'value') runtime managed
  {
  } // end of method IVector`1::InsertAt

  .method public hidebysig newslot abstract virtual 
          instance void RemoveAt ([in] uint32 index) runtime managed
  {
  } // end of method IVector`1::RemoveAt

  .method public hidebysig newslot abstract virtual 
          instance void Append ([in] !T 'value') runtime managed
  {
  } // end of method IVector`1::Append

  .method public hidebysig newslot abstract virtual 
          instance void RemoveAtEnd () runtime managed
  {
  } // end of method IVector`1::RemoveAtEnd

  .method public hidebysig newslot abstract virtual 
          instance void Clear () runtime managed
  {
  } // end of method IVector`1::Clear

  .method public hidebysig newslot abstract virtual 
          instance void ReplaceAll ([in] !T[] items) runtime managed
  {
  } // end of method IVector`1::ReplaceAll
} // end of class Foundation.Collections.IVector`1<T>

.class interface public abstract auto ansi windowsruntime Foundation.Collections.IVectorView`1<T>
    implements class Foundation.Collections.IIterable`1<!T>
{
  .custom instance void [xlang_foundation]Foundation.Metadata.GuidAttribute::.ctor(
        uint32, uint16, uint16, uint8, uint8, uint8, uint8, uint8, uint8, uint8, uint8) 
        = ( 01 00 F7 17 D3 AD C6 43 35 5A 83 93 55 DB 93 DF 21 13 00 00 )

  .method public hidebysig newslot specialname abstract virtual 
          instance uint32 get_Size () runtime managed
  {
  } // end of method IVectorView`1::get_Size

  .method public hidebysig newslot abstract virtual 
          instance !T GetAt ([in] uint32 index) runtime managed
  {
  } // end of method IVectorView`1::GetAt

  .method public hidebysig newslot abstract virtual 
          instance bool IndexOf ([in] !T 'value', [out] uint32& index) runtime managed
  {
  } // end of method IVectorView`1::IndexOf

  .method public hidebysig newslot abstract virtual 
          instance uint32 GetMany ([in] uint32 startIndex, [out] !T[]& items) runtime managed
  {
  } // end of method IVectorView`1::GetMany

  .property instance uint32 Size()
  {
    .get instance uint32 Foundation.Collections.IVectorView`1::get_Size()
  } // end of property IVectorView`1::Size
} // end of class Foundation.Collections.IVectorView`1<T>

