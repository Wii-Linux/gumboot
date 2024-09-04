/*
	mini - a Free Software replacement for the Nintendo/BroadOn IOS.
	PowerPC ELF file loading

Copyright (C) 2008, 2009	Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2009			Andre Heider "dhewg" <dhewg@wiibrew.org>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

#include "types.h"
#include "powerpc.h"
#include "hollywood.h"
#include "gecko.h"
#include "fatfs/ff.h"
#include "powerpc_elf.h"
#include "elf.h"
#include "malloc.h"
#include "string.h"
#include "mini_ipc.h"
#include "log.h"

#define PHDR_MAX 10

static Elf32_Ehdr elfhdr;
static Elf32_Phdr phdrs[PHDR_MAX];

int is_valid_elf(const char *path)
{
	u32 read;
	FIL fd;
	FRESULT fres;

	fres = f_open(&fd, path, FA_READ);
	if (fres != FR_OK) {
		log_printf("could not open ELF %s: %d\n", path, fres);
		return fres;
	}

	fres = f_read(&fd, &elfhdr, sizeof(elfhdr), &read);
	if (fres != FR_OK) {
		log_printf("could not read ELF %s: %d\n", path, fres);
		f_close(&fd);
		return fres;
	}

	if (read != sizeof(elfhdr)) {
		log_printf("expected %d bytes read, but got %d\n", path, sizeof(elfhdr), read);

		f_close(&fd);
		return -100;
	}

	if (memcmp("\x7F" "ELF\x01\x02\x01\x00\x00",elfhdr.e_ident,9)) {
		log_printf("Invalid ELF header! 0x%02x 0x%02x 0x%02x 0x%02x\n",elfhdr.e_ident[0], elfhdr.e_ident[1], elfhdr.e_ident[2], elfhdr.e_ident[3]);

		f_close(&fd);
		return -101;
	}

	// entry point not checked here

	if (elfhdr.e_phoff == 0 || elfhdr.e_phnum == 0) {
		log_printf("ELF has no program headers!\n");

		f_close(&fd);
		return -103;
	}

	if (elfhdr.e_phnum > PHDR_MAX) {
		log_printf("ELF has too many (%d) program headers!\n", elfhdr.e_phnum);

		f_close(&fd);
		return -104;
	}

	fres = f_lseek(&fd, elfhdr.e_phoff);
	if (fres != FR_OK) {
		log_printf("could not seek in ELF %s: %d\n", path, fres);

		f_close(&fd);
		return -fres;
	}

	fres = f_read(&fd, phdrs, sizeof(phdrs[0])*elfhdr.e_phnum, &read);
	if (fres != FR_OK) {
		log_printf("could not read headers from ELF %s: %d\n", path, fres);

		f_close(&fd);
		return -fres;
	}

	if (read != sizeof(phdrs[0])*elfhdr.e_phnum) {
		log_printf("expected %d bytes read from headers, but got %d\n", path, sizeof(phdrs[0])*elfhdr.e_phnum, read);

		f_close(&fd);
		return -105;
	}

	u16 count = elfhdr.e_phnum;
	Elf32_Phdr *phdr = phdrs;

	int found = 0;
	while (count--) {
		if (phdr->p_type == PT_LOAD) {
			found = 1;
			break;
		}
		phdr++;
	}
	if (!found) {
		log_printf("no PT_LOAD header found in ELF %s: %d\n", path, fres);

		f_close(&fd);
		return -200;
	}
	
	f_close(&fd);
	return 0;
}

static char *memstr(char *mem, unsigned int fsize, const char *s) {
	unsigned int i;
	int l = strlen(s);
	int match = 0;
	for(i=0;i<fsize;i++) {
		if (mem[i] == s[match]) {
			match++;
			if (match == l)
				break;
		} else {
			match = 0;
		}
	}
	
	// not found?
	if (match != l)
		return NULL;

	return mem + i - l;
}

const char 	*bootargs_end_marker = "mark.end=1",
			*bootargs_start_marker = "mark.start=1";

static int edit_bootargs(char *mem, unsigned int fsize, const char *args, char *default_args) {
	int l = strlen(args);
	if (!l)
		return 0;

	// find a match for start marker
	char *start = memstr(mem, fsize, bootargs_start_marker);
	if (!start)
		return -1;

	// find a match for end marker
	char *end = memstr(start, fsize-(start-mem), bootargs_end_marker);
	if (!end)
		return -2;
	
	// copy default args
	int offset = strlen(bootargs_start_marker);
	memcpy(default_args, start+offset, end-start-offset);
	default_args[end-start-offset] = 0x0;
		
	end += strlen(bootargs_end_marker);
	int span = (end-start);
	if (l > span)
		return -3;
	memcpy(start, args, l);
	
	// blank rest of bootargs
	int i;
	for(i=l;i<span;i++) {
		start[i] = ' ';
	}

	return 0;
}

// will boot via IPC call to MINI
int powerpc_boot_file(const char *path, const char *args) {
	FRESULT res;
	FILINFO stat;
	FIL fd;
	
	// display information about the booting kernel
	select_font(FONT_HEADING);
	gfx_printf_at(0, 2, "Loading %s...", path);

	res = f_stat(path, &stat);
	if (res != FR_OK) {
		log_printf("could not stat %s: %d\n", path, res);
		return res;
	}

	unsigned int fsize = stat.fsize;
	char *mem = malloc(fsize);
	if (!mem) {
		log_printf("could not allocate %d bytes\n", fsize);
		return -300;
	}

	res = f_open(&fd, path, FA_READ);
	if (res != FR_OK) {
		log_printf("could not open %s: %d\n", path, res);
		return res;
	}
	
	unsigned int read;
	res = f_read(&fd, mem, fsize, &read);
	if (res != FR_OK) {
		log_printf("could not read %d bytes: %d\n", fsize, res);

		free(mem);
		f_close(&fd);
		
		return res;
	}
	if (read != fsize) {
		log_printf("read %d bytes but %d expected\n", read, fsize);
		return -200;
	}
	
	char default_args[1024];
	int err = edit_bootargs(mem, fsize, args, default_args);
	if (err) {
		log_printf("could not edit bootargs: %d\n", err);
		return err;
	}

	if (!strlen(args))
		args = default_args;
	
	// this will be shown for really little time, unless boot via MINI IPC fails
	gfx_clear(0, 2, CONSOLE_COLUMNS-2, 1, config_color_highlight[1]);
	gfx_printf_at(0, 2, "Booting %s... [%s]", path, args);

	err = ipc_powerpc_boot(mem, fsize);
	if (err)
		// in case of error, clear the 'Booting' message
		gfx_clear(0, 2, CONSOLE_COLUMNS-2, 1, config_color_heading[1]);
	return err;
}

int try_boot_file(char *kernel_fn, const char *args) {
	// sanity check
	int err = is_valid_elf(kernel_fn);
	if (err)
		// error message printed by function
		return err;
	
	//TODO: shutdown video to see if kernel 2.6 recovers
	//VIDEO_Shutdown();
	
	err = powerpc_boot_file(kernel_fn, args);
	if (err) {
		log_printf("MINI boot failed: %d\n", err);
		return err;
	}
	
	return 0;
}
