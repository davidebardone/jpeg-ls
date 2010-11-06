/*************************************************************************\
 * JPEG-LS Lossless image encoder/decoder                                *
 * Copyright (C) 2010 Davide Bardone <davide.bardone@gmail.com>          *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
\*************************************************************************/

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <stdio.h>
#include <stdlib.h>
#include "type_defs.h"
#include "parameters.h"
#include "pnm.h"

struct bitstream{
	FILE* bitstream_file;
	uint8 byte_bits;
	uint8 buffer;
	uint32 tot_bits;
} typedef bitstream;

void init_bitstream(char* filename, char mode);
void end_bitstream();
void print_bpp(image_data* im_data);

void append_bit(uint8 bit);
void append_bits(uint32 value, uint8 n_bits);

uint8 read_bit();

void write_header(parameters params, image_data* im_data);
image_data* read_header(parameters* params);

#endif
