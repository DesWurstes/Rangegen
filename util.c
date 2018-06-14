#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int is_valid_base16_char(unsigned char x, const signed char *nothing) {
	(void) nothing;
	return ((x > 47 && x < 58) || (((x | 32) > 96) && ((x | 32) < 103)));
}

int file_reader(const char *file_loc, const char ***pat_list, int *pat_count,
	int *pat_alloc, int longest_addr) {
	FILE *fp = fopen(file_loc, "r");
	if (!fp) {
		fprintf(stderr, "Could not open %s! Error: %s\n", file_loc,
			strerror(errno));
		return 1;
	}
	while (1) {
		if (*pat_alloc == *pat_count) {
			*pat_list = (const char**) realloc(
				*pat_list, 2 * (*pat_alloc) * sizeof(char *));
			if (*pat_list == NULL) {
				fprintf(stderr, "Out of memory!\n");
				free(*pat_list);
				return 1;
			}
			*pat_alloc *= 2;
		}
		char *buff = (char *) malloc((longest_addr + 1) * sizeof(char));
		buff[longest_addr] = 0x7f;
		if (fgets(buff, longest_addr + 1, (FILE *) fp) == NULL) {
			if (feof(fp)) break;
			fprintf(stderr,
				"Couldn't read from input file %s! Error: %s\n",
				file_loc, strerror(errno));
			fclose(fp);
			return 1;
		}
		if (buff[longest_addr] != 0x7f) {
			fprintf(stderr,
				"Line that starts with \"%.15s...\" of the file %s"
				"is too long!\n",
				buff, file_loc);
			fclose(fp);
			return 1;
		}
		/*int i = 0;
		while (buff[i] != '\0') {
			const unsigned char c = buff[i++];
			if (c > 128 || dict[(int) c] == -1) {
				printf("Invalid %s character: %c\n", alphabet,
		c);
			}
		}*/
		(*pat_list)[(*pat_count)++] = (const char *) buff;
		// printf("%d\n", feof(fp));
	}
	fclose(fp);
	return 0;
}

const char * const hex = "0123456789abcdef";

char * encodeHex(const unsigned char *data, int len, char *result_string) {
	for (int i = 0; i < len; i++) {
		result_string[2 * i] = hex[data[i] / 16];
		result_string[2 * i + 1] = hex[data[i] % 16];
	}
	return result_string;
}
