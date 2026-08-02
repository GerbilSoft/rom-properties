/***************************************************************************
 * ROM Properties Page shell extension. (librpcpuid)                         *
 * cpu_dispatch.h: CPU dispatch macros.                                    *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// Check for certain CPUs.
// Reference: https://sourceforge.net/p/predef/wiki/Architectures/
#if defined(__i386__) || defined(__i386) || defined(_M_IX86)
#  define RP_CPU_I386 1
#  define RP_CPU_ARCH_NAME "i386"
#elif defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64) || defined(_M_AMD64)
#  if !defined(_M_ARM64EC)
#    define RP_CPU_AMD64 1
#    define RP_CPU_ARCH_NAME "amd64"
#  endif
#elif defined(__arm__) || defined(__thumb__) || defined(__arm) || defined(_ARM) || defined(_M_ARM)
#  define RP_CPU_ARM 1
#  define RP_CPU_ARCH_NAME "arm"
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define RP_CPU_ARM64 1
#  define RP_CPU_ARCH_NAME "arm64"
#elif defined(_M_ARM64EC)
#  define RP_CPU_ARM64 1
#  define RP_CPU_ARM64EC 1
#  define RP_CPU_ARCH_NAME "arm64ec"
#elif defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || \
    defined(_ARCH_PPC64) || defined(_XENON)
#  define RP_CPU_PPC64 1
#  define RP_CPU_ARCH_NAME "ppc64"
#elif defined(__powerpc) || defined(__powerpc__) || defined(__ppc__) || \
      defined(__PPC__) || defined(_ARCH_PPC) || defined(_M_PPC) || \
      defined(__PPCGECKO__) || defined(__PPCBROADWAY__) || defined(__ppc)
#  define RP_CPU_PPC
#  define RP_CPU_ARCH_NAME "ppc"
#elif defined(__riscv) || defined(__riscvel) || defined(__RISCVEL) || defined(__RISCVEL__)
// NOTE: Not differentiating between 32-bit and 64-bit for RISC-V yet...
#  define RP_CPU_RISCV
#  define RP_CPU_ARCH_NAME "riscv"
#elif defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
#  define RP_CPU_WASM32
#  define RP_CPU_ARCH_NAME "wasm32"
#endif
