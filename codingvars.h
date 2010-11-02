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

#ifndef __CODINGVARS_H__
#define __CODINGVARS_H__

#include "type_defs.h"

struct codingvars{

	bool RunModeProcessing;		// Regular/Run mode processing flag
	uint16 RANGE;			// range of prediction error representation
	uint8 bpp;			// number of bits needed to represent MAXVAL
	uint8 qbpp;			// number of bits needed to represent a mapped error value
	uint8 LIMIT;			// max length in bits of Golomb codewords in regular mode

	uint16 i;
	uint16 comp, row, col;

	int32 N[CONTEXTS + 2];		// occurrences counters for each context
	uint32 A[CONTEXTS + 2];		// prediction error magnitudes accumulator
	int32 B[CONTEXTS];		// bias values
	int32 C[CONTEXTS];		// prediction correction values
	uint8 RUNindex = 0;		// index for run mode order
	uint8 RUNindex_val;	
	uint16 RUNval;			// repetitive reconstructed sample value in a run
	uint16 RUNcnt;			// repetetive sample count for run mode
	uint32 TEMP;			// auxiliary variable for Golomb variable calculation in run interruption coding
	uint8 map;			// auxiliary variable for error mapping at run interruption
	uint8 RItype;			// index for run interruption coding
	
	uint8 J[] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7,8,9,10,11,12,13,14,15};  // order of run-length codes
	int32 Nn[2];			// counters for negative prediction error for run interruption

	uint32 A_init_value;

	uint16 Ra, Rb, Rc, Rd, prevRa=0, Ix;	// pixels used in the causal template
	int32 Rx;				// reconstructed value of the current sample
	int32 Px;				// predicted value for the current sample
	int32 Errval;				// prediction error
	uint32 MErrval;				// Errval mapped to a non-negative integer
	uint32 EMErrval;			// Errval mapped to non-negative integers in run interruption mode 

	int32 D1, D2, D3;			// local gradients
	int8 Q1, Q2, Q3;			// region numbers to quantize local gradients
	uint16 Q;				// context
	int8 SIGN;				// sign of the current context

	uint8 k;				// Golomb coding variable

	int32 MAX_C=127;			// maximum allowed value for C[0..364]
	int32 MIN_C=-128;			// minimum allowed value for C[0..364]

	uint16 BASIC_T1 = 3;			// basic default values for gradient quantization thresholds
	uint16 BASIC_T2 = 7;
	uint16 BASIC_T3 = 21;

	uint16 FACTOR;

} typedef codingvars;


#endif
