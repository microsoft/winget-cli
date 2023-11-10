/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 ***/

#pragma once
// selected MS SAL annotations

#ifdef _In_
#undef _In_
#endif
#define _In_

#ifdef _Inout_
#undef _Inout_
#endif
#define _Inout_

#ifdef _Out_
#undef _Out_
#endif
#define _Out_

#ifdef _In_z_
#undef _In_z_
#endif
#define _In_z_

#ifdef _Out_z_
#undef _Out_z_
#endif
#define _Out_z_

#ifdef _Inout_z_
#undef _Inout_z_
#endif
#define _Inout_z_

#ifdef _In_opt_
#undef _In_opt_
#endif
#define _In_opt_

#ifdef _Out_opt_
#undef _Out_opt_
#endif
#define _Out_opt_

#ifdef _Inout_opt_
#undef _Inout_opt_
#endif
#define _Inout_opt_

#ifdef _Out_writes_
#undef _Out_writes_
#endif
#define _Out_writes_(x)

#ifdef _Out_writes_opt_
#undef _Out_writes_opt_
#endif
#define _Out_writes_opt_(x)

#ifdef _In_reads_
#undef _In_reads_
#endif
#define _In_reads_(x)

#ifdef _Inout_updates_bytes_
#undef _Inout_updates_bytes_
#endif
#define _Inout_updates_bytes_(x)
