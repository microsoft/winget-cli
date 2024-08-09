//////////////////////////////////////////////////////////////////////////////
//
//  Detours Disassembler (disasm.cpp of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

// #define DETOUR_DEBUG 1
#define DETOURS_INTERNAL
#include "detours.h"
#include <limits.h>

#if DETOURS_VERSION != 0x4c0c1   // 0xMAJORcMINORcPATCH
#error detours.h version mismatch
#endif

#undef ASSERT
#define ASSERT(x)

//////////////////////////////////////////////////////////////////////////////
//
//  Special macros to handle the case when we are building disassembler for
//  offline processing.
//


#if defined(DETOURS_X86_OFFLINE_LIBRARY) \
 || defined(DETOURS_X64_OFFLINE_LIBRARY) \
 || defined(DETOURS_ARM_OFFLINE_LIBRARY) \
 || defined(DETOURS_ARM64_OFFLINE_LIBRARY) \
 || defined(DETOURS_IA64_OFFLINE_LIBRARY)

#undef DETOURS_X64
#undef DETOURS_X86
#undef DETOURS_IA64
#undef DETOURS_ARM
#undef DETOURS_ARM64

#if defined(DETOURS_X86_OFFLINE_LIBRARY)

#define DetourCopyInstruction   DetourCopyInstructionX86
#define DetourSetCodeModule     DetourSetCodeModuleX86
#define CDetourDis              CDetourDisX86
#define DETOURS_X86

#elif defined(DETOURS_X64_OFFLINE_LIBRARY)

#if !defined(DETOURS_64BIT)
// Fix this as/if bugs are discovered.
//#error X64 disassembler can only build for 64-bit.
#endif

#define DetourCopyInstruction   DetourCopyInstructionX64
#define DetourSetCodeModule     DetourSetCodeModuleX64
#define CDetourDis              CDetourDisX64
#define DETOURS_X64

#elif defined(DETOURS_ARM_OFFLINE_LIBRARY)

#define DetourCopyInstruction   DetourCopyInstructionARM
#define DetourSetCodeModule     DetourSetCodeModuleARM
#define CDetourDis              CDetourDisARM
#define DETOURS_ARM

#elif defined(DETOURS_ARM64_OFFLINE_LIBRARY)

#define DetourCopyInstruction   DetourCopyInstructionARM64
#define DetourSetCodeModule     DetourSetCodeModuleARM64
#define CDetourDis              CDetourDisARM64
#define DETOURS_ARM64

#elif defined(DETOURS_IA64_OFFLINE_LIBRARY)

#define DetourCopyInstruction   DetourCopyInstructionIA64
#define DetourSetCodeModule     DetourSetCodeModuleIA64
#define DETOURS_IA64

#else

#error

#endif
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  Function:
//      DetourCopyInstruction(PVOID pDst,
//                            PVOID *ppDstPool
//                            PVOID pSrc,
//                            PVOID *ppTarget,
//                            LONG *plExtra)
//  Purpose:
//      Copy a single instruction from pSrc to pDst.
//
//  Arguments:
//      pDst:
//          Destination address for the instruction.  May be NULL in which
//          case DetourCopyInstruction is used to measure an instruction.
//          If not NULL then the source instruction is copied to the
//          destination instruction and any relative arguments are adjusted.
//      ppDstPool:
//          Destination address for the end of the constant pool.  The
//          constant pool works backwards toward pDst.  All memory between
//          pDst and *ppDstPool must be available for use by this function.
//          ppDstPool may be NULL if pDst is NULL.
//      pSrc:
//          Source address of the instruction.
//      ppTarget:
//          Out parameter for any target instruction address pointed to by
//          the instruction.  For example, a branch or a jump insruction has
//          a target, but a load or store instruction doesn't.  A target is
//          another instruction that may be executed as a result of this
//          instruction.  ppTarget may be NULL.
//      plExtra:
//          Out parameter for the number of extra bytes needed by the
//          instruction to reach the target.  For example, lExtra = 3 if the
//          instruction had an 8-bit relative offset, but needs a 32-bit
//          relative offset.
//
//  Returns:
//      Returns the address of the next instruction (following in the source)
//      instruction.  By subtracting pSrc from the return value, the caller
//      can determinte the size of the instruction copied.
//
//  Comments:
//      By following the pTarget, the caller can follow alternate
//      instruction streams.  However, it is not always possible to determine
//      the target based on static analysis.  For example, the destination of
//      a jump relative to a register cannot be determined from just the
//      instruction stream.  The output value, pTarget, can have any of the
//      following outputs:
//          DETOUR_INSTRUCTION_TARGET_NONE:
//              The instruction has no targets.
//          DETOUR_INSTRUCTION_TARGET_DYNAMIC:
//              The instruction has a non-deterministic (dynamic) target.
//              (i.e. the jump is to an address held in a register.)
//          Address:   The instruction has the specified target.
//
//      When copying instructions, DetourCopyInstruction insures that any
//      targets remain constant.  It does so by adjusting any IP relative
//      offsets.
//

#pragma data_seg(".detourd")
#pragma const_seg(".detourc")

//////////////////////////////////////////////////// X86 and X64 Disassembler.
//
//  Includes full support for all x86 chips prior to the Pentium III, and some newer stuff.
//
#if defined(DETOURS_X64) || defined(DETOURS_X86)

class CDetourDis
{
  public:
    CDetourDis(_Out_opt_ PBYTE *ppbTarget,
               _Out_opt_ LONG *plExtra);

    PBYTE   CopyInstruction(PBYTE pbDst, PBYTE pbSrc);
    static BOOL SanityCheckSystem();
    static BOOL SetCodeModule(PBYTE pbBeg, PBYTE pbEnd, BOOL fLimitReferencesToModule);

  public:
    struct COPYENTRY;
    typedef const COPYENTRY * REFCOPYENTRY;

    typedef PBYTE (CDetourDis::* COPYFUNC)(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

    // nFlagBits flags.
    enum {
        DYNAMIC     = 0x1u,
        ADDRESS     = 0x2u,
        NOENLARGE   = 0x4u,
        RAX         = 0x8u,
    };

    // ModR/M Flags
    enum {
        SIB         = 0x10u,
        RIP         = 0x20u,
        NOTSIB      = 0x0fu,
    };

    struct COPYENTRY
    {
        // Many of these fields are often ignored. See ENTRY_DataIgnored.
        ULONG       nOpcode         : 8;    // Opcode (ignored)
        ULONG       nFixedSize      : 4;    // Fixed size of opcode
        ULONG       nFixedSize16    : 4;    // Fixed size when 16 bit operand
        ULONG       nModOffset      : 4;    // Offset to mod/rm byte (0=none)
        ULONG       nRelOffset      : 4;    // Offset to relative target.
        ULONG       nFlagBits       : 4;    // Flags for DYNAMIC, etc.
        COPYFUNC    pfCopy;                 // Function pointer.
    };

  protected:
// These macros define common uses of nFixedSize, nFixedSize16, nModOffset, nRelOffset, nFlagBits, pfCopy.
#define ENTRY_DataIgnored           0, 0, 0, 0, 0,
#define ENTRY_CopyBytes1            1, 1, 0, 0, 0, &CDetourDis::CopyBytes
#ifdef DETOURS_X64
#define ENTRY_CopyBytes1Address     9, 5, 0, 0, ADDRESS, &CDetourDis::CopyBytes
#else
#define ENTRY_CopyBytes1Address     5, 3, 0, 0, ADDRESS, &CDetourDis::CopyBytes
#endif
#define ENTRY_CopyBytes1Dynamic     1, 1, 0, 0, DYNAMIC, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2            2, 2, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2Jump        ENTRY_DataIgnored &CDetourDis::CopyBytesJump
#define ENTRY_CopyBytes2CantJump    2, 2, 0, 1, NOENLARGE, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2Dynamic     2, 2, 0, 0, DYNAMIC, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3            3, 3, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Dynamic     3, 3, 0, 0, DYNAMIC, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Or5         5, 3, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Or5Dynamic  5, 3, 0, 0, DYNAMIC, &CDetourDis::CopyBytes // x86 only
#ifdef DETOURS_X64
#define ENTRY_CopyBytes3Or5Rax      5, 3, 0, 0, RAX, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Or5Target   5, 5, 0, 1, 0, &CDetourDis::CopyBytes
#else
#define ENTRY_CopyBytes3Or5Rax      5, 3, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Or5Target   5, 3, 0, 1, 0, &CDetourDis::CopyBytes
#endif
#define ENTRY_CopyBytes4            4, 4, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes5            5, 5, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes5Or7Dynamic  7, 5, 0, 0, DYNAMIC, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes7            7, 7, 0, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2Mod         2, 2, 1, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2ModDynamic  2, 2, 1, 0, DYNAMIC, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2Mod1        3, 3, 1, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes2ModOperand  6, 4, 1, 0, 0, &CDetourDis::CopyBytes
#define ENTRY_CopyBytes3Mod         3, 3, 2, 0, 0, &CDetourDis::CopyBytes // SSE3 0F 38 opcode modrm
#define ENTRY_CopyBytes3Mod1        4, 4, 2, 0, 0, &CDetourDis::CopyBytes // SSE3 0F 3A opcode modrm .. imm8
#define ENTRY_CopyBytesPrefix       ENTRY_DataIgnored &CDetourDis::CopyBytesPrefix
#define ENTRY_CopyBytesSegment      ENTRY_DataIgnored &CDetourDis::CopyBytesSegment
#define ENTRY_CopyBytesRax          ENTRY_DataIgnored &CDetourDis::CopyBytesRax
#define ENTRY_CopyF2                ENTRY_DataIgnored &CDetourDis::CopyF2
#define ENTRY_CopyF3                ENTRY_DataIgnored &CDetourDis::CopyF3   // 32bit x86 only
#define ENTRY_Copy0F                ENTRY_DataIgnored &CDetourDis::Copy0F
#define ENTRY_Copy0F78              ENTRY_DataIgnored &CDetourDis::Copy0F78
#define ENTRY_Copy0F00              ENTRY_DataIgnored &CDetourDis::Copy0F00 // 32bit x86 only
#define ENTRY_Copy0FB8              ENTRY_DataIgnored &CDetourDis::Copy0FB8 // 32bit x86 only
#define ENTRY_Copy66                ENTRY_DataIgnored &CDetourDis::Copy66
#define ENTRY_Copy67                ENTRY_DataIgnored &CDetourDis::Copy67
#define ENTRY_CopyF6                ENTRY_DataIgnored &CDetourDis::CopyF6
#define ENTRY_CopyF7                ENTRY_DataIgnored &CDetourDis::CopyF7
#define ENTRY_CopyFF                ENTRY_DataIgnored &CDetourDis::CopyFF
#define ENTRY_CopyVex2              ENTRY_DataIgnored &CDetourDis::CopyVex2
#define ENTRY_CopyVex3              ENTRY_DataIgnored &CDetourDis::CopyVex3
#define ENTRY_CopyEvex              ENTRY_DataIgnored &CDetourDis::CopyEvex // 62, 3 byte payload, then normal with implied prefixes like vex
#define ENTRY_CopyXop               ENTRY_DataIgnored &CDetourDis::CopyXop   // 0x8F ... POP /0 or AMD XOP
#define ENTRY_CopyBytesXop          5, 5, 4, 0, 0, &CDetourDis::CopyBytes // 0x8F xop1 xop2 opcode modrm
#define ENTRY_CopyBytesXop1         6, 6, 4, 0, 0, &CDetourDis::CopyBytes // 0x8F xop1 xop2 opcode modrm ... imm8
#define ENTRY_CopyBytesXop4         9, 9, 4, 0, 0, &CDetourDis::CopyBytes // 0x8F xop1 xop2 opcode modrm ... imm32
#define ENTRY_Invalid               ENTRY_DataIgnored &CDetourDis::Invalid
#define ENTRY_End                   ENTRY_DataIgnored NULL

    PBYTE CopyBytes(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyBytesPrefix(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyBytesSegment(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyBytesRax(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyBytesJump(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

    PBYTE Invalid(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

    PBYTE AdjustTarget(PBYTE pbDst, PBYTE pbSrc, UINT cbOp,
                       UINT cbTargetOffset, UINT cbTargetSize);

  protected:
    PBYTE Copy0F(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE Copy0F00(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc); // x86 only sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
    PBYTE Copy0F78(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc); // vmread, 66/extrq/ib/ib, F2/insertq/ib/ib
    PBYTE Copy0FB8(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc); // jmpe or F3/popcnt
    PBYTE Copy66(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE Copy67(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyF2(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyF3(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc); // x86 only
    PBYTE CopyF6(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyF7(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyFF(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyVex2(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyVex3(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyVexCommon(BYTE m, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyVexEvexCommon(BYTE m, PBYTE pbDst, PBYTE pbSrc, BYTE p);
    PBYTE CopyEvex(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);
    PBYTE CopyXop(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc);

  protected:
    static const COPYENTRY  s_rceCopyTable[257];
    static const COPYENTRY  s_rceCopyTable0F[257];
    static const BYTE       s_rbModRm[256];
    static PBYTE            s_pbModuleBeg;
    static PBYTE            s_pbModuleEnd;
    static BOOL             s_fLimitReferencesToModule;

  protected:
    BOOL                m_bOperandOverride;
    BOOL                m_bAddressOverride;
    BOOL                m_bRaxOverride; // AMD64 only
    BOOL                m_bVex;
    BOOL                m_bEvex;
    BOOL                m_bF2;
    BOOL                m_bF3; // x86 only
    BYTE                m_nSegmentOverride;

    PBYTE *             m_ppbTarget;
    LONG *              m_plExtra;

    LONG                m_lScratchExtra;
    PBYTE               m_pbScratchTarget;
    BYTE                m_rbScratchDst[64]; // matches or exceeds rbCode
};

PVOID WINAPI DetourCopyInstruction(_In_opt_ PVOID pDst,
                                   _Inout_opt_ PVOID *ppDstPool,
                                   _In_ PVOID pSrc,
                                   _Out_opt_ PVOID *ppTarget,
                                   _Out_opt_ LONG *plExtra)
{
    UNREFERENCED_PARAMETER(ppDstPool);  // x86 & x64 don't use a constant pool.

    CDetourDis oDetourDisasm((PBYTE*)ppTarget, plExtra);
    return oDetourDisasm.CopyInstruction((PBYTE)pDst, (PBYTE)pSrc);
}

/////////////////////////////////////////////////////////// Disassembler Code.
//
CDetourDis::CDetourDis(_Out_opt_ PBYTE *ppbTarget, _Out_opt_ LONG *plExtra)
{
    m_bOperandOverride = FALSE;
    m_bAddressOverride = FALSE;
    m_bRaxOverride = FALSE;
    m_bF2 = FALSE;
    m_bF3 = FALSE;
    m_bVex = FALSE;
    m_bEvex = FALSE;

    m_ppbTarget = ppbTarget ? ppbTarget : &m_pbScratchTarget;
    m_plExtra = plExtra ? plExtra : &m_lScratchExtra;

    *m_ppbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_NONE;
    *m_plExtra = 0;
}

PBYTE CDetourDis::CopyInstruction(PBYTE pbDst, PBYTE pbSrc)
{
    // Configure scratch areas if real areas are not available.
    if (NULL == pbDst) {
        pbDst = m_rbScratchDst;
    }
    if (NULL == pbSrc) {
        // We can't copy a non-existent instruction.
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }

    // Figure out how big the instruction is, do the appropriate copy,
    // and figure out what the target of the instruction is if any.
    //
    REFCOPYENTRY pEntry = &s_rceCopyTable[pbSrc[0]];
    return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyBytes(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    UINT nBytesFixed;

    if (m_bVex || m_bEvex)
    {
        ASSERT(pEntry->nFlagBits == 0);
        ASSERT(pEntry->nFixedSize == pEntry->nFixedSize16);
    }

    UINT const nModOffset = pEntry->nModOffset;
    UINT const nFlagBits = pEntry->nFlagBits;
    UINT const nFixedSize = pEntry->nFixedSize;
    UINT const nFixedSize16 = pEntry->nFixedSize16;

    if (nFlagBits & ADDRESS) {
        nBytesFixed = m_bAddressOverride ? nFixedSize16 : nFixedSize;
    }
#ifdef DETOURS_X64
    // REX.W trumps 66
    else if (m_bRaxOverride) {
        nBytesFixed = nFixedSize + ((nFlagBits & RAX) ? 4 : 0);
    }
#endif
    else {
        nBytesFixed = m_bOperandOverride ? nFixedSize16 : nFixedSize;
    }

    UINT nBytes = nBytesFixed;
    UINT nRelOffset = pEntry->nRelOffset;
    UINT cbTarget = nBytes - nRelOffset;
    if (nModOffset > 0) {
        ASSERT(nRelOffset == 0);
        BYTE const bModRm = pbSrc[nModOffset];
        BYTE const bFlags = s_rbModRm[bModRm];

        nBytes += bFlags & NOTSIB;

        if (bFlags & SIB) {
            BYTE const bSib = pbSrc[nModOffset + 1];

            if ((bSib & 0x07) == 0x05) {
                if ((bModRm & 0xc0) == 0x00) {
                    nBytes += 4;
                }
                else if ((bModRm & 0xc0) == 0x40) {
                    nBytes += 1;
                }
                else if ((bModRm & 0xc0) == 0x80) {
                    nBytes += 4;
                }
            }
            cbTarget = nBytes - nRelOffset;
        }
#ifdef DETOURS_X64
        else if (bFlags & RIP) {
            nRelOffset = nModOffset + 1;
            cbTarget = 4;
        }
#endif
    }
    CopyMemory(pbDst, pbSrc, nBytes);

    if (nRelOffset) {
        *m_ppbTarget = AdjustTarget(pbDst, pbSrc, nBytes, nRelOffset, cbTarget);
#ifdef DETOURS_X64
        if (pEntry->nRelOffset == 0) {
            // This is a data target, not a code target, so we shouldn't return it.
            *m_ppbTarget = NULL;
        }
#endif
    }
    if (nFlagBits & NOENLARGE) {
        *m_plExtra = -*m_plExtra;
    }
    if (nFlagBits & DYNAMIC) {
        *m_ppbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }
    return pbSrc + nBytes;
}

PBYTE CDetourDis::CopyBytesPrefix(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    pbDst[0] = pbSrc[0];
    pEntry = &s_rceCopyTable[pbSrc[1]];
    return (this->*pEntry->pfCopy)(pEntry, pbDst + 1, pbSrc + 1);
}

PBYTE CDetourDis::CopyBytesSegment(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
{
    m_nSegmentOverride = pbSrc[0];
    return CopyBytesPrefix(0, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyBytesRax(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
{ // AMD64 only
    if (pbSrc[0] & 0x8) {
        m_bRaxOverride = TRUE;
    }
    return CopyBytesPrefix(0, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyBytesJump(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    (void)pEntry;

    PVOID pvSrcAddr = &pbSrc[1];
    PVOID pvDstAddr = NULL;
    LONG_PTR nOldOffset = (LONG_PTR)*(signed char*&)pvSrcAddr;
    LONG_PTR nNewOffset = 0;

    *m_ppbTarget = pbSrc + 2 + nOldOffset;

    if (pbSrc[0] == 0xeb) {
        pbDst[0] = 0xe9;
        pvDstAddr = &pbDst[1];
        nNewOffset = nOldOffset - ((pbDst - pbSrc) + 3);
        *(UNALIGNED LONG*&)pvDstAddr = (LONG)nNewOffset;

        *m_plExtra = 3;
        return pbSrc + 2;
    }

    ASSERT(pbSrc[0] >= 0x70 && pbSrc[0] <= 0x7f);

    pbDst[0] = 0x0f;
    pbDst[1] = 0x80 | (pbSrc[0] & 0xf);
    pvDstAddr = &pbDst[2];
    nNewOffset = nOldOffset - ((pbDst - pbSrc) + 4);
    *(UNALIGNED LONG*&)pvDstAddr = (LONG)nNewOffset;

    *m_plExtra = 4;
    return pbSrc + 2;
}

PBYTE CDetourDis::AdjustTarget(PBYTE pbDst, PBYTE pbSrc, UINT cbOp,
                               UINT cbTargetOffset, UINT cbTargetSize)
{
    PBYTE pbTarget = NULL;
#if 1 // fault injection to test test code
#if defined(DETOURS_X64)
    typedef LONGLONG T;
#else
    typedef LONG T;
#endif
    T nOldOffset;
    T nNewOffset;
    PVOID pvTargetAddr = &pbDst[cbTargetOffset];

    switch (cbTargetSize) {
      case 1:
        nOldOffset = *(signed char*&)pvTargetAddr;
        break;
      case 2:
        nOldOffset = *(UNALIGNED SHORT*&)pvTargetAddr;
        break;
      case 4:
        nOldOffset = *(UNALIGNED LONG*&)pvTargetAddr;
        break;
#if defined(DETOURS_X64)
      case 8:
        nOldOffset = *(UNALIGNED LONGLONG*&)pvTargetAddr;
        break;
#endif
      default:
        ASSERT(!"cbTargetSize is invalid.");
        nOldOffset = 0;
        break;
    }

    pbTarget = pbSrc + cbOp + nOldOffset;
    nNewOffset = nOldOffset - (T)(pbDst - pbSrc);

    switch (cbTargetSize) {
      case 1:
        *(CHAR*&)pvTargetAddr = (CHAR)nNewOffset;
        if (nNewOffset < SCHAR_MIN || nNewOffset > SCHAR_MAX) {
            *m_plExtra = sizeof(ULONG) - 1;
        }
        break;
      case 2:
        *(UNALIGNED SHORT*&)pvTargetAddr = (SHORT)nNewOffset;
        if (nNewOffset < SHRT_MIN || nNewOffset > SHRT_MAX) {
            *m_plExtra = sizeof(ULONG) - 2;
        }
        break;
      case 4:
        *(UNALIGNED LONG*&)pvTargetAddr = (LONG)nNewOffset;
        if (nNewOffset < LONG_MIN || nNewOffset > LONG_MAX) {
            *m_plExtra = sizeof(ULONG) - 4;
        }
        break;
#if defined(DETOURS_X64)
      case 8:
        *(UNALIGNED LONGLONG*&)pvTargetAddr = nNewOffset;
        break;
#endif
    }
#ifdef DETOURS_X64
    // When we are only computing size, source and dest can be
    // far apart, distance not encodable in 32bits. Ok.
    // At least still check the lower 32bits.

    if (pbDst >= m_rbScratchDst && pbDst < (sizeof(m_rbScratchDst) + m_rbScratchDst)) {
        ASSERT((((size_t)pbDst + cbOp + nNewOffset) & 0xFFFFFFFF) == (((size_t)pbTarget) & 0xFFFFFFFF));
    }
    else
#endif
    {
        ASSERT(pbDst + cbOp + nNewOffset == pbTarget);
    }
#endif
    return pbTarget;
}

PBYTE CDetourDis::Invalid(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    (void)pbDst;
    (void)pEntry;
    ASSERT(!"Invalid Instruction");
    return pbSrc + 1;
}

////////////////////////////////////////////////////// Individual Bytes Codes.
//
PBYTE CDetourDis::Copy0F(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    pbDst[0] = pbSrc[0];
    pEntry = &s_rceCopyTable0F[pbSrc[1]];
    return (this->*pEntry->pfCopy)(pEntry, pbDst + 1, pbSrc + 1);
}

PBYTE CDetourDis::Copy0F78(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
{
    // vmread, 66/extrq, F2/insertq

    static const COPYENTRY vmread = { 0x78, ENTRY_CopyBytes2Mod };
    static const COPYENTRY extrq_insertq = { 0x78, ENTRY_CopyBytes4 };

    ASSERT(!(m_bF2 && m_bOperandOverride));

    // For insertq and presumably despite documentation extrq, mode must be 11, not checked.
    // insertq/extrq/78 are followed by two immediate bytes, and given mode == 11, mod/rm byte is always one byte,
    // and the 0x78 makes 4 bytes (not counting the 66/F2/F which are accounted for elsewhere)

    REFCOPYENTRY const pEntry = ((m_bF2 || m_bOperandOverride) ? &extrq_insertq : &vmread);

    return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::Copy0F00(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
{
    // jmpe is 32bit x86 only
    // Notice that the sizes are the same either way, but jmpe is marked as "dynamic".

    static const COPYENTRY other = { 0xB8, ENTRY_CopyBytes2Mod }; // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6 invalid/7
    static const COPYENTRY jmpe = { 0xB8, ENTRY_CopyBytes2ModDynamic }; // jmpe/6 x86-on-IA64 syscalls

    REFCOPYENTRY const pEntry = (((6 << 3) == ((7 << 3) & pbSrc[1])) ?  &jmpe : &other);
    return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::Copy0FB8(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
{
    // jmpe is 32bit x86 only

    static const COPYENTRY popcnt = { 0xB8, ENTRY_CopyBytes2Mod };
    static const COPYENTRY jmpe = { 0xB8, ENTRY_CopyBytes3Or5Dynamic }; // jmpe x86-on-IA64 syscalls
    REFCOPYENTRY const pEntry = m_bF3 ? &popcnt : &jmpe;
    return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::Copy66(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{   // Operand-size override prefix
    m_bOperandOverride = TRUE;
    return CopyBytesPrefix(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::Copy67(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{   // Address size override prefix
    m_bAddressOverride = TRUE;
    return CopyBytesPrefix(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyF2(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    m_bF2 = TRUE;
    return CopyBytesPrefix(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyF3(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{ // x86 only
    m_bF3 = TRUE;
    return CopyBytesPrefix(pEntry, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyF6(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    (void)pEntry;

    // TEST BYTE /0
    if (0x00 == (0x38 & pbSrc[1])) {    // reg(bits 543) of ModR/M == 0
        static const COPYENTRY ce = { 0xf6, ENTRY_CopyBytes2Mod1 };
        return (this->*ce.pfCopy)(&ce, pbDst, pbSrc);
    }
    // DIV /6
    // IDIV /7
    // IMUL /5
    // MUL /4
    // NEG /3
    // NOT /2

    static const COPYENTRY ce = { 0xf6, ENTRY_CopyBytes2Mod };
    return (this->*ce.pfCopy)(&ce, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyF7(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{
    (void)pEntry;

    // TEST WORD /0
    if (0x00 == (0x38 & pbSrc[1])) {    // reg(bits 543) of ModR/M == 0
        static const COPYENTRY ce = { 0xf7, ENTRY_CopyBytes2ModOperand };
        return (this->*ce.pfCopy)(&ce, pbDst, pbSrc);
    }

    // DIV /6
    // IDIV /7
    // IMUL /5
    // MUL /4
    // NEG /3
    // NOT /2
    static const COPYENTRY ce = { 0xf7, ENTRY_CopyBytes2Mod };
    return (this->*ce.pfCopy)(&ce, pbDst, pbSrc);
}

PBYTE CDetourDis::CopyFF(REFCOPYENTRY pEntry, PBYTE pbDst, PBYTE pbSrc)
{   // INC /0
    // DEC /1
    // CALL /2
    // CALL /3
    // JMP /4
    // JMP /5
    // PUSH /6
    // invalid/7
    (void)pEntry;

    static const COPYENTRY ce = { 0xff, ENTRY_CopyBytes2Mod };
    PBYTE pbOut = (this->*ce.pfCopy)(&ce, pbDst, pbSrc);

    BYTE const b1 = pbSrc[1];

    if (0x15 == b1 || 0x25 == b1) {         // CALL [], JMP []
#ifdef DETOURS_X64
        // All segments but FS and GS are equivalent.
        if (m_nSegmentOverride != 0x64 && m_nSegmentOverride != 0x65)
#else
        if (m_nSegmentOverride == 0 || m_nSegmentOverride == 0x2E)
#endif
        {
#ifdef DETOURS_X64
            INT32 offset = *(UNALIGNED INT32*)&pbSrc[2];
            PBYTE *ppbTarget = (PBYTE *)(pbSrc + 6 + offset);
#else
            PBYTE *ppbTarget = (PBYTE *)(SIZE_T)*(UNALIGNED ULONG*)&pbSrc[2];
#endif
            if (s_fLimitReferencesToModule &&
                (ppbTarget < (PVOID)s_pbModuleBeg || ppbTarget >= (PVOID)s_pbModuleEnd)) {

                *m_ppbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
            }
            else {
                // This can access violate on random bytes. Use DetourSetCodeModule.
                *m_ppbTarget = *ppbTarget;
            }
        }
        else {
            *m_ppbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
        }
    }
    else if (0x10 == (0x30 & b1) || // CALL /2 or /3  --> reg(bits 543) of ModR/M == 010 or 011
             0x20 == (0x30 & b1)) { // JMP /4 or /5 --> reg(bits 543) of ModR/M == 100 or 101
        *m_ppbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }
    return pbOut;
}

PBYTE CDetourDis::CopyVexEvexCommon(BYTE m, PBYTE pbDst, PBYTE pbSrc, BYTE p)
// m is first instead of last in the hopes of pbDst/pbSrc being
// passed along efficiently in the registers they were already in.
{
    static const COPYENTRY ceF38 = { 0x38, ENTRY_CopyBytes2Mod };
    static const COPYENTRY ceF3A = { 0x3A, ENTRY_CopyBytes2Mod1 };
    static const COPYENTRY ceInvalid = { 0xC4, ENTRY_Invalid };

    switch (p & 3) {
    case 0: break;
    case 1: m_bOperandOverride = TRUE; break;
    case 2: m_bF3 = TRUE; break;
    case 3: m_bF2 = TRUE; break;
    }

    REFCOPYENTRY pEntry;

    switch (m) {
    default: return Invalid(&ceInvalid, pbDst, pbSrc);
    case 1:  pEntry = &s_rceCopyTable0F[pbSrc[0]];
             return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
    case 2:  return CopyBytes(&ceF38, pbDst, pbSrc);
    case 3:  return CopyBytes(&ceF3A, pbDst, pbSrc);
    }
}

PBYTE CDetourDis::CopyVexCommon(BYTE m, PBYTE pbDst, PBYTE pbSrc)
// m is first instead of last in the hopes of pbDst/pbSrc being
// passed along efficiently in the registers they were already in.
{
    m_bVex = TRUE;
    BYTE const p = (BYTE)(pbSrc[-1] & 3); // p in last byte
    return CopyVexEvexCommon(m, pbDst, pbSrc, p);
}


PBYTE CDetourDis::CopyVex3(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
// 3 byte VEX prefix 0xC4
{
#ifdef DETOURS_X86
    const static COPYENTRY ceLES = { 0xC4, ENTRY_CopyBytes2Mod };
    if ((pbSrc[1] & 0xC0) != 0xC0) {
        REFCOPYENTRY pEntry = &ceLES;
        return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
    }
#endif
    pbDst[0] = pbSrc[0];
    pbDst[1] = pbSrc[1];
    pbDst[2] = pbSrc[2];
#ifdef DETOURS_X64
    m_bRaxOverride |= !!(pbSrc[2] & 0x80); // w in last byte, see CopyBytesRax
#else
    //
    // TODO
    //
    // Usually the VEX.W bit changes the size of a general purpose register and is ignored for 32bit.
    // Sometimes it is an opcode extension.
    // Look in the Intel manual, in the instruction-by-instruction reference, for ".W1",
    // without nearby wording saying it is ignored for 32bit.
    // For example: "VFMADD132PD/VFMADD213PD/VFMADD231PD Fused Multiply-Add of Packed Double-Precision Floating-Point Values".
    //
    // Then, go through each such case and determine if W0 vs. W1 affect the size of the instruction. Probably not.
    // Look for the same encoding but with "W1" changed to "W0".
    // Here is one such pairing:
    // VFMADD132PD/VFMADD213PD/VFMADD231PD Fused Multiply-Add of Packed Double-Precision Floating-Point Values
    //
    // VEX.DDS.128.66.0F38.W1 98 /r A V/V FMA Multiply packed double-precision floating-point values
    // from xmm0 and xmm2/mem, add to xmm1 and
    // put result in xmm0.
    // VFMADD132PD xmm0, xmm1, xmm2/m128
    //
    // VFMADD132PS/VFMADD213PS/VFMADD231PS Fused Multiply-Add of Packed Single-Precision Floating-Point Values
    // VEX.DDS.128.66.0F38.W0 98 /r A V/V FMA Multiply packed single-precision floating-point values
    // from xmm0 and xmm2/mem, add to xmm1 and put
    // result in xmm0.
    // VFMADD132PS xmm0, xmm1, xmm2/m128
    //
#endif
    return CopyVexCommon(pbSrc[1] & 0x1F, pbDst + 3, pbSrc + 3);
}

PBYTE CDetourDis::CopyVex2(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
// 2 byte VEX prefix 0xC5
{
#ifdef DETOURS_X86
    const static COPYENTRY ceLDS = { 0xC5, ENTRY_CopyBytes2Mod };
    if ((pbSrc[1] & 0xC0) != 0xC0) {
        REFCOPYENTRY pEntry = &ceLDS;
        return (this->*pEntry->pfCopy)(pEntry, pbDst, pbSrc);
    }
#endif
    pbDst[0] = pbSrc[0];
    pbDst[1] = pbSrc[1];
    return CopyVexCommon(1, pbDst + 2, pbSrc + 2);
}

PBYTE CDetourDis::CopyEvex(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
// 62, 3 byte payload, x86 with implied prefixes like Vex
// for 32bit, mode 0xC0 else fallback to bound /r
{
    // NOTE: Intel and Wikipedia number these differently.
    // Intel says 0-2, Wikipedia says 1-3.

    BYTE const p0 = pbSrc[1];

#ifdef DETOURS_X86
    const static COPYENTRY ceBound = { 0x62, ENTRY_CopyBytes2Mod };
    if ((p0 & 0xC0) != 0xC0) {
        return CopyBytes(&ceBound, pbDst, pbSrc);
    }
#endif

    static const COPYENTRY ceInvalid = { 0x62, ENTRY_Invalid };

    if ((p0 & 0x0C) != 0)
        return Invalid(&ceInvalid, pbDst, pbSrc);

    BYTE const p1 = pbSrc[2];

    if ((p1 & 0x04) != 0x04)
        return Invalid(&ceInvalid, pbDst, pbSrc);

    // Copy 4 byte prefix.
    *(UNALIGNED ULONG *)pbDst = *(UNALIGNED ULONG*)pbSrc;

    m_bEvex = TRUE;

#ifdef DETOURS_X64
    m_bRaxOverride |= !!(p1 & 0x80); // w
#endif

    return CopyVexEvexCommon(p0 & 3u, pbDst + 4, pbSrc + 4, p1 & 3u);
}

PBYTE CDetourDis::CopyXop(REFCOPYENTRY, PBYTE pbDst, PBYTE pbSrc)
/* 3 byte AMD XOP prefix 0x8F
byte0: 0x8F
byte1: RXBmmmmm
byte2: WvvvvLpp
byte3: opcode
mmmmm >= 8, else pop
mmmmm only otherwise defined for 8, 9, A.
pp is like VEX but only instructions with 0 are defined
*/
{
    const static COPYENTRY cePop = { 0x8F, ENTRY_CopyBytes2Mod };
    const static COPYENTRY ceXop = { 0x8F, ENTRY_CopyBytesXop };
    const static COPYENTRY ceXop1 = { 0x8F, ENTRY_CopyBytesXop1 };
    const static COPYENTRY ceXop4 = { 0x8F, ENTRY_CopyBytesXop4 };

    BYTE const m = (BYTE)(pbSrc[1] & 0x1F);
    ASSERT(m <= 10);
    switch (m)
    {
    default:
        return CopyBytes(&cePop, pbDst, pbSrc);

    case 8: // modrm with 8bit immediate
        return CopyBytes(&ceXop1, pbDst, pbSrc);

    case 9: // modrm with no immediate
        return CopyBytes(&ceXop, pbDst, pbSrc);

    case 10: // modrm with 32bit immediate
        return CopyBytes(&ceXop4, pbDst, pbSrc);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
PBYTE CDetourDis::s_pbModuleBeg = NULL;
PBYTE CDetourDis::s_pbModuleEnd = (PBYTE)~(ULONG_PTR)0;
BOOL CDetourDis::s_fLimitReferencesToModule = FALSE;

BOOL CDetourDis::SetCodeModule(PBYTE pbBeg, PBYTE pbEnd, BOOL fLimitReferencesToModule)
{
    if (pbEnd < pbBeg) {
        return FALSE;
    }

    s_pbModuleBeg = pbBeg;
    s_pbModuleEnd = pbEnd;
    s_fLimitReferencesToModule = fLimitReferencesToModule;

    return TRUE;
}

///////////////////////////////////////////////////////// Disassembler Tables.
//
const BYTE CDetourDis::s_rbModRm[256] = {
    0,0,0,0, SIB|1,RIP|4,0,0, 0,0,0,0, SIB|1,RIP|4,0,0, // 0x
    0,0,0,0, SIB|1,RIP|4,0,0, 0,0,0,0, SIB|1,RIP|4,0,0, // 1x
    0,0,0,0, SIB|1,RIP|4,0,0, 0,0,0,0, SIB|1,RIP|4,0,0, // 2x
    0,0,0,0, SIB|1,RIP|4,0,0, 0,0,0,0, SIB|1,RIP|4,0,0, // 3x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 4x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 5x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 6x
    1,1,1,1, 2,1,1,1, 1,1,1,1, 2,1,1,1,                 // 7x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // 8x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // 9x
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // Ax
    4,4,4,4, 5,4,4,4, 4,4,4,4, 5,4,4,4,                 // Bx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Cx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Dx
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,                 // Ex
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0                  // Fx
};

const CDetourDis::COPYENTRY CDetourDis::s_rceCopyTable[257] =
{
    { 0x00, ENTRY_CopyBytes2Mod },                      // ADD /r
    { 0x01, ENTRY_CopyBytes2Mod },                      // ADD /r
    { 0x02, ENTRY_CopyBytes2Mod },                      // ADD /r
    { 0x03, ENTRY_CopyBytes2Mod },                      // ADD /r
    { 0x04, ENTRY_CopyBytes2 },                         // ADD ib
    { 0x05, ENTRY_CopyBytes3Or5 },                      // ADD iw
#ifdef DETOURS_X64
    { 0x06, ENTRY_Invalid },                            // Invalid
    { 0x07, ENTRY_Invalid },                            // Invalid
#else
    { 0x06, ENTRY_CopyBytes1 },                         // PUSH
    { 0x07, ENTRY_CopyBytes1 },                         // POP
#endif
    { 0x08, ENTRY_CopyBytes2Mod },                      // OR /r
    { 0x09, ENTRY_CopyBytes2Mod },                      // OR /r
    { 0x0A, ENTRY_CopyBytes2Mod },                      // OR /r
    { 0x0B, ENTRY_CopyBytes2Mod },                      // OR /r
    { 0x0C, ENTRY_CopyBytes2 },                         // OR ib
    { 0x0D, ENTRY_CopyBytes3Or5 },                      // OR iw
#ifdef DETOURS_X64
    { 0x0E, ENTRY_Invalid },                            // Invalid
#else
    { 0x0E, ENTRY_CopyBytes1 },                         // PUSH
#endif
    { 0x0F, ENTRY_Copy0F },                             // Extension Ops
    { 0x10, ENTRY_CopyBytes2Mod },                      // ADC /r
    { 0x11, ENTRY_CopyBytes2Mod },                      // ADC /r
    { 0x12, ENTRY_CopyBytes2Mod },                      // ADC /r
    { 0x13, ENTRY_CopyBytes2Mod },                      // ADC /r
    { 0x14, ENTRY_CopyBytes2 },                         // ADC ib
    { 0x15, ENTRY_CopyBytes3Or5 },                      // ADC id
#ifdef DETOURS_X64
    { 0x16, ENTRY_Invalid },                            // Invalid
    { 0x17, ENTRY_Invalid },                            // Invalid
#else
    { 0x16, ENTRY_CopyBytes1 },                         // PUSH
    { 0x17, ENTRY_CopyBytes1 },                         // POP
#endif
    { 0x18, ENTRY_CopyBytes2Mod },                      // SBB /r
    { 0x19, ENTRY_CopyBytes2Mod },                      // SBB /r
    { 0x1A, ENTRY_CopyBytes2Mod },                      // SBB /r
    { 0x1B, ENTRY_CopyBytes2Mod },                      // SBB /r
    { 0x1C, ENTRY_CopyBytes2 },                         // SBB ib
    { 0x1D, ENTRY_CopyBytes3Or5 },                      // SBB id
#ifdef DETOURS_X64
    { 0x1E, ENTRY_Invalid },                            // Invalid
    { 0x1F, ENTRY_Invalid },                            // Invalid
#else
    { 0x1E, ENTRY_CopyBytes1 },                         // PUSH
    { 0x1F, ENTRY_CopyBytes1 },                         // POP
#endif
    { 0x20, ENTRY_CopyBytes2Mod },                      // AND /r
    { 0x21, ENTRY_CopyBytes2Mod },                      // AND /r
    { 0x22, ENTRY_CopyBytes2Mod },                      // AND /r
    { 0x23, ENTRY_CopyBytes2Mod },                      // AND /r
    { 0x24, ENTRY_CopyBytes2 },                         // AND ib
    { 0x25, ENTRY_CopyBytes3Or5 },                      // AND id
    { 0x26, ENTRY_CopyBytesSegment },                   // ES prefix
#ifdef DETOURS_X64
    { 0x27, ENTRY_Invalid },                            // Invalid
#else
    { 0x27, ENTRY_CopyBytes1 },                         // DAA
#endif
    { 0x28, ENTRY_CopyBytes2Mod },                      // SUB /r
    { 0x29, ENTRY_CopyBytes2Mod },                      // SUB /r
    { 0x2A, ENTRY_CopyBytes2Mod },                      // SUB /r
    { 0x2B, ENTRY_CopyBytes2Mod },                      // SUB /r
    { 0x2C, ENTRY_CopyBytes2 },                         // SUB ib
    { 0x2D, ENTRY_CopyBytes3Or5 },                      // SUB id
    { 0x2E, ENTRY_CopyBytesSegment },                   // CS prefix
#ifdef DETOURS_X64
    { 0x2F, ENTRY_Invalid },                            // Invalid
#else
    { 0x2F, ENTRY_CopyBytes1 },                         // DAS
#endif
    { 0x30, ENTRY_CopyBytes2Mod },                      // XOR /r
    { 0x31, ENTRY_CopyBytes2Mod },                      // XOR /r
    { 0x32, ENTRY_CopyBytes2Mod },                      // XOR /r
    { 0x33, ENTRY_CopyBytes2Mod },                      // XOR /r
    { 0x34, ENTRY_CopyBytes2 },                         // XOR ib
    { 0x35, ENTRY_CopyBytes3Or5 },                      // XOR id
    { 0x36, ENTRY_CopyBytesSegment },                   // SS prefix
#ifdef DETOURS_X64
    { 0x37, ENTRY_Invalid },                            // Invalid
#else
    { 0x37, ENTRY_CopyBytes1 },                         // AAA
#endif
    { 0x38, ENTRY_CopyBytes2Mod },                      // CMP /r
    { 0x39, ENTRY_CopyBytes2Mod },                      // CMP /r
    { 0x3A, ENTRY_CopyBytes2Mod },                      // CMP /r
    { 0x3B, ENTRY_CopyBytes2Mod },                      // CMP /r
    { 0x3C, ENTRY_CopyBytes2 },                         // CMP ib
    { 0x3D, ENTRY_CopyBytes3Or5 },                      // CMP id
    { 0x3E, ENTRY_CopyBytesSegment },                   // DS prefix
#ifdef DETOURS_X64
    { 0x3F, ENTRY_Invalid },                            // Invalid
#else
    { 0x3F, ENTRY_CopyBytes1 },                         // AAS
#endif
#ifdef DETOURS_X64 // For Rax Prefix
    { 0x40, ENTRY_CopyBytesRax },                       // Rax
    { 0x41, ENTRY_CopyBytesRax },                       // Rax
    { 0x42, ENTRY_CopyBytesRax },                       // Rax
    { 0x43, ENTRY_CopyBytesRax },                       // Rax
    { 0x44, ENTRY_CopyBytesRax },                       // Rax
    { 0x45, ENTRY_CopyBytesRax },                       // Rax
    { 0x46, ENTRY_CopyBytesRax },                       // Rax
    { 0x47, ENTRY_CopyBytesRax },                       // Rax
    { 0x48, ENTRY_CopyBytesRax },                       // Rax
    { 0x49, ENTRY_CopyBytesRax },                       // Rax
    { 0x4A, ENTRY_CopyBytesRax },                       // Rax
    { 0x4B, ENTRY_CopyBytesRax },                       // Rax
    { 0x4C, ENTRY_CopyBytesRax },                       // Rax
    { 0x4D, ENTRY_CopyBytesRax },                       // Rax
    { 0x4E, ENTRY_CopyBytesRax },                       // Rax
    { 0x4F, ENTRY_CopyBytesRax },                       // Rax
#else
    { 0x40, ENTRY_CopyBytes1 },                         // INC
    { 0x41, ENTRY_CopyBytes1 },                         // INC
    { 0x42, ENTRY_CopyBytes1 },                         // INC
    { 0x43, ENTRY_CopyBytes1 },                         // INC
    { 0x44, ENTRY_CopyBytes1 },                         // INC
    { 0x45, ENTRY_CopyBytes1 },                         // INC
    { 0x46, ENTRY_CopyBytes1 },                         // INC
    { 0x47, ENTRY_CopyBytes1 },                         // INC
    { 0x48, ENTRY_CopyBytes1 },                         // DEC
    { 0x49, ENTRY_CopyBytes1 },                         // DEC
    { 0x4A, ENTRY_CopyBytes1 },                         // DEC
    { 0x4B, ENTRY_CopyBytes1 },                         // DEC
    { 0x4C, ENTRY_CopyBytes1 },                         // DEC
    { 0x4D, ENTRY_CopyBytes1 },                         // DEC
    { 0x4E, ENTRY_CopyBytes1 },                         // DEC
    { 0x4F, ENTRY_CopyBytes1 },                         // DEC
#endif
    { 0x50, ENTRY_CopyBytes1 },                         // PUSH
    { 0x51, ENTRY_CopyBytes1 },                         // PUSH
    { 0x52, ENTRY_CopyBytes1 },                         // PUSH
    { 0x53, ENTRY_CopyBytes1 },                         // PUSH
    { 0x54, ENTRY_CopyBytes1 },                         // PUSH
    { 0x55, ENTRY_CopyBytes1 },                         // PUSH
    { 0x56, ENTRY_CopyBytes1 },                         // PUSH
    { 0x57, ENTRY_CopyBytes1 },                         // PUSH
    { 0x58, ENTRY_CopyBytes1 },                         // POP
    { 0x59, ENTRY_CopyBytes1 },                         // POP
    { 0x5A, ENTRY_CopyBytes1 },                         // POP
    { 0x5B, ENTRY_CopyBytes1 },                         // POP
    { 0x5C, ENTRY_CopyBytes1 },                         // POP
    { 0x5D, ENTRY_CopyBytes1 },                         // POP
    { 0x5E, ENTRY_CopyBytes1 },                         // POP
    { 0x5F, ENTRY_CopyBytes1 },                         // POP
#ifdef DETOURS_X64
    { 0x60, ENTRY_Invalid },                            // Invalid
    { 0x61, ENTRY_Invalid },                            // Invalid
    { 0x62, ENTRY_CopyEvex },                           // EVEX / AVX512
#else
    { 0x60, ENTRY_CopyBytes1 },                         // PUSHAD
    { 0x61, ENTRY_CopyBytes1 },                         // POPAD
    { 0x62, ENTRY_CopyEvex },                           // BOUND /r and EVEX / AVX512
#endif
    { 0x63, ENTRY_CopyBytes2Mod },                      // 32bit ARPL /r, 64bit MOVSXD
    { 0x64, ENTRY_CopyBytesSegment },                   // FS prefix
    { 0x65, ENTRY_CopyBytesSegment },                   // GS prefix
    { 0x66, ENTRY_Copy66 },                             // Operand Prefix
    { 0x67, ENTRY_Copy67 },                             // Address Prefix
    { 0x68, ENTRY_CopyBytes3Or5 },                      // PUSH
    { 0x69, ENTRY_CopyBytes2ModOperand },               // IMUL /r iz
    { 0x6A, ENTRY_CopyBytes2 },                         // PUSH
    { 0x6B, ENTRY_CopyBytes2Mod1 },                     // IMUL /r ib
    { 0x6C, ENTRY_CopyBytes1 },                         // INS
    { 0x6D, ENTRY_CopyBytes1 },                         // INS
    { 0x6E, ENTRY_CopyBytes1 },                         // OUTS/OUTSB
    { 0x6F, ENTRY_CopyBytes1 },                         // OUTS/OUTSW
    { 0x70, ENTRY_CopyBytes2Jump },                     // JO           // 0f80
    { 0x71, ENTRY_CopyBytes2Jump },                     // JNO          // 0f81
    { 0x72, ENTRY_CopyBytes2Jump },                     // JB/JC/JNAE   // 0f82
    { 0x73, ENTRY_CopyBytes2Jump },                     // JAE/JNB/JNC  // 0f83
    { 0x74, ENTRY_CopyBytes2Jump },                     // JE/JZ        // 0f84
    { 0x75, ENTRY_CopyBytes2Jump },                     // JNE/JNZ      // 0f85
    { 0x76, ENTRY_CopyBytes2Jump },                     // JBE/JNA      // 0f86
    { 0x77, ENTRY_CopyBytes2Jump },                     // JA/JNBE      // 0f87
    { 0x78, ENTRY_CopyBytes2Jump },                     // JS           // 0f88
    { 0x79, ENTRY_CopyBytes2Jump },                     // JNS          // 0f89
    { 0x7A, ENTRY_CopyBytes2Jump },                     // JP/JPE       // 0f8a
    { 0x7B, ENTRY_CopyBytes2Jump },                     // JNP/JPO      // 0f8b
    { 0x7C, ENTRY_CopyBytes2Jump },                     // JL/JNGE      // 0f8c
    { 0x7D, ENTRY_CopyBytes2Jump },                     // JGE/JNL      // 0f8d
    { 0x7E, ENTRY_CopyBytes2Jump },                     // JLE/JNG      // 0f8e
    { 0x7F, ENTRY_CopyBytes2Jump },                     // JG/JNLE      // 0f8f
    { 0x80, ENTRY_CopyBytes2Mod1 },                     // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 byte reg, immediate byte
    { 0x81, ENTRY_CopyBytes2ModOperand },               // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 byte reg, immediate word or dword
#ifdef DETOURS_X64
    { 0x82, ENTRY_Invalid },                            // Invalid
#else
    { 0x82, ENTRY_CopyBytes2Mod1 },                     // MOV al,x
#endif
    { 0x83, ENTRY_CopyBytes2Mod1 },                     // ADD/0 OR/1 ADC/2 SBB/3 AND/4 SUB/5 XOR/6 CMP/7 reg, immediate byte
    { 0x84, ENTRY_CopyBytes2Mod },                      // TEST /r
    { 0x85, ENTRY_CopyBytes2Mod },                      // TEST /r
    { 0x86, ENTRY_CopyBytes2Mod },                      // XCHG /r @todo
    { 0x87, ENTRY_CopyBytes2Mod },                      // XCHG /r @todo
    { 0x88, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x89, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x8A, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x8B, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x8C, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x8D, ENTRY_CopyBytes2Mod },                      // LEA /r
    { 0x8E, ENTRY_CopyBytes2Mod },                      // MOV /r
    { 0x8F, ENTRY_CopyXop },                            // POP /0 or AMD XOP
    { 0x90, ENTRY_CopyBytes1 },                         // NOP
    { 0x91, ENTRY_CopyBytes1 },                         // XCHG
    { 0x92, ENTRY_CopyBytes1 },                         // XCHG
    { 0x93, ENTRY_CopyBytes1 },                         // XCHG
    { 0x94, ENTRY_CopyBytes1 },                         // XCHG
    { 0x95, ENTRY_CopyBytes1 },                         // XCHG
    { 0x96, ENTRY_CopyBytes1 },                         // XCHG
    { 0x97, ENTRY_CopyBytes1 },                         // XCHG
    { 0x98, ENTRY_CopyBytes1 },                         // CWDE
    { 0x99, ENTRY_CopyBytes1 },                         // CDQ
#ifdef DETOURS_X64
    { 0x9A, ENTRY_Invalid },                            // Invalid
#else
    { 0x9A, ENTRY_CopyBytes5Or7Dynamic },               // CALL cp
#endif
    { 0x9B, ENTRY_CopyBytes1 },                         // WAIT/FWAIT
    { 0x9C, ENTRY_CopyBytes1 },                         // PUSHFD
    { 0x9D, ENTRY_CopyBytes1 },                         // POPFD
    { 0x9E, ENTRY_CopyBytes1 },                         // SAHF
    { 0x9F, ENTRY_CopyBytes1 },                         // LAHF
    { 0xA0, ENTRY_CopyBytes1Address },                  // MOV
    { 0xA1, ENTRY_CopyBytes1Address },                  // MOV
    { 0xA2, ENTRY_CopyBytes1Address },                  // MOV
    { 0xA3, ENTRY_CopyBytes1Address },                  // MOV
    { 0xA4, ENTRY_CopyBytes1 },                         // MOVS
    { 0xA5, ENTRY_CopyBytes1 },                         // MOVS/MOVSD
    { 0xA6, ENTRY_CopyBytes1 },                         // CMPS/CMPSB
    { 0xA7, ENTRY_CopyBytes1 },                         // CMPS/CMPSW
    { 0xA8, ENTRY_CopyBytes2 },                         // TEST
    { 0xA9, ENTRY_CopyBytes3Or5 },                      // TEST
    { 0xAA, ENTRY_CopyBytes1 },                         // STOS/STOSB
    { 0xAB, ENTRY_CopyBytes1 },                         // STOS/STOSW
    { 0xAC, ENTRY_CopyBytes1 },                         // LODS/LODSB
    { 0xAD, ENTRY_CopyBytes1 },                         // LODS/LODSW
    { 0xAE, ENTRY_CopyBytes1 },                         // SCAS/SCASB
    { 0xAF, ENTRY_CopyBytes1 },                         // SCAS/SCASD
    { 0xB0, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB1, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB2, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB3, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB4, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB5, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB6, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB7, ENTRY_CopyBytes2 },                         // MOV B0+rb
    { 0xB8, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xB9, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBA, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBB, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBC, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBD, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBE, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xBF, ENTRY_CopyBytes3Or5Rax },                   // MOV B8+rb
    { 0xC0, ENTRY_CopyBytes2Mod1 },                     // RCL/2 ib, etc.
    { 0xC1, ENTRY_CopyBytes2Mod1 },                     // RCL/2 ib, etc.
    { 0xC2, ENTRY_CopyBytes3 },                         // RET
    { 0xC3, ENTRY_CopyBytes1 },                         // RET
    { 0xC4, ENTRY_CopyVex3 },                           // LES, VEX 3-byte opcodes.
    { 0xC5, ENTRY_CopyVex2 },                           // LDS, VEX 2-byte opcodes.
    { 0xC6, ENTRY_CopyBytes2Mod1 },                     // MOV
    { 0xC7, ENTRY_CopyBytes2ModOperand },               // MOV/0 XBEGIN/7
    { 0xC8, ENTRY_CopyBytes4 },                         // ENTER
    { 0xC9, ENTRY_CopyBytes1 },                         // LEAVE
    { 0xCA, ENTRY_CopyBytes3Dynamic },                  // RET
    { 0xCB, ENTRY_CopyBytes1Dynamic },                  // RET
    { 0xCC, ENTRY_CopyBytes1Dynamic },                  // INT 3
    { 0xCD, ENTRY_CopyBytes2Dynamic },                  // INT ib
#ifdef DETOURS_X64
    { 0xCE, ENTRY_Invalid },                            // Invalid
#else
    { 0xCE, ENTRY_CopyBytes1Dynamic },                  // INTO
#endif
    { 0xCF, ENTRY_CopyBytes1Dynamic },                  // IRET
    { 0xD0, ENTRY_CopyBytes2Mod },                      // RCL/2, etc.
    { 0xD1, ENTRY_CopyBytes2Mod },                      // RCL/2, etc.
    { 0xD2, ENTRY_CopyBytes2Mod },                      // RCL/2, etc.
    { 0xD3, ENTRY_CopyBytes2Mod },                      // RCL/2, etc.
#ifdef DETOURS_X64
    { 0xD4, ENTRY_Invalid },                            // Invalid
    { 0xD5, ENTRY_Invalid },                            // Invalid
#else
    { 0xD4, ENTRY_CopyBytes2 },                         // AAM
    { 0xD5, ENTRY_CopyBytes2 },                         // AAD
#endif
    { 0xD6, ENTRY_Invalid },                            // Invalid
    { 0xD7, ENTRY_CopyBytes1 },                         // XLAT/XLATB
    { 0xD8, ENTRY_CopyBytes2Mod },                      // FADD, etc.
    { 0xD9, ENTRY_CopyBytes2Mod },                      // F2XM1, etc.
    { 0xDA, ENTRY_CopyBytes2Mod },                      // FLADD, etc.
    { 0xDB, ENTRY_CopyBytes2Mod },                      // FCLEX, etc.
    { 0xDC, ENTRY_CopyBytes2Mod },                      // FADD/0, etc.
    { 0xDD, ENTRY_CopyBytes2Mod },                      // FFREE, etc.
    { 0xDE, ENTRY_CopyBytes2Mod },                      // FADDP, etc.
    { 0xDF, ENTRY_CopyBytes2Mod },                      // FBLD/4, etc.
    { 0xE0, ENTRY_CopyBytes2CantJump },                 // LOOPNE cb
    { 0xE1, ENTRY_CopyBytes2CantJump },                 // LOOPE cb
    { 0xE2, ENTRY_CopyBytes2CantJump },                 // LOOP cb
    { 0xE3, ENTRY_CopyBytes2CantJump },                 // JCXZ/JECXZ
    { 0xE4, ENTRY_CopyBytes2 },                         // IN ib
    { 0xE5, ENTRY_CopyBytes2 },                         // IN id
    { 0xE6, ENTRY_CopyBytes2 },                         // OUT ib
    { 0xE7, ENTRY_CopyBytes2 },                         // OUT ib
    { 0xE8, ENTRY_CopyBytes3Or5Target },                // CALL cd
    { 0xE9, ENTRY_CopyBytes3Or5Target },                // JMP cd
#ifdef DETOURS_X64
    { 0xEA, ENTRY_Invalid },                            // Invalid
#else
    { 0xEA, ENTRY_CopyBytes5Or7Dynamic },               // JMP cp
#endif
    { 0xEB, ENTRY_CopyBytes2Jump },                     // JMP cb
    { 0xEC, ENTRY_CopyBytes1 },                         // IN ib
    { 0xED, ENTRY_CopyBytes1 },                         // IN id
    { 0xEE, ENTRY_CopyBytes1 },                         // OUT
    { 0xEF, ENTRY_CopyBytes1 },                         // OUT
    { 0xF0, ENTRY_CopyBytesPrefix },                    // LOCK prefix
    { 0xF1, ENTRY_CopyBytes1Dynamic },                  // INT1 / ICEBP somewhat documented by AMD, not by Intel
    { 0xF2, ENTRY_CopyF2 },                             // REPNE prefix
//#ifdef DETOURS_X86
    { 0xF3, ENTRY_CopyF3 },                             // REPE prefix
//#else
// This does presently suffice for AMD64 but it requires tracing
// through a bunch of code to verify and seems not worth maintaining.
//  { 0xF3, ENTRY_CopyBytesPrefix },                    // REPE prefix
//#endif
    { 0xF4, ENTRY_CopyBytes1 },                         // HLT
    { 0xF5, ENTRY_CopyBytes1 },                         // CMC
    { 0xF6, ENTRY_CopyF6 },                             // TEST/0, DIV/6
    { 0xF7, ENTRY_CopyF7 },                             // TEST/0, DIV/6
    { 0xF8, ENTRY_CopyBytes1 },                         // CLC
    { 0xF9, ENTRY_CopyBytes1 },                         // STC
    { 0xFA, ENTRY_CopyBytes1 },                         // CLI
    { 0xFB, ENTRY_CopyBytes1 },                         // STI
    { 0xFC, ENTRY_CopyBytes1 },                         // CLD
    { 0xFD, ENTRY_CopyBytes1 },                         // STD
    { 0xFE, ENTRY_CopyBytes2Mod },                      // DEC/1,INC/0
    { 0xFF, ENTRY_CopyFF },                             // CALL/2
    { 0, ENTRY_End },
};

const CDetourDis::COPYENTRY CDetourDis::s_rceCopyTable0F[257] =
{
#ifdef DETOURS_X86
    { 0x00, ENTRY_Copy0F00 },                           // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
#else
    { 0x00, ENTRY_CopyBytes2Mod },                      // sldt/0 str/1 lldt/2 ltr/3 err/4 verw/5 jmpe/6/dynamic invalid/7
#endif
    { 0x01, ENTRY_CopyBytes2Mod },                      // INVLPG/7, etc.
    { 0x02, ENTRY_CopyBytes2Mod },                      // LAR/r
    { 0x03, ENTRY_CopyBytes2Mod },                      // LSL/r
    { 0x04, ENTRY_Invalid },                            // _04
    { 0x05, ENTRY_CopyBytes1 },                         // SYSCALL
    { 0x06, ENTRY_CopyBytes1 },                         // CLTS
    { 0x07, ENTRY_CopyBytes1 },                         // SYSRET
    { 0x08, ENTRY_CopyBytes1 },                         // INVD
    { 0x09, ENTRY_CopyBytes1 },                         // WBINVD
    { 0x0A, ENTRY_Invalid },                            // _0A
    { 0x0B, ENTRY_CopyBytes1 },                         // UD2
    { 0x0C, ENTRY_Invalid },                            // _0C
    { 0x0D, ENTRY_CopyBytes2Mod },                      // PREFETCH
    { 0x0E, ENTRY_CopyBytes1 },                         // FEMMS (3DNow -- not in Intel documentation)
    { 0x0F, ENTRY_CopyBytes2Mod1 },                     // 3DNow Opcodes
    { 0x10, ENTRY_CopyBytes2Mod },                      // MOVSS MOVUPD MOVSD
    { 0x11, ENTRY_CopyBytes2Mod },                      // MOVSS MOVUPD MOVSD
    { 0x12, ENTRY_CopyBytes2Mod },                      // MOVLPD
    { 0x13, ENTRY_CopyBytes2Mod },                      // MOVLPD
    { 0x14, ENTRY_CopyBytes2Mod },                      // UNPCKLPD
    { 0x15, ENTRY_CopyBytes2Mod },                      // UNPCKHPD
    { 0x16, ENTRY_CopyBytes2Mod },                      // MOVHPD
    { 0x17, ENTRY_CopyBytes2Mod },                      // MOVHPD
    { 0x18, ENTRY_CopyBytes2Mod },                      // PREFETCHINTA...
    { 0x19, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1A, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1B, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1C, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1D, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1E, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop, not documented by Intel, documented by AMD
    { 0x1F, ENTRY_CopyBytes2Mod },                      // NOP/r multi byte nop
    { 0x20, ENTRY_CopyBytes2Mod },                      // MOV/r
    { 0x21, ENTRY_CopyBytes2Mod },                      // MOV/r
    { 0x22, ENTRY_CopyBytes2Mod },                      // MOV/r
    { 0x23, ENTRY_CopyBytes2Mod },                      // MOV/r
#ifdef DETOURS_X64
    { 0x24, ENTRY_Invalid },                            // _24
#else
    { 0x24, ENTRY_CopyBytes2Mod },                      // MOV/r,TR TR is test register on 80386 and 80486, removed in Pentium
#endif
    { 0x25, ENTRY_Invalid },                            // _25
#ifdef DETOURS_X64
    { 0x26, ENTRY_Invalid },                            // _26
#else
    { 0x26, ENTRY_CopyBytes2Mod },                      // MOV TR/r TR is test register on 80386 and 80486, removed in Pentium
#endif
    { 0x27, ENTRY_Invalid },                            // _27
    { 0x28, ENTRY_CopyBytes2Mod },                      // MOVAPS MOVAPD
    { 0x29, ENTRY_CopyBytes2Mod },                      // MOVAPS MOVAPD
    { 0x2A, ENTRY_CopyBytes2Mod },                      // CVPI2PS &
    { 0x2B, ENTRY_CopyBytes2Mod },                      // MOVNTPS MOVNTPD
    { 0x2C, ENTRY_CopyBytes2Mod },                      // CVTTPS2PI &
    { 0x2D, ENTRY_CopyBytes2Mod },                      // CVTPS2PI &
    { 0x2E, ENTRY_CopyBytes2Mod },                      // UCOMISS UCOMISD
    { 0x2F, ENTRY_CopyBytes2Mod },                      // COMISS COMISD
    { 0x30, ENTRY_CopyBytes1 },                         // WRMSR
    { 0x31, ENTRY_CopyBytes1 },                         // RDTSC
    { 0x32, ENTRY_CopyBytes1 },                         // RDMSR
    { 0x33, ENTRY_CopyBytes1 },                         // RDPMC
    { 0x34, ENTRY_CopyBytes1 },                         // SYSENTER
    { 0x35, ENTRY_CopyBytes1 },                         // SYSEXIT
    { 0x36, ENTRY_Invalid },                            // _36
    { 0x37, ENTRY_CopyBytes1 },                         // GETSEC
    { 0x38, ENTRY_CopyBytes3Mod },                      // SSE3 Opcodes
    { 0x39, ENTRY_Invalid },                            // _39
    { 0x3A, ENTRY_CopyBytes3Mod1 },                      // SSE3 Opcodes
    { 0x3B, ENTRY_Invalid },                            // _3B
    { 0x3C, ENTRY_Invalid },                            // _3C
    { 0x3D, ENTRY_Invalid },                            // _3D
    { 0x3E, ENTRY_Invalid },                            // _3E
    { 0x3F, ENTRY_Invalid },                            // _3F
    { 0x40, ENTRY_CopyBytes2Mod },                      // CMOVO (0F 40)
    { 0x41, ENTRY_CopyBytes2Mod },                      // CMOVNO (0F 41)
    { 0x42, ENTRY_CopyBytes2Mod },                      // CMOVB & CMOVNE (0F 42)
    { 0x43, ENTRY_CopyBytes2Mod },                      // CMOVAE & CMOVNB (0F 43)
    { 0x44, ENTRY_CopyBytes2Mod },                      // CMOVE & CMOVZ (0F 44)
    { 0x45, ENTRY_CopyBytes2Mod },                      // CMOVNE & CMOVNZ (0F 45)
    { 0x46, ENTRY_CopyBytes2Mod },                      // CMOVBE & CMOVNA (0F 46)
    { 0x47, ENTRY_CopyBytes2Mod },                      // CMOVA & CMOVNBE (0F 47)
    { 0x48, ENTRY_CopyBytes2Mod },                      // CMOVS (0F 48)
    { 0x49, ENTRY_CopyBytes2Mod },                      // CMOVNS (0F 49)
    { 0x4A, ENTRY_CopyBytes2Mod },                      // CMOVP & CMOVPE (0F 4A)
    { 0x4B, ENTRY_CopyBytes2Mod },                      // CMOVNP & CMOVPO (0F 4B)
    { 0x4C, ENTRY_CopyBytes2Mod },                      // CMOVL & CMOVNGE (0F 4C)
    { 0x4D, ENTRY_CopyBytes2Mod },                      // CMOVGE & CMOVNL (0F 4D)
    { 0x4E, ENTRY_CopyBytes2Mod },                      // CMOVLE & CMOVNG (0F 4E)
    { 0x4F, ENTRY_CopyBytes2Mod },                      // CMOVG & CMOVNLE (0F 4F)
    { 0x50, ENTRY_CopyBytes2Mod },                      // MOVMSKPD MOVMSKPD
    { 0x51, ENTRY_CopyBytes2Mod },                      // SQRTPS &
    { 0x52, ENTRY_CopyBytes2Mod },                      // RSQRTTS RSQRTPS
    { 0x53, ENTRY_CopyBytes2Mod },                      // RCPPS RCPSS
    { 0x54, ENTRY_CopyBytes2Mod },                      // ANDPS ANDPD
    { 0x55, ENTRY_CopyBytes2Mod },                      // ANDNPS ANDNPD
    { 0x56, ENTRY_CopyBytes2Mod },                      // ORPS ORPD
    { 0x57, ENTRY_CopyBytes2Mod },                      // XORPS XORPD
    { 0x58, ENTRY_CopyBytes2Mod },                      // ADDPS &
    { 0x59, ENTRY_CopyBytes2Mod },                      // MULPS &
    { 0x5A, ENTRY_CopyBytes2Mod },                      // CVTPS2PD &
    { 0x5B, ENTRY_CopyBytes2Mod },                      // CVTDQ2PS &
    { 0x5C, ENTRY_CopyBytes2Mod },                      // SUBPS &
    { 0x5D, ENTRY_CopyBytes2Mod },                      // MINPS &
    { 0x5E, ENTRY_CopyBytes2Mod },                      // DIVPS &
    { 0x5F, ENTRY_CopyBytes2Mod },                      // MASPS &
    { 0x60, ENTRY_CopyBytes2Mod },                      // PUNPCKLBW/r
    { 0x61, ENTRY_CopyBytes2Mod },                      // PUNPCKLWD/r
    { 0x62, ENTRY_CopyBytes2Mod },                      // PUNPCKLWD/r
    { 0x63, ENTRY_CopyBytes2Mod },                      // PACKSSWB/r
    { 0x64, ENTRY_CopyBytes2Mod },                      // PCMPGTB/r
    { 0x65, ENTRY_CopyBytes2Mod },                      // PCMPGTW/r
    { 0x66, ENTRY_CopyBytes2Mod },                      // PCMPGTD/r
    { 0x67, ENTRY_CopyBytes2Mod },                      // PACKUSWB/r
    { 0x68, ENTRY_CopyBytes2Mod },                      // PUNPCKHBW/r
    { 0x69, ENTRY_CopyBytes2Mod },                      // PUNPCKHWD/r
    { 0x6A, ENTRY_CopyBytes2Mod },                      // PUNPCKHDQ/r
    { 0x6B, ENTRY_CopyBytes2Mod },                      // PACKSSDW/r
    { 0x6C, ENTRY_CopyBytes2Mod },                      // PUNPCKLQDQ
    { 0x6D, ENTRY_CopyBytes2Mod },                      // PUNPCKHQDQ
    { 0x6E, ENTRY_CopyBytes2Mod },                      // MOVD/r
    { 0x6F, ENTRY_CopyBytes2Mod },                      // MOV/r
    { 0x70, ENTRY_CopyBytes2Mod1 },                     // PSHUFW/r ib
    { 0x71, ENTRY_CopyBytes2Mod1 },                     // PSLLW/6 ib,PSRAW/4 ib,PSRLW/2 ib
    { 0x72, ENTRY_CopyBytes2Mod1 },                     // PSLLD/6 ib,PSRAD/4 ib,PSRLD/2 ib
    { 0x73, ENTRY_CopyBytes2Mod1 },                     // PSLLQ/6 ib,PSRLQ/2 ib
    { 0x74, ENTRY_CopyBytes2Mod },                      // PCMPEQB/r
    { 0x75, ENTRY_CopyBytes2Mod },                      // PCMPEQW/r
    { 0x76, ENTRY_CopyBytes2Mod },                      // PCMPEQD/r
    { 0x77, ENTRY_CopyBytes1 },                         // EMMS
    // extrq/insertq require mode=3 and are followed by two immediate bytes
    { 0x78, ENTRY_Copy0F78 },                           // VMREAD/r, 66/EXTRQ/r/ib/ib, F2/INSERTQ/r/ib/ib
    // extrq/insertq require mod=3, therefore ENTRY_CopyBytes2, but it ends up the same
    { 0x79, ENTRY_CopyBytes2Mod },                      // VMWRITE/r, 66/EXTRQ/r, F2/INSERTQ/r
    { 0x7A, ENTRY_Invalid },                            // _7A
    { 0x7B, ENTRY_Invalid },                            // _7B
    { 0x7C, ENTRY_CopyBytes2Mod },                      // HADDPS
    { 0x7D, ENTRY_CopyBytes2Mod },                      // HSUBPS
    { 0x7E, ENTRY_CopyBytes2Mod },                      // MOVD/r
    { 0x7F, ENTRY_CopyBytes2Mod },                      // MOV/r
    { 0x80, ENTRY_CopyBytes3Or5Target },                // JO
    { 0x81, ENTRY_CopyBytes3Or5Target },                // JNO
    { 0x82, ENTRY_CopyBytes3Or5Target },                // JB,JC,JNAE
    { 0x83, ENTRY_CopyBytes3Or5Target },                // JAE,JNB,JNC
    { 0x84, ENTRY_CopyBytes3Or5Target },                // JE,JZ,JZ
    { 0x85, ENTRY_CopyBytes3Or5Target },                // JNE,JNZ
    { 0x86, ENTRY_CopyBytes3Or5Target },                // JBE,JNA
    { 0x87, ENTRY_CopyBytes3Or5Target },                // JA,JNBE
    { 0x88, ENTRY_CopyBytes3Or5Target },                // JS
    { 0x89, ENTRY_CopyBytes3Or5Target },                // JNS
    { 0x8A, ENTRY_CopyBytes3Or5Target },                // JP,JPE
    { 0x8B, ENTRY_CopyBytes3Or5Target },                // JNP,JPO
    { 0x8C, ENTRY_CopyBytes3Or5Target },                // JL,NGE
    { 0x8D, ENTRY_CopyBytes3Or5Target },                // JGE,JNL
    { 0x8E, ENTRY_CopyBytes3Or5Target },                // JLE,JNG
    { 0x8F, ENTRY_CopyBytes3Or5Target },                // JG,JNLE
    { 0x90, ENTRY_CopyBytes2Mod },                      // CMOVO (0F 40)
    { 0x91, ENTRY_CopyBytes2Mod },                      // CMOVNO (0F 41)
    { 0x92, ENTRY_CopyBytes2Mod },                      // CMOVB & CMOVC & CMOVNAE (0F 42)
    { 0x93, ENTRY_CopyBytes2Mod },                      // CMOVAE & CMOVNB & CMOVNC (0F 43)
    { 0x94, ENTRY_CopyBytes2Mod },                      // CMOVE & CMOVZ (0F 44)
    { 0x95, ENTRY_CopyBytes2Mod },                      // CMOVNE & CMOVNZ (0F 45)
    { 0x96, ENTRY_CopyBytes2Mod },                      // CMOVBE & CMOVNA (0F 46)
    { 0x97, ENTRY_CopyBytes2Mod },                      // CMOVA & CMOVNBE (0F 47)
    { 0x98, ENTRY_CopyBytes2Mod },                      // CMOVS (0F 48)
    { 0x99, ENTRY_CopyBytes2Mod },                      // CMOVNS (0F 49)
    { 0x9A, ENTRY_CopyBytes2Mod },                      // CMOVP & CMOVPE (0F 4A)
    { 0x9B, ENTRY_CopyBytes2Mod },                      // CMOVNP & CMOVPO (0F 4B)
    { 0x9C, ENTRY_CopyBytes2Mod },                      // CMOVL & CMOVNGE (0F 4C)
    { 0x9D, ENTRY_CopyBytes2Mod },                      // CMOVGE & CMOVNL (0F 4D)
    { 0x9E, ENTRY_CopyBytes2Mod },                      // CMOVLE & CMOVNG (0F 4E)
    { 0x9F, ENTRY_CopyBytes2Mod },                      // CMOVG & CMOVNLE (0F 4F)
    { 0xA0, ENTRY_CopyBytes1 },                         // PUSH
    { 0xA1, ENTRY_CopyBytes1 },                         // POP
    { 0xA2, ENTRY_CopyBytes1 },                         // CPUID
    { 0xA3, ENTRY_CopyBytes2Mod },                      // BT  (0F A3)
    { 0xA4, ENTRY_CopyBytes2Mod1 },                     // SHLD
    { 0xA5, ENTRY_CopyBytes2Mod },                      // SHLD
    { 0xA6, ENTRY_CopyBytes2Mod },                      // XBTS
    { 0xA7, ENTRY_CopyBytes2Mod },                      // IBTS
    { 0xA8, ENTRY_CopyBytes1 },                         // PUSH
    { 0xA9, ENTRY_CopyBytes1 },                         // POP
    { 0xAA, ENTRY_CopyBytes1 },                         // RSM
    { 0xAB, ENTRY_CopyBytes2Mod },                      // BTS (0F AB)
    { 0xAC, ENTRY_CopyBytes2Mod1 },                     // SHRD
    { 0xAD, ENTRY_CopyBytes2Mod },                      // SHRD

    // 0F AE mod76=mem mod543=0 fxsave
    // 0F AE mod76=mem mod543=1 fxrstor
    // 0F AE mod76=mem mod543=2 ldmxcsr
    // 0F AE mod76=mem mod543=3 stmxcsr
    // 0F AE mod76=mem mod543=4 xsave
    // 0F AE mod76=mem mod543=5 xrstor
    // 0F AE mod76=mem mod543=6 saveopt
    // 0F AE mod76=mem mod543=7 clflush
    // 0F AE mod76=11b mod543=5 lfence
    // 0F AE mod76=11b mod543=6 mfence
    // 0F AE mod76=11b mod543=7 sfence
    // F3 0F AE mod76=11b mod543=0 rdfsbase
    // F3 0F AE mod76=11b mod543=1 rdgsbase
    // F3 0F AE mod76=11b mod543=2 wrfsbase
    // F3 0F AE mod76=11b mod543=3 wrgsbase
    { 0xAE, ENTRY_CopyBytes2Mod },                      // fxsave fxrstor ldmxcsr stmxcsr xsave xrstor saveopt clflush lfence mfence sfence rdfsbase rdgsbase wrfsbase wrgsbase
    { 0xAF, ENTRY_CopyBytes2Mod },                      // IMUL (0F AF)
    { 0xB0, ENTRY_CopyBytes2Mod },                      // CMPXCHG (0F B0)
    { 0xB1, ENTRY_CopyBytes2Mod },                      // CMPXCHG (0F B1)
    { 0xB2, ENTRY_CopyBytes2Mod },                      // LSS/r
    { 0xB3, ENTRY_CopyBytes2Mod },                      // BTR (0F B3)
    { 0xB4, ENTRY_CopyBytes2Mod },                      // LFS/r
    { 0xB5, ENTRY_CopyBytes2Mod },                      // LGS/r
    { 0xB6, ENTRY_CopyBytes2Mod },                      // MOVZX/r
    { 0xB7, ENTRY_CopyBytes2Mod },                      // MOVZX/r
#ifdef DETOURS_X86
    { 0xB8, ENTRY_Copy0FB8 },                           // jmpe f3/popcnt
#else
    { 0xB8, ENTRY_CopyBytes2Mod },                      // f3/popcnt
#endif
    { 0xB9, ENTRY_Invalid },                            // _B9
    { 0xBA, ENTRY_CopyBytes2Mod1 },                     // BT & BTC & BTR & BTS (0F BA)
    { 0xBB, ENTRY_CopyBytes2Mod },                      // BTC (0F BB)
    { 0xBC, ENTRY_CopyBytes2Mod },                      // BSF (0F BC)
    { 0xBD, ENTRY_CopyBytes2Mod },                      // BSR (0F BD)
    { 0xBE, ENTRY_CopyBytes2Mod },                      // MOVSX/r
    { 0xBF, ENTRY_CopyBytes2Mod },                      // MOVSX/r
    { 0xC0, ENTRY_CopyBytes2Mod },                      // XADD/r
    { 0xC1, ENTRY_CopyBytes2Mod },                      // XADD/r
    { 0xC2, ENTRY_CopyBytes2Mod1 },                     // CMPPS &
    { 0xC3, ENTRY_CopyBytes2Mod },                      // MOVNTI
    { 0xC4, ENTRY_CopyBytes2Mod1 },                     // PINSRW /r ib
    { 0xC5, ENTRY_CopyBytes2Mod1 },                     // PEXTRW /r ib
    { 0xC6, ENTRY_CopyBytes2Mod1 },                     // SHUFPS & SHUFPD
    { 0xC7, ENTRY_CopyBytes2Mod },                      // CMPXCHG8B (0F C7)
    { 0xC8, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xC9, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xCA, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xCB, ENTRY_CopyBytes1 },                         // CVTPD2PI BSWAP 0F C8 + rd
    { 0xCC, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xCD, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xCE, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xCF, ENTRY_CopyBytes1 },                         // BSWAP 0F C8 + rd
    { 0xD0, ENTRY_CopyBytes2Mod },                      // ADDSUBPS (untestd)
    { 0xD1, ENTRY_CopyBytes2Mod },                      // PSRLW/r
    { 0xD2, ENTRY_CopyBytes2Mod },                      // PSRLD/r
    { 0xD3, ENTRY_CopyBytes2Mod },                      // PSRLQ/r
    { 0xD4, ENTRY_CopyBytes2Mod },                      // PADDQ
    { 0xD5, ENTRY_CopyBytes2Mod },                      // PMULLW/r
    { 0xD6, ENTRY_CopyBytes2Mod },                      // MOVDQ2Q / MOVQ2DQ
    { 0xD7, ENTRY_CopyBytes2Mod },                      // PMOVMSKB/r
    { 0xD8, ENTRY_CopyBytes2Mod },                      // PSUBUSB/r
    { 0xD9, ENTRY_CopyBytes2Mod },                      // PSUBUSW/r
    { 0xDA, ENTRY_CopyBytes2Mod },                      // PMINUB/r
    { 0xDB, ENTRY_CopyBytes2Mod },                      // PAND/r
    { 0xDC, ENTRY_CopyBytes2Mod },                      // PADDUSB/r
    { 0xDD, ENTRY_CopyBytes2Mod },                      // PADDUSW/r
    { 0xDE, ENTRY_CopyBytes2Mod },                      // PMAXUB/r
    { 0xDF, ENTRY_CopyBytes2Mod },                      // PANDN/r
    { 0xE0, ENTRY_CopyBytes2Mod  },                     // PAVGB
    { 0xE1, ENTRY_CopyBytes2Mod },                      // PSRAW/r
    { 0xE2, ENTRY_CopyBytes2Mod },                      // PSRAD/r
    { 0xE3, ENTRY_CopyBytes2Mod },                      // PAVGW
    { 0xE4, ENTRY_CopyBytes2Mod },                      // PMULHUW/r
    { 0xE5, ENTRY_CopyBytes2Mod },                      // PMULHW/r
    { 0xE6, ENTRY_CopyBytes2Mod },                      // CTDQ2PD &
    { 0xE7, ENTRY_CopyBytes2Mod },                      // MOVNTQ
    { 0xE8, ENTRY_CopyBytes2Mod },                      // PSUBB/r
    { 0xE9, ENTRY_CopyBytes2Mod },                      // PSUBW/r
    { 0xEA, ENTRY_CopyBytes2Mod },                      // PMINSW/r
    { 0xEB, ENTRY_CopyBytes2Mod },                      // POR/r
    { 0xEC, ENTRY_CopyBytes2Mod },                      // PADDSB/r
    { 0xED, ENTRY_CopyBytes2Mod },                      // PADDSW/r
    { 0xEE, ENTRY_CopyBytes2Mod },                      // PMAXSW /r
    { 0xEF, ENTRY_CopyBytes2Mod },                      // PXOR/r
    { 0xF0, ENTRY_CopyBytes2Mod },                      // LDDQU
    { 0xF1, ENTRY_CopyBytes2Mod },                      // PSLLW/r
    { 0xF2, ENTRY_CopyBytes2Mod },                      // PSLLD/r
    { 0xF3, ENTRY_CopyBytes2Mod },                      // PSLLQ/r
    { 0xF4, ENTRY_CopyBytes2Mod },                      // PMULUDQ/r
    { 0xF5, ENTRY_CopyBytes2Mod },                      // PMADDWD/r
    { 0xF6, ENTRY_CopyBytes2Mod },                      // PSADBW/r
    { 0xF7, ENTRY_CopyBytes2Mod },                      // MASKMOVQ
    { 0xF8, ENTRY_CopyBytes2Mod },                      // PSUBB/r
    { 0xF9, ENTRY_CopyBytes2Mod },                      // PSUBW/r
    { 0xFA, ENTRY_CopyBytes2Mod },                      // PSUBD/r
    { 0xFB, ENTRY_CopyBytes2Mod },                      // FSUBQ/r
    { 0xFC, ENTRY_CopyBytes2Mod },                      // PADDB/r
    { 0xFD, ENTRY_CopyBytes2Mod },                      // PADDW/r
    { 0xFE, ENTRY_CopyBytes2Mod },                      // PADDD/r
    { 0xFF, ENTRY_Invalid },                            // _FF
    { 0, ENTRY_End },
};

BOOL CDetourDis::SanityCheckSystem()
{
    ULONG n = 0;
    for (; n < 256; n++) {
        REFCOPYENTRY pEntry = &s_rceCopyTable[n];

        if (n != pEntry->nOpcode) {
            ASSERT(n == pEntry->nOpcode);
            return FALSE;
        }
    }
    if (s_rceCopyTable[256].pfCopy != NULL) {
        ASSERT(!"Missing end marker.");
        return FALSE;
    }

    for (n = 0; n < 256; n++) {
        REFCOPYENTRY pEntry = &s_rceCopyTable0F[n];

        if (n != pEntry->nOpcode) {
            ASSERT(n == pEntry->nOpcode);
            return FALSE;
        }
    }
    if (s_rceCopyTable0F[256].pfCopy != NULL) {
        ASSERT(!"Missing end marker.");
        return FALSE;
    }

    return TRUE;
}
#endif // defined(DETOURS_X64) || defined(DETOURS_X86)

/////////////////////////////////////////////////////////// IA64 Disassembler.
//
#ifdef DETOURS_IA64

#if defined(_IA64_) != defined(DETOURS_IA64_OFFLINE_LIBRARY)
// Compile DETOUR_IA64_BUNDLE for native IA64 or cross, but not both -- we get duplicates otherwise.
const DETOUR_IA64_BUNDLE::DETOUR_IA64_METADATA DETOUR_IA64_BUNDLE::s_rceCopyTable[33] =
{
    { 0x00, M_UNIT,      I_UNIT,      I_UNIT,   },
    { 0x01, M_UNIT,      I_UNIT,      I_UNIT,   },
    { 0x02, M_UNIT,      I_UNIT,      I_UNIT,   },
    { 0x03, M_UNIT,      I_UNIT,      I_UNIT,   },
    { 0x04, M_UNIT,      L_UNIT,      X_UNIT,   },
    { 0x05, M_UNIT,      L_UNIT,      X_UNIT,   },
    { 0x06, 0,           0,           0,        },
    { 0x07, 0,           0,           0,        },
    { 0x08, M_UNIT,      M_UNIT,      I_UNIT,   },
    { 0x09, M_UNIT,      M_UNIT,      I_UNIT,   },
    { 0x0a, M_UNIT,      M_UNIT,      I_UNIT,   },
    { 0x0b, M_UNIT,      M_UNIT,      I_UNIT,   },
    { 0x0c, M_UNIT,      F_UNIT,      I_UNIT,   },
    { 0x0d, M_UNIT,      F_UNIT,      I_UNIT,   },
    { 0x0e, M_UNIT,      M_UNIT,      F_UNIT,   },
    { 0x0f, M_UNIT,      M_UNIT,      F_UNIT,   },
    { 0x10, M_UNIT,      I_UNIT,      B_UNIT,   },
    { 0x11, M_UNIT,      I_UNIT,      B_UNIT,   },
    { 0x12, M_UNIT,      B_UNIT,      B_UNIT,   },
    { 0x13, M_UNIT,      B_UNIT,      B_UNIT,   },
    { 0x14, 0,           0,           0,        },
    { 0x15, 0,           0,           0,        },
    { 0x16, B_UNIT,      B_UNIT,      B_UNIT,   },
    { 0x17, B_UNIT,      B_UNIT,      B_UNIT,   },
    { 0x18, M_UNIT,      M_UNIT,      B_UNIT,   },
    { 0x19, M_UNIT,      M_UNIT,      B_UNIT,   },
    { 0x1a, 0,           0,           0,        },
    { 0x1b, 0,           0,           0,        },
    { 0x1c, M_UNIT,      F_UNIT,      B_UNIT,   },
    { 0x1d, M_UNIT,      F_UNIT,      B_UNIT,   },
    { 0x1e, 0,           0,           0,        },
    { 0x1f, 0,           0,           0,        },
    { 0x00, 0,           0,           0,        },
};

// 120 112 104 96 88 80 72 64 56 48 40 32 24 16  8  0
//  f.  e.  d. c. b. a. 9. 8. 7. 6. 5. 4. 3. 2. 1. 0.

//                                      00
// f.e. d.c. b.a. 9.8. 7.6. 5.4. 3.2. 1.0.
// 0000 0000 0000 0000 0000 0000 0000 001f : Template [4..0]
// 0000 0000 0000 0000 0000 03ff ffff ffe0 : Zero [ 41..  5]
// 0000 0000 0000 0000 0000 3c00 0000 0000 : Zero [ 45.. 42]
// 0000 0000 0007 ffff ffff c000 0000 0000 : One  [ 82.. 46]
// 0000 0000 0078 0000 0000 0000 0000 0000 : One  [ 86.. 83]
// 0fff ffff ff80 0000 0000 0000 0000 0000 : Two  [123.. 87]
// f000 0000 0000 0000 0000 0000 0000 0000 : Two  [127..124]
BYTE DETOUR_IA64_BUNDLE::GetTemplate() const
{
    return (data[0] & 0x1f);
}

BYTE DETOUR_IA64_BUNDLE::GetInst0() const
{
    return ((data[5] & 0x3c) >> 2);
}

BYTE DETOUR_IA64_BUNDLE::GetInst1() const
{
    return ((data[10] & 0x78) >> 3);
}

BYTE DETOUR_IA64_BUNDLE::GetInst2() const
{
    return ((data[15] & 0xf0) >> 4);
}

BYTE DETOUR_IA64_BUNDLE::GetUnit(BYTE slot) const
{
    switch (slot) {
    case 0: return GetUnit0();
    case 1: return GetUnit1();
    case 2: return GetUnit2();
    }
    __debugbreak();
    return 0;
}

BYTE DETOUR_IA64_BUNDLE::GetUnit0() const
{
    return s_rceCopyTable[data[0] & 0x1f].nUnit0;
}

BYTE DETOUR_IA64_BUNDLE::GetUnit1() const
{
    return s_rceCopyTable[data[0] & 0x1f].nUnit1;
}

BYTE DETOUR_IA64_BUNDLE::GetUnit2() const
{
    return s_rceCopyTable[data[0] & 0x1f].nUnit2;
}

UINT64 DETOUR_IA64_BUNDLE::GetData0() const
{
    return (((wide[0] & 0x000003ffffffffe0) >> 5));
}

UINT64 DETOUR_IA64_BUNDLE::GetData1() const
{
    return (((wide[0] & 0xffffc00000000000) >> 46) |
            ((wide[1] & 0x000000000007ffff) << 18));
}

UINT64 DETOUR_IA64_BUNDLE::GetData2() const
{
    return (((wide[1] & 0x0fffffffff800000) >> 23));
}

VOID DETOUR_IA64_BUNDLE::SetInst(BYTE slot, BYTE nInst)
{
    switch (slot)
    {
    case 0: SetInst0(nInst); return;
    case 1: SetInst1(nInst); return;
    case 2: SetInst2(nInst); return;
    }
    __debugbreak();
}

VOID DETOUR_IA64_BUNDLE::SetInst0(BYTE nInst)
{
    data[5] = (data[5] & ~0x3c) | ((nInst << 2) & 0x3c);
}

VOID DETOUR_IA64_BUNDLE::SetInst1(BYTE nInst)
{
    data[10] = (data[10] & ~0x78) | ((nInst << 3) & 0x78);
}

VOID DETOUR_IA64_BUNDLE::SetInst2(BYTE nInst)
{
    data[15] = (data[15] & ~0xf0) | ((nInst << 4) & 0xf0);
}

VOID DETOUR_IA64_BUNDLE::SetData(BYTE slot, UINT64 nData)
{
    switch (slot)
    {
    case 0: SetData0(nData); return;
    case 1: SetData1(nData); return;
    case 2: SetData2(nData); return;
    }
    __debugbreak();
}

VOID DETOUR_IA64_BUNDLE::SetData0(UINT64 nData)
{
    wide[0] = (wide[0] & ~0x000003ffffffffe0) | (( nData << 5)  & 0x000003ffffffffe0);
}

VOID DETOUR_IA64_BUNDLE::SetData1(UINT64 nData)
{
    wide[0] = (wide[0] & ~0xffffc00000000000) | ((nData << 46) & 0xffffc00000000000);
    wide[1] = (wide[1] & ~0x000000000007ffff) | ((nData >> 18) & 0x000000000007ffff);
}

VOID DETOUR_IA64_BUNDLE::SetData2(UINT64 nData)
{
    wide[1] = (wide[1] & ~0x0fffffffff800000) | ((nData << 23) & 0x0fffffffff800000);
}

UINT64 DETOUR_IA64_BUNDLE::GetInstruction(BYTE slot) const
{
    switch (slot) {
    case 0: return GetInstruction0();
    case 1: return GetInstruction1();
    case 2: return GetInstruction2();
    }
    __debugbreak();
    return 0;
}

UINT64 DETOUR_IA64_BUNDLE::GetInstruction0() const
{
    // 41 bits from wide[0], skipping the 5 bit template.
    return GetBits(wide[0], DETOUR_IA64_INSTRUCTION0_OFFSET, DETOUR_IA64_INSTRUCTION_SIZE);
}

UINT64 DETOUR_IA64_BUNDLE::GetInstruction1() const
{
    // 64-46 bits from wide[0] and the rest from wide[1].
    const UINT count0 = 64 - DETOUR_IA64_INSTRUCTION1_OFFSET;
    const UINT count1 = DETOUR_IA64_INSTRUCTION_SIZE - count0;
    return GetBits(wide[0], DETOUR_IA64_INSTRUCTION1_OFFSET, count0) | (GetBits(wide[1], 0, count1) << count0);
}

UINT64 DETOUR_IA64_BUNDLE::GetInstruction2() const
{
    // Upper 41 bits of wide[1].
    return wide[1] >> (64 - DETOUR_IA64_INSTRUCTION_SIZE);
}

void DETOUR_IA64_BUNDLE::SetInstruction(BYTE slot, UINT64 instruction)
{
    switch (slot) {
    case 0: SetInstruction0(instruction); return;
    case 1: SetInstruction1(instruction); return;
    case 2: SetInstruction2(instruction); return;
    }
    __debugbreak();
}

void DETOUR_IA64_BUNDLE::SetInstruction0(UINT64 instruction)
{
    wide[0] = SetBits(wide[0], DETOUR_IA64_INSTRUCTION0_OFFSET, DETOUR_IA64_INSTRUCTION_SIZE, instruction);
}

void DETOUR_IA64_BUNDLE::SetInstruction1(UINT64 instruction)
{
    UINT const count0 = 64 - DETOUR_IA64_INSTRUCTION1_OFFSET;
    UINT const count1 = DETOUR_IA64_INSTRUCTION_SIZE - count0;
    UINT64 const wide0 = SetBits(wide[0], DETOUR_IA64_INSTRUCTION1_OFFSET, count0, instruction);
    UINT64 const wide1 = SetBits(wide[1], 0, count1, instruction >> count0);
    wide[0] = wide0;
    wide[1] = wide1;
}

void DETOUR_IA64_BUNDLE::SetInstruction2(UINT64 instruction)
{
    // Set upper 41 bits of wide[1].
    wide[1] = SetBits(wide[1], 64 - DETOUR_IA64_INSTRUCTION_SIZE, DETOUR_IA64_INSTRUCTION_SIZE, instruction);
}

UINT64 DETOUR_IA64_BUNDLE::SignExtend(UINT64 Value, UINT64 Offset)
// This definition is from the IA64 manual.
{
    if ((Value & (((UINT64)1) << (Offset - 1))) == 0)
        return Value;
    UINT64 const new_value = Value | ((~(UINT64)0) << Offset);
    return new_value;
}

UINT64 DETOUR_IA64_BUNDLE::GetBits(UINT64 Value, UINT64 Offset, UINT64 Count)
{
    UINT64 const new_value = (Value >> Offset) & ~(~((UINT64)0) << Count);
    return new_value;
}

UINT64 DETOUR_IA64_BUNDLE::SetBits(UINT64 Value, UINT64 Offset, UINT64 Count, UINT64 Field)
{
    UINT64 const mask = (~((~(UINT64)0) << Count)) << Offset;
    UINT64 const new_value = (Value & ~mask) | ((Field << Offset) & mask);
    return new_value;
}

UINT64 DETOUR_IA64_BUNDLE::GetOpcode(UINT64 instruction)
// Get 4bit primary opcode.
{
    UINT64 const opcode = GetBits(instruction, DETOUR_IA64_INSTRUCTION_SIZE - 4, 4);
    return opcode;
}

UINT64 DETOUR_IA64_BUNDLE::GetX(UINT64 instruction)
// Get 1bit opcode extension.
{
    UINT64 const x = GetBits(instruction, 33, 1);
    return x;
}

UINT64 DETOUR_IA64_BUNDLE::GetX3(UINT64 instruction)
// Get 3bit opcode extension.
{
    UINT64 const x3 = GetBits(instruction, 33, 3);
    return x3;
}

UINT64 DETOUR_IA64_BUNDLE::GetX6(UINT64 instruction)
// Get 6bit opcode extension.
{
    UINT64 const x6 = GetBits(instruction, 27, 6);
    return x6;
}

UINT64 DETOUR_IA64_BUNDLE::GetImm7a(UINT64 instruction)
{
    UINT64 const imm7a = GetBits(instruction, 6, 7);
    return imm7a;
}

UINT64 DETOUR_IA64_BUNDLE::SetImm7a(UINT64 instruction, UINT64 imm7a)
{
    UINT64 const new_instruction = SetBits(instruction, 6, 7, imm7a);
    return new_instruction;
}

UINT64 DETOUR_IA64_BUNDLE::GetImm13c(UINT64 instruction)
{
    UINT64 const imm13c = GetBits(instruction, 20, 13);
    return imm13c;
}

UINT64 DETOUR_IA64_BUNDLE::SetImm13c(UINT64 instruction, UINT64 imm13c)
{
    UINT64 const new_instruction = SetBits(instruction, 20, 13, imm13c);
    return new_instruction;
}

UINT64 DETOUR_IA64_BUNDLE::GetSignBit(UINT64 instruction)
{
    UINT64 const signBit = GetBits(instruction, 36, 1);
    return signBit;
}

UINT64 DETOUR_IA64_BUNDLE::SetSignBit(UINT64 instruction, UINT64 signBit)
{
    UINT64 const new_instruction = SetBits(instruction, 36, 1, signBit);
    return new_instruction;
}

UINT64 DETOUR_IA64_BUNDLE::GetImm20a(UINT64 instruction)
{
    UINT64 const imm20a = GetBits(instruction, 6, 20);
    return imm20a;
}

UINT64 DETOUR_IA64_BUNDLE::SetImm20a(UINT64 instruction, UINT64 imm20a)
{
    UINT64 const new_instruction = SetBits(instruction, 6, 20, imm20a);
    return new_instruction;
}

UINT64 DETOUR_IA64_BUNDLE::GetImm20b(UINT64 instruction)
{
    UINT64 const imm20b = GetBits(instruction, 13, 20);
    return imm20b;
}

UINT64 DETOUR_IA64_BUNDLE::SetImm20b(UINT64 instruction, UINT64 imm20b)
{
    UINT64 const new_instruction = SetBits(instruction, 13, 20, imm20b);
    return new_instruction;
}

bool DETOUR_IA64_BUNDLE::RelocateInstruction(_Inout_ DETOUR_IA64_BUNDLE* pDst,
                                             _In_ BYTE slot,
                                             _Inout_opt_ DETOUR_IA64_BUNDLE* pBundleExtra) const
/*
    If pBundleExtra is provided and instruction is IP-relative,
    this function relocates instruction to target pBundleExtra,
    pBundleExtra is set to brl the original target, and return true.

    [Not used] If pBundleExtra is not provided and instruction is IP-relative, return true.

    Else return false.

    The following IP-relative forms are recognized:
        br and br.call
        chk.s.m integer and float
        chk.a.nc integer and float
        chk.a.clr integer and float
        chk.s.i
        fchkf

    Brl is handled elsewhere, because the code was previously written.

    Branch prediction hints are not relocated.
*/
{
    UINT64 const instruction = GetInstruction(slot);
    UINT64 const opcode = GetOpcode(instruction);
    size_t const dest = (size_t)pDst;
    size_t const extra = (size_t)pBundleExtra;

    switch (GetUnit(slot)) {
    case F_UNIT:
        // F14 fchkf
        if (opcode == 0 && GetX(instruction) == 0 && GetX6(instruction) == 8) {
            goto imm20a;
        }
        return false;

    case M_UNIT:
        // M20 x3 == 1 integer chk.s.m
        // M21 x3 == 3 floating point chk.s
        if (opcode == 1) {
            UINT64 const x3 = GetX3(instruction);
            if (x3 == 1 || x3 == 3) {
                goto imm13_7;
            }
        }

        // M22 x3 == 4 integer chk.a.nc
        // M22 x3 == 5 integer chk.a.clr
        // M23 x3 == 6 floating point chk.a.nc
        // M23 x3 == 7 floating point chk.a.clr
        if (opcode == 0) {
            UINT64 const x3 = GetX3(instruction);
            if (x3 == 4 || x3 == 5 || x3 == 6 || x3 == 7) {
                goto imm20b;
            }
        }
        return false;
    case I_UNIT:
        // I20
        if (opcode == 0 && GetX3(instruction) == 1) { // chk.s.i
            goto imm13_7;
        }
        return false;
    case B_UNIT:
        // B1 B2 B3
        // 4 br
        // 5 br.call
        if (opcode == 4 || opcode == 5) {
            goto imm20b;
        }
        return false;
    }
    return false;

    UINT64 imm;
    UINT64 new_instruction;

imm13_7:
    imm = SignExtend((GetSignBit(instruction) << 20) | (GetImm13c(instruction) << 7) | GetImm7a(instruction), 21) << 4;
    new_instruction = SetSignBit(SetImm13c(SetImm7a(instruction, (extra - dest) >> 4), (extra - dest) >> 11), extra < dest);
    goto set_brl;

imm20a:
    imm = SignExtend((GetSignBit(instruction) << 20) | GetImm20a(instruction), 21) << 4;
    new_instruction = SetSignBit(SetImm20a(instruction, (extra - dest) >> 4), extra < dest);
    goto set_brl;

imm20b:
    imm = SignExtend((GetSignBit(instruction) << 20) | GetImm20b(instruction), 21) << 4;
    new_instruction = SetSignBit(SetImm20b(instruction, (extra - dest) >> 4), extra < dest);
    goto set_brl;

set_brl:
    if (pBundleExtra != NULL) {
        pDst->SetInstruction(slot, new_instruction);
        pBundleExtra->SetBrl((size_t)this + imm);
    }
    return true;
}

UINT DETOUR_IA64_BUNDLE::RelocateBundle(_Inout_ DETOUR_IA64_BUNDLE* pDst,
                                        _Inout_opt_ DETOUR_IA64_BUNDLE* pBundleExtra) const
/*
    Having already copied the bundle unchanged, then relocate its instructions one at a time.
    Return how many extra bytes are required to relocate the bundle.
*/
{
    UINT nExtraBytes = 0;
    for (BYTE slot = 0; slot < DETOUR_IA64_INSTRUCTIONS_PER_BUNDLE; ++slot) {
        if (!RelocateInstruction(pDst, slot, pBundleExtra)) {
            continue;
        }
        pBundleExtra -= !!pBundleExtra;
        nExtraBytes += sizeof(DETOUR_IA64_BUNDLE);
    }
    return nExtraBytes;
}

BOOL DETOUR_IA64_BUNDLE::IsBrl() const
{
    // f.e. d.c. b.a. 9.8. 7.6. 5. 4. 3. 2. 1. 0.
    // c000 0070 0000 0000 0000 00 01 00 00 00 05 : brl.sptk.few
    // c8ff fff0 007f fff0 ffff 00 01 00 00 00 05 : brl.sptk.few
    // c000 0048 0000 0000 0001 00 00 00 00 00 05 : brl.sptk.many
    return ((wide[0] & 0x000000000000001e) == 0x0000000000000004 && // 4 or 5.
            (wide[1] & 0xe000000000000000) == 0xc000000000000000);  // c or d.
}

VOID DETOUR_IA64_BUNDLE::SetBrl()
{
    wide[0] = 0x0000000100000005;   // few
    //wide[0] = 0x0000000180000005; // many
    wide[1] = 0xc000000800000000;
}

UINT64 DETOUR_IA64_BUNDLE::GetBrlImm() const
{
    return (
            //          0x0000000000fffff0
            ((wide[1] & 0x00fffff000000000) >> 32) |    // all 20 bits of imm20b.
            //          0x000000ffff000000
            ((wide[0] & 0xffff000000000000) >> 24) |    // bottom 16 bits of imm39.
            //          0x7fffff0000000000
            ((wide[1] & 0x00000000007fffff) << 40) |    // top 23 bits of imm39.
            //          0x8000000000000000
            ((wide[1] & 0x0800000000000000) <<  4)      // single bit of i.
           );
}

VOID DETOUR_IA64_BUNDLE::SetBrlImm(UINT64 imm)
{
    wide[0] = ((wide[0] & ~0xffff000000000000) |
               //      0xffff000000000000
               ((imm & 0x000000ffff000000) << 24)       // bottom 16 bits of imm39.
              );
    wide[1] = ((wide[1] & ~0x08fffff0007fffff) |
               //      0x00fffff000000000
               ((imm & 0x0000000000fffff0) << 32) |     // all 20 bits of imm20b.
               //      0x00000000007fffff
               ((imm & 0x7fffff0000000000) >> 40) |     // top 23 bits of imm39.
               //      0x0800000000000000
               ((imm & 0x8000000000000000) >>  4)       // single bit of i.
              );
}

UINT64 DETOUR_IA64_BUNDLE::GetBrlTarget() const
{
    return (UINT64)this + GetBrlImm();
}

VOID DETOUR_IA64_BUNDLE::SetBrl(UINT64 target)
{
    UINT64 imm = target - (UINT64)this;
    SetBrl();
    SetBrlImm(imm);
}

VOID DETOUR_IA64_BUNDLE::SetBrlTarget(UINT64 target)
{
    UINT64 imm = target - (UINT64)this;
    SetBrlImm(imm);
}

BOOL DETOUR_IA64_BUNDLE::IsMovlGp() const
{
    // f.e. d.c. b.a. 9.8. 7.6. 5.4. 3.2. 1.0.
    // 6fff f7f0 207f ffff ffff c001 0000 0004
    // 6000 0000 2000 0000 0000 0001 0000 0004
    return ((wide[0] & 0x00003ffffffffffe) == 0x0000000100000004 &&
            (wide[1] & 0xf000080fff800000) == 0x6000000020000000);
}

UINT64 DETOUR_IA64_BUNDLE::GetMovlGp() const
{
    UINT64 raw = (
                  //          0x0000000000000070
                  ((wide[1] & 0x000007f000000000) >> 36) |
                  //          0x000000000000ff80
                  ((wide[1] & 0x07fc000000000000) >> 43) |
                  //          0x00000000001f0000
                  ((wide[1] & 0x0003e00000000000) >> 29) |
                  //          0x0000000000200000
                  ((wide[1] & 0x0000100000000000) >> 23) |
                  //          0x000000ffffc00000
                  ((wide[0] & 0xffffc00000000000) >> 24) |
                  //          0x7fffff0000000000
                  ((wide[1] & 0x00000000007fffff) << 40) |
                  //          0x8000000000000000
                  ((wide[1] & 0x0800000000000000) <<  4)
                 );

    return (INT64)raw;
}

VOID DETOUR_IA64_BUNDLE::SetMovlGp(UINT64 gp)
{
    UINT64 raw = (UINT64)gp;

    wide[0] = (0x0000000100000005 |
               //      0xffffc00000000000
               ((raw & 0x000000ffffc00000) << 24)
              );
    wide[1] = (
               0x6000000020000000 |
               //      0x0000070000000000
               ((raw & 0x0000000000000070) << 36) |
               //      0x07fc000000000000
               ((raw & 0x000000000000ff80) << 43) |
               //      0x0003e00000000000
               ((raw & 0x00000000001f0000) << 29) |
               //      0x0000100000000000
               ((raw & 0x0000000000200000) << 23) |
               //      0x00000000007fffff
               ((raw & 0x7fffff0000000000) >> 40) |
               //      0x0800000000000000
               ((raw & 0x8000000000000000) >>  4)
              );
}

UINT DETOUR_IA64_BUNDLE::Copy(_Out_ DETOUR_IA64_BUNDLE *pDst,
                              _Inout_opt_ DETOUR_IA64_BUNDLE* pBundleExtra) const
{
    // Copy the bytes unchanged.

#pragma warning(suppress:6001) // using uninitialized *pDst
    pDst->wide[0] = wide[0];
    pDst->wide[1] = wide[1];

    // Relocate if necessary.

    UINT nExtraBytes = RelocateBundle(pDst, pBundleExtra);

    if (GetUnit1() == L_UNIT && IsBrl()) {
        pDst->SetBrlTarget(GetBrlTarget());
    }

    return nExtraBytes;
}

BOOL DETOUR_IA64_BUNDLE::SetNop(BYTE slot)
{
    switch (GetUnit(slot)) {
      case I_UNIT:
      case M_UNIT:
      case F_UNIT:
        SetInst(slot, 0);
        SetData(slot, 0x8000000);
        return true;
      case B_UNIT:
        SetInst(slot, 2);
        SetData(slot, 0);
        return true;
    }
    DebugBreak();
    return false;
}

BOOL DETOUR_IA64_BUNDLE::SetNop0()
{
    return SetNop(0);
}

BOOL DETOUR_IA64_BUNDLE::SetNop1()
{
    return SetNop(1);
}

BOOL DETOUR_IA64_BUNDLE::SetNop2()
{
    return SetNop(2);
}

VOID DETOUR_IA64_BUNDLE::SetStop()
{
    data[0] |= 0x01;
}

#endif // DETOURS_IA64

PVOID WINAPI DetourCopyInstruction(_In_opt_ PVOID pDst,
                                   _Inout_opt_ PVOID *ppDstPool,
                                   _In_ PVOID pSrc,
                                   _Out_opt_ PVOID *ppTarget,
                                   _Out_opt_ LONG *plExtra)
{
    LONG nExtra;
    DETOUR_IA64_BUNDLE bExtra;
    DETOUR_IA64_BUNDLE *pbSrc = (DETOUR_IA64_BUNDLE *)pSrc;
    DETOUR_IA64_BUNDLE *pbDst = pDst ? (DETOUR_IA64_BUNDLE *)pDst : &bExtra;

    plExtra = plExtra ? plExtra : &nExtra;
    *plExtra = 0;

    if (ppTarget != NULL) {
        if (pbSrc->IsBrl()) {
            *ppTarget = (PVOID)pbSrc->GetBrlTarget();
        }
        else {
            *ppTarget = DETOUR_INSTRUCTION_TARGET_NONE;
        }
    }
    *plExtra = (LONG)pbSrc->Copy(pbDst, ppDstPool ? ((DETOUR_IA64_BUNDLE*)*ppDstPool) - 1 : (DETOUR_IA64_BUNDLE*)NULL);
    return pbSrc + 1;
}

#endif // DETOURS_IA64

#ifdef DETOURS_ARM

#define DETOURS_PFUNC_TO_PBYTE(p)  ((PBYTE)(((ULONG_PTR)(p)) & ~(ULONG_PTR)1))
#define DETOURS_PBYTE_TO_PFUNC(p)  ((PBYTE)(((ULONG_PTR)(p)) | (ULONG_PTR)1))

#define c_PCAdjust  4       // The PC value of an instruction is the PC address plus 4.
#define c_PC        15      // The register number for the Program Counter
#define c_LR        14      // The register number for the Link Register
#define c_SP        13      // The register number for the Stack Pointer
#define c_NOP       0xbf00  // A nop instruction
#define c_BREAK     0xdefe  // A nop instruction

class CDetourDis
{
  public:
    CDetourDis();

    PBYTE   CopyInstruction(PBYTE pDst,
                            PBYTE *ppDstPool,
                            PBYTE pSrc,
                            PBYTE *ppTarget,
                            LONG *plExtra);

  public:
    typedef BYTE (CDetourDis::* COPYFUNC)(PBYTE pbDst, PBYTE pbSrc);

    struct COPYENTRY {
        USHORT      nOpcode;
        COPYFUNC    pfCopy;
    };

    typedef const COPYENTRY * REFCOPYENTRY;

    struct Branch5
    {
        DWORD Register : 3;
        DWORD Imm5 : 5;
        DWORD Padding : 1;
        DWORD I : 1;
        DWORD OpCode : 6;
    };

    struct Branch5Target
    {
        DWORD Padding : 1;
        DWORD Imm5 : 5;
        DWORD I : 1;
        DWORD Padding2 : 25;
    };

    struct Branch8
    {
        DWORD Imm8 : 8;
        DWORD Condition : 4;
        DWORD OpCode : 4;
    };

    struct Branch8Target
    {
        DWORD Padding : 1;
        DWORD Imm8 : 8;
        DWORD Padding2 : 23;
    };

    struct Branch11
    {
        DWORD Imm11 : 11;
        DWORD OpCode : 5;
    };

    struct Branch11Target
    {
        DWORD Padding : 1;
        DWORD Imm11 : 11;
        DWORD Padding2 : 20;
    };

    struct Branch20
    {
        DWORD Imm11 : 11;
        DWORD J2 : 1;
        DWORD IT : 1;
        DWORD J1 : 1;
        DWORD Other : 2;
        DWORD Imm6 : 6;
        DWORD Condition : 4;
        DWORD Sign : 1;
        DWORD OpCode : 5;
    };

    struct Branch20Target
    {
        DWORD Padding : 1;
        DWORD Imm11 : 11;
        DWORD Imm6 : 6;
        DWORD J1 : 1;
        DWORD J2 : 1;
        DWORD Sign : 1;
        INT32 Padding2 : 11;
    };

    struct Branch24
    {
        DWORD Imm11             : 11;
        DWORD J2                : 1;
        DWORD InstructionSet    : 1;
        DWORD J1                : 1;
        DWORD Link              : 1;
        DWORD Branch            : 1;
        DWORD Imm10             : 10;
        DWORD Sign              : 1;
        DWORD OpCode            : 5;
    };

    struct Branch24Target
    {
        DWORD Padding : 1;
        DWORD Imm11 : 11;
        DWORD Imm10 : 10;
        DWORD I2 : 1;
        DWORD I1 : 1;
        DWORD Sign : 1;
        INT32 Padding2 : 7;
    };

    struct LiteralLoad8
    {
        DWORD Imm8 : 8;
        DWORD Register : 3;
        DWORD OpCode : 5;
    };

    struct LiteralLoad8Target
    {
        DWORD Padding : 2;
        DWORD Imm8 : 8;
        DWORD Padding2 : 22;
    };

    struct LiteralLoad12
    {
        DWORD Imm12 : 12;
        DWORD Register : 4;
        DWORD OpCodeSuffix : 7;
        DWORD Add : 1;
        DWORD OpCodePrefix : 8;
    };

    struct LiteralLoad12Target
    {
        DWORD Imm12 : 12;
        DWORD Padding : 20;
    };

    struct ImmediateRegisterLoad32
    {
        DWORD Imm12 : 12;
        DWORD DestinationRegister : 4;
        DWORD SourceRegister: 4;
        DWORD OpCode : 12;
    };

    struct ImmediateRegisterLoad16
    {
        DWORD DestinationRegister : 3;
        DWORD SourceRegister: 3;
        DWORD OpCode : 10;
    };

    struct TableBranch
    {
        DWORD IndexRegister : 4;
        DWORD HalfWord : 1;
        DWORD OpCodeSuffix : 11;
        DWORD BaseRegister : 4;
        DWORD OpCodePrefix : 12;
    };

    struct Shift
    {
        DWORD Imm2 : 2;
        DWORD Imm3 : 3;
    };

    struct Add32
    {
        DWORD SecondOperandRegister : 4;
        DWORD Type : 2;
        DWORD Imm2 : 2;
        DWORD DestinationRegister : 4;
        DWORD Imm3 : 3;
        DWORD Padding : 1;
        DWORD FirstOperandRegister : 4;
        DWORD SetFlags : 1;
        DWORD OpCode : 11;
    };

    struct LogicalShiftLeft32
    {
        DWORD SourceRegister : 4;
        DWORD Padding : 2;
        DWORD Imm2 : 2;
        DWORD DestinationRegister : 4;
        DWORD Imm3 : 3;
        DWORD Padding2 : 5;
        DWORD SetFlags : 1;
        DWORD OpCode : 11;
    };

    struct StoreImmediate12
    {
        DWORD Imm12 : 12;
        DWORD SourceRegister : 4;
        DWORD BaseRegister : 4;
        DWORD OpCode : 12;
    };

  protected:
    BYTE    PureCopy16(BYTE* pSource, BYTE* pDest);
    BYTE    PureCopy32(BYTE* pSource, BYTE* pDest);
    BYTE    CopyMiscellaneous16(BYTE* pSource, BYTE* pDest);
    BYTE    CopyConditionalBranchOrOther16(BYTE* pSource, BYTE* pDest);
    BYTE    CopyUnConditionalBranch16(BYTE* pSource, BYTE* pDest);
    BYTE    CopyLiteralLoad16(BYTE* pSource, BYTE* pDest);
    BYTE    CopyBranchExchangeOrDataProcessing16(BYTE* pSource, BYTE* pDest);
    BYTE    CopyBranch24(BYTE* pSource, BYTE* pDest);
    BYTE    CopyBranchOrMiscellaneous32(BYTE* pSource, BYTE* pDest);
    BYTE    CopyLiteralLoad32(BYTE* pSource, BYTE* pDest);
    BYTE    CopyLoadAndStoreSingle(BYTE* pSource, BYTE* pDest);
    BYTE    CopyLoadAndStoreMultipleAndSRS(BYTE* pSource, BYTE* pDest);
    BYTE    CopyTableBranch(BYTE* pSource, BYTE* pDest);
    BYTE    BeginCopy32(BYTE* pSource, BYTE* pDest);

    LONG    DecodeBranch5(ULONG opcode);
    USHORT  EncodeBranch5(ULONG originalOpCode, LONG delta);
    LONG    DecodeBranch8(ULONG opcode);
    USHORT  EncodeBranch8(ULONG originalOpCode, LONG delta);
    LONG    DecodeBranch11(ULONG opcode);
    USHORT  EncodeBranch11(ULONG originalOpCode, LONG delta);
    BYTE    EmitBranch11(PUSHORT& pDest, LONG relativeAddress);
    LONG    DecodeBranch20(ULONG opcode);
    ULONG   EncodeBranch20(ULONG originalOpCode, LONG delta);
    LONG    DecodeBranch24(ULONG opcode, BOOL& fLink);
    ULONG   EncodeBranch24(ULONG originalOpCode, LONG delta, BOOL fLink);
    LONG    DecodeLiteralLoad8(ULONG instruction);
    LONG    DecodeLiteralLoad12(ULONG instruction);
    BYTE    EmitLiteralLoad8(PUSHORT& pDest, BYTE targetRegister, PBYTE pLiteral);
    BYTE    EmitLiteralLoad12(PUSHORT& pDest, BYTE targetRegister, PBYTE pLiteral);
    BYTE    EmitImmediateRegisterLoad32(PUSHORT& pDest, BYTE reg);
    BYTE    EmitImmediateRegisterLoad16(PUSHORT& pDest, BYTE reg);
    BYTE    EmitLongLiteralLoad(PUSHORT& pDest, BYTE reg, PVOID pTarget);
    BYTE    EmitLongBranch(PUSHORT& pDest, PVOID pTarget);
    USHORT  CalculateExtra(BYTE sourceLength, BYTE* pDestStart, BYTE* pDestEnd);

  protected:
    ULONG GetLongInstruction(BYTE* pSource)
    {
        return (((PUSHORT)pSource)[0] << 16) | (((PUSHORT)pSource)[1]);
    }

    BYTE EmitLongInstruction(PUSHORT& pDstInst, ULONG instruction)
    {
        *pDstInst++ = (USHORT)(instruction >> 16);
        *pDstInst++ = (USHORT)instruction;
        return sizeof(ULONG);
    }

    BYTE EmitShortInstruction(PUSHORT& pDstInst, USHORT instruction)
    {
        *pDstInst++ = instruction;
        return sizeof(USHORT);
    }

    PBYTE Align4(PBYTE pValue)
    {
        return (PBYTE)(((size_t)pValue) & ~(ULONG)3u);
    }

    PBYTE CalculateTarget(PBYTE pSource, LONG delta)
    {
        return (pSource + delta + c_PCAdjust);
    }

    LONG CalculateNewDelta(PBYTE pTarget, BYTE* pDest)
    {
        return (LONG)(pTarget - (pDest + c_PCAdjust));
    }

    BYTE    EmitAdd32(PUSHORT& pDstInst, BYTE op1Reg, BYTE op2Reg, BYTE dstReg, BYTE shiftAmount)
    {
        Shift& shift = (Shift&)(shiftAmount);
        const BYTE shiftType = 0x00; // LSL
        Add32 add = { op2Reg, shiftType, shift.Imm2, dstReg, shift.Imm3,
                      0x0, op1Reg, 0x0, 0x758 };
        return EmitLongInstruction(pDstInst, (ULONG&)add);
    }

    BYTE    EmitLogicalShiftLeft32(PUSHORT& pDstInst, BYTE srcReg, BYTE dstReg, BYTE shiftAmount)
    {
        Shift& shift = (Shift&)(shiftAmount);
        LogicalShiftLeft32 shiftLeft = { srcReg, 0x00, shift.Imm2, dstReg, shift.Imm3, 0x1E,
                                         0x00, 0x752 };
        return EmitLongInstruction(pDstInst, (ULONG&)shiftLeft);
    }

    BYTE    EmitStoreImmediate12(PUSHORT& pDstInst, BYTE srcReg, BYTE baseReg, USHORT offset)
    {
        StoreImmediate12 store = { offset, srcReg, baseReg, 0xF8C };
        return EmitLongInstruction(pDstInst, (ULONG&)store);
    }

  protected:
    PBYTE   m_pbTarget;
    PBYTE   m_pbPool;
    LONG    m_lExtra;

    BYTE    m_rbScratchDst[64]; // matches or exceeds rbCode

    static const COPYENTRY s_rceCopyTable[33];
};

LONG CDetourDis::DecodeBranch5(ULONG opcode)
{
    Branch5& branch = (Branch5&)(opcode);

    Branch5Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm5 = branch.Imm5;
    target.I = branch.I;

    // Return zero-extended value
    return (LONG&)target;
}

USHORT CDetourDis::EncodeBranch5(ULONG originalOpCode, LONG delta)
{
    // Too large for a 5 bit branch (5 bit branches can be up to 7 bits due to I and the trailing 0)
    if (delta < 0 || delta > 0x7F) {
        return 0;
    }

    Branch5& branch = (Branch5&)(originalOpCode);
    Branch5Target& target = (Branch5Target&)(delta);

    branch.Imm5 = target.Imm5;
    branch.I = target.I;

    return (USHORT&)branch;
}

LONG CDetourDis::DecodeBranch8(ULONG opcode)
{
    Branch8& branch = (Branch8&)(opcode);

    Branch8Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm8 = branch.Imm8;

    // Return sign extended value
    return (((LONG&)target) << 23) >> 23;
}

USHORT CDetourDis::EncodeBranch8(ULONG originalOpCode, LONG delta)
{
    // Too large for 8 bit branch (8 bit branches can be up to 9 bits due to the trailing 0)
    if (delta < (-(int)0x100) || delta > 0xFF) {
        return 0;
    }

    Branch8& branch = (Branch8&)(originalOpCode);
    Branch8Target& target = (Branch8Target&)(delta);

    branch.Imm8 = target.Imm8;

    return (USHORT&)branch;
}

LONG CDetourDis::DecodeBranch11(ULONG opcode)
{
    Branch11& branch = (Branch11&)(opcode);

    Branch11Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm11 = branch.Imm11;

    // Return sign extended value
    return (((LONG&)target) << 20) >> 20;
}

USHORT CDetourDis::EncodeBranch11(ULONG originalOpCode, LONG delta)
{
    // Too large for an 11 bit branch (11 bit branches can be up to 12 bits due to the trailing 0)
    if (delta < (-(int)0x800) || delta > 0x7FF) {
        return 0;
    }

    Branch11& branch = (Branch11&)(originalOpCode);
    Branch11Target& target = (Branch11Target&)(delta);

    branch.Imm11 = target.Imm11;

    return (USHORT&)branch;
}

BYTE CDetourDis::EmitBranch11(PUSHORT& pDest, LONG relativeAddress)
{
    Branch11Target& target = (Branch11Target&)(relativeAddress);
    Branch11 branch11 = { target.Imm11, 0x1C };

    *pDest++ = (USHORT&)branch11;
    return sizeof(USHORT);
}

LONG CDetourDis::DecodeBranch20(ULONG opcode)
{
    Branch20& branch = (Branch20&)(opcode);

    Branch20Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm11 = branch.Imm11;
    target.Imm6 = branch.Imm6;
    target.Sign = branch.Sign;
    target.J1 = branch.J1;
    target.J2 = branch.J2;

    // Sign extend
    if (target.Sign) {
        target.Padding2 = -1;
    }

    return (LONG&)target;
}

ULONG CDetourDis::EncodeBranch20(ULONG originalOpCode, LONG delta)
{
    // Too large for 20 bit branch (20 bit branches can be up to 21 bits due to the trailing 0)
    if (delta < (-(int)0x100000) || delta > 0xFFFFF) {
        return 0;
    }

    Branch20& branch = (Branch20&)(originalOpCode);
    Branch20Target& target = (Branch20Target&)(delta);

    branch.Imm11 = target.Imm11;
    branch.Imm6 = target.Imm6;
    branch.Sign = target.Sign;
    branch.J1 = target.J1;
    branch.J2 = target.J2;

    return (ULONG&)branch;
}

LONG CDetourDis::DecodeBranch24(ULONG opcode, BOOL& fLink)
{
    Branch24& branch = (Branch24&)(opcode);

    Branch24Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm11 = branch.Imm11;
    target.Imm10 = branch.Imm10;
    target.Sign = branch.Sign;
    target.I1 = ~(branch.J1 ^ target.Sign);
    target.I2 = ~(branch.J2 ^ target.Sign);
    fLink = branch.Link;

    // Sign extend
    if (target.Sign) {
        target.Padding2 = -1;
    }

    return (LONG&)target;
}

ULONG CDetourDis::EncodeBranch24(ULONG originalOpCode, LONG delta, BOOL fLink)
{
    // Too large for 24 bit branch (24 bit branches can be up to 25 bits due to the trailing 0)
    if (delta < static_cast<int>(0xFF000000) || delta > static_cast<int>(0xFFFFFF)) {
        return 0;
    }

    Branch24& branch = (Branch24&)(originalOpCode);
    Branch24Target& target = (Branch24Target&)(delta);

    branch.Imm11 = target.Imm11;
    branch.Imm10 = target.Imm10;
    branch.Link = fLink;
    branch.Sign = target.Sign;
    branch.J1 = ~(target.I1 ^ branch.Sign);
    branch.J2 = ~(target.I2 ^ branch.Sign);

    return (ULONG&)branch;
}

LONG CDetourDis::DecodeLiteralLoad8(ULONG instruction)
{
    LiteralLoad8& load = (LiteralLoad8&)(instruction);

    LiteralLoad8Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm8 = load.Imm8;

    return (LONG&)target;
}

BYTE CDetourDis::EmitLiteralLoad8(PUSHORT& pDest, BYTE targetRegister, PBYTE pLiteral)
{
    // Note: We add 2 (which gets rounded down) because literals must be 32-bit
    //       aligned, but the ldr can be 16-bit aligned.
    LONG newDelta = CalculateNewDelta((PBYTE)pLiteral + 2, (PBYTE)pDest);
    LONG relative = ((newDelta > 0 ? newDelta : -newDelta) & 0x3FF);

    LiteralLoad8Target& target = (LiteralLoad8Target&)(relative);
    LiteralLoad8 load = { target.Imm8, targetRegister, 0x9 };

    return EmitShortInstruction(pDest, (USHORT&)load);
}

LONG CDetourDis::DecodeLiteralLoad12(ULONG instruction)
{
    LiteralLoad12& load = (LiteralLoad12&)(instruction);

    LiteralLoad12Target target;
    ZeroMemory(&target, sizeof(target));
    target.Imm12 = load.Imm12;

    return (LONG&)target;
}

BYTE CDetourDis::EmitLiteralLoad12(PUSHORT& pDest, BYTE targetRegister, PBYTE pLiteral)
{
    // Note: We add 2 (which gets rounded down) because literals must be 32-bit
    //       aligned, but the ldr can be 16-bit aligned.
    LONG newDelta = CalculateNewDelta((PBYTE)pLiteral + 2, (PBYTE)pDest);
    LONG relative = ((newDelta > 0 ? newDelta : -newDelta) & 0xFFF);

    LiteralLoad12Target& target = (LiteralLoad12Target&)(relative);
    target.Imm12 -= target.Imm12 & 3;
    LiteralLoad12 load = { target.Imm12, targetRegister, 0x5F, (DWORD)(newDelta > 0),  0xF8 };

    return EmitLongInstruction(pDest, (ULONG&)load);
}

BYTE CDetourDis::EmitImmediateRegisterLoad32(PUSHORT& pDest, BYTE reg)
{
    ImmediateRegisterLoad32 load = { 0, reg, reg, 0xF8D };
    return EmitLongInstruction(pDest, (ULONG&)load);
}

BYTE CDetourDis::EmitImmediateRegisterLoad16(PUSHORT& pDest, BYTE reg)
{
    ImmediateRegisterLoad16 load = { reg, reg, 0x680 >> 2 };
    return EmitShortInstruction(pDest, (USHORT&)load);
}

BYTE CDetourDis::EmitLongLiteralLoad(PUSHORT& pDest, BYTE targetRegister, PVOID pTarget)
{
    *--((PULONG&)m_pbPool) = (ULONG)(size_t)pTarget;

    // ldr rn, target.
    BYTE size = EmitLiteralLoad12(pDest, targetRegister, m_pbPool);

    // This only makes sense if targetRegister != PC;
    // otherwise, we would have branched with the previous instruction anyway
    if (targetRegister != c_PC) {
        // ldr rn, [rn]
        if (targetRegister <= 7) {
            size = (BYTE)(size + EmitImmediateRegisterLoad16(pDest, targetRegister));
        }
        else {
            size = (BYTE)(size + EmitImmediateRegisterLoad32(pDest, targetRegister));
        }
    }

    return size;
}

BYTE CDetourDis::EmitLongBranch(PUSHORT& pDest, PVOID pTarget)
{
    // Emit a long literal load into PC
    BYTE size = EmitLongLiteralLoad(pDest, c_PC, DETOURS_PBYTE_TO_PFUNC(pTarget));
    return size;
}

BYTE CDetourDis::PureCopy16(BYTE* pSource, BYTE* pDest)
{
    *(USHORT *)pDest = *(USHORT *)pSource;
    return sizeof(USHORT);
}

BYTE CDetourDis::PureCopy32(BYTE* pSource, BYTE* pDest)
{
    *(UNALIGNED ULONG *)pDest = *(UNALIGNED ULONG*)pSource;
    return sizeof(DWORD);
}

USHORT CDetourDis::CalculateExtra(BYTE sourceLength, BYTE* pDestStart, BYTE* pDestEnd)
{
    ULONG destinationLength = (ULONG)(pDestEnd - pDestStart);
    return static_cast<USHORT>((destinationLength > sourceLength) ? (destinationLength - sourceLength) : 0);
}

BYTE CDetourDis::CopyMiscellaneous16(BYTE* pSource, BYTE* pDest)
{
    USHORT instruction = *(PUSHORT)(pSource);

    // Compare and branch imm5 (CBZ, CBNZ)
    if ((instruction & 0x100) && !(instruction & 0x400)) { // (1011x0x1xxxxxxxx)
        LONG oldDelta = DecodeBranch5(instruction);
        PBYTE pTarget = CalculateTarget(pSource, oldDelta);
        m_pbTarget = pTarget;

        LONG newDelta = CalculateNewDelta(pTarget, pDest);
        instruction = EncodeBranch5(instruction, newDelta);

        if (instruction) {
            // Copy the 16 bit instruction over
            *(PUSHORT)(pDest) = instruction;
            return sizeof(USHORT); // The source instruction was 16 bits
        }

        // If that fails, re-encode with 'conditional branch' logic, without using the condition flags
        // For example, cbz r2,+0x56 (0x90432) becomes:
        //
        //  001df73a b92a     cbnz        r2,001df748
        //  001df73c e002     b           001df744
        //  001df73e bf00     nop
        //  001df740 0432     dc.h        0432
        //  001df742 0009     dc.h        0009
        //  001df744 f85ff008 ldr         pc,=0x90432
        //

        // Store where we will be writing our conditional branch, and move past it so we can emit a long branch
        PUSHORT pDstInst = (PUSHORT)(pDest);
        PUSHORT pConditionalBranchInstruction = pDstInst++;

        // Emit the long branch instruction
        BYTE longBranchSize = EmitLongBranch(pDstInst, pTarget);

        // Invert the CBZ/CBNZ instruction to move past our 'long branch' if the inverse comparison succeeds
        // Write the CBZ/CBNZ instruction *before* the long branch we emitted above
        // This had to be done out of order, since the size of a long branch can vary due to alignment restrictions
        instruction = EncodeBranch5(*(PUSHORT)(pSource), longBranchSize - c_PCAdjust + sizeof(USHORT));
        Branch5& branch = (Branch5&)(instruction);
        branch.OpCode = (branch.OpCode & 0x02) ? 0x2C : 0x2E; // Invert the CBZ/CBNZ comparison
        *pConditionalBranchInstruction = instruction;

        // Compute the extra space needed for the branch sequence
        m_lExtra = CalculateExtra(sizeof(USHORT), pDest, (BYTE*)(pDstInst));
        return sizeof(USHORT); // The source instruction was 16 bits
    }

    // If-Then Instruction (IT)
    if ((instruction >> 8 == 0xBF) && (instruction & 0xF)) { //(10111111xxxx(mask != 0b0000))
        // ToDo: Implement IT handler
        ASSERT(false);
        return sizeof(USHORT);
    }

    // ADD/SUB, SXTH, SXTB, UXTH, UXTB, CBZ, CBNZ, PUSH, POP, REV, REV15, REVSH, NOP, YIELD, WFE, WFI, SEV, etc.
    return PureCopy16(pSource, pDest);
}

BYTE CDetourDis::CopyConditionalBranchOrOther16(BYTE* pSource, BYTE* pDest)
{
    USHORT instruction = *(PUSHORT)(pSource);

    // Could be a conditional branch, an Undefined instruction or a Service System Call
    // Only the former needs special logic
    if ((instruction & 0xE00) != 0xE00) { // 1101(!=111x)xxxxxxxx
        LONG oldDelta = DecodeBranch8(instruction);
        PBYTE pTarget = CalculateTarget(pSource, oldDelta);
        m_pbTarget = pTarget;

        LONG newDelta = CalculateNewDelta(pTarget, pDest);
        instruction = EncodeBranch8(instruction, newDelta);
        if (instruction) {
            // Copy the 16 bit instruction over
            *(PUSHORT)(pDest) = instruction;
            return sizeof(USHORT); // The source instruction was 16 bits
        }

        // If that fails, re-encode as a sequence of branches
        // For example, bne +0x6E (0x90452) becomes:
        //
        // 001df758 d100     bne         001df75c
        // 001df75a e005     b           001df768
        // 001df75c e002     b           001df764
        // 001df75e bf00     nop
        // 001df760 0452     dc.h        0452
        // 001df762 0009     dc.h        0009
        // 001df764 f85ff008 ldr         pc,=0x90452
        //

        // First, reuse the existing conditional branch to, if successful, branch down to a 'long branch' that we will emit below
        USHORT newInstruction = EncodeBranch8(*(PUSHORT)(pSource), 0); // Due to the size of c_PCAdjust a zero-length branch moves 4 bytes forward, past the following unconditional branch
        ASSERT(newInstruction);
        PUSHORT pDstInst = (PUSHORT)(pDest);
        *pDstInst++ = newInstruction;

        // Next, prepare to insert an unconditional branch that will be hit if the condition above is not met.  This branch will branch over the following 'long branch'
        // We can't actually encode this branch yet though, because 'long branches' can vary in size
        PUSHORT pUnconditionalBranchInstruction = pDstInst++;

        // Then, emit a 'long branch' that will be hit if the original condition is met
        BYTE longBranchSize = EmitLongBranch(pDstInst, pTarget);

        // Finally, encode and emit the unconditional branch that will be used to branch past the 'long branch' if the initial condition was not met
        Branch11 branch11 = { 0x00, 0x1C };
        newInstruction = EncodeBranch11(*(DWORD*)(&branch11), longBranchSize - c_PCAdjust + sizeof(USHORT));
        ASSERT(newInstruction);
        *pUnconditionalBranchInstruction = newInstruction;

        // Compute the extra space needed for the branch sequence
        m_lExtra = CalculateExtra(sizeof(USHORT), pDest, (BYTE*)(pDstInst));
        return sizeof(USHORT); // The source instruction was 16 bits
    }

    return PureCopy16(pSource, pDest);
}

BYTE CDetourDis::CopyUnConditionalBranch16(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = *(PUSHORT)(pSource);

    LONG oldDelta = DecodeBranch11(instruction);
    PBYTE pTarget = CalculateTarget(pSource, oldDelta);
    m_pbTarget = pTarget;

    LONG newDelta = CalculateNewDelta(pTarget, pDest);
    instruction = EncodeBranch11(instruction, newDelta);
    if (instruction) {
        // Copy the 16 bit instruction over
        *(PUSHORT)(pDest) = (USHORT)instruction;
        return sizeof(USHORT); // The source instruction was 16 bits
    }

    // If that fails, re-encode as 32-bit
    PUSHORT pDstInst = (PUSHORT)(pDest);
    instruction = EncodeBranch24(0xf0009000, newDelta, FALSE);
    if (instruction) {
        // Copy both bytes of the instruction
        EmitLongInstruction(pDstInst, instruction);

        m_lExtra = sizeof(DWORD) - sizeof(USHORT); // The destination instruction was 32 bits
        return sizeof(USHORT); // The source instruction was 16 bits
    }

    // If that fails, emit as a 'long branch'
    if (!instruction) {
        // For example, b +0x7FE (00090be6) becomes:
        // 003f6d02 e001     b           003f6d08
        // 003f6d04 0be6     dc.h        0be6
        // 003f6d06 0009     dc.h        0009
        // 003f6d08 f85ff008 ldr         pc,=0x90BE6
        EmitLongBranch(pDstInst, pTarget);

        // Compute the extra space needed for the branch sequence
        m_lExtra = CalculateExtra(sizeof(USHORT), pDest, (BYTE*)(pDstInst));
        return sizeof(USHORT); // The source instruction was 16 bits
    }

    return sizeof(USHORT); // The source instruction was 16 bits
}

BYTE CDetourDis::CopyLiteralLoad16(BYTE* pSource, BYTE* pDest)
{
    PBYTE pStart = pDest;
    USHORT instruction = *(PUSHORT)(pSource);

    LONG oldDelta = DecodeLiteralLoad8(instruction);
    PBYTE pTarget = CalculateTarget(Align4(pSource), oldDelta);

    // Re-encode as a 'long literal load'
    // For example, ldr r0, [PC + 1E0] (0x905B4) becomes:
    //
    // 001df72c f85f0008 ldr         r0,=0x905B4
    // 001df730 f8d00000 ldr.w       r0,[r0]
    LiteralLoad8& load8 = (LiteralLoad8&)(instruction);
    EmitLongLiteralLoad((PUSHORT&)pDest, load8.Register, pTarget);

    m_lExtra = (LONG)(pDest - pStart - sizeof(USHORT));
    return sizeof(USHORT); // The source instruction was 16 bits
}

BYTE CDetourDis::CopyBranchExchangeOrDataProcessing16(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = *(PUSHORT)(pSource);

    // BX
    if ((instruction & 0xff80) == 0x4700) {
        // The target is stored in a register
        m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }

    // AND, LSR, TST, ADD, CMP, MOV
    return PureCopy16(pSource, pDest);
}

const CDetourDis::COPYENTRY CDetourDis::s_rceCopyTable[33] =
{
    // Shift by immediate, move register
    // ToDo: Not handling moves from PC
    /* 0b00000 */ { 0x00, &CDetourDis::PureCopy16 },
    /* 0b00001 */ { 0x01, &CDetourDis::PureCopy16 },
    /* 0b00010 */ { 0x02, &CDetourDis::PureCopy16 },

    // Add/subtract register
    // Add/subtract immediate
    /* 0b00011 */ { 0x03, &CDetourDis::PureCopy16},

    // Add/subtract/compare/move immediate
    /* 0b00100 */ { 0x04, &CDetourDis::PureCopy16 },
    /* 0b00101 */ { 0x05, &CDetourDis::PureCopy16 },
    /* 0b00110 */ { 0x06, &CDetourDis::PureCopy16 },
    /* 0b00111 */ { 0x07, &CDetourDis::PureCopy16 },

    // Data-processing register
    // Special data processing
    // Branch/exchange instruction set
    /* 0b01000 */ { 0x08, &CDetourDis::CopyBranchExchangeOrDataProcessing16 },

    // Load from literal pool
    /* 0b01001 */ { 0x09, &CDetourDis::CopyLiteralLoad16 },

    // Load/store register offset
    /* 0b01010 */ { 0x0a, &CDetourDis::PureCopy16 },
    /* 0b01011 */ { 0x0b, &CDetourDis::PureCopy16 },

    //  Load/store word/byte immediate offset.
    /* 0b01100 */ { 0x0c, &CDetourDis::PureCopy16 },
    /* 0b01101 */ { 0x0d, &CDetourDis::PureCopy16 },
    /* 0b01110 */ { 0x0e, &CDetourDis::PureCopy16 },
    /* 0b01111 */ { 0x0f, &CDetourDis::PureCopy16 },

    //  Load/store halfword immediate offset.
    /* 0b10000 */ { 0x10, &CDetourDis::PureCopy16 },
    /* 0b10001 */ { 0x11, &CDetourDis::PureCopy16 },

    // Load from or store to stack
    /* 0b10010 */ { 0x12, &CDetourDis::PureCopy16 },
    /* 0b10011 */ { 0x13, &CDetourDis::PureCopy16 },

    // Add to SP or PC
    /* 0b10100 */ { 0x14, &CDetourDis::PureCopy16 },
    //   ToDo: Is ADR (T1) blitt-able?
    //     It adds a value to PC and stores the result in a register.
    //     Does this count as a 'target' for detours?
    /* 0b10101 */ { 0x15, &CDetourDis::PureCopy16 },

    // Miscellaneous
    /* 0b10110 */ { 0x16, &CDetourDis::CopyMiscellaneous16 },
    /* 0b10111 */ { 0x17, &CDetourDis::CopyMiscellaneous16 },

    // Load/store multiple
    /* 0b11000 */ { 0x18, &CDetourDis::PureCopy16 },
    /* 0b11001 */ { 0x19, &CDetourDis::PureCopy16 },
    //   ToDo: Are we sure these are all safe?
    //     LDMIA, for example, can include an 'embedded' branch.
    //     Does this count as a 'target' for detours?

    // Conditional branch
    /* 0b11010 */ { 0x1a, &CDetourDis::CopyConditionalBranchOrOther16 },

    // Conditional branch
    // Undefined instruction
    // Service (system) call
    /* 0b11011 */ { 0x1b, &CDetourDis::CopyConditionalBranchOrOther16 },

    // Unconditional branch
    /* 0b11100 */ { 0x1c, &CDetourDis::CopyUnConditionalBranch16 },

    // 32-bit instruction
    /* 0b11101 */ { 0x1d, &CDetourDis::BeginCopy32 },
    /* 0b11110 */ { 0x1e, &CDetourDis::BeginCopy32 },
    /* 0b11111 */ { 0x1f, &CDetourDis::BeginCopy32 },
    { 0, NULL }
};

BYTE CDetourDis::CopyBranch24(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = GetLongInstruction(pSource);
    BOOL fLink;
    LONG oldDelta = DecodeBranch24(instruction, fLink);
    PBYTE pTarget = CalculateTarget(pSource, oldDelta);
    m_pbTarget = pTarget;

    // Re-encode as 32-bit
    PUSHORT pDstInst = (PUSHORT)(pDest);
    LONG newDelta = CalculateNewDelta(pTarget, pDest);
    instruction = EncodeBranch24(instruction, newDelta, fLink);
    if (instruction) {
        // Copy both bytes of the instruction
        EmitLongInstruction(pDstInst, instruction);
        return sizeof(DWORD);
    }

    // If that fails, re-encode as a 'long branch'
    EmitLongBranch(pDstInst, pTarget);

    // Compute the extra space needed for the instruction
    m_lExtra = CalculateExtra(sizeof(DWORD), pDest, (BYTE*)(pDstInst));
    return sizeof(DWORD); // The source instruction was 32 bits
}

BYTE CDetourDis::CopyBranchOrMiscellaneous32(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = GetLongInstruction(pSource);
    if ((instruction & 0xf800d000) == 0xf0008000) { // B<c>.W <label>
        LONG oldDelta = DecodeBranch20(instruction);
        PBYTE pTarget = CalculateTarget(pSource, oldDelta);
        m_pbTarget = pTarget;

        // Re-encode as 32-bit
        PUSHORT pDstInst = (PUSHORT)(pDest);
        LONG newDelta = CalculateNewDelta(pTarget, pDest);
        instruction = EncodeBranch20(instruction, newDelta);
        if (instruction) {
            // Copy both bytes of the instruction
            EmitLongInstruction(pDstInst, instruction);
            return sizeof(DWORD);
        }

        // If that fails, re-encode as a sequence of branches
        // For example, bls.w +0x86 (00090480)| becomes:
        //
        // 001df788 f2408001 bls.w       001df78e
        // 001df78c e004     b           001df798
        // 001df78e e001     b           001df794
        // 001df790 0480     dc.h        0480
        // 001df792 0009     dc.h        0009
        // 001df794 f85ff008 ldr         pc,=0x90480
        //

        // First, reuse the existing conditional branch to, if successful,
        // branch down to a 'long branch' that we will emit below
        instruction = EncodeBranch20(GetLongInstruction(pSource), 2);
        // Due to the size of c_PCAdjust a two-length branch moves 6 bytes forward,
        // past the following unconditional branch
        ASSERT(instruction);
        EmitLongInstruction(pDstInst, instruction);

        // Next, prepare to insert an unconditional branch that will be hit
        // if the condition above is not met.  This branch will branch over
        // the following 'long branch'
        // We can't actually encode this branch yet though, because
        // 'long branches' can vary in size
        PUSHORT pUnconditionalBranchInstruction = pDstInst++;

        // Then, emit a 'long branch' that will be hit if the original condition is met
        BYTE longBranchSize = EmitLongBranch(pDstInst, pTarget);

        // Finally, encode and emit the unconditional branch that will be used
        // to branch past the 'long branch' if the initial condition was not met
        Branch11 branch11 = { 0x00, 0x1C };
        instruction = EncodeBranch11(*(DWORD*)(&branch11), longBranchSize - c_PCAdjust + sizeof(USHORT));
        ASSERT(instruction);
        *pUnconditionalBranchInstruction = static_cast<USHORT>(instruction);

        // Compute the extra space needed for the instruction
        m_lExtra = CalculateExtra(sizeof(DWORD), pDest, (BYTE*)(pDstInst));
        return sizeof(DWORD); // The source instruction was 32 bits
    }

    if ((instruction & 0xf800d000) == 0xf0009000) { // B.W <label>
        // B <label>  11110xxxxxxxxxxx10xxxxxxxxxxxxxx
        return CopyBranch24(pSource, pDest);
    }

    if ((instruction & 0xf800d000) == 0xf000d000) { // BL.W <label>
        // B <label>  11110xxxxxxxxxxx10xxxxxxxxxxxxxx

        PUSHORT pDstInst = (PUSHORT)(pDest);
        BOOL fLink;
        LONG oldDelta = DecodeBranch24(instruction, fLink);
        PBYTE pTarget = CalculateTarget(pSource, oldDelta);
        m_pbTarget = pTarget;

        *--((PULONG&)m_pbPool) = (ULONG)(size_t)DETOURS_PBYTE_TO_PFUNC(pTarget);

        // ldr lr, target.
        EmitLiteralLoad12(pDstInst, c_LR, m_pbPool);
        // blx lr
        EmitShortInstruction(pDstInst, 0x47f0);

        // Compute the extra space needed for the instruction
        m_lExtra = CalculateExtra(sizeof(DWORD), pDest, (BYTE*)(pDstInst));
        return sizeof(DWORD); // The source instruction was 32 bits
    }

    if ((instruction & 0xFFF0FFFF) == 0xF3C08F00) {
        // BXJ 111100111100xxxx1000111100000000
        // BXJ switches to Jazelle mode, which is not supported
        ASSERT(false);
    }

    if ((instruction & 0xFFFFFF00) == 0xF3DE8F00) {
        // SUBS PC, LR 111100111101111010001111xxxxxxxx
        m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }

    // Everything else should be blitt-able
    return PureCopy32(pSource, pDest);
}

BYTE CDetourDis::CopyLiteralLoad32(BYTE* pSource, BYTE* pDest)
{
    BYTE* pStart = pDest;
    ULONG instruction = GetLongInstruction(pSource);

    LONG oldDelta = DecodeLiteralLoad12(instruction);
    PBYTE pTarget = CalculateTarget(Align4(pSource), oldDelta);

    LiteralLoad12& load = (LiteralLoad12&)(instruction);

    EmitLongLiteralLoad((PUSHORT&)pDest, load.Register, pTarget);

    m_lExtra = (LONG)(pDest - pStart - sizeof(DWORD));

    return sizeof(DWORD); // The source instruction was 32 bits
}

BYTE CDetourDis::CopyLoadAndStoreSingle(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = GetLongInstruction(pSource);

    // Note: The following masks only look at the interesting bits
    // (not the opCode prefix, since that check was performed in
    // order to get to this function)
    if (!(instruction & 0x100000)) {
        // 1111 100x xxx0 xxxxxxxxxxxxxxxxxxxx : STR, STRB, STRH, etc.
        return PureCopy32(pSource, pDest);
    }

    if ((instruction & 0xF81F0000) == 0xF81F0000) {
        // 1111100xxxx11111xxxxxxxxxxxxxxxx : PC +/- Imm12
        return CopyLiteralLoad32(pSource, pDest);
    }

    if ((instruction & 0xFE70F000) == 0xF81FF000) {
        // 1111100xx001xxxx1111xxxxxxxxxxxx : PLD, PLI
        // Convert PC-Relative PLD/PLI instructions to noops (1111100Xx00111111111xxxxxxxxxxxx)
        if ((instruction & 0xFE7FF000) == 0xF81FF000) {
            PUSHORT pDstInst = (PUSHORT)(pDest);
            *pDstInst++ = c_NOP;
            *pDstInst++ = c_NOP;
            return sizeof(DWORD);  // The source instruction was 32 bits
        }

        // All other PLD/PLI instructions are blitt-able
        return PureCopy32(pSource, pDest);
    }

    // If the load is writing to PC
    if ((instruction & 0xF950F000) == 0xF850F000) {
        m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    }

    // All other loads LDR (immediate), etc.
    return PureCopy32(pSource, pDest);
}

BYTE CDetourDis::CopyLoadAndStoreMultipleAndSRS(BYTE* pSource, BYTE* pDest)
{
    // Probably all blitt-able, although not positive since some of these can result in a branch (LDMIA, POP, etc.)
    return PureCopy32(pSource, pDest);
}

BYTE CDetourDis::CopyTableBranch(BYTE* pSource, BYTE* pDest)
{
    m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
    ULONG instruction = GetLongInstruction(pSource);
    TableBranch& tableBranch = (TableBranch&)(instruction);

    // If the base register is anything other than PC, we can simply copy the instruction
    if (tableBranch.BaseRegister != c_PC) {
        return PureCopy32(pSource, pDest);
    }

    __debugbreak();

    // If the base register is PC, we need to manually perform the table lookup
    // For example, this:
    //
    //        7ef40000 e8dff002 tbb         [pc,r2]
    //
    // becomes this:
    //
    //        7ef40404 b401     push        {r0}            ; pushed as a placeholder for the target address
    //        7ef40406 e92d0005 push.w      {r0,r2}         ; scratch register and another register are pushed; there's a minimum of two registers in the list for push.w
    //        7ef40410 4820     ldr         r0,=0x7EF40004  ; load the table address from the literal pool
    //        7ef40414 eb000042 add         r0,r0,r2,lsl #1 ; add the index value to the address of the table to get the table entry; lsl only used if it's a TBH instruction
    //        7ef40418 f8d00000 ldr.w       r0,[r0]         ; dereference the table entry to get the value of the target
    //        7ef4041c ea4f0040 lsl         r0,r0,#1        ; multiply the offset by 2 (per the spec)
    //        7ef40420 eb00000f add.w       r0,r0,pc        ; Add the offset to pc to get the target address
    //        7ef40424 f8cd000c str.w       r0,[sp,#0xC]    ; store the target address on the stack (into the first push)
    //        7ef40428 e8bd0005 pop.w       {r0,r2}         ; scratch register and another register are popped; there's a minimum of two registers in the list for pop.w
    //        7ef4042c bd00     pop         {pc}            ; pop the address into pc
    //

    // Push r0 to make room for our jump address on the stack
    PUSHORT pDstInst = (PUSHORT)(pDest);
    *pDstInst++ = 0xb401;

    // Locate a scratch register
    BYTE scrReg = 0;
    while (scrReg == tableBranch.IndexRegister) {
        ++scrReg;
    }

    // Push scrReg and tableBranch.IndexRegister (push.w doesn't support pushing just 1 register)
    DWORD pushInstruction = 0xe92d0000;
    pushInstruction |= 1 << scrReg;
    pushInstruction |= 1 << tableBranch.IndexRegister;
    EmitLongInstruction(pDstInst, pushInstruction);

    // Write the target address out to the 'literal pool';
    // when the base register of a TBB/TBH is PC,
    // the branch table immediately follows the instruction
    BYTE* pTarget = CalculateTarget(pSource, 0);
    *--((PUSHORT&)m_pbPool) = (USHORT)((size_t)pTarget & 0xffff);
    *--((PUSHORT&)m_pbPool) = (USHORT)((size_t)pTarget >> 16);

    // Load the literal pool value into our scratch register (this contains the address of the branch table)
    // ldr rn, target
    EmitLiteralLoad8(pDstInst, scrReg, m_pbPool);

    // Add the index offset to the address of the branch table; the result will be the value within the table that contains the branch offset
    // We need to multiply the index by two if we are using halfword indexing
    // Will shift tableBranch.IndexRegister by 1 (multiply by 2) if using a TBH
    EmitAdd32(pDstInst, scrReg, tableBranch.IndexRegister, scrReg, tableBranch.HalfWord);

    // Dereference rn into rn, to load the value within the table
    // ldr rn, [rn]
    if (scrReg < 0x7) {
        EmitImmediateRegisterLoad16(pDstInst, scrReg);
    }
    else {
        EmitImmediateRegisterLoad32(pDstInst, scrReg);
    }

    // Multiply the offset by two to get the true offset value (as per the spec)
    EmitLogicalShiftLeft32(pDstInst, scrReg, scrReg, 1);

    // Add the offset to PC to get the target
    EmitAdd32(pDstInst, scrReg, c_PC, scrReg, 0);

    // Now write the contents of scrReg to the stack, so we can pop it into PC
    // Write the address of the branch table entry to the stack, so we can pop it into PC
    EmitStoreImmediate12(pDstInst, scrReg, c_SP, sizeof(DWORD) * 3);

    // Pop scrReg and tableBranch.IndexRegister (pop.w doesn't support popping just 1 register)
    DWORD popInstruction = 0xe8bd0000;
    popInstruction |= 1 << scrReg;
    popInstruction |= 1 << tableBranch.IndexRegister;
    EmitLongInstruction(pDstInst, popInstruction);

    // Pop PC
    *pDstInst++ = 0xbd00;

    // Compute the extra space needed for the branch sequence
    m_lExtra = CalculateExtra(sizeof(USHORT), pDest, (BYTE*)(pDstInst));
    return sizeof(DWORD);
}

BYTE CDetourDis::BeginCopy32(BYTE* pSource, BYTE* pDest)
{
    ULONG instruction = GetLongInstruction(pSource);

    // Immediate data processing instructions; ADD, SUB, MOV, MOVN, ADR, MOVT, BFC, SSAT16, etc.
    if ((instruction & 0xF8008000) == 0xF0000000) { // 11110xxxxxxxxxxx0xxxxxxxxxxxxxxx
        // Should all be blitt-able
        // ToDo: What about ADR?  Is it safe to do a straight-copy?
        // ToDo: Not handling moves to or from PC
        return PureCopy32(pSource, pDest);
    }

    // Non-Immediate data processing instructions; ADD, EOR, TST, etc.
    if ((instruction & 0xEE000000) == 0xEA000000) { // 111x101xxxxxxxxxxxxxxxxxxxxxxx
        // Should all be blitt-able
        return PureCopy32(pSource, pDest);
    }

    // Load and store single data item, memory hints
    if ((instruction & 0xFE000000) == 0xF8000000) { // 1111100xxxxxxxxxxxxxxxxxxxxxxxxx
        return CopyLoadAndStoreSingle(pSource, pDest);
    }

    // Load and store, double and exclusive, and table branch
    if ((instruction & 0xFE400000) == 0xE8400000) { // 1110100xx1xxxxxxxxxxxxxxxxxxxxxx
        // Load and store double
        if (instruction & 0x1200000) {
            // LDRD, STRD (immediate) : xxxxxxxPxxWxxxxxxxxxxxxxxxxxxxxx where PW != 0b00
            // The source register is PC
            if ((instruction & 0xF0000) == 0xF0000) {
                // ToDo: If the source register is PC, what should we do?
                ASSERT(false);
            }

            // If either target registers are PC
            if (((instruction & 0xF000) == 0xF000) ||
                ((instruction & 0xF00) == 0xF00)) {
                m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
            }

            return PureCopy32(pSource, pDest);
        }

        // Load and store exclusive
        if (!(instruction & 0x800000)) { // LDREX, STREX : xxxxxxxx0xxxxxxxxxxxxxxxxxxxxxxx
            if ((instruction & 0xF000) == 0xF000) { // xxxxxxxxxxxx1111xxxxxxxxxxxx
                m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_DYNAMIC;
            }
            return PureCopy32(pSource, pDest);
        }

        // Table branch
        if ((instruction & 0x1000F0) == 0x100000 ||  // TBB : xxxxxxxxxxx1xxxxxxxxxxxx0000xxxx
            (instruction & 0x1000F0) == 0x100010) { // TBH : xxxxxxxxxxx1xxxxxxxxxxxx0001xxxx
            return CopyTableBranch(pSource, pDest);
        }

        // Load and store exclusive byte, halfword, doubleword (LDREXB, LDREXH, LDREXD, STREXB, STREXH, STREXD, etc.)
        return PureCopy32(pSource, pDest);
    }

    // Load and store multiple, RFE and SRS
    if ((instruction & 0xFE400000) == 0xE8000000) { // 1110100xx0xxxxxxxxxxxxxxxxxxxxxx
        // Return from exception (RFE)
        if ((instruction & 0xE9900000) == 0xE9900000 || // 1110100110x1xxxxxxxxxxxxxxxxxxxx
            (instruction & 0xE8100000) == 0xE8100000) { // 1110100000x1xxxxxxxxxxxxxxxxxxxx
            return PureCopy32(pSource, pDest);
        }

        return CopyLoadAndStoreMultipleAndSRS(pSource, pDest);
    }

    // Branches, miscellaneous control
    if ((instruction & 0xF8008000) == 0xF0008000) { // 11110xxxxxxxxxxx0xxxxxxxxxxxxxxx
        // Branches, miscellaneous control
        return CopyBranchOrMiscellaneous32(pSource, pDest);
    }

    // Coprocessor instructions
    if ((instruction & 0xEC000000) == 0xEC000000) { // 111x11xxxxxxxxxxxxxxxxxxxxxxxxxx
        return PureCopy32(pSource, pDest);
    }

    // Unhandled instruction; should never make it this far
    ASSERT(false);
    return PureCopy32(pSource, pDest);
}

/////////////////////////////////////////////////////////// Disassembler Code.
//
CDetourDis::CDetourDis()
{
    m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_NONE;
    m_pbPool = NULL;
    m_lExtra = 0;
}

PBYTE CDetourDis::CopyInstruction(PBYTE pDst,
                                  PBYTE *ppDstPool,
                                  PBYTE pSrc,
                                  PBYTE *ppTarget,
                                  LONG *plExtra)
{
    if (pDst && ppDstPool && ppDstPool != NULL) {
        m_pbPool = (PBYTE)*ppDstPool;
    }
    else {
        pDst = m_rbScratchDst;
        m_pbPool = m_rbScratchDst + sizeof(m_rbScratchDst);
    }
    // Make sure the constant pool is 32-bit aligned.
    m_pbPool -= ((ULONG_PTR)m_pbPool) & 3;

    REFCOPYENTRY pEntry = &s_rceCopyTable[pSrc[1] >> 3];
    ULONG size = (this->*pEntry->pfCopy)(pSrc, pDst);

    pSrc += size;

    // If the target is needed, store our target
    if (ppTarget) {
        *ppTarget = m_pbTarget;
    }
    if (plExtra) {
        *plExtra = m_lExtra;
    }
    if (ppDstPool) {
        *ppDstPool = m_pbPool;
    }

    return pSrc;
}


PVOID WINAPI DetourCopyInstruction(_In_opt_ PVOID pDst,
                                   _Inout_opt_ PVOID *ppDstPool,
                                   _In_ PVOID pSrc,
                                   _Out_opt_ PVOID *ppTarget,
                                   _Out_opt_ LONG *plExtra)
{
    CDetourDis state;
    return (PVOID)state.CopyInstruction((PBYTE)pDst,
                                        (PBYTE*)ppDstPool,
                                        (PBYTE)pSrc,
                                        (PBYTE*)ppTarget,
                                        plExtra);
}

#endif // DETOURS_ARM

#ifdef DETOURS_ARM64

#define c_LR        30          // The register number for the Link Register
#define c_SP        31          // The register number for the Stack Pointer
#define c_NOP       0xd503201f  // A nop instruction
#define c_BREAK     (0xd4200000 | (0xf000 << 5)) // A break instruction

//
// Problematic instructions:
//
// ADR     0ll10000 hhhhhhhh hhhhhhhh hhhddddd  & 0x9f000000 == 0x10000000  (l = low, h = high, d = Rd)
// ADRP    1ll10000 hhhhhhhh hhhhhhhh hhhddddd  & 0x9f000000 == 0x90000000  (l = low, h = high, d = Rd)
//
// B.cond  01010100 iiiiiiii iiiiiiii iii0cccc  & 0xff000010 == 0x54000000  (i = delta = SignExtend(imm19:00, 64), c = cond)
//
// B       000101ii iiiiiiii iiiiiiii iiiiiiii  & 0xfc000000 == 0x14000000  (i = delta = SignExtend(imm26:00, 64))
// BL      100101ii iiiiiiii iiiiiiii iiiiiiii  & 0xfc000000 == 0x94000000  (i = delta = SignExtend(imm26:00, 64))
//
// CBNZ    z0110101 iiiiiiii iiiiiiii iiittttt  & 0x7f000000 == 0x35000000  (z = size, i = delta = SignExtend(imm19:00, 64), t = Rt)
// CBZ     z0110100 iiiiiiii iiiiiiii iiittttt  & 0x7f000000 == 0x34000000  (z = size, i = delta = SignExtend(imm19:00, 64), t = Rt)
//
// LDR Wt  00011000 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x18000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDR Xt  01011000 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x58000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDRSW   10011000 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x98000000  (i = SignExtend(imm19:00, 64), t = Rt)
// PRFM    11011000 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0xd8000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDR St  00011100 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x1c000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDR Dt  01011100 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x5c000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDR Qt  10011100 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0x9c000000  (i = SignExtend(imm19:00, 64), t = Rt)
// LDR inv 11011100 iiiiiiii iiiiiiii iiittttt  & 0xff000000 == 0xdc000000  (i = SignExtend(imm19:00, 64), t = Rt)
//
// TBNZ    z0110111 bbbbbiii iiiiiiii iiittttt  & 0x7f000000 == 0x37000000  (z = size, b = bitnum, i = SignExtend(imm14:00, 64), t = Rt)
// TBZ     z0110110 bbbbbiii iiiiiiii iiittttt  & 0x7f000000 == 0x36000000  (z = size, b = bitnum, i = SignExtend(imm14:00, 64), t = Rt)
//

class CDetourDis
{
  public:
    CDetourDis();

    PBYTE   CopyInstruction(PBYTE pDst,
                            PBYTE pSrc,
                            PBYTE *ppTarget,
                            LONG *plExtra);

  public:
    typedef BYTE (CDetourDis::* COPYFUNC)(PBYTE pbDst, PBYTE pbSrc);

    union AddImm12
    {
        DWORD Assembled;
        struct
        {
            DWORD Rd : 5;           // Destination register
            DWORD Rn : 5;           // Source register
            DWORD Imm12 : 12;       // 12-bit immediate
            DWORD Shift : 2;        // shift (must be 0 or 1)
            DWORD Opcode1 : 7;      // Must be 0010001 == 0x11
            DWORD Size : 1;         // 0 = 32-bit, 1 = 64-bit
        } s;
        static DWORD Assemble(DWORD size, DWORD rd, DWORD rn, ULONG imm, DWORD shift)
        {
            AddImm12 temp;
            temp.s.Rd = rd;
            temp.s.Rn = rn;
            temp.s.Imm12 = imm & 0xfff;
            temp.s.Shift = shift;
            temp.s.Opcode1 = 0x11;
            temp.s.Size = size;
            return temp.Assembled;
        }
        static DWORD AssembleAdd32(DWORD rd, DWORD rn, ULONG imm, DWORD shift) { return Assemble(0, rd, rn, imm, shift); }
        static DWORD AssembleAdd64(DWORD rd, DWORD rn, ULONG imm, DWORD shift) { return Assemble(1, rd, rn, imm, shift); }
    };

    union Adr19
    {
        DWORD Assembled;
        struct
        {
            DWORD Rd : 5;           // Destination register
            DWORD Imm19 : 19;       // 19-bit upper immediate
            DWORD Opcode1 : 5;      // Must be 10000 == 0x10
            DWORD Imm2 : 2;         // 2-bit lower immediate
            DWORD Type : 1;         // 0 = ADR, 1 = ADRP
        } s;
        inline LONG Imm() const { DWORD Imm = (s.Imm19 << 2) | s.Imm2; return (LONG)(Imm << 11) >> 11; }
        static DWORD Assemble(DWORD type, DWORD rd, LONG delta)
        {
            Adr19 temp;
            temp.s.Rd = rd;
            temp.s.Imm19 = (delta >> 2) & 0x7ffff;
            temp.s.Opcode1 = 0x10;
            temp.s.Imm2 = delta & 3;
            temp.s.Type = type;
            return temp.Assembled;
        }
        static DWORD AssembleAdr(DWORD rd, LONG delta) { return Assemble(0, rd, delta); }
        static DWORD AssembleAdrp(DWORD rd, LONG delta) { return Assemble(1, rd, delta); }
    };

    union Bcc19
    {
        DWORD Assembled;
        struct
        {
            DWORD Condition : 4;    // Condition
            DWORD Opcode1 : 1;      // Must be 0
            DWORD Imm19 : 19;       // 19-bit immediate
            DWORD Opcode2 : 8;      // Must be 01010100 == 0x54
        } s;
        inline LONG Imm() const { return (LONG)(s.Imm19 << 13) >> 11; }
        static DWORD AssembleBcc(DWORD condition, LONG delta)
        {
            Bcc19 temp;
            temp.s.Condition = condition;
            temp.s.Opcode1 = 0;
            temp.s.Imm19 = delta >> 2;
            temp.s.Opcode2 = 0x54;
            return temp.Assembled;
        }
    };

    union Branch26
    {
        DWORD Assembled;
        struct
        {
            DWORD Imm26 : 26;       // 26-bit immediate
            DWORD Opcode1 : 5;      // Must be 00101 == 0x5
            DWORD Link : 1;         // 0 = B, 1 = BL
        } s;
        inline LONG Imm() const { return (LONG)(s.Imm26 << 6) >> 4; }
        static DWORD Assemble(DWORD link, LONG delta)
        {
            Branch26 temp;
            temp.s.Imm26 = delta >> 2;
            temp.s.Opcode1 = 0x5;
            temp.s.Link = link;
            return temp.Assembled;
        }
        static DWORD AssembleB(LONG delta) { return Assemble(0, delta); }
        static DWORD AssembleBl(LONG delta) { return Assemble(1, delta); }
    };

    union Br
    {
        DWORD Assembled;
        struct
        {
            DWORD Opcode1 : 5;      // Must be 00000 == 0
            DWORD Rn : 5;           // Register number
            DWORD Opcode2 : 22;     // Must be 1101011000011111000000 == 0x3587c0 for Br
                                    //                                   0x358fc0 for Brl
        } s;
        static DWORD Assemble(DWORD rn, bool link)
        {
            Br temp;
            temp.s.Opcode1 = 0;
            temp.s.Rn = rn;
            temp.s.Opcode2 = 0x3587c0;
            if (link)
                temp.Assembled |= 0x00200000;
            return temp.Assembled;
        }
        static DWORD AssembleBr(DWORD rn)
        {
            return Assemble(rn, false);
        }
        static DWORD AssembleBrl(DWORD rn)
        {
            return Assemble(rn, true);
        }
    };

    union Cbz19
    {
        DWORD Assembled;
        struct
        {
            DWORD Rt : 5;           // Register to test
            DWORD Imm19 : 19;       // 19-bit immediate
            DWORD Nz : 1;           // 0 = CBZ, 1 = CBNZ
            DWORD Opcode1 : 6;      // Must be 011010 == 0x1a
            DWORD Size : 1;         // 0 = 32-bit, 1 = 64-bit
        } s;
        inline LONG Imm() const { return (LONG)(s.Imm19 << 13) >> 11; }
        static DWORD Assemble(DWORD size, DWORD nz, DWORD rt, LONG delta)
        {
            Cbz19 temp;
            temp.s.Rt = rt;
            temp.s.Imm19 = delta >> 2;
            temp.s.Nz = nz;
            temp.s.Opcode1 = 0x1a;
            temp.s.Size = size;
            return temp.Assembled;
        }
    };

    union LdrLit19
    {
        DWORD Assembled;
        struct
        {
            DWORD Rt : 5;           // Destination register
            DWORD Imm19 : 19;       // 19-bit immediate
            DWORD Opcode1 : 2;      // Must be 0
            DWORD FpNeon : 1;       // 0 = LDR Wt/LDR Xt/LDRSW/PRFM, 1 = LDR St/LDR Dt/LDR Qt
            DWORD Opcode2 : 3;      // Must be 011 = 3
            DWORD Size : 2;         // 00 = LDR Wt/LDR St, 01 = LDR Xt/LDR Dt, 10 = LDRSW/LDR Qt, 11 = PRFM/invalid
        } s;
        inline LONG Imm() const { return (LONG)(s.Imm19 << 13) >> 11; }
        static DWORD Assemble(DWORD size, DWORD fpneon, DWORD rt, LONG delta)
        {
            LdrLit19 temp;
            temp.s.Rt = rt;
            temp.s.Imm19 = delta >> 2;
            temp.s.Opcode1 = 0;
            temp.s.FpNeon = fpneon;
            temp.s.Opcode2 = 3;
            temp.s.Size = size;
            return temp.Assembled;
        }
    };

    union LdrFpNeonImm9
    {
        DWORD Assembled;
        struct
        {
            DWORD Rt : 5;           // Destination register
            DWORD Rn : 5;           // Base register
            DWORD Imm12 : 12;       // 12-bit immediate
            DWORD Opcode1 : 1;      // Must be 1 == 1
            DWORD Opc : 1;          // Part of size
            DWORD Opcode2 : 6;      // Must be 111101 == 0x3d
            DWORD Size : 2;         // Size (0=8-bit, 1=16-bit, 2=32-bit, 3=64-bit, 4=128-bit)
        } s;
        static DWORD Assemble(DWORD size, DWORD rt, DWORD rn, ULONG imm)
        {
            LdrFpNeonImm9 temp;
            temp.s.Rt = rt;
            temp.s.Rn = rn;
            temp.s.Imm12 = imm;
            temp.s.Opcode1 = 1;
            temp.s.Opc = size >> 2;
            temp.s.Opcode2 = 0x3d;
            temp.s.Size = size & 3;
            return temp.Assembled;
        }
    };

    union Mov16
    {
        DWORD Assembled;
        struct
        {
            DWORD Rd : 5;           // Destination register
            DWORD Imm16 : 16;       // Immediate
            DWORD Shift : 2;        // Shift amount (0=0, 1=16, 2=32, 3=48)
            DWORD Opcode : 6;       // Must be 100101 == 0x25
            DWORD Type : 2;         // 0 = MOVN, 1 = reserved, 2 = MOVZ, 3 = MOVK
            DWORD Size : 1;         // 0 = 32-bit, 1 = 64-bit
        } s;
        static DWORD Assemble(DWORD size, DWORD type, DWORD rd, DWORD imm, DWORD shift)
        {
            Mov16 temp;
            temp.s.Rd = rd;
            temp.s.Imm16 = imm;
            temp.s.Shift = shift;
            temp.s.Opcode = 0x25;
            temp.s.Type = type;
            temp.s.Size = size;
            return temp.Assembled;
        }
        static DWORD AssembleMovn32(DWORD rd, DWORD imm, DWORD shift) { return Assemble(0, 0, rd, imm, shift); }
        static DWORD AssembleMovn64(DWORD rd, DWORD imm, DWORD shift) { return Assemble(1, 0, rd, imm, shift); }
        static DWORD AssembleMovz32(DWORD rd, DWORD imm, DWORD shift) { return Assemble(0, 2, rd, imm, shift); }
        static DWORD AssembleMovz64(DWORD rd, DWORD imm, DWORD shift) { return Assemble(1, 2, rd, imm, shift); }
        static DWORD AssembleMovk32(DWORD rd, DWORD imm, DWORD shift) { return Assemble(0, 3, rd, imm, shift); }
        static DWORD AssembleMovk64(DWORD rd, DWORD imm, DWORD shift) { return Assemble(1, 3, rd, imm, shift); }
    };

    union Tbz14
    {
        DWORD Assembled;
        struct
        {
            DWORD Rt : 5;           // Register to test
            DWORD Imm14 : 14;       // 14-bit immediate
            DWORD Bit : 5;          // 5-bit index
            DWORD Nz : 1;           // 0 = TBZ, 1 = TBNZ
            DWORD Opcode1 : 6;      // Must be 011011 == 0x1b
            DWORD Size : 1;         // 0 = 32-bit, 1 = 64-bit
        } s;
        inline LONG Imm() const { return (LONG)(s.Imm14 << 18) >> 16; }
        static DWORD Assemble(DWORD size, DWORD nz, DWORD rt, DWORD bit, LONG delta)
        {
            Tbz14 temp;
            temp.s.Rt = rt;
            temp.s.Imm14 = delta >> 2;
            temp.s.Bit = bit;
            temp.s.Nz = nz;
            temp.s.Opcode1 = 0x1b;
            temp.s.Size = size;
            return temp.Assembled;
        }
    };


  protected:
    BYTE    PureCopy32(BYTE* pSource, BYTE* pDest);
    BYTE    EmitMovImmediate(PULONG& pDstInst, BYTE rd, UINT64 immediate);
    BYTE    CopyAdr(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyBcc(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyB(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyBl(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyB_or_Bl(BYTE* pSource, BYTE* pDest, ULONG instruction, bool link);
    BYTE    CopyCbz(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyTbz(BYTE* pSource, BYTE* pDest, ULONG instruction);
    BYTE    CopyLdrLiteral(BYTE* pSource, BYTE* pDest, ULONG instruction);

  protected:
    ULONG GetInstruction(BYTE* pSource)
    {
        return ((PULONG)pSource)[0];
    }

    BYTE EmitInstruction(PULONG& pDstInst, ULONG instruction)
    {
        *pDstInst++ = instruction;
        return sizeof(ULONG);
    }

  protected:
    PBYTE   m_pbTarget;
    BYTE    m_rbScratchDst[128]; // matches or exceeds rbCode
};

BYTE CDetourDis::PureCopy32(BYTE* pSource, BYTE* pDest)
{
    *(ULONG *)pDest = *(ULONG*)pSource;
    return sizeof(DWORD);
}

/////////////////////////////////////////////////////////// Disassembler Code.
//
CDetourDis::CDetourDis()
{
    m_pbTarget = (PBYTE)DETOUR_INSTRUCTION_TARGET_NONE;
}

PBYTE CDetourDis::CopyInstruction(PBYTE pDst,
                                  PBYTE pSrc,
                                  PBYTE *ppTarget,
                                  LONG *plExtra)
{
    if (pDst == NULL) {
        pDst = m_rbScratchDst;
    }

    DWORD Instruction = GetInstruction(pSrc);

    ULONG CopiedSize;
    if ((Instruction & 0x1f000000) == 0x10000000) {
        CopiedSize = CopyAdr(pSrc, pDst, Instruction);
    } else if ((Instruction & 0xff000010) == 0x54000000) {
        CopiedSize = CopyBcc(pSrc, pDst, Instruction);
    } else if ((Instruction & 0x7c000000) == 0x14000000) {
        CopiedSize = CopyB_or_Bl(pSrc, pDst, Instruction, (Instruction & 0x80000000) != 0);
    } else if ((Instruction & 0x7e000000) == 0x34000000) {
        CopiedSize = CopyCbz(pSrc, pDst, Instruction);
    } else if ((Instruction & 0x7e000000) == 0x36000000) {
        CopiedSize = CopyTbz(pSrc, pDst, Instruction);
    } else if ((Instruction & 0x3b000000) == 0x18000000) {
        CopiedSize = CopyLdrLiteral(pSrc, pDst, Instruction);
    } else {
        CopiedSize = PureCopy32(pSrc, pDst);
    }

    // If the target is needed, store our target
    if (ppTarget) {
        *ppTarget = m_pbTarget;
    }
    if (plExtra) {
        *plExtra = CopiedSize - sizeof(DWORD);
    }

    return pSrc + 4;
}

BYTE CDetourDis::EmitMovImmediate(PULONG& pDstInst, BYTE rd, UINT64 immediate)
{
    DWORD piece[4];
    piece[3] = (DWORD)((immediate >> 48) & 0xffff);
    piece[2] = (DWORD)((immediate >> 32) & 0xffff);
    piece[1] = (DWORD)((immediate >> 16) & 0xffff);
    piece[0] = (DWORD)((immediate >> 0) & 0xffff);
    int count = 0;

    // special case: MOVN with 32-bit dest
    if (piece[3] == 0 && piece[2] == 0 && piece[1] == 0xffff)
    {
        EmitInstruction(pDstInst, Mov16::AssembleMovn32(rd, piece[0] ^ 0xffff, 0));
        count++;
    }

    // MOVN/MOVZ with 64-bit dest
    else
    {
        int zero_pieces = (piece[3] == 0x0000) + (piece[2] == 0x0000) + (piece[1] == 0x0000) + (piece[0] == 0x0000);
        int ffff_pieces = (piece[3] == 0xffff) + (piece[2] == 0xffff) + (piece[1] == 0xffff) + (piece[0] == 0xffff);
        DWORD defaultPiece = (ffff_pieces > zero_pieces) ? 0xffff : 0x0000;
        bool first = true;
        for (int pieceNum = 3; pieceNum >= 0; pieceNum--)
        {
            DWORD curPiece = piece[pieceNum];
            if (curPiece != defaultPiece || (pieceNum == 0 && first))
            {
                count++;
                if (first)
                {
                    if (defaultPiece == 0xffff)
                    {
                        EmitInstruction(pDstInst, Mov16::AssembleMovn64(rd, curPiece ^ 0xffff, pieceNum));
                    }
                    else
                    {
                        EmitInstruction(pDstInst, Mov16::AssembleMovz64(rd, curPiece, pieceNum));
                    }
                    first = false;
                }
                else
                {
                    EmitInstruction(pDstInst, Mov16::AssembleMovk64(rd, curPiece, pieceNum));
                }
            }
        }
    }
    return (BYTE)(count * sizeof(DWORD));
}

BYTE CDetourDis::CopyAdr(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    Adr19& decoded = (Adr19&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    // ADR case
    if (decoded.s.Type == 0)
    {
        BYTE* pTarget = pSource + decoded.Imm();
        LONG64 delta = pTarget - pDest;
        LONG64 deltaPage = ((ULONG_PTR)pTarget >> 12) - ((ULONG_PTR)pDest >> 12);

        // output as ADR
        if (delta >= -(1 << 20) && delta < (1 << 20))
        {
            EmitInstruction(pDstInst, Adr19::AssembleAdr(decoded.s.Rd, (LONG)delta));
        }

        // output as ADRP; ADD
        else if (deltaPage >= -(1 << 20) && (deltaPage < (1 << 20)))
        {
            EmitInstruction(pDstInst, Adr19::AssembleAdrp(decoded.s.Rd, (LONG)deltaPage));
            EmitInstruction(pDstInst, AddImm12::AssembleAdd32(decoded.s.Rd, decoded.s.Rd, ((ULONG)(ULONG_PTR)pTarget) & 0xfff, 0));
        }

        // output as immediate move
        else
        {
            EmitMovImmediate(pDstInst, decoded.s.Rd, (ULONG_PTR)pTarget);
        }
    }

    // ADRP case
    else
    {
        BYTE* pTarget = (BYTE*)((((ULONG_PTR)pSource >> 12) + decoded.Imm()) << 12);
        LONG64 deltaPage = ((ULONG_PTR)pTarget >> 12) - ((ULONG_PTR)pDest >> 12);

        // output as ADRP
        if (deltaPage >= -(1 << 20) && (deltaPage < (1 << 20)))
        {
            EmitInstruction(pDstInst, Adr19::AssembleAdrp(decoded.s.Rd, (LONG)deltaPage));
        }

        // output as immediate move
        else
        {
            EmitMovImmediate(pDstInst, decoded.s.Rd, (ULONG_PTR)pTarget);
        }
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}

BYTE CDetourDis::CopyBcc(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    Bcc19& decoded = (Bcc19&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    BYTE* pTarget = pSource + decoded.Imm();
    m_pbTarget = pTarget;
    LONG64 delta = pTarget - pDest;
    LONG64 delta4 = pTarget - (pDest + 4);

    // output as BCC
    if (delta >= -(1 << 20) && delta < (1 << 20))
    {
        EmitInstruction(pDstInst, Bcc19::AssembleBcc(decoded.s.Condition, (LONG)delta));
    }

    // output as BCC <skip>; B
    else if (delta4 >= -(1 << 27) && (delta4 < (1 << 27)))
    {
        EmitInstruction(pDstInst, Bcc19::AssembleBcc(decoded.s.Condition ^ 1, 8));
        EmitInstruction(pDstInst, Branch26::AssembleB((LONG)delta4));
    }

    // output as MOV x17, Target; BCC <skip>; BR x17 (BIG assumption that x17 isn't being used for anything!!)
    else
    {
        EmitMovImmediate(pDstInst, 17, (ULONG_PTR)pTarget);
        EmitInstruction(pDstInst, Bcc19::AssembleBcc(decoded.s.Condition ^ 1, 8));
        EmitInstruction(pDstInst, Br::AssembleBr(17));
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}

BYTE CDetourDis::CopyB_or_Bl(BYTE* pSource, BYTE* pDest, ULONG instruction, bool link)
{
    Branch26& decoded = (Branch26&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    BYTE* pTarget = pSource + decoded.Imm();
    m_pbTarget = pTarget;
    LONG64 delta = pTarget - pDest;

    // output as B or BRL
    if (delta >= -(1 << 27) && (delta < (1 << 27)))
    {
        EmitInstruction(pDstInst, Branch26::Assemble(link, (LONG)delta));
    }

    // output as MOV x17, Target; BR or BRL x17 (BIG assumption that x17 isn't being used for anything!!)
    else
    {
        EmitMovImmediate(pDstInst, 17, (ULONG_PTR)pTarget);
        EmitInstruction(pDstInst, Br::Assemble(17, link));
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}

BYTE CDetourDis::CopyB(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    return CopyB_or_Bl(pSource, pDest, instruction, false);
}

BYTE CDetourDis::CopyBl(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    return CopyB_or_Bl(pSource, pDest, instruction, true);
}

BYTE CDetourDis::CopyCbz(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    Cbz19& decoded = (Cbz19&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    BYTE* pTarget = pSource + decoded.Imm();
    m_pbTarget = pTarget;
    LONG64 delta = pTarget - pDest;
    LONG64 delta4 = pTarget - (pDest + 4);

    // output as CBZ/NZ
    if (delta >= -(1 << 20) && delta < (1 << 20))
    {
        EmitInstruction(pDstInst, Cbz19::Assemble(decoded.s.Size, decoded.s.Nz, decoded.s.Rt, (LONG)delta));
    }

    // output as CBNZ/Z <skip>; B
    else if (delta4 >= -(1 << 27) && (delta4 < (1 << 27)))
    {
        EmitInstruction(pDstInst, Cbz19::Assemble(decoded.s.Size, decoded.s.Nz ^ 1, decoded.s.Rt, 8));
        EmitInstruction(pDstInst, Branch26::AssembleB((LONG)delta4));
    }

    // output as MOV x17, Target; CBNZ/Z <skip>; BR x17 (BIG assumption that x17 isn't being used for anything!!)
    else
    {
        EmitMovImmediate(pDstInst, 17, (ULONG_PTR)pTarget);
        EmitInstruction(pDstInst, Cbz19::Assemble(decoded.s.Size, decoded.s.Nz ^ 1, decoded.s.Rt, 8));
        EmitInstruction(pDstInst, Br::AssembleBr(17));
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}

BYTE CDetourDis::CopyTbz(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    Tbz14& decoded = (Tbz14&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    BYTE* pTarget = pSource + decoded.Imm();
    m_pbTarget = pTarget;
    LONG64 delta = pTarget - pDest;
    LONG64 delta4 = pTarget - (pDest + 4);

    // output as TBZ/NZ
    if (delta >= -(1 << 13) && delta < (1 << 13))
    {
        EmitInstruction(pDstInst, Tbz14::Assemble(decoded.s.Size, decoded.s.Nz, decoded.s.Rt, decoded.s.Bit, (LONG)delta));
    }

    // output as TBNZ/Z <skip>; B
    else if (delta4 >= -(1 << 27) && (delta4 < (1 << 27)))
    {
        EmitInstruction(pDstInst, Tbz14::Assemble(decoded.s.Size, decoded.s.Nz ^ 1, decoded.s.Rt, decoded.s.Bit, 8));
        EmitInstruction(pDstInst, Branch26::AssembleB((LONG)delta4));
    }

    // output as MOV x17, Target; TBNZ/Z <skip>; BR x17 (BIG assumption that x17 isn't being used for anything!!)
    else
    {
        EmitMovImmediate(pDstInst, 17, (ULONG_PTR)pTarget);
        EmitInstruction(pDstInst, Tbz14::Assemble(decoded.s.Size, decoded.s.Nz ^ 1, decoded.s.Rt, decoded.s.Bit, 8));
        EmitInstruction(pDstInst, Br::AssembleBr(17));
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}

BYTE CDetourDis::CopyLdrLiteral(BYTE* pSource, BYTE* pDest, ULONG instruction)
{
    LdrLit19& decoded = (LdrLit19&)(instruction);
    PULONG pDstInst = (PULONG)(pDest);

    BYTE* pTarget = pSource + decoded.Imm();
    LONG64 delta = pTarget - pDest;

    // output as LDR
    if (delta >= -(1 << 21) && delta < (1 << 21))
    {
        EmitInstruction(pDstInst, LdrLit19::Assemble(decoded.s.Size, decoded.s.FpNeon, decoded.s.Rt, (LONG)delta));
    }

    // output as move immediate
    else if (decoded.s.FpNeon == 0)
    {
        UINT64 value = 0;
        switch (decoded.s.Size)
        {
            case 0: value = *(ULONG*)pTarget;       break;
            case 1: value = *(UINT64*)pTarget;   break;
            case 2: value = *(LONG*)pTarget;        break;
        }
        EmitMovImmediate(pDstInst, decoded.s.Rt, value);
    }

    // FP/NEON register: compute address in x17 and load from there (BIG assumption that x17 isn't being used for anything!!)
    else
    {
        EmitMovImmediate(pDstInst, 17, (ULONG_PTR)pTarget);
        EmitInstruction(pDstInst, LdrFpNeonImm9::Assemble(2 + decoded.s.Size, decoded.s.Rt, 17, 0));
    }

    return (BYTE)((BYTE*)pDstInst - pDest);
}


PVOID WINAPI DetourCopyInstruction(_In_opt_ PVOID pDst,
                                   _Inout_opt_ PVOID *ppDstPool,
                                   _In_ PVOID pSrc,
                                   _Out_opt_ PVOID *ppTarget,
                                   _Out_opt_ LONG *plExtra)
{
    UNREFERENCED_PARAMETER(ppDstPool);

    CDetourDis state;
    return (PVOID)state.CopyInstruction((PBYTE)pDst,
                                        (PBYTE)pSrc,
                                        (PBYTE*)ppTarget,
                                        plExtra);
}

#endif // DETOURS_ARM64

BOOL WINAPI DetourSetCodeModule(_In_ HMODULE hModule,
                                _In_ BOOL fLimitReferencesToModule)
{
#if defined(DETOURS_X64) || defined(DETOURS_X86)
    PBYTE pbBeg = NULL;
    PBYTE pbEnd = (PBYTE)~(ULONG_PTR)0;

    if (hModule != NULL) {
        ULONG cbModule = DetourGetModuleSize(hModule);

        pbBeg = (PBYTE)hModule;
        pbEnd = (PBYTE)hModule + cbModule;
    }

    return CDetourDis::SetCodeModule(pbBeg, pbEnd, fLimitReferencesToModule);
#elif defined(DETOURS_ARM) || defined(DETOURS_ARM64) || defined(DETOURS_IA64)
    (void)hModule;
    (void)fLimitReferencesToModule;
    return TRUE;
#else
#error unknown architecture (x86, x64, arm, arm64, ia64)
#endif
}

//
///////////////////////////////////////////////////////////////// End of File.
