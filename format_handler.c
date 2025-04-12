#include "format_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

// WAV files can have extra metadata chunks before the actual audio data
// so we scan forward looking for the data marker instead of assuming 44 bytes
static size_t find_wav_data_offset(const FileBuffer *buf) {
	// start at byte 12, right after the RIFF/WAVE header
	size_t i = 12;

	while (i + 8 < buf->size) {
		// each chunk starts with a 4 byte id and a 4 byte size
		// if we find data that's where the audio starts
		if (buf->data[i]	 == 'd' &&
			buf->data[i + 1] == 'a' &&
			buf->data[i + 2] == 't' &&
			buf->data[i + 3] == 'a') {
			// skip past the data id and the 4 byte size field
			return i + 8;
		}

		// not data yet, so skip this chunk entirely
		// chunk size is stored as a little-endian 32 bit int at bytes i+4 through i+7
		uint32_t chunk_size = (uint32_t)buf->data[i + 4]
							| ((uint32_t)buf->data[i + 5] << 8)
							| ((uint32_t)buf->data[i + 6] << 16)
							| ((uint32_t)buf->data[i + 7] << 24);

		i += 8 + chunk_size;
	}

	// couldn't find it, fall back to 44
	fprintf(stderr, "warning: couldn't find WAV data chunk, defaulting to 44 byte skip\n");
	return 44;
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
		1,
		0 // compressed huffman stream, will likely break
	};

	// PNG: 8 byte signature, then chunk structure starts
	static const FileFormat png = {
		"png",
		{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A},
		8,
		{{0, 8}},
		1,
		0 // deflate compressed, same problem as jpeg
	};

	// BMP: 54 byte header, then raw pixel data. safest format to glitch
	static const FileFormat bmp = {
		"bmp",
		{0x42, 0x4D},	// 'BM' in ASCII
		2,
		{{0, 54}},
		1,
		1 // raw pixels after 54 byte header, corruption always survives
	};

	 // WAV: header can be more than 44 bytes in some cases
	static const unsigned char wav_magic[] = {0x52, 0x49, 0x46, 0x46};
	if (buf->size >= 4 && memcmp(buf->data, wav_magic, 4) == 0) {
		size_t data_offset = find_wav_data_offset(buf);
		printf("WAV data chunk found at byte %zu\n", data_offset);
		FileFormat wav = {
			"wav",
			{0x52, 0x49, 0x46, 0x46},	// 'RIFF'
			4,
			{{0, data_offset}},
			1,
			1 // raw audio samples after 44 byte header, corruption always survives
		};
		return wav;
	}

	// MP3: either starts with ID3 tag or with sync bytes FF FB
	static const FileFormat mp3 = {
		"mp3",
		{0x49, 0x44, 0x33},   // 'ID3'
		3,
		{{0, 10}},
		1,
		0 // compressed frames, light corruption might work but no guarantees
	};

	// check for format using magic bytes
	static const FileFormat *formats[] = {&jpeg, &png, &bmp, &mp3};
	static const int format_count = 4;

	for (int i = 0; i < format_count; i++) {
		const FileFormat *fmt = formats[i];
		if (buf->size >= fmt->magic_len &&
			memcmp(buf->data, fmt->magic, fmt->magic_len) == 0) {
			return *fmt;
		}
	}

	// unknown format: no safe ranges, corrupt everything
	static const FileFormat unknown = {"unknown", {0}, 0, {{0, 0}}, 0, 0};
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
