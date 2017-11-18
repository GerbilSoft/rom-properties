#include "i18n.hpp"
#include <cassert>

/**
 * Initialize the internationalization subsystem.
 * @param dirname Directory name.
 * @return 0 on success; non-zero on error.
 */
int rp_i18n_init(const char *dirname)
{
	const char *base = bindtextdomain(RP_I18N_DOMAIN, dirname);
	if (!base) {
		// bindtextdomain() failed.
		return -1;
	}

	// Initialized.
	return 0;
}
