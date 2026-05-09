/************************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                            *
 * IOwnerDataCallback_guid.c: GUID for IOwnerDataCallback interface. (undocumented) *
 *                                                                                  *
 * Based on the Undocumented List View Features tutorial on CodeProject:            *
 * https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features       *
 ************************************************************************************/

#include "RpWin32_sdk.h"
#include <unknwn.h>

extern const IID IID_IOwnerDataCallback;
const IID IID_IOwnerDataCallback = {0x44C09D56, 0x8D3B, 0x419D, {0xA4, 0x62, 0x7B, 0x95, 0x6B, 0x10, 0x5B, 0x47}};
