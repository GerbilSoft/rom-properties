# Check if OpenBSD's pledge() syscall is available, and if so,
# which variant of pledge() or tame() is available.
#
# The following cache variables are set:
# - HAVE_TAME: tame() is available. (OpenBSD 5.8)
# - HAVE_PLEDGE: pledge() is available, (OpenBSD 5.9+)
# - HAVE_PLEDGE_EXECPROMISES: pledge() with execpromises is available. (OpenBSD 6.3+)

MACRO(CHECK_OPENBSD_PLEDGE)
	CHECK_SYMBOL_EXISTS(pledge "unistd.h" HAVE_PLEDGE)
	IF(NOT HAVE_PLEDGE)
		CHECK_SYMBOL_EXISTS(pledge "sys/tame.h" HAVE_TAME)
	ENDIF(NOT HAVE_PLEDGE)
	IF(HAVE_PLEDGE)
		# pledge() has a different syntax in OpenBSD 6.3 and later:
		# - 5.9-6.2:  pledge(const char *promises, const char *paths[])
		# -     6.3+: pledge(const char *promises, const char *execpromises)
		CHECK_C_SOURCE_COMPILES("
#include <unistd.h>
int main(void)
{
	pledge(\"\", \"\");
}" HAVE_PLEDGE_EXECPROMISES)
	ENDIF(HAVE_PLEDGE)
ENDMACRO(CHECK_OPENBSD_PLEDGE)
