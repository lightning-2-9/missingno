#include "format_handler.h"
#include <stdio.h>
#include <stdlib.h>

int read_file(const char *path, FileBuffer *buf) {
	FILE *f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "can't open file: %s\n", path);
		return 0;
	}

	// jump to end to figure out file size
	fseek(f, 0, SEEK_END);
	buf->size = ftell(f);
	// then jump back
	fseek(f, 0, SEEK_SET);

	buf->data = malloc(buf->size);
	if (!buf->data) {
		fprintf(stderr, "out of memory!\n");
		fclose(f);
		return 0;
	}

	fread(buf->data, 1, buf->size, f);
	fclose(f);
	return 1;
}

int write_file(const char *path, const FileBuffer *buf) {
	FILE *f = fopen(path, "wb");
	if(!f) {
		fprintf(stderr, "can't write file: %s\n", path);
		return 0;
	}

	fwrite(buf->data, 1, buf->size, f);
	fclose(f);
	return 1;
}

void free_buffer(FileBuffer *buf) {
	free(buf->data);
	buf->data = NULL;
	buf->size = 0;
}
