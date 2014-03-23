/*
    This file is part of GEMS Toolkit.

    GEMS Toolkit is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GEMS Toolkit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with GEMS Toolkit.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include "instruments.h"

void usage()
{
	printf(
"Usage: .exe input_format input output_format output\n"
"\n"
"      input, output - file path\n"
"      input_format, output_format - one from list:\n"
"\n"
"        gems - internal GEMS 2.8 format\n"
"        tyi  - Tiido's instrument file\n"
"        tfi  - TFM Music Maker instrument file\n"
"        eif  - ECHO instrument file\n"
"        y12  - GensKMOD YM2612 channel dump\n"
"        vgi  - VGM Music Maker instrument file\n" 
"        dmp  - DefleMask instrument file\n" 
"        dmp0 - DefleMask instrument file version 0\n" 
"        smps - internal SMPS format (Sonic 3)\n" 
"\n"
"      Warning: all outputs will be overwrited automatically.\n"
"\n"
"Author: r57shell@uralweb.ru\n");
}

unsigned char data[200];

int main(int argc, char **args)
{
	if (argc != 5)
	{
		usage();
		return 0;
	}

	int input_format = InstrumentConverter::FormatByName(args[1]);
	int output_format = InstrumentConverter::FormatByName(args[3]);
	if (input_format == -1)
	{
		printf("Error: Unknown format: %s\n\n",args[1]);
		usage();
		return 1;
	}

	if (output_format == -1)
	{
		printf("Error: Unknown format: %s\n\n",args[3]);
		usage();
		return 2;
	}

	FILE *f = fopen(args[2],"rb");
	if (!f)
	{
		printf("Error: Can't open file: %s\n\n",args[2]);
		usage();
		return 3;
	}
	int in_size = InstrumentConverter::FormatSize(input_format);
	int in_len = fread(data,1,in_size,f);
	fclose(f);

	if (in_len < in_size)
	{
		if (!(input_format == InstrumentConverter::DMP && in_len == 49)) // avoid DMP v < 5 warning
			printf("Warrning: input file size %d < %d size of input format type\n",
				in_len, in_size);
	}

	InstrumentConverter ic;
	int ret = ic.Import(input_format, data);
	if (ret == 1)
	{
		printf("Error: this DMP version is not supported\n");
		return 6;
	}
	if (ret == 2)
	{
		printf("Error: DMP with STD instrument is not supported\n");
		return 7;
	}

	ic.Export(output_format, data);

	f = fopen(args[4],"wb");
	if (!f)
	{
		printf("Error: Can't open file: %s\n\n",args[4]);
		usage();
		return 4;
	}
	int out_size = InstrumentConverter::FormatSize(output_format);
	int out_len = fwrite(data,1,out_size,f);
	fclose(f);

	if (out_len != out_size)
	{
		printf("Error: Can't write file \"%s\"\n\n",args[4]);
		usage();
		return 5;
	}
	return 0;
}
