/**
 * Undocumented Win32 APIs for Dark Mode functionality.
 *
 * Based on ysc3839's win32-darkmode example application:
 * https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/ListViewUtil.h
 * Copyright (c) 2019 Richard Yu
 * SPDX-License-Identifier: MIT
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void DarkMode_InitListView(HWND hListView);

#ifdef __cplusplus
}
#endif
