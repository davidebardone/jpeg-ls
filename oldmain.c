#include <stdio.h>
#include <pam.h>
#include <glib.h>
#include <string.h>
#include <netinet/in.h> 
#include <math.h>
#include "type_def.h"
#include "golomb.h"
#include "options.h"
#include "jpeg_headers.h"
#include "run_length.h"


static inline Uint32 Minimum(Uint32 a,Uint32 b){ return (a > b) ? b : a; }
static inline Uint32 Maximum(Uint32 a,Uint32 b){ return (a > b) ? a : b; }
static inline Uint32 Abs(Int32 x) { return (Uint32)((x < 0) ? -x : x); }
static inline double log2(double x) { return log(x)/M_LN2; }
static inline double Log(double x)	{ return (log2(x)); }
static inline Uint32 Ceiling(double x)	{ return ceil(x); }
static inline Uint32 FloorDivision(Uint32 n,Uint32 d){ return floor(n/d); }

static inline void clampPredictedValue(Int32 *X,Int32 MAXVAL){
	if      ((*X) > MAXVAL)	(*X)=MAXVAL;
	else if ((*X) < 0)	(*X)=0;
}

static Uint16 determineGolombParameter(Uint32 n,Uint32 a){
	Uint16 k;
	for (k=0;(n<<k) < a; ++k);	// Number of occurrences vs accumulated error magnitude	
	return k;
}


Uint16 getGvalue(Uint16 RGvalue){

	Uint16 value = 0;
	int i = 0;
	int j = 0;

	for(i=0;i<16;i++){
		value |= (((RGvalue>>i)&0x01)<<j);
		j++;		
		i++;
	}

	return value;

}

Uint16 getRvalue(Uint16 RGvalue){

	Uint16 value = 0;
	int i = 0;
	int j = 0;

	for(i=1;i<16;i++){
		value |= (((RGvalue>>i)&0x01)<<j);
		j++;		
		i++;
	}

	return value;

}


Uint16 joinFreq16[365][131071];
Uint16 joinFreq8[365][511];
Uint16 Freq16[131071];
Uint16 Freq8[511];
Uint16 condFreqCnt[365];
Uint32 n_of_pixs = 0;

void initStats(int rows, int cols, Uint16 bpp);

void updateStatistics(Uint16 Q, Int32 Errval, Uint16 bpp);

void printCondEntropy(Uint16 bpp);

void updateRGmeans(double *mr, double *mg, int *cnt, double mr_val, double mg_val);

Uint16 computeContext(Int16 Ra, Int16 Rb, Int16 Rc, Int16 Rd, Uint16 T1, Uint16 T2, Uint16 T3);


int main(int argc, char *argv[]){

	FILE* errs = fopen("residuals.mat","w");
	FILE* cont = fopen("contexts.mat","w");

	FILE* resG = fopen("residualsG.mat","w");
	FILE* resR = fopen("residualsR.mat","w");
	FILE* resB = fopen("residualsB.mat","w");


	//int gctx[512][512];
	//int rctx[512][512];

	
	/* Command line options */
	

	// image dimensions
	int colsP = -1;
	int rowsP = -1;
	// max image pixel value	
	pixval maxvalP = 255;
	
	// decoding flag
	int decompressing = 0;
	// run mode flag
	int useRunMode = 1;
	// jpeg-ls headers flag
	int useJpegHeaders = 1;

	//options strings
	
	GString *in_image = NULL;	// image to be encoded

	GString *out_image = NULL;	// decoded image name
	GString *r_bs_file = NULL;	// r channel bitstream
	GString *g_bs_file = NULL;	// g channel bitstream
	GString *b_bs_file = NULL;	// b channel bitstream


	readCommandLineOpts(argc, argv, &in_image, &out_image, &r_bs_file, &g_bs_file, &b_bs_file, &colsP, &rowsP, &maxvalP, &decompressing);



	
	/* Files, pixels arrays, coding structures */


	FILE* image_file = NULL;	// image file to be encoded
	
	FILE* bitstream_r = NULL;	// r channel bitstream file
	FILE* bitstream_g = NULL;	// g channel bitstream file
	FILE* bitstream_b = NULL;	// b channel bitstream file

	GRCoder *gr_r;			// r channel Golomb coder
	GRCoder *gr_g;			// g channel Golomb coder
	GRCoder *gr_b;			// b channel Golomb coder

	pixel** image;			// encoded/decoded image

	//encoder	
	if(!decompressing){	
		
		image_file = fopen(in_image->str,"r");		

		if(image_file==NULL){
			printf("Cannot find %s\n", in_image->str);
			exit(1);
		}

		image = ppm_readppm(image_file, &colsP, &rowsP, &maxvalP);	// array of pixels to be encoded
		
		// creating bitstream files
		char **tempBuf=g_strsplit(in_image->str,".",2);
		GString *bs = g_string_new(tempBuf[0]);
		bs = g_string_append(bs, "_r.bs");
		bitstream_r = fopen(bs->str, "w");
		bs = g_string_truncate(bs, strlen(tempBuf[0]));
		bs = g_string_append(bs, "_g.bs");
		bitstream_g = fopen(bs->str, "w");
		bs = g_string_truncate(bs, strlen(tempBuf[0]));
		bs = g_string_append(bs, "_b.bs");
		bitstream_b = fopen(bs->str, "w");
		g_strfreev(tempBuf);
		g_string_free(in_image, 1);
		g_string_free(bs, 1);		
	}

	// decoder
	else{
		image_file = fopen(out_image->str,"w");			// creating decoded image file
		
		image = ppm_allocarray(colsP,rowsP); 			// decoded pixels array
		
		// opening bitstream files
		bitstream_r = fopen(r_bs_file->str, "r");
		if(bitstream_r==NULL){
			printf("Cannot find %s\n", r_bs_file->str);
			exit(1);
		}
		bitstream_g = fopen(g_bs_file->str, "r");
		if(bitstream_g==NULL){
			printf("Cannot find %s\n", g_bs_file->str);
			exit(1);
		}
		bitstream_b = fopen(b_bs_file->str, "r");
		if(bitstream_b==NULL){
			printf("Cannot find %s\n", b_bs_file->str);
			exit(1);
		}

		g_string_free(out_image, 1);
		g_string_free(r_bs_file, 1);
		g_string_free(g_bs_file, 1);
		g_string_free(b_bs_file, 1);
	}
	

	// Golomb coder - bitstream association
	gr_r = gr_new(bitstream_r);
	gr_g = gr_new(bitstream_g);
	gr_b = gr_new(bitstream_b);




	/* LOCOI parameters */ 


	Uint16 NEAR=0;	
	Uint16 T1=0;
	Uint16 T2=0;
	Uint16 T3=0;
	Uint16 RESET=0;
	
	Uint32 ROWS=0;
	Uint32 COLUMNS=0;

	Uint32 MAXVAL=0;
	
	Uint16 P=0;

	if(decompressing && useJpegHeaders){
		// parsing JPEG-LS headers
		parseJpegHeaders(gr_g->bs,&P,&ROWS,&COLUMNS,&MAXVAL,&T1,&T2,&T3,&RESET,&NEAR);
		//printf("T1 %d T2 %d T3 %d max %d reset %d\n", T1, T2, T3, MAXVAL, RESET);
		//printf("P %d ROWS %d COL %d\n", P, ROWS, COLUMNS);
	}

	if(NEAR!=0){
		printf("Only lossless mode supported!\n");
		exit(1);
	}


	if(!decompressing){
		ROWS = rowsP;
		COLUMNS = colsP;
		MAXVAL = maxvalP;
	}
	
	if(!ROWS) ROWS=rowsP;

	if(!COLUMNS) COLUMNS=colsP;

	// bit per pixel
	if (!P) P = Maximum(2,Ceiling(Log(maxvalP+1)));	

	if (!MAXVAL) MAXVAL=(1ul<<P)-1;

	if (!RESET) RESET=64;	

	Uint16 bpp = P;	

	// context gradients default thresholds

	Uint32 BASIC_T1;
	Uint32 BASIC_T2;
	Uint32 BASIC_T3;

	if(bpp<=8){
		BASIC_T1 = 3;
		BASIC_T2 = 7;
		BASIC_T3 = 21;
	}
	else if(bpp>8){
		BASIC_T1 = 18;
		BASIC_T2 = 67;
		BASIC_T3 = 276;
	}


	// the value of glimit for a sample encoded in regular mode	
	Uint16 LIMIT = 2*(bpp+Maximum(8,bpp));	

	/*if(bpp<=8)
		LIMIT=23;	
	else if(bpp>8)
		//LIMIT=47;
		LIMIT=35;*/

	//printf("\tT1 : %d , T2 : %d,  T3 : %d,  LIMIT : %d MAXVAL %d\n", BASIC_T1, BASIC_T2, BASIC_T3, LIMIT, MAXVAL);

	#define CLAMP_1(i)	((i > MAXVAL || i < NEAR+1) ? NEAR+1 : i)
	#define CLAMP_2(i)	((i > MAXVAL || i < T1) ? T1 : i)
	#define CLAMP_3(i)	((i > MAXVAL || i < T2) ? T2 : i)

	// setting T1, T2, T3 values 
		
	if (MAXVAL >= 128) {
		Uint32 FACTOR=FloorDivision(Minimum(MAXVAL,4095)+128,256);
		if (!T1) T1=CLAMP_1(FACTOR*(BASIC_T1-2)+2+3*NEAR);
		if (!T2) T2=CLAMP_2(FACTOR*(BASIC_T2-3)+3+5*NEAR);
		if (!T3) T3=CLAMP_3(FACTOR*(BASIC_T3-4)+4+7*NEAR);
	}
	else {
		Uint32 FACTOR=FloorDivision(256,MAXVAL+1);
		if (!T1) T1=CLAMP_1(Maximum(2,BASIC_T1/FACTOR+3*NEAR));	
		if (!T2) T2=CLAMP_2(Maximum(3,BASIC_T2/FACTOR+5*NEAR));
		if (!T3) T3=CLAMP_3(Maximum(4,BASIC_T3/FACTOR+7*NEAR));
	}
		
	Int32 RANGE = FloorDivision(MAXVAL+2*NEAR,2*NEAR+1)+1;
	
	// Number of bits needed to represent MAXVAL with a minumum of 2
	// Uint16 bpp = Maximum(2,Ceiling(Log(MAXVAL+1)));	
	
	// Number of bits needed to represent a mapped error value
	Uint16 qbpp = Ceiling(Log(RANGE));


	const Uint16 nContexts = 365;	
	
	// limit C array values (used in bias correction) 
	const Int32 MIN_C = -128;	
	const Int32 MAX_C =  127;

	
	Uint32 A_Init_Value = Maximum(2,FloorDivision(RANGE+(1lu<<5),(1lu<<6)));
	
	/// counters for context type occurence [0..nContexts+2-1]
	// [nContexts],[nContexts+1] for run mode interruption
	Int32 N[367]; 
		
	// accumulated prediction error magnitude [0..nContexts-1]
	// [nContexts],[nContexts+1] for run mode interruption
	Uint32 A[367];		
		
	// auxilliary counters for bias cancellation [0..nContexts-1]
	Int32 B[365];
		
	// counters indicating bias correction value [0..nContexts-1]

	Int32 C[365];	 

	Uint32 row=0;
		
	pixel *thisRow = NULL;
	pixel *prevRow = NULL;

	Int32 Rx;	// Reconstructed value - not Uint16 to allow overrange before clamping

	// value at edges (first row and first col is zero) ...

	// valore dei pixel adiacenti nel pattern			
				
	/*Uint16 Ra = 0;
	Uint16 Rb = 0;
	Uint16 Rc = 0;
	Uint16 Rd = 0;*/

	Int16 Ra = 0;
	Int16 Rb = 0;
	Int16 Rc = 0;
	Int16 Rd = 0;

	Uint32 col=0;
	Uint16 prevRa0 = 0;

	Int32 D1, D2, D3;
	Int16 Q1, Q2, Q3;

	Int16 SIGN;

	Uint16 Q = 0;
	Int32 Px;	// Predicted value - not Uint16 to allow overrange before clamping

	Uint16 k = 0;			
	Uint32 MErrval = 0;
	Int32 Errval = 0;
	Int32 updateErrval = 0;

	Int32 Ix = 0;	// Input value - not Uint16 to allow overrange before clamping


	if (!decompressing && useJpegHeaders)  {
		writeSOI(gr_g->bs);
		writeSOF55(gr_g->bs,P,ROWS,COLUMNS);
		//writeLSE1(gr_g->bs,MAXVAL,T1,T2,T3,RESET);
		writeSOS(gr_g->bs,NEAR);
	}	

	int contextsCnt[365];
	double con_mr[365];
	double con_mg[365];


	// G, R e B planes encoding/decoding
	
	unsigned h, i;

	Int32 uno=0;
	Int32 due=0;
	Int32 tre=0;

	Int32 Gerr[1024][1024];	

	for(h=0; h<3; ++h){
		double mr, mg, mrg;
		double sr, sg, crg;
		double rho = 0;

		for(i=0;i<365; i++){
			contextsCnt[i]=0;
			con_mr[i]=0;
			con_mg[i]=0;
		}

		initStats(ROWS, COLUMNS, bpp);

		if(!decompressing){
			if(h==0)
				printf("Encoding G bitstream...\n");
			if(h==1)
				printf("Encoding R bitstream...\n");
			if(h==2)
				printf("Encoding B bitstream...\n");
		}
		else{
			if(h==0)
				printf("Decoding G bitstream...\n");
			if(h==1)
				printf("Decoding R bitstream...\n");
			if(h==2)
				printf("Decoding B bitstream...\n");
		}


		// Inizializzazione delle variabili
	
		for (i=0; i<nContexts; ++i) {
			N[i] = 1;
			A[i] = A_Init_Value;
			B[i] = 0;
			C[i] = 0;
		}
	
		N[nContexts] = 1;
		N[nContexts+1] = 1;
		A[nContexts] = A_Init_Value;
		A[nContexts+1] = A_Init_Value;
		

		initRunLengthVars();
			
		for (row=0; row<ROWS; ++row) {

			Uint16 segmStart = 0;
			Uint16 segmEnd = 0;

			//leggo la riga attuale e la metto in thisRow
			thisRow = image[row];	

			if(row>0)
				prevRow = image[row-1];				

			for (col=0; col<COLUMNS; ++col) {
				mr = mg = mrg = 0;
				sr = sg = crg = 0;

				//	pattern:
				//
				//	c b d .
				//	a x . .
				//	. . . .
				
				Uint16 gRa, gRb, gRc, gRd;

				if(h==0){
					if (row > 0) {
						Rb=(prevRow[col]).g;
						Rc=(col > 0) ? prevRow[col-1].g : prevRa0;
						Ra=(col > 0) ? thisRow[col-1].g : (prevRa0=Rb);
						Rd=(col+1 < COLUMNS) ? prevRow[col+1].g : Rb;
					}
					else {
						Rb=Rc=Rd=0;
						Ra=(col > 0) ? thisRow[col-1].g : (prevRa0=0);
					}
				}
				
				if(h==1){
					if (row > 0) {

						Rb=(prevRow[col]).r;
						Rc=(col > 0) ? prevRow[col-1].r : prevRa0;
						Ra=(col > 0) ? thisRow[col-1].r : (prevRa0=Rb);
						Rd=(col+1 < COLUMNS) ? prevRow[col+1].r : Rb;

						gRb=(prevRow[col]).g;
						gRc=(col > 0) ? prevRow[col-1].g : gRb;
						gRa=(col > 0) ? thisRow[col-1].g : gRb;
						gRd=(col+1 < COLUMNS) ? prevRow[col+1].g : gRb;

						/***/

						/*if(col>1){
							Int16 d = thisRow[col-2].r-thisRow[col-1].r;
							if(abs(d)>10 && abs(d)<15){
								segmStart = col-1;
								segmEnd = col-1;
							}
							else
								segmEnd++;
						}

						int m;

						for(m=segmStart;m<=segmEnd;m++){
							mr += thisRow[m].r;
							mg += thisRow[m].g;
						}
			
						mr /= segmEnd - segmStart + 1;
						mg /= segmEnd - segmStart + 1;

						for(m=segmStart;m<=segmEnd;m++){
							sr += (thisRow[m].r-mr)*(thisRow[m].r-mr);
							sg += (thisRow[m].g-mg)*(thisRow[m].g-mg);
							crg += (thisRow[m].r-mr)*(thisRow[m].g-mg);
						}*/

						/***/

						mr = (Ra + Rb + Rc + Rd)/4;
						mg = (gRa + gRb + gRc + gRd)/4;
					
								/* we try to use all the pixel values in the same context */

								/***/

								//Q = computeContext(Ra-gRa, Rb-gRb, Rc-gRc, Rd-gRd, T1, T2, T3);

								//updateRGmeans(&con_mr[Q],&con_mg[Q],&contextsCnt[Q], mr,mg);

								//mr = con_mr[Q];
								//mg = con_mg[Q];

								/***/

						sr = (Ra - mr) * (Ra - mr) + (Rb - mr) * (Rb - mr) + (Rc - mr) * (Rc - mr) + (Rd - mr) * (Rd - mr);
						sg = (gRa - mg) * (gRa - mg) + (gRb - mg) * (gRb - mg) + (gRc - mg) * (gRc - mg) + (gRd - mg) * (gRd - mg);
						crg = (Ra - mr) * (gRa - mg) + (Rb - mr) * (gRb - mg) + (Rc - mr) * (gRc - mg) + (Rd - mr) * (gRd - mg);

						if(sr > 0 && sg > 0)
							rho = crg / sqrt(sr * sg);
						else
							rho = 1.0;
						rho = 0;

						Ra = Ra - (int)floor(rho * (double)gRa + .5);
						Rb = Rb - (int)floor(rho * (double)gRb + .5);
						Rc = Rc - (int)floor(rho * (double)gRc + .5);
						Rd = Rd - (int)floor(rho * (double)gRd + .5);
					}
					else {
						Rb=Rc=Rd=0;
						Ra=(col > 0) ? thisRow[col-1].r : (prevRa0=0);
						Ra=(col > 0) ? thisRow[col-1].r  - thisRow[col - 1].g : (prevRa0=0);
					}
				}

				if(h==2){
					if (row > 0) {

						//Uint16 gRa, gRb, gRc, gRd;			
	
						Rb=(prevRow[col]).b;
						Rc=(col > 0) ? prevRow[col-1].b : prevRa0;
						Ra=(col > 0) ? thisRow[col-1].b : (prevRa0=Rb);
						Rd=(col+1 < COLUMNS) ? prevRow[col+1].b : Rb;

						gRb=(prevRow[col]).g;
						gRc=(col > 0) ? prevRow[col-1].g : gRb;
						gRa=(col > 0) ? thisRow[col-1].g : gRb;
						gRd=(col+1 < COLUMNS) ? prevRow[col+1].g : gRb;


						mr = (Ra + Rb + Rc + Rd)/4;
						mg = (gRa + gRb + gRc + gRd)/4;

						/*updateRGmeans(&con_mr[Q],&con_mg[Q],&contextsCnt[Q], mr,mg);

						mr = con_mr[Q];
						mg = con_mg[Q];*/

						sr = (Ra - mr) * (Ra - mr) + (Rb - mr) * (Rb - mr) + (Rc - mr) * (Rc - mr) + (Rd - mr) * (Rd - mr);
						sg = (gRa - mg) * (gRa - mg) + (gRb - mg) * (gRb - mg) + (gRc - mg) * (gRc - mg) + (gRd - mg) * (gRd - mg);
						crg = (Ra - mr) * (gRa - mg) + (Rb - mr) * (gRb - mg) + (Rc - mr) * (gRc - mg) + (Rd - mr) * (gRd - mg);

						if(sr > 0 && sg > 0)
							rho = crg / sqrt(sr * sg);
						else
							rho = 1.0;
						rho = 0;

						Ra = Ra - (int)floor(rho * (double)gRa + .5);
						Rb = Rb - (int)floor(rho * (double)gRb + .5);
						Rc = Rc - (int)floor(rho * (double)gRc + .5);
						Rd = Rd - (int)floor(rho * (double)gRd + .5);

					}
					else {
						Rb=Rc=Rd=0;
						Ra=(col > 0) ? thisRow[col-1].b : (prevRa0=0);
					}
				}
									
				// NB. We want the Reconstructed values, which are the same
				// in lossless mode, but if NEAR != 0 take care to write back
				// reconstructed values into the row buffers in previous positions

				// Calcolo dei gradienti locali


				D1=(Int32)Rd-Rb;
				D2=(Int32)Rb-Rc;
				D3=(Int32)Rc-Ra;				


				// Check for run mode ... 

				if (Abs(D1) <= NEAR && Abs(D2) <= NEAR && Abs(D3) <= NEAR && useRunMode) {

					// Run mode

					if (decompressing) 
						decodeRunLength(h,&Ra,&Rb,&col,&row,image,prevRow,COLUMNS,gr_r,gr_g,gr_b,RANGE,NEAR,MAXVAL,qbpp,LIMIT,RESET,A,N);

					else
						encodeRunLength(h,&Ra,&Rb,&col,&row,thisRow,prevRow,COLUMNS,gr_r,gr_g,gr_b,RANGE,NEAR,MAXVAL,qbpp,LIMIT,RESET,A,N);

				}	
				else{
				
					// Regular mode

					// Quantizzazione dei gradienti
				
				
					if      (D1 <= -T3)   Q1=-4;
					else if (D1 <= -T2)   Q1=-3;
					else if (D1 <= -T1)   Q1=-2;
					else if (D1 <  -NEAR) Q1=-1;
					else if (D1 <=  NEAR) Q1= 0;
					else if (D1 <   T1)   Q1= 1;
					else if (D1 <   T2)   Q1= 2;
					else if (D1 <   T3)   Q1= 3;
					else                  Q1= 4;
				
					if      (D2 <= -T3)   Q2=-4;
					else if (D2 <= -T2)   Q2=-3;
					else if (D2 <= -T1)   Q2=-2;
					else if (D2 <  -NEAR) Q2=-1;
					else if (D2 <=  NEAR) Q2= 0;
					else if (D2 <   T1)   Q2= 1;
					else if (D2 <   T2)   Q2= 2;
					else if (D2 <   T3)   Q2= 3;
					else                  Q2= 4;
				
					if      (D3 <= -T3)   Q3=-4;
					else if (D3 <= -T2)   Q3=-3;
					else if (D3 <= -T1)   Q3=-2;
					else if (D3 <  -NEAR) Q3=-1;
					else if (D3 <=  NEAR) Q3= 0;
					else if (D3 <   T1)   Q3= 1;
					else if (D3 <   T2)   Q3= 2;
					else if (D3 <   T3)   Q3= 3;
					else                  Q3= 4;

					//printf("D: %d %d %d\n", D1, D2, D3);
				
				
					// Unione dei contesti e determinazione del segno
				
					// Se il primo elemento non nullo è negativo viene considerato uguale
					// al contesto con tutti i segni dei gradienti opposti, e viene settato SIGN a -1
				
					if ( Q1 < 0
					 || (Q1 == 0 && Q2 < 0)
					 || (Q1 == 0 && Q2 == 0 && Q3 < 0) ) {
						
						Q1=-Q1;
						Q2=-Q2;
						Q3=-Q3;
						SIGN=-1;	// signifies -ve
					}
					else {
						SIGN=1;		// signifies +ve
					}
				
				
					// Derivation of Q 

					//Q = Q1 * 81 + Q2 * 9 + Q3;

					if (Q1 == 0) {
						if (Q2 == 0) {
							Q=360+Q3;		// fills 360..364
						}
						else {	// Q2 is 1 to 4
						Q=324+(Q2-1)*9+(Q3+4);		// fills 324..359
						}
					}
					else {		// Q1 is 1 to 4
						Q=(Q1-1)*81+(Q2+4)*9+(Q3+4);	// fills 0..323
					}

				
					/* PREDICTION */
				
					
					if      (Rc >= Maximum(Ra,Rb)){	Px = Minimum(Ra,Rb); uno++;}
					else if (Rc <= Minimum(Ra,Rb)){	Px = Maximum(Ra,Rb); due++;}
					else{	Px = (Int32)Ra+Rb-Rc; tre++;}

					//if(h==1)
					//	Px = floor(rho * (double)thisRow[col].g + .5);	


					/*interlacciati*/
					/*Uint16 Pxg, Pxr;
					if      (getGvalue(Rc) >= Maximum(getGvalue(Ra),getGvalue(Rb))) Pxg = Minimum(getGvalue(Ra),getGvalue(Rb));
					else if (getGvalue(Rc) <= Minimum(getGvalue(Ra),getGvalue(Rb))) Pxg = Maximum(getGvalue(Ra),getGvalue(Rb));
					else	Pxg = (Int32)getGvalue(Ra)+getGvalue(Rb)-getGvalue(Rc); 

					if      (getRvalue(Rc) >= Maximum(getRvalue(Ra),getRvalue(Rb))) Pxr = Minimum(getRvalue(Ra),getRvalue(Rb));
					else if (getRvalue(Rc) <= Minimum(getRvalue(Ra),getRvalue(Rb))) Pxr = Maximum(getRvalue(Ra),getRvalue(Rb));
					else	Pxr = (Int32)getRvalue(Ra)+getRvalue(Rb)-getRvalue(Rc);

					int i;
				       unsigned int ret = 0;

					Uint16 R = Pxr;
					Uint16 G = Pxg;

				       for(i = 0; i < 8; i++) {
				               int b = (R & 0x80) ? 1 : 0;
				               ret <<= 1;
				               ret |= b;

				               b = (G & 0x80) ? 1 : 0;
				               ret <<= 1;
				               ret |= b;

				               R <<= 1;
				               G <<= 1;
				       }
					Px = ret;*/
	
					/**/	

				
					// Figure A.6 Prediction correction and clamping ...
				
					Px = Px + ((SIGN > 0) ? C[Q] : -C[Q]);
				
					clampPredictedValue(&Px,MAXVAL);
				
					// Figure A.10 Prediction error Golomb encoding and decoding...
				
					k = determineGolombParameter(N[Q],A[Q]);

					if (decompressing) {
						// Decode Golomb mapped error from input...			
	
						if (h==0)					
							MErrval = gr_decode(gr_g, k, qbpp, LIMIT);
						else if(h==1)
							MErrval = gr_decode(gr_r, k, qbpp, LIMIT);
						else if(h==2)
							MErrval = gr_decode(gr_b, k, qbpp, LIMIT);


						// Unmap error from non-negative (inverse of A.5.2 Figure A.11) ...
						
						if (NEAR == 0 && k == 0 && 2*B[Q] <= -N[Q]) {
							if (MErrval%2 != 0)
								Errval=((Int32)MErrval-1)/2;	//  1 becomes  0,  3 becomes  1,  5 becomes  2
							else
								Errval=-(Int32)MErrval/2 - 1;	//  0 becomes -1,  2 becomes -2,  4 becomes -3
						}
						else {
							if (MErrval%2 == 0)
								Errval=(Int32)MErrval/2;	//  0 becomes  0, 2 becomes  1,  4 becomes  2
							else
								Errval=-((Int32)MErrval + 1)/2;	//  1 becomes -1, 3 becomes -2
						}
				
						updateErrval=Errval;			// NB. Before dequantization and sign correction
				
						if (SIGN < 0) Errval=-Errval;		// if "context type" was negative

						Rx=Px+Errval;
				
						// modulo(RANGE*(2*NEAR+1)) 
				
						if (Rx < -NEAR)
							Rx+=RANGE*(2*NEAR+1);
						else if (Rx > MAXVAL+NEAR)
							Rx-=RANGE*(2*NEAR+1);
				
						clampPredictedValue(&Rx,MAXVAL);
				
						// Apply inverse point transform and mapping table when implemented
/*****if*****/
						Rx += (Int32)floor(rho * (double)thisRow[col].g + .5);


						if(h==0)
							image[row][col].g=(pixval)Rx;
						else if(h==1)
							image[row][col].r=(pixval)Rx;
						else if(h==2)
							image[row][col].b=(pixval)Rx;


						thisRow = image[row];

									
					}
				
				
					else {	// compressing ...
				
						// Input value
					
						double check = 0;

									
						if(h==0)
							Ix = thisRow[col].g;
						else if(h==1) {
							
							//check = rho * (double)thisRow[col].g + .5;
							Ix = thisRow[col].r;
							//Ix = (Int32)thisRow[col].r - (Int32)floor(check);
							//Ix = (Int32)thisRow[col].r - (Int32)image[row][col].g;
							//fprintf(stderr, "%d %d\n", (Int32)thisRow[col].r, (Int32)thisRow[col].g);
							//fprintf(stderr, "%d %d %f %f %d %d\n", thisRow[col].r, thisRow[col].g, rho, check, Ix, (Int32)floor(check));
						} else if(h==2) {
							
							check = rho * (double)thisRow[col].g + .5;
				
							Ix = (Int32)thisRow[col].b - (Int32)floor(check);
							//fprintf(stderr, "%d %d %f %f %d %d\n", thisRow[col].r, thisRow[col].g, rho, check, Ix, (Int32)floor(check));
							//Ix = thisRow[col].b;
						}

						Errval = Ix - Px;

						if(h==0)
							fprintf(resG,"%d ", Errval);
						if(h==1)
							fprintf(resR,"%d ", Errval);
						if(h==2)
							fprintf(resB,"%d ", Errval);

						//if(h==1&&col==50&&row==3)
						//	printf("Ix %d Px %d Errval %d G %d\n", Ix, Px, Errval, thisRow[col].g);

						updateStatistics(Q,Errval, bpp);
						//updateStatistics(Q,Ix, bpp);		

						
						if (SIGN < 0) Errval=-Errval;	// if "context type" was negative
				
						// Modulo reduction of the prediction error

						if (Errval < 0)			Errval=Errval+RANGE;
						if (Errval >= (RANGE+1)/2)	Errval=Errval-RANGE;
						updateErrval=Errval;			// NB. After sign correction but before mapping

						// Prediction error encoding (A.5)
				
						// Golomb k parameter determined already outside decompress/compress test
				
						// Map Errval to non-negative
				
						if (NEAR == 0 && k == 0 && 2*B[Q] <= -N[Q]) {
							if (Errval >= 0)
								MErrval =  2*Errval + 1;		//  0 becomes 1,  1 becomes 3,  2 becomes 5
							else
								MErrval = -2*(Errval+1);		// -1 becomes 0, -2 becomes 2, -3 becomes 4
						}
						else {
							if (Errval >= 0)
								MErrval =  2*Errval;			//  0 becomes 0,  1 becomes 2,  2 becomes 4
							else
								MErrval = -2*Errval - 1;		// -1 becomes 1, -2 becomes 3
						}
	
						// encode MErrval

						if(h==0) {
							gr_encode(gr_g, MErrval, k, qbpp, LIMIT);
							//gctx[row][col] = (int)determineGolombParameter(1, MErrval);
						} else if(h==1) {
							gr_encode(gr_r, MErrval, k, qbpp, LIMIT);
						} else if(h==2) {
							//fprintf(stderr, "%d %d %d %d\n", gctx[row][col], rctx[row][col], k, determineGolombParameter(1, MErrval));
							gr_encode(gr_b, MErrval, k, qbpp, LIMIT);
						}
					}
					
					// Update variables (A.6) ...
			
					B[Q]=B[Q]+updateErrval*(2*NEAR+1);
					A[Q]=A[Q]+Abs(updateErrval);
					if (N[Q] == RESET) {
						A[Q]=A[Q]>>1;
						B[Q]=B[Q]>>1;
						N[Q]=N[Q]>>1;
					}
					++N[Q];


					// A.6.2 Context dependent bias cancellation ...

					if (B[Q] <= -N[Q]) {
						B[Q]+=N[Q];
						if (C[Q] > MIN_C) --C[Q];
						if (B[Q] <= -N[Q]) B[Q]=-N[Q]+1;
					}
					else if (B[Q] > 0) {
						B[Q]-=N[Q];
						if (C[Q] < MAX_C) ++C[Q];
						if (B[Q] > 0) B[Q]=0;
					}

					
				}
				
			}
			pixel *tmpRow=thisRow;
			thisRow=prevRow;
			prevRow=tmpRow;
		}

		if(!decompressing)
			printCondEntropy(bpp);

	}

	if (!decompressing){
		gr_flush(gr_r);
		gr_flush(gr_g);
		gr_flush(gr_b);		
		float bppG = (float)(gr_g->out_bits) / (float)(rowsP*colsP);
		float bppR = (float)(gr_r->out_bits) / (float)(rowsP*colsP);
		float bppB = (float)(gr_b->out_bits) / (float)(rowsP*colsP);
		float tot = (bppG + bppR + bppB)/3;

		printf("\nCompression results\t:\n\n\tG channel\t:\t%.3f bpp\n\tR channel\t:\t%.3f bpp\n\tB channel\t:\t%.3f bpp\n\tTotal\t\t:\t%.3f bpp\n",bppG,bppR,bppB,tot);

		//printf("uno %d due %d tre %d\n", uno, due, tre);

	} 

	if(decompressing){
		// write pixels on image file
		ppm_writeppm(image_file,  image, COLUMNS, ROWS,  MAXVAL,  0);
	}

	// free image array
	ppm_freearray(image, rowsP);
	
	g_free(gr_r);
	g_free(gr_g);
	g_free(gr_b);
	fclose(image_file);
	fclose(bitstream_r);
	fclose(bitstream_g);
	fclose(bitstream_b);

	fclose(errs);
	fclose(cont);

	fclose(resG);
	fclose(resR);
	fclose(resB);

	return 0;

}




void initStats(int rows, int cols, Uint16 bpp){

	n_of_pixs = rows*cols;
	int i,j;

	if(bpp==8){
		for(i=0;i<365;i++)
			for(j=0;j<511;j++)
				joinFreq8[i][j]=0;
		for(j=0;j<511;j++)
			Freq8[j]=0;
	}	

	else{
		for(i=0;i<365;i++)
			for(j=0;j<131071;j++)
				joinFreq16[i][j]=0;
		for(j=0;j<131071;j++)
			Freq16[j]=0;
	}

	for(i=0;i<365; i++)
		condFreqCnt[i]=0;
	

}

void updateStatistics(Uint16 Q, Int32 Errval, Uint16 bpp){

	condFreqCnt[Q]++;

	if(bpp==8){
		joinFreq8[Q][Errval+255]++;
		Freq8[Errval+255]++;
	}
	if(bpp==16){
		joinFreq16[Q][Errval+65535]++;
		Freq16[Errval+65535]++;
	}

}

void printCondEntropy(Uint16 bpp){

	float CondEn = 0;
	float en = 0;
	Uint16 Q;
	Int32 Errval;


	if(bpp==8)
		for(Q=0;Q<365;Q++)
			for(Errval=0;Errval<511;Errval++)
				if(joinFreq8[Q][Errval]!=0  && condFreqCnt[Q]!=0)
					CondEn += ((float)joinFreq8[Q][Errval]/(float)n_of_pixs) * ((float)log2 ((float)joinFreq8[Q][Errval]/(float)condFreqCnt[Q]));
			
	if(bpp==16)
		for(Q=0;Q<365;Q++)
			for(Errval=0;Errval<131071;Errval++)
				if(joinFreq16[Q][Errval]!=0 && condFreqCnt[Q]!=0)
					CondEn += ((float)joinFreq16[Q][Errval]/(float)n_of_pixs) * ((float)log2((float)joinFreq16[Q][Errval]/(float)condFreqCnt[Q]));

	if(bpp==8)
		for(Errval=0;Errval<511;Errval++)
			if(Freq8[Errval]!=0)
				en += ((float)Freq8[Errval]/(float)n_of_pixs) * ((float)log2 (((float)1/(float)((float)Freq8[Errval]/(float)n_of_pixs))));
			
	if(bpp==16)
		for(Errval=0;Errval<131071;Errval++)
			if(Freq16[Errval]!=0)
				en += ((float)Freq16[Errval]/(float)n_of_pixs) * ((float)log2 (((float)1/(float)((float)Freq16[Errval]/(float)n_of_pixs))));


	printf("\t\tH(Errval) = %f\t\t", en);
	printf("H(Q|Errval) = %f\n", -CondEn);

}

void updateRGmeans(double *mr, double *mg, int *cnt, double mr_val, double mg_val){

	(*mr) = (((*mr)*(*cnt))+mr_val)/((*cnt)+1);
	(*mg) = (((*mg)*(*cnt))+mg_val)/((*cnt)+1);
	(*cnt)++;

}


Uint16 computeContext(Int16 Ra, Int16 Rb, Int16 Rc, Int16 Rd, Uint16 T1, Uint16 T2, Uint16 T3){

	Int32 D1, D2, D3;
	Int16 Q1, Q2, Q3;

	Int16 SIGN;

	Uint16 NEAR = 0;

	Uint16 Q = 0;

	D1=(Int32)Rd-Rb;
	D2=(Int32)Rb-Rc;
	D3=(Int32)Rc-Ra;

	if      (D1 <= -T3)   Q1=-4;
	else if (D1 <= -T2)   Q1=-3;
	else if (D1 <= -T1)   Q1=-2;
	else if (D1 <  -NEAR) Q1=-1;
	else if (D1 <=  NEAR) Q1= 0;
	else if (D1 <   T1)   Q1= 1;
	else if (D1 <   T2)   Q1= 2;
	else if (D1 <   T3)   Q1= 3;
	else                  Q1= 4;
				
	if      (D2 <= -T3)   Q2=-4;
	else if (D2 <= -T2)   Q2=-3;
	else if (D2 <= -T1)   Q2=-2;
	else if (D2 <  -NEAR) Q2=-1;
	else if (D2 <=  NEAR) Q2= 0;
	else if (D2 <   T1)   Q2= 1;
	else if (D2 <   T2)   Q2= 2;
	else if (D2 <   T3)   Q2= 3;
	else                  Q2= 4;
		
	if      (D3 <= -T3)   Q3=-4;
	else if (D3 <= -T2)   Q3=-3;
	else if (D3 <= -T1)   Q3=-2;
	else if (D3 <  -NEAR) Q3=-1;
	else if (D3 <=  NEAR) Q3= 0;
	else if (D3 <   T1)   Q3= 1;
	else if (D3 <   T2)   Q3= 2;
	else if (D3 <   T3)   Q3= 3;
	else                  Q3= 4;
				
				
	// Unione dei contesti e determinazione del segno
				
	// Se il primo elemento non nullo è negativo viene considerato uguale
	// al contesto con tutti i segni dei gradienti opposti, e viene settato SIGN a -1
				
	if ( Q1 < 0
	 || (Q1 == 0 && Q2 < 0)
	 || (Q1 == 0 && Q2 == 0 && Q3 < 0) ) {
						
		Q1=-Q1;
		Q2=-Q2;
		Q3=-Q3;
		SIGN=-1;	// signifies -ve
	}
	else {
		SIGN=1;		// signifies +ve
	}
				
				
	// Derivation of Q 

	//Q = Q1 * 81 + Q2 * 9 + Q3;

	if (Q1 == 0) {
		if (Q2 == 0) {
			Q=360+Q3;		// fills 360..364
		}
		else {	// Q2 is 1 to 4
			Q=324+(Q2-1)*9+(Q3+4);		// fills 324..359
		}
	}
	else {		// Q1 is 1 to 4
		Q=(Q1-1)*81+(Q2+4)*9+(Q3+4);	// fills 0..323
	}

	return Q;
}
