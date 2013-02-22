#include <cstdio>
#include <algorithm>
#include <vector>
#include <cassert>
#include "instruments.h"

char *inst_type_name[] = {"FM", "DAC", "PSG", "NOISE"};

char *fm_format[] =
{
	"??????tt",
	"????ovvv",
	"?m??????",
	"??fffaaa",
	"lraa?fff",
	
	"?dddmmmm",
	"?ttttttt",
	"rr?aaaaa",
	"a??ddddd",
	"???sssss",
	"ssssrrrr",

	"?dddmmmm",
	"?ttttttt",
	"rr?aaaaa",
	"a??ddddd",
	"???sssss",
	"ssssrrrr",
	
	"?dddmmmm",
	"?ttttttt",
	"rr?aaaaa",
	"a??ddddd",
	"???sssss",
	"ssssrrrr",
	
	"?dddmmmm",
	"?ttttttt",
	"rr?aaaaa",
	"a??ddddd",
	"???sssss",
	"ssssrrrr",
	
	"????????","????????",
	"????????","????????",
	"????????","????????",
	"????????","????????",
	
	"????kkkk",
	"????????"
};


char *dac_format[] =
{
	"??????tt"
};


char *psg_format[] =
{
	"??????tt"
};

char *noise_format[] =
{
	"??????tt"
};

BYTE buff[1000];
BYTE instruments_file[0x10100];

bool triple_pointers;

int inst_offs(int i)
{
	if (triple_pointers)
		return GetTriple(instruments_file + 3*i);
	return GetWordLE(instruments_file + 2*i);
}

int inst_type(int i)
{
	int off = inst_offs(i);
	if (off >= sizeof(instruments_file))
		return 0xFF;
	return instruments_file[off];
}

std::vector<int> inst;

bool inst_cmp(int a, int b)
{
	int ia = inst_type(a);
	int ib = inst_type(b);
	if (ia != ib)
		return ia<ib;
	return a<b;
}

void usage()
{
	printf(
"Usage: .exe instruments mode [triple]\n"
"\n"
"       mode: 0 - bits\n"
"             1 - mask\n"
"             2 - values\n"
"\n"
"       triple - use any third argument,\n"
"                to use 3 bytes pointers.\n"
"\n"
"Author: r57shell@uralweb.ru\n");
	exit(0);
}

int main(int argc, char **args)
{
	if (argc != 3 && argc != 4)
		usage();

	FILE *f = fopen(args[1],"rb");
	if (!f)
	{
		printf("Can't open instruments\n");
		usage();
	}
	int flen = fread(instruments_file, 1, sizeof(instruments_file), f);
	fclose(f);
	
	int mode = atoi(args[2]);
	triple_pointers = (argc == 4);

	// assume that offset of first instrument = end of offsets table
	int inst_count = inst_offs(0);
	if (triple_pointers)
		inst_count /= 3;
	else
		inst_count /= 2;

	if (inst_count > 0x100)
	{
		printf("This is not instruments bank\n");
		usage();
	}
	inst.resize(inst_count);

	// index table for sort.
	for (int z = 0; z < inst.size(); ++z)
		inst[z] = z;

	std::sort(inst.begin(), inst.end(), inst_cmp);

	for (int z=0; z<inst_count; ++z)
	{
		int off = inst_offs(inst[z]);
		if (off >= sizeof(instruments_file))
			continue;
		// instrument type
		int it = inst_type(inst[z]);

		if (it < 4)
			printf("Patch %02X (%s):",inst[z],inst_type_name[it]);
		else
			printf("Patch %02X (%02X):",inst[z],it);

		// assume that offset of next instrument = end of previous
		int off_e = inst_offs(inst[z]+1);
		if (inst[z] == inst_count - 1)
			off_e = flen; // end of last instrument = end of file

		unsigned char *ym = instruments_file + off;
		char **format;
		int format_size = 0;
		switch(it)
		{
			case GEMSI_FM:
				format = fm_format;
				format_size = sizeof(fm_format)/sizeof(char*);
				break;

			case GEMSI_DAC:
				format = dac_format;
				format_size = sizeof(dac_format)/sizeof(char*);
				break;

			case GEMSI_PSG:
				format = noise_format;
				format_size = sizeof(noise_format)/sizeof(char*);
				break;

			case GEMSI_NOISE:
				format = noise_format;
				format_size = sizeof(noise_format)/sizeof(char*);
				break;

			default:
				format = 0;
				format_size = 0;
				break;
		}

		if (mode == 0 || mode == 1)
		for (int zz = 0; zz<off_e-off; ++zz)
		{
			int c = ym[zz];
			printf(" ");
			for (int j=0; j<8; ++j,c<<=1)
			{
				if (!mode
					|| (zz >= format_size || format[zz][j]=='?'))
					printf("%c",c&0x80?'1':'0');
				else
					printf("%c",fm_format[zz][j]);
			}
		}
		
		if (mode == 2)
		{
			if (it == GEMSI_FM)
			{
				printf("\n");
				GemsFM fm;
				fm.Set(ym);

				printf("FMS: %X\n", (int)fm.FMS);
				printf("AMS: %X\n", (int)fm.AMS);
				printf("FB: %X\n", (int)fm.FB);
				printf("ALG: %X\n", (int)fm.ALG);
				printf("LR: %c%c\n", (int)fm.L,(int)fm.R);
				printf("CH3_on: %X\n", (int)fm.CH3_on);
				printf("LFO_on: %X\n", (int)fm.LFO_on);
				printf("LFO_val: %X\n", (int)fm.LFO_val);
				printf("KEY: ");
				for (int i=0; i<4; ++i)
					printf(fm.IsOn(i) ? "1" : "0");
				printf("\n");
				for (int i=0; i<4; ++i)
				{
					printf("Operator %d:\n", i+1);
					printf("TL: %X\n", (int)fm.OP[i].TL);
					printf("DT: %X\n", (int)fm.OP[i].DT);
					printf("MT: %X\n", (int)fm.OP[i].MT);
					printf("RS: %X\n", (int)fm.OP[i].RS);
					printf("AR: %X\n", (int)fm.OP[i].AR);
					printf("AM: %X\n", (int)fm.OP[i].AM);
					printf("DR: %X\n", (int)fm.OP[i].DR);
					printf("SDR: %X\n", (int)fm.OP[i].SDR);
					printf("SL: %X\n", (int)fm.OP[i].SL);
					printf("RR: %X\n", (int)fm.OP[i].RR);
					printf("\n");
				}
			}
			else if (it == GEMSI_DAC)
			{
				printf(" %02X",(int)ym[1]);
			}
			else
			{
				for (int i = 1; i < off_e - off; ++i)
					printf(" %02X", (int)ym[i]);
			}
		}
		printf("\n");
		fflush(stdout);
	}

	return 0;
}