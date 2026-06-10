// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

// Icon file structure and definitions:
// https://msdn.microsoft.com/en-us/library/ms997538.aspx

// .ico icon structures

// Icon entry struct
typedef struct
{
    BYTE        bWidth;         // Width, in pixels, of the image
    BYTE        bHeight;        // Height, in pixels, of the image
    BYTE        bColorCount;    // Number of colors in image (0 if >=8bpp)
    BYTE        bReserved;      // Reserved ( must be 0)
    WORD        wPlanes;        // Color Planes
    WORD        wBitCount;      // Bits per pixel
    DWORD       dwBytesInRes;    // How many bytes in this resource?
    DWORD       dwImageOffset;  // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;


// Icon directory struct
typedef struct
{
    WORD            idReserved;   // Reserved (must be 0)
    WORD            idType;       // Resource Type (1 for icons)
    WORD            idCount;      // How many images?
    ICONDIRENTRY    idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;


// Image struct
typedef struct
{
    BITMAPINFOHEADER    icHeader;      // DIB header
    RGBQUAD             icColors[1];   // Color table
    BYTE                icXOR[1];      // DIB bits for XOR mask
    BYTE                icAND[1];      // DIB bits for AND mask
} ICONIMAGE, * LPICONIMAGE;


// .exe and .dll icon structures

// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
#pragma pack( push )
#pragma pack( 2 )

typedef struct
{
    BYTE   bWidth;               // Width, in pixels, of the image
    BYTE   bHeight;              // Height, in pixels, of the image
    BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE   bReserved;            // Reserved
    WORD   wPlanes;              // Color Planes
    WORD   wBitCount;            // Bits per pixel
    DWORD  dwBytesInRes;         // how many bytes in this resource?
    WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;


typedef struct
{
    WORD            idReserved;   // Reserved (must be 0)
    WORD            idType;       // Resource type (1 for icons)
    WORD            idCount;      // How many images?
    GRPICONDIRENTRY idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;

#pragma pack( pop )
