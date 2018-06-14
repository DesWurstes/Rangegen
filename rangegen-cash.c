#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "sort.h"

#include "getopt.h"

#include "util.h"

#define version "v0.1"

static void usage(const char *progname) {
	// clang-format off
	fprintf(stderr,
"Rangegen Cash %s\n"
"Usage: %s [-s] -f <file> -o <file> [<pattern>...]\n"
"-f            File containing list of patterns, one per line\n"
"-s            Disable sorting\n"
"-o            Output file (will overwrite!)\n",
		version, progname);
	// clang-format on
}

const char * const alphabet_cash = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

const signed char dict_cash[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
	 1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};

// Skips the first byte
static void convertBitsFiveToEight(const unsigned char *__restrict bytes,
	unsigned char *__restrict converted) {
	int a = 0, b = 1;
	while (a < 32) {
		converted[a] = bytes[b++] % (1 << 2) << 6;
		converted[a] |= bytes[b++] << 1;
		converted[a++] |= bytes[b] >> 4;
		converted[a] = bytes[b++] % (1 << 4) << 4;
		converted[a++] |= bytes[b] >> 1;
		converted[a] = bytes[b++] % (1 << 1) << 7;
		converted[a] |= bytes[b++] << 2;
		converted[a++] |= bytes[b] >> 3;
		converted[a] = bytes[b++] % (1 << 3) << 5;
		converted[a++] |= bytes[b++];
		converted[a] = bytes[b++] << 3;
		converted[a++] |= bytes[b] >> 2;
	}
	// assert(bytes[b] % (1 << 2) == 0);
}

int main(int argc, char **argv) {
	if (argc == 1) {
	args:
		usage(argv[0]);
		return 1;
	}
	const char **pat = NULL;
	int pat_count = 0;
	int pat_alloc = 0;
	int sort = 1;
	char *output = NULL;
	int opt;
	while ((opt = getopt(argc, argv, "o:sf:")) != -1) {
		switch (opt) {
		case 'f':
			if (file_reader(optarg, &pat, &pat_count, &pat_alloc,
				    32))
				return 1;
			break;
		case 'o':
			if (output != NULL) {
				fprintf(stderr, "Only one output file!\n");
				return 1;
			}
			output = optarg;
			break;
		case 's': sort = 0; break;
		default: goto args;
		}
	}

	if (output == NULL) {
		fprintf(stderr, "No output file specified!\n");
		return 1;
	}
	if (access(output, F_OK) != -1) {
		fprintf(stderr, "Output file already exists!\n");
		return 1;
	}

	if (!pat_count) {
		if (optind >= argc) {
			printf("No patterns!\n");
			usage(argv[0]);
			return 1;
		}
		pat_count = argc - optind;
		pat = (const char **) malloc(pat_count * sizeof(char *));
		pat_alloc = pat_count;
		for (int i = 0; i < pat_count; i++)
			pat[i] = argv[optind + i];
	} else if (argc - optind > 0) {
		fprintf(stderr, "Please add patterns using either the -f args or directly!\n");
		return 1;
	}

	unsigned char range[pat_count][40];
	int range_count = 0;

	for (int i = 0; i < pat_count; i++) {
		const unsigned char *current_pat = (const unsigned char *) pat[i];
		int current_pat_len = 0;
		while (current_pat[current_pat_len] > 32) {
			current_pat_len++;
		}
		if (current_pat_len > 32) {
			fprintf(stderr, "Pattern %s is too long! Must be no longer than 32 characters.\n",
				current_pat);
			return 1;
		}
		if (current_pat_len == 0) {
			continue;
		}
		if (current_pat[0] != 'q') {
			fprintf(stderr, "The pattern %.*s doesn't start with 'q'!\n", current_pat_len, current_pat);
			return 1;
		}
		switch (current_pat[1]) {
			case 'q':
			case 'p':
			case 'z':
			case 'r':
				break;
			default:
				fprintf(stderr, "Unlike pattern %.*s, a pattern's second letter must be 'p', 'q', 'r' or 'z'!\n", current_pat_len, current_pat);
				return 1;
		}
		unsigned char scratch[34];
		scratch[0] = 0;
		if (current_pat_len != 1) {
			scratch[1] = (unsigned char) dict_cash[(int) current_pat[1]];
		}
		for (int k = 2; k < current_pat_len; k++) {
			char z;
			if (current_pat[k] > 127 || (z = dict_cash[(int) current_pat[k]]) == -1) {
				fprintf(stderr, "Non-CashAddr character in pattern %.*s: %c\n", current_pat_len, current_pat, current_pat[k]);
				return 1;
			}
			scratch[k] = (unsigned char) z;
		}
		convertBitsFiveToEight(scratch, range[range_count]);
		for (int k = 0; k < 20; k++) {
			range[range_count][20 + k] = range[range_count][k];
		}
		int fill = 20 + (current_pat_len * 5) / 8 - 1;
		if (current_pat_len != 1) {
			// subtract one for version byte
			range[range_count][fill] |= (unsigned char) ((1 << (8 - ((current_pat_len * 5) % 8))) - 1);
		}
		for (int i = fill + 1; i < 40; i++) {
			range[range_count][i] = 255;
		}
		range_count++;
	}

	if (sort) {
		if(sorter(range, range_count) == 0) {
			fprintf(stderr, "Couldn't allocate enough memory, can't sort."
				"Exiting.\n");
			return 1;
		}
	}

	FILE *foutput = fopen(output, "w+");
	if (!foutput) {
		fprintf(stderr, "Could not open %s! Error: %s\n", output,
			strerror(errno));
		return 1;
	}
	char range_hex[80];
	for (int i = 0; i < range_count; i++) {
		if (fprintf(foutput, "%.80s\n", encodeHex(range[i], 40, range_hex)) >= 0)
			continue;
		fprintf(stderr, "An error occured while writing to %s\n", output);
		fclose(foutput);
		return 1;
	}
	fclose(foutput);
	return 0;
}
