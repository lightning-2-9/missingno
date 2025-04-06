#ifndef MISSINGNO_H
#define MISSINGNO_H

#include "format_handler.h"

typedef enum {
	GLITCH_BITFLIP, // flips a single random bit in a byte
	GLITCH_SWAP,    // swaps two bytes at different positions
	GLITCH_SHIFT,   // nudges a byte's value up or down slightly
	GLITCH_REVERSE, // reverses a small chunk of bytes
	GLITCH_NOISE,   // slams a complete random value in 
	GLITCH_RANDOM,  // picks randomly from above for every single byte
} GlitchType;

// the main settings for a glitch run
typedef struct {
	GlitchType type;
	float intensity; // 0.00 to 1.00, controls how many bytes get hit
	int spacing;     // minimum bytes between corruptions (0 should be pure chaos)
	int chunk_size;  // how many consecutive bytes to corrupt per hit (0 = single byte, classic mode)
	int verbose;     // print every corruption we make
} GlitchConfig;

void glitch_file(FileBuffer *buf, const FileFormat *fmt, const GlitchConfig *cfg);

#endif
