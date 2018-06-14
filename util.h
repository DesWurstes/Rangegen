int file_reader(const char *file_loc, const char ***pat_list, int *pat_count,
	int *pat_alloc, int longest_addr);

char * encodeHex(const unsigned char *data, int len, char *result_string);
