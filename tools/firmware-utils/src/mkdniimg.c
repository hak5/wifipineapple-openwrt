/*
 *  Copyright (C) 2009 Gabor Juhos <juhosg@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>     /* for unlink() */
#include <libgen.h>
#include <getopt.h>     /* for getopt() */
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>

#define DNI_HDR_LEN	128

#define COL(x)      "\033[;" #x "m"  
#define RED         COL(31)  
#define GREEN       COL(32)  
#define YELLOW      COL(33)  
#define BLUE        COL(34)  
#define MAGENTA     COL(35)  
#define CYAN        COL(36)  
#define WHITE       COL(0)  
#define GRAY        "\033[0m"  

/*
 * Globals
 */
static char *ifname;
static char *progname;
static char *ofname;
static char *version = "1.00.00";
static char *region = "";
static char *hd_id;

static char *board_id;

/*
 * special board that do not have infomation header
 * */
static char *board_spec[] = {
	"PINEAPPLE_TETRA",
	"WNDR4300",
//  "WNDR3700v4",
};

/*
 * Message macros
 */
#define ERR(fmt, ...) do { \
fflush(0); \
fprintf(stderr, "[%s] *** error: " fmt "\n", \
	progname, ## __VA_ARGS__ ); \
} while (0)

#define ERRS(fmt, ...) do { \
int save = errno; \
fflush(0); \
fprintf(stderr, "[%s] *** error: " fmt "\n", \
	progname, ## __VA_ARGS__, strerror(save)); \
} while (0)

void usage(int status)
{
	FILE *stream = (status != EXIT_SUCCESS) ? stderr : stdout;
	struct board_info *board;

	fprintf(stream, "Usage: %s [OPTIONS...]\n", progname);
	fprintf(stream,
		"\n"
		"Options:\n"
		"  -B <board>      create image for the board specified with <board>\n"
		"  -i <file>       read input from the file <file>\n"
		"  -o <file>       write output to the file <file>\n"
		"  -v <version>    set image version to <version>\n"
		"  -r <region>     set image region to <region>\n"
		"  -H <hd_id>      set image hardware id to <hd_id>\n"
		"  -h              show this screen\n"
		);

	exit(status);
}

char is_board_spec(char **board_buf, char *board_id, 
	unsigned int board_buf_len)
{
	unsigned int index;
	char is_spec = 0;

	for(index=0; index<board_buf_len; ++index){
		if(strcmp(board_buf[index], board_id) == 0){
			is_spec = 1;
			break;
		}
	}
	
	return is_spec;
}

int main(int argc, char *argv[])
{
	int res = EXIT_FAILURE;
	int buflen;
	int err;
	struct stat st;
	char *buf = NULL;
	int pos, rem, i;
	uint8_t csum;
	char is_spec = 0;

	FILE *outfile, *infile;

	progname = basename(argv[0]);

	while ( 1 ) {
		int c;

		c = getopt(argc, argv, "B:i:o:v:r:H:h");
		if (c == -1)
			break;

		switch (c) {
			case 'B':
			board_id = optarg;
			break;
			case 'i':
			ifname = optarg;
			break;
			case 'o':
			ofname = optarg;
			break;
			case 'v':
			version = optarg;
			break;
			case 'r':
			region = optarg;
			break;
			case 'H':
			hd_id = optarg;
			break;
			case 'h':
			usage(EXIT_SUCCESS);
			break;
			default:
			usage(EXIT_FAILURE);
			break;
		}
	}

	if (board_id == NULL) {
		ERR("no board specified");
		goto err;
	}
	
	is_spec = is_board_spec(board_spec, board_id,
		sizeof(board_spec)/sizeof(board_spec[0]));

	if (ifname == NULL) {
		ERR("no input file specified");
		goto err;
	}

	if (ofname == NULL) {
		ERR("no output file specified");
		goto err;
	}

	err = stat(ifname, &st);
	if (err){
		ERRS("stat failed on %s", ifname);
		goto err;
	}

	if(is_spec)
		buflen = st.st_size;
	else
		buflen = st.st_size + DNI_HDR_LEN + 1;

	buf = malloc(buflen);
	memset(buf, 0, buflen);

	if (!buf) {
		ERR("no memory for buffer\n");
		goto err;
	}

	if(!is_spec){
		pos = snprintf(buf, DNI_HDR_LEN, "device:%s\nversion:V%s\nregion:%s\n",
			board_id, version, region);
		rem = DNI_HDR_LEN - pos;
		if (pos >= 0 && rem > 1 && hd_id) {
			snprintf(buf + pos, rem, "hd_id:%s\n", hd_id);
		}

	}

	infile = fopen(ifname, "r");
	if (infile == NULL) {
		ERRS("could not open \"%s\" for reading", ifname);
		goto err_free;
	}

	errno = 0;

	if(is_spec)
		fread(buf, st.st_size, 1, infile);
	else
		fread(buf + DNI_HDR_LEN, st.st_size, 1, infile);

	if (errno != 0) {
		ERRS("unable to read from file %s", ifname);
		goto err_close_in;
	}

	if(!is_spec){
		csum = 0;
		for (i = 0; i < (st.st_size + DNI_HDR_LEN); i++)
			csum += buf[i];

		csum = 0xff - csum;
		buf[st.st_size + DNI_HDR_LEN] = csum;
	}

	outfile = fopen(ofname, "w");
	if (outfile == NULL) {
		ERRS("could not open \"%s\" for writing", ofname);
		goto err_close_in;
	}

	errno = 0;
	fwrite(buf, buflen, 1, outfile);
	if (errno) {
		ERRS("unable to write to file %s", ofname);
		goto err_close_out;
	}

	res = EXIT_SUCCESS;

	out_flush:
	fflush(outfile);

	err_close_out:
	fclose(outfile);
	if (res != EXIT_SUCCESS) {
		unlink(ofname);
	}

	err_close_in:
	fclose(infile);

	err_free:
	free(buf);

	err:
	return res;
}