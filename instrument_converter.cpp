#include <cstdio>
#include <ctype.h>
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

char *formats[] = {
	"gems",
	"tyi",
	"tfi",
	"eif",
	"y12",
	"vgi",
	"dmp",
	"dmp0",
	"smps",
};

int format_size[] = {
	39,  // GEMS
	32,  // TYI
	42,  // TFI
	29,  // EIF
	128, // Y12
	43,  // VGI
	51,  // DMP
	49,  // DMP v0
	25,  // SMPS
};

int getformat(const char *name)
{
	for (int i=0; i<sizeof(format_size)/sizeof(format_size[0]); ++i)
	{
		bool same = true;
		int j;
		for (j=0; name[j] && formats[i][j]; ++j)
			if (tolower(name[j]) != tolower(formats[i][j]))
				same = false;
		if (same && !name[j] && !(formats[i][j]))
			return i;
	}
	return -1;
}

int main(int argc, char **args)
{
	if (argc != 5)
	{
		usage();
		return 0;
	}

	int input_format = getformat(args[1]);
	int output_format = getformat(args[3]);
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
	int in_len = fread(data,1,200,f);
	fclose(f);

	if (in_len < format_size[input_format])
	{
		if (!(input_format == 6 && in_len == 49)) // avoid DMP v < 5 warning
			printf("Warrning: input file size %d < %d size of input format type\n",
				in_len, format_size[input_format]);
	}

	InstrumentConverter ic;
	switch(input_format)
	{
		case 0: // GEMS
			ic.ImportGems(data);
			break;
		case 1: // TYI
			ic.ImportTYI(data);
			break;
		case 2: // TFI
			ic.ImportTFI(data);
			break;
		case 3: // EIF
			ic.ImportEIF(data);
			break;
		case 4: // Y12
			ic.ImportY12(data);
			break;
		case 5: // VGI
			ic.ImportVGI(data);
			break;
		case 6: // DMP
		case 7: // DMPv0
		{
			int ret = ic.ImportDMP(data);
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
		}
			break;
		case 8: // SMPS
			ic.ImportSMPS(data);
			break;
	}

	switch(output_format)
	{
		case 0: // GEMS
			ic.ExportGems(data);
			break;
		case 1: // TYI
			ic.ExportTYI(data);
			break;
		case 2: // TFI
			ic.ExportTFI(data);
			break;
		case 3: // EIF
			ic.ExportEIF(data);
			break;
		case 4: // Y12
			ic.ExportY12(data);
			break;
		case 5: // VGI
			ic.ExportVGI(data);
			break;
		case 6: // DMP
			ic.ExportDMP(data);
			break;
		case 7: // DMPv0
			ic.ExportDMPv0(data);
			break;
		case 8: // SMPS
			ic.ExportSMPS(data);
			break;
	}

	f = fopen(args[4],"wb");
	if (!f)
	{
		printf("Error: Can't open file: %s\n\n",args[4]);
		usage();
		return 4;
	}
	int out_len = fwrite(data,1,format_size[output_format],f);
	fclose(f);

	if (out_len != format_size[output_format])
	{
		printf("Error: Can't write file \"%s\"\n\n",args[4]);
		usage();
		return 5;
	}
	return 0;
}
