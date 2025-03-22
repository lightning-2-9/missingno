#ifndef FORMAT_HANDLER_H
#define FORMAT_HANDLER_H

#include <stddef.h>

// everything we need to represent the file we loaded
typedef struct {
	unsigned char *data; // pointer should tell us where the address of the file starts
	size_t size; // total bytes of the file so we know where it ends
} FileBuffer;

// protected range of bytes we never touch
typedef struct {
	size_t start;
	size_t end;
} SafeRange;

// everything we know about a particular file format
typedef struct {
	const char *name;
	unsigned char magic[8]; //the identifying bytes at the start of the file
	size_t magic_len; // how many of those bytes to check
	SafeRange ranges[8]; // byte ranges to never corrupt
	int range_count; // how many ranges we actually have
	int supported; // 1 if we can glitch this reliably, 0 if we're just guessing
} FileFormat;

// read the file into memory
int read_file(const char *path, FileBuffer *buf);

//write buffer back out to disk
int write_file(const char *path, const FileBuffer *buf);

//free the memory we allocated for the buffer
void free_buffer(FileBuffer *buf);

// look at the first few bytes and detect what format this is
FileFormat detect_format(const FileBuffer *buf);

// returns 1 if this position is safe to corrupt, 0 if we should skip it
int is_safe_position(size_t pos, const FileFormat *format);

#endif
