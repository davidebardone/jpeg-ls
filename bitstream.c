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

#include <stdio.h>
#include <stdlib.h>
#include "type_defs.h"
#include "bitstream.h"

bitstream bs;

uint8 bytemask[] = {0x0, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

void init_bitstream(char* filename, char mode)
{
	FILE* fp;
	if(mode=='w')
		fp=fopen(filename, "w");
	else if(mode=='r')		
		fp=fopen(filename, "r");
	else
	{
		fprintf(stderr, "%c : not valid bitstream mode.\n", mode);
		exit(EXIT_FAILURE);
	}

	if(fp==NULL)
	{
		fprintf(stderr, "Impossible to open/create file %s\n", filename);
		exit(EXIT_FAILURE);
	}
	bs.bitstream_file = fp;
	bs.byte_bits = 0;
}

void end_bitstream()
{
	fwrite(&bs.buffer,1,1,bs.bitstream_file);
	append_bits(0xffd9, 16);			// End Of Image (EOI) marker
	fclose(bs.bitstream_file);
	printf("%d\n", bs.tot_bits);
	return;
}

void append_bit(uint8 bit)
{
	//printf("%d",bit);
	bs.tot_bits++;
	if(bs.byte_bits==8)
	{
		fwrite(&bs.buffer,1,1,bs.bitstream_file);
		bs.buffer = 0;
		bs.byte_bits = 0;
	}

	bs.byte_bits++;
	if (bit==1)
		bs.buffer = bs.buffer | bytemask[bs.byte_bits];

	return;
}

void append_bits(uint32 value, uint8 bits)
{
	while(bits > 0)
	{
		append_bit((value>>(bits-1))&0x1);
		bits--;
	}
}

uint8 read_bit()
{
	uint8 bit;	

	if(bs.byte_bits==0)
	{
		if(fread(&bs.buffer,1,1,bs.bitstream_file)<1)
		{
			fprintf(stderr, "Unexpected end of file");		
			exit(EXIT_FAILURE);
		}
		bs.byte_bits = 8;
	}

	bit = bs.buffer | bytemask[bs.byte_bits];
	bs.byte_bits--;

	return bit;
}

void write_header(params_struct params, image_data* im_data)
{
	uint8 comp;

	append_bits(0xffd8, 16);			// Start Of Image (SOI) marker
	append_bits(0xfff7, 16);			// Start Of JPEG-LS frame (SOF55) marker

	append_bits(2+6+3*im_data->n_comp, 16);		// Length of marker segment
	append_bits(log2(params.MAXVAL + 1), 8);	// Precision (P)

	append_bits(im_data->height, 16);		// Number of lines (Y)
	append_bits(im_data->width, 16);		// Number of columns (X)

	append_bits(im_data->n_comp, 8);		// Number of components (Nf)

	for(comp=0;comp<im_data->n_comp;comp++)
	{
		append_bits(comp+1, 8);			// Component ID
		append_bits(0x11, 8);			// Sub-sampling H=1 V=1
		append_bits(0x0, 8);			// Tq	
	}

	append_bits(0xffda, 16);			// Start Of Scan (SOS) marker
	append_bits(3+3+2*im_data->n_comp, 16);		// Length of marker segment
	append_bits(im_data->n_comp, 8);		// Number of components for this scan (Ns)
	
	for(comp=0;comp<im_data->n_comp;comp++)
	{
		append_bits(comp+1, 8);			// Component ID
		append_bits(0x00, 8);			// Mapping table index (0 = no mapping table)
	}

	append_bits(params.NEAR, 8);			// Near-lossless maximum error
	append_bits(0x00, 8);				// ILV (0 = no interleave)
	append_bits(0x00, 8);				// Ai Ah (0 = no point transform)

}


