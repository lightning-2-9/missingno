#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "format_handler.h"
#include "missingno.h"

int main(int argc, char *argv[]) {
	if (argc < 4) {
		printf("usage: missingno <input> <intensity> <output>\n");
		return 1;
	}

	FileBuffer buf;

	if(!read_file(argv[1], &buf)) return 1;
	printf("read %zu bytes from %s\n", buf.size, argv[1]);

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

	GlitchConfig cfg = {
		.type	   = GLITCH_BITFLIP,
		.intensity = atof(argv[2]),
		.spacing   = 0,
		.verbose   = 0
	};

	glitch_file(&buf, &fmt, &cfg);

	if(!write_file(argv[3], &buf)) return 1;
	printf("wrote %zu bytes to %s\n", buf.size, argv[3]);

	free_buffer(&buf);	

	return 0;
} 
