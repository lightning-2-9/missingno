#include <stdio.h>
#include "format_handler.h"

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("usage: missingno <input> <output>\n");
		return 1;
	}

	FileBuffer buf;

	if(!read_file(argv[1], &buf)) return 1;
	printf("read %zu bytes from %s\n", buf.size, argv[1]);

	FileFormat fmt = detect_format(&buf);
	printf("detected format: %s\n", fmt.name);

	if(!write_file(argv[2], &buf)) return 1;
	printf("wrote %zu bytes to %s\n", buf.size, argv[2]);

	free_buffer(&buf);	

	return 0;
} 
