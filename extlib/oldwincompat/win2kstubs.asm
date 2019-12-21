; Stubs for functions introduced on Windows XP SP3.
; References:
; - https://stackoverflow.com/questions/19516796/visual-studio-2012-win32-project-targeting-windows-2000/53548116
; - https://stackoverflow.com/a/53548116

.386
.MODEL flat, stdcall

USE_W_STRINGS	EQU	1		; Flag on which GetModuleHandle version to use
USE_WIN9X	EQU	1		; Flag on if to include Win9x specific requirements

EXTRN STDCALL ImplementGetModuleHandleExW@12	: PROC
EXTRN STDCALL ImplementSetFilePointerEx@20	: PROC

;; Declare functions that we will call statically
IF USE_W_STRINGS
	EXTRN STDCALL _imp__GetModuleHandleW@4	: DWORD
ELSE
	EXTRN STDCALL _imp__GetModuleHandleA@4	: DWORD
ENDIF

EXTRN STDCALL _imp__GetProcAddress@8	: DWORD


.DATA
	;; Override the import symbols from kernel32.dll
	__imp__InitializeSListHead@4	DWORD DownlevelInitializeSListHead
	__imp__GetModuleHandleExW@12	DWORD DownlevelGetModuleHandleExW
	__imp__EncodePointer@4		DWORD DownlevelEncodeDecodePointer
	__imp__DecodePointer@4		DWORD DownlevelEncodeDecodePointer
	;__imp__HeapSetInformation@16	DWORD DownlevelHeapSetInformation
	__imp__SetFilePointerEx@20	DWORD ImplementSetFilePointerEx@20

	EXTERNDEF STDCALL __imp__InitializeSListHead@4	: DWORD
	EXTERNDEF STDCALL __imp__GetModuleHandleExW@12	: DWORD
	EXTERNDEF STDCALL __imp__EncodePointer@4	: DWORD
	EXTERNDEF STDCALL __imp__DecodePointer@4	: DWORD
	;EXTERNDEF STDCALL __imp__HeapSetInformation@16	: DWORD
	EXTERNDEF STDCALL __imp__SetFilePointerEx@20	: DWORD


	; For Win9x support - need to change return value to TRUE for the crt startup
	IF USE_WIN9X
		__imp__InitializeCriticalSectionAndSpinCount@8 DWORD DownlevelInitializeCriticalSectionAndSpinCount
		EXTERNDEF STDCALL __imp__InitializeCriticalSectionAndSpinCount@8 : DWORD
	ENDIF

CONST SEGMENT
	IF USE_W_STRINGS
		kszKernel32	DB 'k', 00H, 'e', 00H, 'r', 00H, 'n', 00H, 'e', 00H, 'l', 00H, '3', 00H, '2', 00H, 00H, 00H
		kszAdvApi32	DB 'a', 00H, 'd', 00H, 'v', 00H, 'a', 00H, 'p', 00H, 'i', 00H, '3', 00H, '2', 00H, 00H, 00H
	ELSE
		kszKernel32	DB "kernel32", 00H
		kszAdvApi32	DB "advapi32", 00H
	ENDIF

	kszInitializeSListHead	DB "InitializeSListHead", 00H
	kszGetModuleHandleExW	DB "GetModuleHandleExW", 00H

	kszInitializeCriticalSectionAndSpinCount DB "InitializeCriticalSectionAndSpinCount", 00H
	kszGetVersion DB "GetVersion", 00H

	; Windows XP and Server 2003 and later have RtlGenRandom, which is exported as SystemFunction036.
	; If needed, we could fall back to CryptGenRandom(), but that will be much slower
	; because it has to drag in the entire crypto API.
	; (See also: https://blogs.msdn.microsoft.com/michael_howard/2005/01/14/cryptographically-secure-random-number-on-windows-without-using-cryptoapi/)
	;kszSystemFunction036	DB "SystemFunction036", 00H
CONST ENDS

.CODE

; C++ translation:
; extern "C" VOID WINAPI DownlevelInitializeSListHead(PSLIST_HEADER pHead)
; {
;	const HMODULE hmodKernel32 = ::GetModuleHandleW(L"kernel32");
;	typedef decltype(InitializeSListHead)* pfnInitializeSListHead;
;	const pfnInitializeSListHead pfn = reinterpret_cast<pfnInitializeSListHead>(::GetProcAddress(hmodKernel32, "InitializeSListHead"));
;	if (pfn)
;	{
;		// call WinAPI function
;		pfn(pHead);
;	}
;	else
;	{
;		// fallback implementation for downlevel
;		pHead->Alignment = 0;
;	}
; }

DownlevelInitializeSListHead PROC
	;; Get a handle to the DLL containing the function of interest.
	push  OFFSET kszKernel32
IF USE_W_STRINGS
	call	DWORD PTR _imp__GetModuleHandleW@4	; Returns the handle to the library in EAX.
ELSE
	call	DWORD PTR _imp__GetModuleHandleA@4	; Returns the handle to the library in EAX.
ENDIF

	;; Attempt to obtain a pointer to the function of interest.
	push	OFFSET kszInitializeSListHead		; Push 2nd parameter (string containing function's name).
	push	eax					; Push 1st parameter (handle to the library).
	call	DWORD PTR _imp__GetProcAddress@8	; Returns the pointer to the function in EAX.

	;; Test for success, and call the function if we succeeded.
	test	eax, eax				; See if we successfully retrieved a pointer to the function.
	je	SHORT FuncNotSupported			; Jump on failure (ptr == 0), or fall through in the most-likely case.
	jmp	eax					; We succeeded (ptr != 0), so tail-call the function.

	;; The dynamic call failed, presumably because the function isn't available.
	;; So do what _RtlInitializeSListHead@4 (which is what we jump to on uplevel platforms) does,
	;; which is to set pHead->Alignment to 0. It is a QWORD-sized value, so 32-bit code must
	;; clear both of the DWORD halves.
FuncNotSupported:
	mov	edx, DWORD PTR [esp+4]	; get pHead->Alignment
	xor	eax, eax
	mov	DWORD PTR [edx],   eax	; pHead->Alignment = 0
	mov	DWORD PTR [edx+4], eax
	ret	4
DownlevelInitializeSListHead ENDP


; C++ translation:
; extern "C" BOOL WINAPI DownlevelGetModuleHandleExW(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE* phModule)
; {
;	const HMODULE hmodKernel32 = ::GetModuleHandleW(L"kernel32");
;	typedef decltype(GetModuleHandleExW)* pfnGetModuleHandleExW;
;	const pfnGetModuleHandleExW pfn = reinterpret_cast<pfnGetModuleHandleExW>(::GetProcAddress(hmodKernel32, "GetModuleHandleExW"));
;	if (pfn)
;	{
;		// call WinAPI function
;		return pfn(dwFlags, lpModuleName, phModule);
;	}
;	else
;	{
;		// fallback for downlevel: return failure
;		return FALSE;
;	}
; }
DownlevelGetModuleHandleExW PROC
	;; Get a handle to the DLL containing the function of interest.
	push	OFFSET kszKernel32
IF USE_W_STRINGS
	call	DWORD PTR _imp__GetModuleHandleW@4	; Returns the handle to the library in EAX.
ELSE
	call	DWORD PTR _imp__GetModuleHandleA@4	; Returns the handle to the library in EAX.
ENDIF

	;; Attempt to obtain a pointer to the function of interest.
	push	OFFSET kszGetModuleHandleExW		; Push 2nd parameter (string containing function's name).
	push	eax					; Push 1st parameter (handle to the library).
	call	DWORD PTR _imp__GetProcAddress@8	; Returns the pointer to the function in EAX.

	;; Test for success, and call the function if we succeeded.
	test	eax, eax				; See if we successfully retrieved a pointer to the function.
	je	SHORT FuncNotSupported			; Jump on failure (ptr == 0), or fall through in the most-likely case.
	jmp	eax					; We succeeded (ptr != 0), so tail-call the function.

	;; The dynamic call failed, presumably because the function isn't available.
	;; The basic VS 2015 CRT (used in a simple Win32 app) only calls this function
	;; in try_cor_exit_process(), as called from common_exit (both in exit.cpp),
	;; where it uses it to attempt to get a handle to the module mscoree.dll.
	;; Since we don't care about managed apps, that attempt should rightfully fail.
	;; If this turns out to be used in other contexts, we'll need to revisit this
	;; and implement a proper fallback.
FuncNotSupported:
	jmp	ImplementGetModuleHandleExW@12

DownlevelGetModuleHandleExW ENDP

DownlevelEncodeDecodePointer proc
	mov eax, [esp+4]
	ret 4
DownlevelEncodeDecodePointer endp

; DownlevelHeapSetInformation proc
;	mov eax, 1
;	ret 10h
; DownlevelHeapSetInformation endp

; DownlevelSystemFunction036 PROC
;	int 3   ; break --- stub unimplemented
;	ret 8
; DownlevelSystemFunction036 ENDP


; Win9x section
IF USE_WIN9X

; here we need to return 1 for the crt startup code which checks the return value which on 9x is "none" (throws exception instead)
; This section will change the imported function on the first call for NT based OS so that every call doesn't require additional processing.

DownlevelInitializeCriticalSectionAndSpinCount proc

	push	OFFSET kszKernel32
IF USE_W_STRINGS
	call	DWORD PTR _imp__GetModuleHandleW@4	; Returns the handle to the library in EAX.
ELSE
	call	DWORD PTR _imp__GetModuleHandleA@4	; Returns the handle to the library in EAX.
ENDIF

	;; Save copy of handle
	push	eax

	;; Attempt to obtain a pointer to the function of interest.
	push	OFFSET kszInitializeCriticalSectionAndSpinCount	; Push 2nd parameter (string containing function's name).
	push	eax						; Push 1st parameter (handle to the library).
	call	DWORD PTR _imp__GetProcAddress@8		; Returns the pointer to the function in EAX.

	;; Test for success, and call the function if we succeeded.
	test	eax, eax			; See if we successfully retrieved a pointer to the function.
	jz	SHORT FuncNotSupported		; Jump on failure (ptr == 0), or fall through in the most-likely case.

	;; save address and get back kernel32 handle
	xchg	dword ptr [esp],eax

	;; Attempt to obtain a pointer to the function of interest.
	push	OFFSET kszGetVersion			; Push 2nd parameter (string containing function's name).
	push	eax					; Push 1st parameter (handle to the library).
	call	DWORD PTR _imp__GetProcAddress@8	; Returns the pointer to the function in EAX.

	;; See if function exists
	test	eax,eax
	jz	SHORT FuncNotSupported

	;; call GetVersion
	call  eax

	;; check if win9x
	test	eax,80000000h
	pop	eax					; get back address of InitializeCriticalSectionAndSpinCount function
	jz	WinNT

	;; for Win9x we need to call and then change return code
	push	[esp+8]					; Copy the 1st parameter (+4 for IP return address, +4 for fist param)
	push	[esp+8]					; Copy the 2nd parameter (recall esp moves on push)
	call	eax
	mov	eax,1
	ret	8

WinNT:
	;; Change the call address for next calls and call directly
	mov	__imp__InitializeCriticalSectionAndSpinCount@8,eax
	jmp	eax


;; We should never end up here but if we do, fail the call
FuncNotSupported:
	pop	eax
	xor	eax,eax
	ret	8

DownlevelInitializeCriticalSectionAndSpinCount endp

ENDIF

END
