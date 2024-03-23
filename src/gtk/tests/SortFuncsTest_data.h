/***************************************************************************
 * ROM Properties Page shell extension. (gtk/tests)                        *
 * SortFuncsTest_data.h: Data for SortFuncsTest                            *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Sorted strings
static const char *const sorted_strings_asc[4][24] = {
	// Column 0: Greek alphabet, standard sort
	{"Alpha", "Epsilon", "Eta", "Gamma",
	 "Iota", "Lambda", "Nu", "Omicron",
	 "Phi", "Psi", "Rho", "Tau",
	 "bEta", "cHi", "dElta", "kAppa",
	 "mU", "oMega", "pI", "sIgma",
	 "tHeta", "uPsilon", "xI", "zEta"},
	// Column 1: Greek alphabet, case-insensitive sort
	{"Alpha", "bEta", "cHi", "dElta",
	 "Epsilon", "Eta", "Gamma", "Iota",
	 "kAppa", "Lambda", "mU", "Nu",
	 "oMega", "Omicron", "Phi", "pI",
	 "Psi", "Rho", "sIgma", "Tau",
	 "tHeta", "uPsilon", "xI", "zEta"},
	// Column 2: Numbers, standard sort
	{"1", "10", "11", "12",
	 "13", "14", "15", "16",
	 "17", "18", "19", "2",
	 "20", "21", "22", "23",
	 "24", "3", "4", "5",
	 "6", "7", "8", "9"},
	// Column 3: Numbers, numeric sort
	{"1", "2", "3", "4",
	 "5", "6", "7", "8",
	 "9", "10", "11", "12",
	 "13", "14", "15", "16",
	 "17", "18", "19", "20",
	 "21", "22", "23", "24"}
};

// "Randomized" list data.
// NOTE: Outer vector is rows, not columns!
static const char *const list_data_randomized[24][4] = {
	{"pI", "tHeta", "2", "7"},
	{"cHi", "Iota", "15", "1"},
	{"uPsilon", "Alpha", "1", "22"},
	{"Psi", "mU", "14", "15"},
	{"xI", "Nu", "20", "16"},
	{"Gamma", "Phi", "17", "12"},
	{"Epsilon", "Rho", "11", "23"},
	{"zEta", "pI", "5", "8"},
	{"Lambda", "Eta", "8", "5"},
	{"Nu", "bEta", "18", "19"},
	{"Iota", "Tau", "10", "13"},
	{"Eta", "Lambda", "13", "20"},
	{"kAppa", "Psi", "23", "9"},
	{"Omicron", "Gamma", "4", "18"},
	{"tHeta", "sIgma", "7", "4"},
	{"dElta", "zEta", "3", "21"},
	{"sIgma", "Omicron", "21", "14"},
	{"mU", "oMega", "6", "24"},
	{"bEta", "Epsilon", "24", "11"},
	{"oMega", "cHi", "16", "6"},
	{"Tau", "xI", "19", "17"},
	{"Alpha", "uPsilon", "22", "2"},
	{"Phi", "dElta", "12", "10"},
	{"Rho", "kAppa", "9", "3"}
};

#ifdef __cplusplus
}
#endif
