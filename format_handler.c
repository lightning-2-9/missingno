#include "format_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

FileFormat detect_format(const FileBuffer *buf) {
	// JPEG: we learned the hard way this needs a big safe zone
	// the EXIF block can stretch way further than you'd expect
	// especially from software that stuffs thumbnails in there
	static const FileFormat jpeg = {
		"jpeg",
		{0xFF, 0xD8, 0xFF},
		3,
		{{0, 14000}},
		1
	};

	// PNG: 8 byte signature, then chunk structure starts
	static const FileFormat png = {
		"png",
		{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A},
		8,
		{{0, 8}},
		1
	};

	// BMP: 54 byte header, then raw pixel data. safest format to glitch
	static const FileFormat bmp = {
		"bmp",
		{0x42, 0x4D},	// 'BM' in ASCII
		2,
		{{0, 54}},
		1
	};

	// WAV: header is always exactly 44 bytes
	static const FileFormat wav = {
		"wav",
		{0x52, 0x49, 0x46, 0x46},	// 'RIFF'
		4,
		{{0, 44}},
		1
	};

	// MP3: either starts with ID3 tag or with sync bytes FF FB
	static const FileFormat mp3 = {
		"mp3",
		{0x49, 0x44, 0x33},   // 'ID3'
		3,
		{{0, 10}},
		1
	};

	static const FileFormat *formats[] = {&jpeg, &png, &bmp, &wav, &mp3};
	static const int format_count = 5;

	for (int i = 0; i < format_count; i++) {
		const FileFormat *fmt = formats[i];
		if (buf->size >= fmt->magic_len &&
			memcmp(buf->data, fmt->magic, fmt->magic_len) == 0) {
			return *fmt;
		}
	}

	// unknown format: no safe ranges, corrupt everything
	static const FileFormat unknown = {"unknown", {0}, 0, {{0, 0}}, 0};
	return unknown;
}

int is_safe_position(size_t pos, const FileFormat *format) {
	for (int i = 0; i < format->range_count; i++) {
		if (pos >= format->ranges[i].start && pos < format->ranges[i].end) {
			// this byte is inside a protected range, don't touch it
			return 0;
		}
	}
	return 1;
}
