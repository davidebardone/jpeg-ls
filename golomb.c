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

#include "golomb.h"
#include "bitstream.h"

void limited_length_Golomb_encode(uint32 MErrval, uint8 k, uint8 LIMIT, uint8 qbpp)
{
	// A.5.3 Mapped-error encoding

	// bits generated are packed into 8-bit bytes
	// first output bit is on the msb

	uint32 hMErrval;
	uint8 lim = LIMIT-qbpp-1;

	hMErrval = MErrval>>k;

	if(hMErrval < lim)
	{
		// unary part		
		while(hMErrval>0)
		{
			append_bit(0);
			hMErrval--;
		}
		append_bit(1);
		//binary part
		while(k>0)
		{
			append_bit((MErrval>>(k-1))&0x1);
			k--;		
		}
	}
	else
	{
		// unary part		
		while(lim>0)
		{
			append_bit(0);
			lim--;
		}
		append_bit(1);
		//binary part
		while(qbpp>0)
		{
			append_bit(((MErrval-1)>>(qbpp-1))&0x1);
			qbpp--;		
		}
	}

	return;
}


uint32 limited_length_Golomb_decode(uint8 k, uint8 LIMIT, uint8 qbpp)
{
	
	uint32 MErrval = 0;
	uint8 lim = LIMIT-qbpp-1;

	while(read_bit()==0)
		MErrval++;

	if(MErrval<lim)
	{
		while(k>0)
		{
			MErrval = (MErrval<<1)|read_bit();
			k--;
		}
	}
	else
	{
		MErrval = 0;
		while(qbpp>0)
		{
			MErrval = (MErrval<<1)|read_bit();
			qbpp--;
		}
		MErrval += 1;
	}

	return MErrval;

}
