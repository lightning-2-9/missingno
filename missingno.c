#include "missingno.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// flip one random bit in a byte
// least destructive, only changes the value by a small amount
static void bit_flip(FileBuffer *buf, size_t pos) {
	int bit = rand() % 8;
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

// returns the name of a glitch type as a string for verbose printing
static const char *type_name(GlitchType type) {
	switch (type) {
		case GLITCH_BITFLIP: return "bitflip";
		case GLITCH_SWAP:    return "swap";
		case GLITCH_SHIFT:   return "shift";
		case GLITCH_REVERSE: return "reverse";
		case GLITCH_NOISE:   return "noise";
		default:             return "unknown";
	}
}

// applies a single glitch operation at the given position and logs it if verbose
static void apply_glitch(FileBuffer *buf, const FileFormat *fmt, 
						  GlitchType type, size_t pos, int verbose) {
	unsigned char before = buf->data[pos];

	switch (type) {
		case GLITCH_BITFLIP: bit_flip(buf, pos);        break;
		case GLITCH_SWAP:    byte_swap(buf, pos, fmt);  break;
		case GLITCH_SHIFT:   byte_shift(buf, pos);      break;
		case GLITCH_REVERSE: byte_reverse(buf, pos);    break;
		case GLITCH_NOISE:   byte_noise(buf, pos);      break;
		case GLITCH_RANDOM:  break;
	}

	if (verbose) {
		printf("%s @ byte %zu | %02x -> %02x\n",
			type_name(type), pos, before, buf->data[pos]);
	}
}

void glitch_file(FileBuffer *buf, const FileFormat *fmt, const GlitchConfig *cfg) {
	srand(time(NULL));

	size_t corrupted = 0;

	// spacing mode: march through the file linearly, hit every (spacing) bytes
	// this creates rhythmic repeating patterns instead of random scatter
	// intensity is ignored here. The spacing gap itself controls how much gets hit
	if (cfg->spacing > 0) {
		if (cfg->verbose) printf("spacing mode: hitting every %d bytes\n", cfg->spacing);

		// start right after the header, never touch protected ranges
		size_t pos = fmt->ranges[0].end;

		while (pos < buf->size) {
			if (is_safe_position(pos, fmt)) {
				GlitchType type = cfg->type;
				if (type == GLITCH_RANDOM) type = (GlitchType)(rand() % 5);

				if (cfg->chunk_size > 0) {
					// spacing + chunk together: corrupt a run of bytes,
					// then skip spacing bytes, then corrupt another run
					// great for rhythmic audio artifacts and image banding
					if (cfg->verbose) printf("chunk @ byte %zu | size %d\n", pos, cfg->chunk_size);

					for (int j = 0; j < cfg->chunk_size; j++) {
						size_t cur = pos + j;
						if (cur >= buf->size) break;
						if (!is_safe_position(cur, fmt)) break;
						apply_glitch(buf, fmt, type, cur, 0);
						corrupted++;
					}

					// jump past the chunk we just corrupted plus the spacing gap
					pos += cfg->chunk_size + cfg->spacing;
				} else {
					// spacing only: single byte hit every N bytes
					apply_glitch(buf, fmt, type, pos, cfg->verbose);
					corrupted++;
					pos += cfg->spacing;
				}
			} else {
				// landed in a protected range, nudge forward one byte and try again
				pos++;
			}
		}

	} else {
		// classic random scatter mode: pick random positions based on intensity
		// if chunk_size is set we treat num_ops as number of chunks, not individual bytes
		// so we divide down, otherwise at 0.05 intensity you'd corrupt 5% * chunk_size
		// which gets out of hand fast

		// figure out how many corruptions to make based on intensity
		size_t num_ops = (size_t)(buf->size * cfg->intensity);
		size_t num_chunks = cfg->chunk_size > 0
			? num_ops / cfg->chunk_size
			: num_ops;

		if (cfg->verbose) printf("scatter mode: planning %zu operations\n", num_chunks);

		for (size_t i = 0; i < num_chunks; i++) {
			size_t pos = rand() % buf->size;
			if (!is_safe_position(pos, fmt)) continue;

			GlitchType type = cfg->type;
			if (type == GLITCH_RANDOM) type = (GlitchType)(rand() % 5);

			if (cfg->chunk_size > 0) {
				// chunk mode: corrupt consecutive bytes from this position
				// each byte in the chunk gets the same glitch type applied
				if (cfg->verbose) printf("chunk @ byte %zu | size %d\n", pos, cfg->chunk_size);

				for (int j = 0; j < cfg->chunk_size; j++) {
					size_t cur = pos + j;
					if (cur >= buf->size) break;
					if (!is_safe_position(cur, fmt)) break;
					apply_glitch(buf, fmt, type, cur, 0);  // verbose handled at chunk level
					corrupted++;
				}
			} else {
				// classic mode: single byte hit
				apply_glitch(buf, fmt, type, pos, cfg->verbose);
				corrupted++;
			}
		}
	}

	printf("done... corrupted %zu bytes\n", corrupted);
}
