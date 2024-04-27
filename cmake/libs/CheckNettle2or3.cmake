# Check for Nettle v2 or v3.
# If either version is found, HAVE_NETTLE will be set.
# If Nettle v3 is found, HAVE_NETTLE_3 will be set.

FUNCTION(CHECK_NETTLE_2_OR_3)
	FIND_PACKAGE(NETTLE)
	SET(HAVE_NETTLE ${NETTLE_FOUND} PARENT_SCOPE)
	IF(HAVE_NETTLE)
		# Check if this is Nettle 3.x.
		# Nettle 3.1 added version.h, which isn't available
		# in older verisons, so we can't simply check that.
		INCLUDE(CheckSymbolExists)
		SET(CMAKE_REQUIRED_INCLUDES ${NETTLE_INCLUDE_DIRS})
		SET(CMAKE_REQUIRED_LIBRARIES ${NETTLE_LIBRARY})
		CHECK_SYMBOL_EXISTS(aes128_set_decrypt_key "nettle/aes.h" HAVE_NETTLE_3)
		IF(HAVE_NETTLE_3)
			# Check for Nettle versioning symbols.
			# Nettle 3.1 added version.h.
			CHECK_SYMBOL_EXISTS(NETTLE_VERSION_MAJOR "nettle/version.h" HAVE_NETTLE_VERSION_H)
			CHECK_SYMBOL_EXISTS(nettle_version_major "nettle/version.h" HAVE_NETTLE_VERSION_FUNCTIONS)
		ENDIF(HAVE_NETTLE_3)
	ELSE(HAVE_NETTLE)
		# Disable decryption.
		SET(ENABLE_DECRYPTION OFF CACHE INTERNAL "Enable decryption for newer ROM and disc images." FORCE)
	ENDIF(HAVE_NETTLE)
ENDFUNCTION(CHECK_NETTLE_2_OR_3)
