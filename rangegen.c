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

const char * const alphabet_b58 =
	"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

const signed char dict_b58[128] = {
	// clang-format off
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1,
	-1,  9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
	22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
	-1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
	47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1
	// clang-format on
};

// magical constant
// github.com/cashaddress/vanitygen-cash/files/2005886/Base58table.ods.xlsx
const signed char trial_tree[58][2] = {
	{-0,-0}, {-0,2}, {-1,-2}, {1,4}, {-3,5},
	{-4,-5}, {3,9}, {-6,8}, {-7,-8}, {7,11},
	{-9,-10}, {10,12}, {-11,-12}, {6,20}, {-13,15},
	{-14,-15}, {14,18}, {-16,-17}, {17,19}, {-18,-19},
	{16,24}, {-20,-21}, {21,23}, {-22,-23}, {22,26},
	{-24,-25}, {25,27}, {-26,-27}, {13,43}, {-28,30},
	{-29,-30}, {29,33}, {-31,-32}, {32,34}, {-33,-34},
	{31,39}, {-35,-36}, {36,38}, {-37,-38}, {37,41},
	{-39,-40}, {40,42}, {-41,-42}, {35,50}, {-43,45},
	{-44,-45}, {44,48}, {-46,-47}, {47,49}, {-48,-49},
	{46,54}, {-50,-51}, {51,53}, {-52,-53}, {52,56},
	{-54,-55}, {55,57}, {-56,-57}
};

#define version "v0.1"

static void usage(const char *progname) {
	// clang-format off
	fprintf(stderr,
"Rangegen %s\n"
"Usage: %s [-si] [-c <network_byte>] -f <file> -o <file> [<pattern>...]\n"
"-f            File containing list of patterns, one per line\n"
"-c            Network byte of the cryptocurrency (default: bitcoin, 0)\n"
"-i            Case insensitivity\n"
"-s            Disable sorting\n"
"-o            Output file (will overwrite!)\n",
		version, progname);
	// clang-format on
}

// TODO: optimize w/ logarithms
static int DecodeBase58(
	const unsigned char *str, int len, unsigned char result[30]) {
	result[0] = 0;
	int resultlen = 1;
	for (int i = 0; i < len; i++) {
		//unsigned int carry = (unsigned int) dict_b58[str[i]];
		unsigned int carry = (unsigned int) str[i];
		for (int j = 0; j < resultlen; j++) {
			carry += (unsigned int) (result[j]) * 58;
			result[j] = (unsigned char) (carry & 0xff);
			carry >>= 8;
		}
		while (carry > 0) {
			result[resultlen++] = (unsigned int) (carry & 0xff);
			carry >>= 8;
		}
	}

	for (int i = 0; i < len && str[i] == 0/*'1'*/; i++)
		result[resultlen++] = 0;

	//for (int i = 0; i < resultlen / 2; i++)
	//	result[i] = result[resultlen - 1 - i];
	for (int i = resultlen - 1, z = (resultlen >> 1) + (resultlen & 1);
		i >= z; i--) {
		int k = result[i];
		result[i] = result[resultlen - i - 1];
		result[resultlen - i - 1] = k;
	}

	//result[resultlen] = 0;
	return resultlen;
}

static int DecodeBase58Len(
	const unsigned char *str, int len) {
	int resultlen = 1;
	//char result[len * 74 / 100];
	char result[30];
	result[0] = 0;
	for (int i = 0; i < len; i++) {
		int carry = (unsigned int) str[i];
		for (int j = 0; j < resultlen; j++) {
			carry += (unsigned int) (result[j]) * 58;
			result[j] = (unsigned char) (carry & 0xff);
			carry >>= 8;
		}
		while (carry > 0) {
			result[resultlen++] = (unsigned char) (carry & 0xff);
			carry >>= 8;
		}
	}
	for (int i = 0; i < len && str[i] == '1'; i++, resultlen++);
	return resultlen;
}

// max. 36
static int EncodeBase58(
	const unsigned char *bytes, int len, unsigned char result[40]) {
	// Actually len * log_58 (256)
	unsigned char digits[40];
	digits[0] = 0;
	int digitslen = 1;
	for (int i = 0; i < len; i++) {
		unsigned int carry = (unsigned int) bytes[i];
		for (int j = 0; j < digitslen; j++) {
			carry += (unsigned int) (digits[j]) << 8;
			digits[j] = (unsigned char) (carry % 58);
			carry /= 58;
		}
		while (carry > 0) {
			digits[digitslen++] = (unsigned char) (carry % 58);
			carry /= 58;
		}
	}
	int resultlen = 0;
	// leading zero bytes
	for (; resultlen < len && bytes[resultlen] == 0;)
		result[resultlen++] = '1';
	// reverse
	for (int i = 0; i < digitslen; i++)
		result[resultlen + i] = alphabet_b58[digits[digitslen - 1 - i]];
	result[digitslen + resultlen] = 0;
	return digitslen + resultlen;
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
	int case_insensitive = 0;
	int sort = 1;
	char *output = NULL;
	char network_byte = 0;
	int opt;
	while ((opt = getopt(argc, argv, "c:iso:f:")) != -1) {
		switch (opt) {
		case 'c': network_byte = atoi(optarg); break;
		case 'f':
			if (file_reader(optarg, &pat, &pat_count, &pat_alloc,
				    37))
				return 1;
			break;
		case 'i': case_insensitive = 1; break;
		case 's': sort = 0; break;
		case 'o':
			if (output != NULL) {
				fprintf(stderr, "Only one output file!\n");
				return 1;
			}
			output = optarg;
			break;
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

	if (pat_count == 0) {
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

	// Because one pattern may have two ranges
	unsigned char range[pat_count * 2][40];
	int range_count = 0;

	for (int i = 0; i < pat_count; i++) {
		const unsigned char *current_pat = (const unsigned char *) pat[i];
		int current_pat_len = 0;
		while (current_pat[current_pat_len] > 32) {
			current_pat_len++;
		}
		if (current_pat_len > 20) {
			fprintf(stderr, "Pattern %s is too long! Must be no longer than 20 characters.\n",
				current_pat);
			return 1;
		}
		if (current_pat_len == 0) {
			continue;
		}
		unsigned char buf[37] = {0};
		unsigned char buf2[30];
		for (int k = 0; k < current_pat_len; k++) {
			const char z = dict_b58[(int) current_pat[k]];
			if (current_pat[k] > 127 || z == -1) {
				fprintf(stderr, "Non-base58 character in pattern %.*s: %c\n", current_pat_len, current_pat, current_pat[k]);
				return 1;
			}
			buf[k] = (unsigned char) z;
		}

		int c = 0;
		int lens[network_byte == 0x00 ? 2 : 1];
		// memcpy(buf, current_pat, current_pat_len);
		for (int k = current_pat_len; k < 37; k++) {
// TODO: replace this with the len function.
			const int addr_len = DecodeBase58((const unsigned char*) buf, k, (unsigned char*) buf2);
			if (addr_len > 25) break;
			if (addr_len < 25 || buf2[0] != network_byte) {printf("%d, %d, %d\n", addr_len, k, buf2[0]);continue;}
			// Will never happen
			if (c == 2 || (network_byte != 0x00 && c == 1)) {
				printf("Unknown error!\n");
				return 1;
			}
			lens[c++] = k;
			memcpy(range[range_count++], &buf2[1], 20);
		}
		if (c == 0) {
			fprintf(stderr, "Pattern impossible: %s\n", current_pat);
			if (network_byte == 0x00) {
				fprintf(stderr, "May help: Bitcoin addresses start with '1'.\n");
			} else {
				fprintf(stderr, "Perhaps you should search online: with which character"
					" does the coin's addresses start?\n");
			}
			return 1;
		}
	char_trier:
		for (int z = current_pat_len; z < lens[0]; z++) {
			//assert(DecodeBase58((const unsigned char*) buf, lens[0], (unsigned char*) buf2) == 25);
			// TODO: here/len
#define TREE_INDEX \
	DecodeBase58((const unsigned char*) buf, lens[0], (unsigned char*) buf2) == 25
			buf[z] = 57;
			if (TREE_INDEX) continue;
			int best_char = 28;
			buf[z] = 28;

			// Loop unrolling
			best_char = trial_tree[best_char][TREE_INDEX];
			buf[z] = best_char;
			best_char = trial_tree[best_char][TREE_INDEX];
			buf[z] = best_char;
			best_char = trial_tree[best_char][TREE_INDEX];
			buf[z] = best_char;
			best_char = trial_tree[best_char][TREE_INDEX];
			buf[z] = best_char;
			best_char = trial_tree[best_char][TREE_INDEX];
			if (best_char > 0) {
				buf[z] = best_char;
				best_char = trial_tree[best_char][TREE_INDEX];
			}
			buf[z] = -best_char;
			assert(DecodeBase58((const unsigned char*) buf, lens[0], (unsigned char*) buf2) == 25);
			// DEBUG
			// TODO: here
			/*for (int i = 0; i < lens[0]; i++) {
				printf("%c", buf[i]);
			}
			printf("\n");*/
		}
#if 0
		{
			// DEBUG
			unsigned char result[40] = {0};
			EncodeBase58(buf2, 25, result);
			printf("res%s\n", result);
		}
#endif


		if (c == 2) {
			c--;
			lens[0] = lens[1];
			memcpy(&range[range_count - 2][20], &buf2[1], 20);
			memset(&buf[current_pat_len], '1', 37 - current_pat_len);
			goto char_trier;
		}
		memcpy(&range[range_count - 1][20], &buf2[1], 20);
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
