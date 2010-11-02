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

#include "type_defs.h"

void init_codingvars(codingvars vars)
{
	/* A.2 Initializations and conventions */

	/* A.2.1 Initializations */

	RANGE = floor( (float64)(params.MAXVAL + 2*params.NEAR) / (2*params.NEAR + 1) ) + 1;
	qbpp = ceil( log2(RANGE) );
	bpp = max( 2, log2(params.MAXVAL + 1) );
	LIMIT = 2*( bpp + max(8,bpp) );

	A_init_value = max( 2, floor( (float64)(RANGE + 32)/64 ) );

	for(i=0; i<CONTEXTS; i++)
	{
		A[i] = A_init_value;
		N[i] = 1;
		B[i] = 0;
		C[i] = 0;
	}
	A[CONTEXTS] = A_init_value;
	A[CONTEXTS+1] = A_init_value;
	N[CONTEXTS] = 1;
	N[CONTEXTS+1] = 1;
	Nn[0] = 0;
	Nn[1] = 0;
}

