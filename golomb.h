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

#ifndef __GOLOMB_H__
#define __GOLOMB_H__

#include <stdio.h>
#include <stdlib.h>
#include "type_defs.h"

void limited_length_Golomb_encode(uint32 MErrval, uint8 k, uint8 LIMIT, uint8 qbpp);
uint32 limited_length_Golomb_decode(uint8 k, uint8 LIMIT, uint8 qbpp);

#endif
