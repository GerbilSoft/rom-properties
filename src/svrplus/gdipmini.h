// Mini version of gdiplusinit.h for C code.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DebugEventLevelFatal,
	DebugEventLevelWarning
} DebugEventLevel;

typedef VOID (WINAPI *DebugEventProc)(DebugEventLevel level, CHAR *message);

typedef int /*Status*/ (WINAPI *NotificationHookProc)(OUT ULONG_PTR *token);
typedef VOID (WINAPI *NotificationUnhookProc)(ULONG_PTR token);

typedef struct _GdiplusStartupInput {
	UINT32 GdiplusVersion;
	DebugEventProc DebugEventCallback;
	BOOL SuppressBackgroundThread;
	BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef struct _GdiplusStartupOutput {
	NotificationHookProc NotificationHook;
	NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

int /*Status*/ WINAPI GdiplusStartup(
	OUT ULONG_PTR *token,
	const GdiplusStartupInput *input,
	OUT GdiplusStartupOutput *output);

VOID WINAPI GdiplusShutdown(ULONG_PTR token);

#ifdef __cplusplus
}
#endif
