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

/* Conforming to ITU-T Reccomendation T.87 */

#include <stdio.h>
#include <math.h>
#include "type_defs.h"
#include "parameters.h"
#include "codingvars.h"
#include "pnm.h"
#include "golomb.h"
#include "bitstream.h"

static inline uint32 min(uint32 a,uint32 b){ return (a > b) ? b : a; }
static inline uint32 max(uint32 a,uint32 b){ return (a > b) ? a : b; }
static inline int32 CLAMP(int32 a,int32 b,int32 c){ return (a > c || a < b) ? b : a; }

#define		CONTEXTS	365

int main(int argc, char* argv[])
{

	parameters params;
	codingvars vars;	
	image_data* im_data = NULL;
	
	// parsing command line parameters and/or JPEG-LS header
	params = coding_parameters(argc, argv);

	if(params.decoding_flag == false)
	{
		// encoding process

		// loading image data
		im_data = load_image(params.input_file);

		// bitstream initialization
		init_bitstream(params.output_file, 'w');

		// setting parameters
		params.MAXVAL = im_data->maxval;
		if(params.specified_T == false)
		{
			/* C.2.4.1.1 Default threshold values */
			if(params.MAXVAL>=128)
			{
				FACTOR = floor((float64)(min(params.MAXVAL,4095)+128)/256);
				params.T1 = CLAMP(FACTOR*(vars.BASIC_T1-2)+2+3*params.NEAR,params.NEAR+1,params.MAXVAL);
				params.T2 = CLAMP(FACTOR*(vars.BASIC_T2-3)+3+5*params.NEAR,params.T1,params.MAXVAL);
				params.T3 = CLAMP(FACTOR*(vars.BASIC_T3-4)+4+7*params.NEAR,params.T2,params.MAXVAL);
			}
			else
			{
				FACTOR = floor( 256.0/(params.MAXVAL + 1) );
				params.T1 = CLAMP(max(2,floor((float64)vars.BASIC_T1/FACTOR)+3*params.NEAR),params.NEAR+1,params.MAXVAL);
				params.T2 = CLAMP(max(2,floor((float64)vars.BASIC_T2/FACTOR)+5*params.NEAR),params.T1,params.MAXVAL);
				params.T3 = CLAMP(max(2,floor((float64)vars.BASIC_T3/FACTOR)+7*params.NEAR),params.T2,params.MAXVAL);
			}
		}
		if(params.verbose)
		{
				fprintf(stdout,	"Encoding %s...\n\n\twidth\t\t%d\n\theight\t\t%d\n\tcomponents\t%d\n",
				params.input_file, im_data->width, im_data->height, im_data->n_comp);
				fprintf(stdout, "\tMAXVAL\t\t%d\n\tNEAR\t\t%d\n\tT1\t\t%d\n\tT2\t\t%d\n\tT3\t\t%d\n\tRESET\t\t%d\n",
				params.MAXVAL, params.NEAR, params.T1, params.T2, params.T3, params.RESET);
		}

		write_header(params, im_data);

	}
	else
	{
		// decoding process
	
		// bitstream initialization
		init_bitstream(params.input_file, 'r');
	}

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

	// data structures initializations should go here
	for(comp=0; comp<im_data->n_comp; comp++)
		for(row=0; row<im_data->height; row++)
			for(col=0; col<im_data->width; col++)
			{
				/* A.3 Context determination */

				// causal template construction
				// c b d
				// a x

				Rb = (row==0) ? 0 : im_data->image[comp][row-1][col];
				Rd = (col==im_data->width-1 || row==0) ? Rb : im_data->image[comp][row-1][col+1];
				if(col>0)
					Rc = (row==0) ? 0 : im_data->image[comp][row-1][col-1];
				else
					Rc = prevRa;
				Ra = (col==0) ? prevRa=Rb : im_data->image[comp][row][col-1];
		
				Ix = im_data->image[comp][row][col];

				/* A.3.1 Local gradient computation */

				D1 = Rd - Rb;
				D2 = Rb - Rc;
				D3 = Rc - Ra;

				/* A.3.2 Mode selection */

				if((abs(D1) <= params.NEAR)&&(abs(D2) <= params.NEAR)&&(abs(D3) <= params.NEAR))
					RunModeProcessing = true;
				else
					RunModeProcessing = false;
				
				if(!RunModeProcessing)	// regular mode
				{
					/* A.3.3 Local gradients quantization */

					if      (D1 <= -params.T3)   	Q1 =-4;
					else if (D1 <= -params.T2)   	Q1 =-3;
					else if (D1 <= -params.T1)   	Q1 =-2;
					else if (D1 <  -params.NEAR) 	Q1 =-1;
					else if (D1 <=  params.NEAR) 	Q1 = 0;
					else if (D1 <   params.T1)   	Q1 = 1;
					else if (D1 <   params.T2)   	Q1 = 2;
					else if (D1 <   params.T3)   	Q1 = 3;
					else                  		Q1 = 4;

					if      (D2 <= -params.T3)   	Q2 =-4;
					else if (D2 <= -params.T2)   	Q2 =-3;
					else if (D2 <= -params.T1)   	Q2 =-2;
					else if (D2 <  -params.NEAR) 	Q2 =-1;
					else if (D2 <=  params.NEAR) 	Q2 = 0;
					else if (D2 <   params.T1)   	Q2 = 1;
					else if (D2 <   params.T2)   	Q2 = 2;
					else if (D2 <   params.T3)   	Q2 = 3;
					else                  		Q2 = 4;

					if      (D3 <= -params.T3)   	Q3=-4;
					else if (D3 <= -params.T2)   	Q3=-3;
					else if (D3 <= -params.T1)   	Q3=-2;
					else if (D3 <  -params.NEAR) 	Q3=-1;
					else if (D3 <=  params.NEAR) 	Q3= 0;
					else if (D3 <   params.T1)   	Q3= 1;
					else if (D3 <   params.T2)   	Q3= 2;
					else if (D3 <   params.T3)   	Q3= 3;
					else 				Q3= 4;

					/* A.3.4 Quantized gradient merging */

					if( (Q1<0) || (Q1==0 && Q2<0) || (Q1==0 && Q2==0 && Q3<0) )
					{
						SIGN = -1;
						Q1 = -Q1;
						Q2 = -Q2;
						Q3 = -Q3;
					}
					else
						SIGN = 1;

					// one-to-one mapping of the vector (Q1,Q2,Q3) to the integer Q
					if (Q1 == 0)
					{
						if (Q2 == 0)
							Q=360+Q3;
						else
							Q=324+(Q2-1)*9+(Q3+4);
					}
					else
						Q=(Q1-1)*81+(Q2+4)*9+(Q3+4);

					/* A.4 Prediction*/

					/* A.4.1 Edge-detecting predictor */

					if(Rc>=max(Ra,Rb))
						Px = min(Ra,Rb);
					else
					{
						if(Rc<=min(Ra,Rb))
							Px = max(Ra,Rb);
						else
							Px = Ra + Rb - Rc;
					}

					/* A.4.2 Prediction correction */

					Px += (SIGN == 1)? C[Q] : -C[Q];

					if(Px>params.MAXVAL)
						Px = params.MAXVAL;
					else if(Px<0)
						Px = 0;

					if(params.decoding_flag == false) // encoding
					{

						/* A.4.2 Computation of prediction error */

						Errval = Ix - Px;
						if(SIGN==-1)
							Errval = -Errval;

						/* A.4.4 Error quantization for near-lossless coding, and reconstructed value computation */

						if(Errval>0)
							Errval = (Errval + params.NEAR)/(2*params.NEAR + 1);
						else
							Errval = -(params.NEAR - Errval)/(2*params.NEAR + 1);	

						Rx = Px + SIGN*Errval*(2*params.NEAR + 1);
						if(Rx<0)
							Rx = 0;
						else if(Rx>params.MAXVAL)
							Rx = params.MAXVAL;
						im_data->image[comp][row][col] = Rx;

						// modulo reduction of the error
						if(Errval<0)
							Errval = Errval + RANGE;
						if(Errval>=((RANGE + 1)/2))
							Errval = Errval - RANGE;

						/* A.5 Prediction error encoding */

						/* A.5.1 Golomb coding variable computation */

						for(k=0;(N[Q]<<k)<A[Q];k++);

						/* A.5.2 Error mapping */

						if((params.NEAR==0)&&(k==0)&&(2*B[Q]<=-N[Q]))
							if(Errval>=0)
								MErrval = 2*Errval + 1;
							else
								MErrval = -2*(Errval + 1);
						else
							if(Errval>=0)
								MErrval = 2*Errval;
							else
								MErrval = -2*Errval -1;

						/* A.5.3 Mapped-error encoding */

						limited_length_Golomb_encode(MErrval, k, LIMIT, qbpp);

					}
					else	// decoding
					{

						/* A.5.1 Golomb coding variable computation */

						for(k=0;(N[Q]<<k)<A[Q];k++);

						/* Mapped-error decoding */

						MErrval = limited_length_Golomb_decode(k, LIMIT, qbpp);

						/* Inverse Error mapping */

						if((params.NEAR==0)&&(k==0)&&(2*B[Q]<=-N[Q]))
							if(MErrval%2==0)
								Errval = -(MErrval / 2) - 1;
							else
								Errval = (MErrval - 1) / 2;
						else
							if(MErrval%2==0)
								Errval = MErrval / 2;
							else
								Errval = -(MErrval + 1) / 2;

						Errval = Errval * (2*params.NEAR + 1);

						if(SIGN==-1)
							Errval = -Errval;

						Rx = (Errval + Px) % ( RANGE*(2*params.NEAR+1) );

						if( Rx < -params.NEAR)
							Rx = Rx + RANGE*(2*params.NEAR+1);
						else if( Rx > params.MAXVAL + params.NEAR)
							Rx = Rx - RANGE*(2*params.NEAR+1);

						if( Rx<0 )
							Rx = 0;
						else if( Rx>params.MAXVAL)
							Rx = params.MAXVAL;


					}

					/* A.6 Update variables */

					/* A.6.1 Update */

					B[Q] += Errval*(2*params.NEAR + 1);
					A[Q] += abs(Errval);
					if(N[Q] == params.RESET)
					{
						A[Q] = A[Q]>>1;
						if(B[Q]>=0)
							B[Q] = B[Q]>>1;
						else
							B[Q] = -((1-B[Q])>>1);
						N[Q] = N[Q]>>1;
					}
					N[Q] += 1;

					/* A.6.2 Bias computation */

					if(B[Q] <= -N[Q])
					{
						B[Q] += N[Q];
						if(C[Q] > MIN_C)
							C[Q]--;
						if(B[Q] <= -N[Q])
							B[Q] = -N[Q] + 1;
					}
					else if(B[Q] > 0)
					{
						B[Q] -= N[Q];
						if(C[Q] < MAX_C)
							C[Q]++;
						if(B[Q] > 0)
							B[Q] = 0;
					}

				}
				else	// run mode
				{
					if(params.decoding_flag == false) // encoding
					{

						/* A.7.1 Run scanning and run-length coding */
			
						RUNval = Ra;
						RUNcnt = 0;
						while(abs(Ix - RUNval) <= params.NEAR)
						{
							RUNcnt += 1;
							Rx = RUNval;
							if(col == (im_data->width-1))
								break;
							else
							{
								col++;
								Ix = im_data->image[comp][row][col];
							}
						}

						/* A.7.1.2 Run-length coding */

						while(RUNcnt >= (1<<J[RUNindex]))
						{
							append_bit(1);
							RUNcnt -= (1<<J[RUNindex]);
							if(RUNindex<31)
								RUNindex += 1;
						}

						RUNindex_val = RUNindex;

						if(abs(Ix - RUNval) > params.NEAR)
						{
							append_bit(0);
							append_bits(RUNcnt,J[RUNindex]);
							if(RUNindex > 0)
								RUNindex -= 1;
						}
						else if(RUNcnt>0)
							append_bit(1);

						/* A.7.2 Run interruption sample encoding */

						// index computation
						if(abs(Ra - Rb) <= params.NEAR)
							RItype = 1;
						else
							RItype = 0;
	
						// prediction error for a run interruption sample
						if(RItype == 1)
							Px = Ra;
						else
							Px = Rb;
						Errval = Ix - Px;

						// error computation for a run interruption sample
						if((RItype == 0) && (Ra > Rb))
						{
							Errval = -Errval;
							SIGN = -1;
						}
						else
							SIGN = 1;
					
						if(params.NEAR > 0)
						{
							// error quantization
							if(Errval>0)
								Errval = (Errval + params.NEAR)/(2*params.NEAR + 1);
							else
								Errval = -(params.NEAR - Errval)/(2*params.NEAR + 1);

							// reconstructed value computation
							Rx = Px + SIGN*Errval*(2*params.NEAR + 1);
							if(Rx<0)
								Rx = 0;
							else if(Rx>params.MAXVAL)
								Rx = params.MAXVAL;
							im_data->image[comp][row][col] = Rx;
						}
		
						// modulo reduction of the error
						if(Errval<0)
							Errval = Errval + RANGE;
						if(Errval>=((RANGE + 1)/2))
							Errval = Errval - RANGE;

						// computation of the auxiliary variable TEMP
						if(RItype == 0)
							TEMP = A[365];
						else
							TEMP = A[366] + (N[366]>>1);

						// Golomb coding variable computation
						Q = RItype + 365;
						for(k=0;(N[Q]<<k)<TEMP;k++);

						// computation of map for Errval mapping
						if((k==0)&&(Errval>0)&&(2*Nn[Q-365]<N[Q]))
							map = 1;
						else if((Errval<0)&&(2*Nn[Q-365]>=N[Q]))
							map = 1;
						else if((Errval<0)&&(k!=0))
							map = 1;
						else
							map = 0;
			
						// Errval mapping for run interruption sample
						EMErrval = 2*abs(Errval) - RItype - map;

						// limited length Golomb encoding
						limited_length_Golomb_encode(EMErrval, k, LIMIT - J[RUNindex_val] - 1, qbpp);

						// update of variables for run interruption sample
						if(Errval<0)
							Nn[Q-365] = Nn[Q-365] + 1;
						A[Q] = A[Q] + ((EMErrval + 1 + RItype)>>1);
						if(N[Q] == params.RESET)
						{
							A[Q] = A[Q]>>1;
							N[Q] = N[Q]>>1;
							Nn[Q-365] = Nn[Q-365]>>1;
						}
					}
					else	// decoding
					{
						if( read_bit()==1)
						{
							;
						}
						else
						{
							;
						}
					}
				}
			}

	end_bitstream();
	//write_image("lena_decoded.ppm", im_data);

	return 0;
}
