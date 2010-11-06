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

#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_defs.h"

#define MAX_FILENAME_LEN	100

struct parameters{
	char input_file[MAX_FILENAME_LEN];	// input filename
	char output_file[MAX_FILENAME_LEN];	// output filename
	bool decoding_flag;			// encoding/decoding flag
	bool verbose;				// verbose flag
	uint8 NEAR;				// difference bound for near lossless coding
	uint16 MAXVAL;				// max image sample value
	uint16 T1, T2, T3;			// thresholds for local gradients
	bool specified_T;
	uint16 RESET;				// threshold value at which A,B, and N are halved
	uint8 ILV;				// interleave     
} typedef parameters;

parameters coding_parameters(int argc, char* argv[]);

#endif
