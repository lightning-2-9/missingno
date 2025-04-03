#include "missingno.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// flip one random bit in a byte
// least destructive, only changes the value by a small amount
static void bit_flip(FileBuffer *buf, size_t pos) {
	int bit = rand() % 8;
	if (/* verbose check */ 0) printf("bitflip @ %zu | %02x -> ", pos, buf->data[pos]);
	buf->data[pos] ^= (1 << bit);
}

// swap two bytes at completely different positions
// creates streaks and color shifts in images
static void byte_swap(FileBuffer *buf, size_t pos, const FileFormat *fmt) {
	// pick a second position that's also safe to corrupt
	size_t pos2;
	int attempts = 0;
	do {
		pos2 = rand() % buf->size;
		attempts++;
		// give up after 10 tries to avoid infinite loop on heavily protected formats
		if (attempts > 10) return;
	} while (!is_safe_position(pos2, fmt));

	unsigned char tmp = buf->data[pos];
	buf->data[pos] = buf->data[pos2];
	buf->data[pos2] = tmp;
}

// nudge a byte's value up or down by a small amount
// subtle color shifts in images, slight volume changes in audio
static void byte_shift(FileBuffer *buf, size_t pos) {
	int amount = (rand() % 64) - 32;   // anywhere from -32 to +32
	int val = (int)buf->data[pos] + amount;
	// clamp so it stays a valid byte value
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	buf->data[pos] = (unsigned char)val;
}

// reverse a small chunk of bytes starting at pos
// creates mirrored blocks in images, weird reversals in audio
static void byte_reverse(FileBuffer *buf, size_t pos) {
	size_t length = 50 + rand() % 200;
	// make sure we don't run off the end of the file
	if (pos + length > buf->size) length = buf->size - pos;

	size_t left = pos;
	size_t right = pos + length - 1;
	while (left < right) {
		unsigned char tmp = buf->data[left];
		buf->data[left] = buf->data[right];
		buf->data[right] = tmp;
		left++;
		right--;
	}
}

// slam a completely random value into the byte
// most destructive mode, creates harsh color/noise artifacts
static void byte_noise(FileBuffer *buf, size_t pos) {
	buf->data[pos] = rand() % 256;
}

void glitch_file(FileBuffer *buf, const FileFormat *fmt, const GlitchConfig *cfg) {
	srand(time(NULL));

	// figure out how many corruptions to make based on intensity
	size_t num_ops = (size_t)(buf->size * cfg->intensity);
	size_t corrupted = 0;

	if (cfg->verbose) printf("planning %zu corruptions across %zu bytes\n", num_ops, buf->size);

	for (size_t i = 0; i < num_ops; i++) {
		// pick a random position
		size_t pos = rand() % buf->size;

		// skip if this byte is inside a protected header range
		if (!is_safe_position(pos, fmt)) continue;

		// figure out which glitch type to use this iteration
		GlitchType type = cfg->type;
		if (type == GLITCH_RANDOM) {
			type = (GlitchType)(rand() % 5);  // pick any of the 5 real types
		}

		if (cfg->verbose) printf("%s @ byte %zu | was %02x",
			type == GLITCH_BITFLIP ? "bitflip" :
			type == GLITCH_SWAP    ? "swap"    :
			type == GLITCH_SHIFT   ? "shift"   :
			type == GLITCH_REVERSE ? "reverse" : "noise",
			pos, buf->data[pos]);

		switch (type) {
			case GLITCH_BITFLIP: bit_flip(buf, pos);		   break;
			case GLITCH_SWAP:	 byte_swap(buf, pos, fmt);	   break;
			case GLITCH_SHIFT:	 byte_shift(buf, pos);		   break;
			case GLITCH_REVERSE: byte_reverse(buf, pos);	   break;
			case GLITCH_NOISE:	 byte_noise(buf, pos);		   break;
			case GLITCH_RANDOM:  break;  // never reaches here
		}

		if (cfg->verbose) printf(" -> now %02x\n", buf->data[pos]);
		corrupted++;
	}

	printf("done... corrupted %zu bytes\n", corrupted);
}
