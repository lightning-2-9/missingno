#ifndef FORMAT_HANDLER_H
#define FORMAT_HANDLER_H

#include <stddef.h>

// everything we need to represent the file we loaded
typedef struct {
	unsigned char *data; // pointer should tell us where the address of the file starts
	size_t size; // total bytes of the file so we know where it ends
} FileBuffer;

// read the file into memory
int read_file(const char *path, FileBuffer *buf);

//write buffer back out to disk
int write_file(const char *path, const FileBuffer *buf);

//free the memory we allocated for the buffer
void free_buffer(FileBuffer *buf);

#endif
