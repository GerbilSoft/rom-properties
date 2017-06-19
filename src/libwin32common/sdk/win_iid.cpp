/******************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                      *
 * win_iid.cpp: IIDs for undocumented ListView interfaces.                    *
 *                                                                            *
 * Based on the Undocumented List View Features tutorial on CodeProject:      *
 * https://www.codeproject.com/Articles/35197/Undocumented-List-View-Features *
 ******************************************************************************/

#include "IListView.hpp"
#include "IOwnerDataCallback.hpp"

extern "C" {

// IListView
const IID IID_IListView_WinVista = {0x2FFE2979, 0x5928, 0x4386, {0x9C, 0xDB, 0x8E, 0x1F, 0x15, 0xB7, 0x2F, 0xB4}};
const IID IID_IListView_Win7 = {0xE5B16AF2, 0x3990, 0x4681, {0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69}};

// IOwnerDataCallback
const IID IID_IOwnerDataCallback = {0x44C09D56, 0x8D3B, 0x419D, {0xA4, 0x62, 0x7B, 0x95, 0x6B, 0x10, 0x5B, 0x47}};

}
