#include <stdio.h>

// TODO: simplify Base58 + CashAddr
// FUZZ, make ranges smaller (not for case insensitive)
// COMPRESS, eliminate smallest ranges
#define version "v0.1"

static void usage(const char *progname) {
	// clang-format off
	fprintf(stderr,
"Simplifier %s\n"
"Usage: %s [-sF] -f <file> -o <filename>\n"
"-f            File containing ranges, may be multiple\n"
"-s            Disable sorting\n"
"-F            Fuzz, make ranges slightly smaller.\n"
"-o            Output file (will overwrite!)\n",
		version, progname);
	// clang-format on
}
