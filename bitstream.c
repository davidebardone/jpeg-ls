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

#include "bitstream.h"

bitstream bs;

uint8 w_bytemask[] = {0x0, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
uint8 r_bytemask[] = {0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

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
	return;
}

void print_bpp(image_data* im_data)
{
	float32 bpp = 	(float32)bs.tot_bits / (im_data->height * im_data->width * im_data->n_comp);
	fprintf(stdout, "\n%.2f bpp\n", bpp);
}

void append_bit(uint8 bit)
{
	bs.tot_bits++;
	if(bs.byte_bits==8)
	{
		fwrite(&bs.buffer,1,1,bs.bitstream_file);
		bs.buffer = 0;
		bs.byte_bits = 0;
	}

	bs.byte_bits++;
	if (bit==1)
		// starting from msb
		bs.buffer = bs.buffer | w_bytemask[bs.byte_bits];

	return;
}

void append_bits(uint32 value, uint8 bits)
{
	while(bits > 0)
	{
		// msb first
		append_bit((value>>(bits-1))&0x1);
		bits--;
	}
}

uint8 read_Byte_bit()
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

	// msb first
	bit = (bs.buffer & w_bytemask[bs.byte_bits])>>(8-bs.byte_bits);
	bs.byte_bits--;

	return bit;
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

	// msb first
	bit = (bs.buffer & r_bytemask[bs.byte_bits])>>(bs.byte_bits-1);
	bs.byte_bits--;

	return bit;
}

uint8 read_byte()
{
	uint8 B = 0x0;
	int8 cnt = 8;
	uint8 bit;
	while( cnt>0 )
	{
		bit = read_Byte_bit();
		B = B | (bit<<(8-cnt));
		cnt--;
	}
	return B;
}

uint16 read_word()
{
	uint16 W = 0x0;
	W = (W | (0x00FF & read_byte()))<<8;
	W = W | (0x00FF & read_byte());		
	return W;
}

void write_header(parameters params, image_data* im_data)
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


image_data* read_header(parameters* params)
{
	uint8 comp = 0;
	uint16 length;
	uint8 precision;
	image_data* im_data;

	if(read_word() != 0xffd8)
	{
		fprintf(stderr, "Invalid SOI marker.\n");
		exit(EXIT_FAILURE);
	}

	if(read_word() != 0xfff7)
	{
		fprintf(stderr, "Invalid JPEG-LS SOF55 marker.\n");
		exit(EXIT_FAILURE);
	}

	length = read_word();
	precision = read_byte();
	(*params).MAXVAL = (2<<(precision-1))-1;

	im_data = allocate_image_data();

	im_data->maxval = (*params).MAXVAL;
	im_data->height = read_word();
	im_data->width = read_word();

	im_data->n_comp = read_byte();

	for(comp=0;comp<im_data->n_comp;comp++)
	{
		read_byte();			// Component ID
		read_byte();			// Sub-sampling H=1 V=1
		read_byte();			// Tq	
	}

	if(read_word() != 0xffda)
	{
		fprintf(stderr, "Invalid SOS marker.\n");
		exit(EXIT_FAILURE);
	}
	
	length = read_word();
	read_byte();					// Number of components for this scan (Ns)
	
	for(comp=0;comp<im_data->n_comp;comp++)
	{
		read_byte();				// Component ID
		read_byte();				// Mapping table index (0 = no mapping table)
	}

	(*params).NEAR = read_byte();			// Near-lossless maximum error
	(*params).ILV = read_byte();			// ILV (0 = no interleave)
	read_byte();					// Ai Ah (0 = no point transform)

	return im_data;
}

