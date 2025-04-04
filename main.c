#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "format_handler.h"
#include "missingno.h"

static void print_usage(char *argv[]);
static GlitchType parse_type(const char *str);

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("%s: missing required arguments (-i input, -o output)\n", argv[0]);
		printf("Try '%s -h' for more information.\n", argv[0]);
		return 1;
	}

	// these 3 are required, everything else has a default
	const char *input_path	= NULL;
	const char *output_path = NULL;
	float intensity         = -1.0f;

	GlitchConfig cfg = {
		.type	   = GLITCH_BITFLIP,
		.intensity = 0.0f,
		.spacing   = 0,
		.verbose   = 0
	};

	int opt;
	while ((opt = getopt(argc, argv, "i:o:a:t:s:vh")) != -1) {
		switch (opt) {
			case 'i': input_path  = optarg;             break;
			case 'o': output_path = optarg;             break;
			case 'a': intensity   = atof(optarg);       break;
			case 't': cfg.type    = parse_type(optarg); break;
			case 's': cfg.spacing = atoi(optarg);       break;
			case 'v': cfg.verbose = 1;                  break;
			case 'h': print_usage(argv);                return 0;
			default:
				fprintf(stderr, "unknown flag, use -h for help\n");
				return 1;
		}
	}

	// make sure the 3 required args were actually provided
	if (!input_path) {
		fprintf(stderr, "error: no input file specified (-i)\n");
		return 1;
	}
	if (!output_path) {
		fprintf(stderr, "error: no output file specified (-o)\n");
		return 1;
	}
	if (intensity < 0.0f) {
		fprintf(stderr, "error: no intensity specified (-a)\n");
		return 1;
	}
	if (intensity > 1.0f) {
		fprintf(stderr, "error: intensity must be between 0.0 and 1.0\n");
		return 1;
	}

	cfg.intensity = intensity;

	FileBuffer buf;
	if(!read_file(input_path, &buf)) return 1;
	printf("read %zu bytes from %s\n", buf.size, input_path);

	FileFormat fmt = detect_format(&buf);
	printf("detected format: %s\n", fmt.name);

	// warn the user if we can't guarantee the file will survive
	if(!fmt.supported) {
		printf("warning: %s is compressed and may not open after corruption.\n", fmt.name);
		printf("results are unpredictable. proceed? (y/n): ");
		char response;
		scanf("%c", &response);
		if (response != 'y' && response != 'Y') {
			printf("aborted.\n");
			free_buffer(&buf);
			return 0;
		}
	}

	glitch_file(&buf, &fmt, &cfg);

	if(!write_file(output_path, &buf)) return 1;
	printf("wrote %zu bytes to %s\n", buf.size, output_path);

	free_buffer(&buf);	

	return 0;
}

static void print_usage(char *argv[]) {
	printf("usage: %s -i <input> -a <intensity> [options] -o <output>\n\n", argv[0]);
	printf("options:\n");
	printf("  -i <file>     input file\n");
	printf("  -o <file>     output file\n");
	printf("  -a <amount>   intensity 0.00 to 1.00\n");
	printf("  -t <type>     glitch type: bitflip, swap, shift, reverse, noise, random\n");
	printf("  -s <spacing>  min bytes between corruptions (0 = chaos mode)\n");
	printf("  -v            verbose - print every corruption\n");
	printf("  -h            show this help\n\n");
}

static GlitchType parse_type(const char *str) {
	if (strcmp(str, "bitflip") == 0) return GLITCH_BITFLIP;
	if (strcmp(str, "swap")    == 0) return GLITCH_SWAP;
	if (strcmp(str, "shift")   == 0) return GLITCH_SHIFT;
	if (strcmp(str, "reverse") == 0) return GLITCH_REVERSE;
	if (strcmp(str, "noise")   == 0) return GLITCH_NOISE;
	if (strcmp(str, "random")  == 0) return GLITCH_RANDOM;

	fprintf(stderr, "unknown glitch type '%s', defaulting to bitflip\n", str);
	return GLITCH_BITFLIP;
}
