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

#ifndef __PNM_H__
#define __PNM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "type_defs.h"

#define 	HEAD_BUFF_LEN		20
#define 	PPM_COMPONENTS		3

struct image_data{
        uint16*** 	image;		// pointer to image data
        uint16 		width;		// max supported image size is 65535x65535 pixels (~4.3*10^9 pixels)
        uint16 		height;
        uint16 		maxval;		// max precision is 16bpp for each component
	uint8 		n_comp;		// number of components (max 255)
} typedef image_data;

image_data* load_image(char* image_name);
image_data*  allocate_image_data();
void allocate_image(image_data* im_data); 
void write_image(char* image_name, image_data* im_data);

#endif
