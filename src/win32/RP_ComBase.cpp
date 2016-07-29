// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ComBase.hpp"

// References of all objects.
volatile ULONG RP_ulTotalRefCount = 0;
