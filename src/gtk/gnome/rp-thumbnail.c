/***************************************************************************
 * ROM Properties Page shell extension. (GNOME)                            *
 * rp-thumbnail.c: Thumbnail wrapper program.                              *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

/**
 * thumbnail.c is a wrapper program for the GNOME plugin.
 * It parses the command line and then searches for installed
 * thumbnailing plugins. If found, it runs the thumbnail function.
 */
#include "config.version.h"
#include "config.gnome.h"

#include <dlfcn.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * rp_create_thumbnail() function pointer.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
typedef int (*PFN_RP_CREATE_THUMBNAIL)(const char *source_file, const char *output_file, int maximum_size);

static void show_version(void)
{
	puts(RP_DESCRIPTION "\n"
		"Thumbnailer wrapper program for GNOME.\n"
		"Copyright (c) 2016-2017 by David Korth.\n"
		"\n"
		"rom-properties version: " RP_VERSION_STRING "\n"
		// TODO: git version
		"\n"
		"This program is licensed under the GNU GPL v2.\n"
		"See http://www.gnu.org/licenses/gpl-2.0.html for more information.");
}

static void show_help(const char *argv0)
{
	show_version();
	printf("\n"
		"Usage: %s [-s size] source_file output_file\n"
		"\n"
		"If source_file is a supported ROM image, a thumbnail is\n"
		"extracted and saved as output_file.\n"
		"\n"
		"Options:\n"
		"  -s, --size\t\t\tMaximum thumbnail size. (default is 256px)\n"
		"  -h, --help\t\t\tDisplay this help and exit.\n"
		"  -V, --version\t\t\tOutput version information and exit.\n"
		, argv0);
}

int main(int argc, char *argv[])
{
	// Command line syntax:
	// thumbnail [-s size] path output
	// TODO: Support URIs in addition to paths?

	static const struct option long_options[] = {
		{"size",	required_argument,	nullptr, 's'},
		{"help",	no_argument,		nullptr, 'h'},
		{"version",	no_argument,		nullptr, 'V'},
		// TODO: Option to scan for installed plugins.
	};

	// Default to 256x256.
	int maximum_size = 256;
	int c, option_index;
	while ((c = getopt_long(argc, argv, "s:hV", long_options, &option_index)) != -1) {
		switch (c) {
			case 's': {
				char *endptr;
				errno = 0;
				long lTmp = (int)strtol(optarg, &endptr, 10);
				if (errno == ERANGE || *endptr != 0) {
					fprintf(stderr, "%s: invalid size '%s'\n"
						"Try '%s --help' for more information.\n",
						argv[0], optarg, argv[0]);
					return EXIT_FAILURE;
				}
				else if (lTmp <= 0 || lTmp > 1048576) {
					fprintf(stderr, "%s: size '%s' is out of range\n"
						"Try '%s --help' for more information.\n",
						argv[0], optarg, argv[0]);
					return EXIT_FAILURE;
				}
				maximum_size = (int)lTmp;
				break;
			}

			case 'h':
				show_help(argv[0]);
				return EXIT_SUCCESS;

			case 'V':
				show_version();
				return EXIT_SUCCESS;
		}
	}

	// We must have 2 filenames specified.
	if (optind == argc) {
		fprintf(stderr, "%s: missing source and output file parameters\n"
			"Try '%s --help' for more information.\n",
			argv[0], argv[0]);
		return EXIT_FAILURE;
	} else if (optind+1 == argc) {
		fprintf(stderr, "%s: missing output file parameter\n"
			"Try '%s --help' for more information.\n",
			argv[0], argv[0]);
		return EXIT_FAILURE;
	} else if (optind+3 < argc) {
		fprintf(stderr, "%s: too many parameters specified\n"
			"Try '%s --help' for more information.\n",
			argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	// Attempt to open the GNOME plugin.
	// TODO: Try multiple plugins?
	// TODO: Get the proper installation directory.
	void *rp_plugin = dlopen(LibNautilusExtension_EXTENSION_DIR "/rom-properties-gnome.so", RTLD_LOCAL|RTLD_LAZY);
	if (!rp_plugin) {
		fprintf(stderr, "*** ERROR: Could not open the rom-properties GNOME plugin.\n");
		puts(dlerror());
		return EXIT_FAILURE;
	}

	// Get the "rp_create_thumbnail" function.
	PFN_RP_CREATE_THUMBNAIL rp_create_thumbnail = dlsym(rp_plugin, "rp_create_thumbnail");
	if (!rp_create_thumbnail) {
		dlclose(rp_plugin);
		fprintf(stderr, "*** ERROR: Could not find rp_create_thumbnail() in the rom-properties GNOME plugin.\n");
		return EXIT_FAILURE;
	}

	// Create the thumbnail.
	const char *const source_file = argv[optind];
	const char *const output_file = argv[optind+1];
	int ret = rp_create_thumbnail(source_file, output_file, maximum_size);
	dlclose(rp_plugin);
	if (ret != 0) {
		fprintf(stderr, "*** ERROR: rp_create_thumbnail() returned %d.\n", ret);
	}
	return ret;
}
