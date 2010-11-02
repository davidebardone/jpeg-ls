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

#include "parameters.h"


void usage(int exit_type)
{
	fprintf(stdout, "Usage:\tjpegls -e [-v] [-N <value>] [-R <value>] filename\n");
	fprintf(stdout, "\tjpegls -d [-v] filename\n");
	fprintf(stdout, "\tjpegls -h\n");
	fprintf(stdout, "Options:\n\t-e\t\tEncode\n\t-d\t\tDecode\n\t-h\t\tDisplay this information\n\t-M <value>\n\t-N <value>\n\t-T <value>\n\t-R <value>\n");
	exit(exit_type);
}

parameters coding_parameters(int argc, char* argv[])
{
	parameters params;
	params.verbose = false;
	params.NEAR = 0;
	params.RESET = 64;
	params.specified_T = false;
	char fname[MAX_FILENAME_LEN];
	
	if(argc<2)
		usage(EXIT_FAILURE);

	while(argc>1)
	{
		if(argv[1][0]=='-')
			switch(argv[1][1])
			{
				case 'e': 
					params.decoding_flag = false;
					break;
				case 'd': 
					params.decoding_flag = true;
					break;
				case 'v':
					params.verbose = true;
					break;
				case 'h':
					usage(EXIT_SUCCESS);
					break;
				default:
					usage(EXIT_FAILURE);
					break;
			}
		else
		{
			if(strlen(argv[1]) < MAX_FILENAME_LEN)
			{
				strcpy(params.input_file, argv[1]);
				strcpy(fname, argv[1]);
				if (params.decoding_flag == false)	
					strcpy(params.output_file, strcat(strtok(fname,"."),".jls"));
				else
					strcpy(params.output_file, strcat(strtok(fname,"."),".ppm"));
			}
			else
			{
				fprintf(stderr, "%s: filename too long,\n", argv[1]);
				exit(EXIT_FAILURE);
			}
		}

		argv++;
		argc--;
	}

	return params;
}
