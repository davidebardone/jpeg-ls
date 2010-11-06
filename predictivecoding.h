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

#ifndef __PREDICTIVECODING_H__
#define __PREDICTIVECODING_H__

#include <math.h>
#include "type_defs.h"
#include "golomb.h"
#include "bitstream.h"
#include "parameters.h"
#include "codingvars.h"

void context_determination(codingvars* vars, parameters params, image_data* im_data);
void predict_sample_value(codingvars* vars, parameters params);
void encode_prediction_error(codingvars* vars, parameters params, image_data* im_data);
void decode_prediction_error(codingvars* vars, parameters params, image_data* im_data);
void encode_run(codingvars* vars, parameters params, image_data* im_data);
void decode_run(codingvars* vars, parameters params, image_data* im_data);

#endif
