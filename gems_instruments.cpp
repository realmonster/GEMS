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
#include <algorithm>
#include <vector>
#include <cassert>
#include "instruments.h"

char *inst_type_name[] = {"FM", "DAC", "PSG", "NOISE"};

char *fm_format[] =
{
	"??????tt",
	"????ovvv",
	"mm??????",
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
	"??????tt",
	"?????nnn",
	"aaaaaaaa",
	"????ssss",
	"????llll",
	"dddddddd",
	"rrrrrrrr"
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
"             3 - opm\n"
"             4 - tyi\n"
"             5 - tfi\n"
"\n"
"       Warning: \n"
"          4, 5 mode writes 00.ext in current directory\n"
"          and overwrites files if exists\n"
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

		if (mode != 3)
		{
			if (it < 4)
				printf("Patch %02X (%s):",inst[z],inst_type_name[it]);
			else
				printf("Patch %02X (%02X):",inst[z],it);
		}

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
			case GEMSI_NOISE:
				format = psg_format;
				format_size = sizeof(psg_format)/sizeof(char*);
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
					printf("%c",format[zz][j]);
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
				printf("LR: %d%d\n", (int)fm.L,(int)fm.R);
				printf("CH3: %X\n", (int)fm.CH3);
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
					printf("DT: %X=%d\n", (int)fm.OP[i].DT,
						fm.OP[i].DT&4
						? -((fm.OP[i].DT)&3)
						: fm.OP[i].DT);
					printf("MUL: %X\n",(int)fm.OP[i].MUL);
					printf("RS: %X\n", (int)fm.OP[i].RS);
					printf("AR: %X\n", (int)fm.OP[i].AR);
					printf("AM: %X\n", (int)fm.OP[i].AM);
					printf("DR: %X\n", (int)fm.OP[i].DR);
					printf("SDR: %X\n",(int)fm.OP[i].SDR);
					printf("SL: %X\n", (int)fm.OP[i].SL);
					printf("RR: %X\n", (int)fm.OP[i].RR);
					printf("\n");
				}
				for (int i=0; i<4; ++i)
					printf("Channel 3 Operator %d Frequency: %X\n", i+1,(int)fm.CH3_F[i]);
			}
			else if (it == GEMSI_DAC)
			{
				printf(" %02X",(int)ym[1]);
			}
			else if (it == GEMSI_PSG || it == GEMSI_NOISE)
			{
				printf("\n");
				GemsPSG psg;
				psg.Set(ym);
				printf("ND: %X\n", (int)psg.ND);
				printf("AR: %X\n", (int)psg.AR);
				printf("SL: %X\n", (int)psg.SL);
				printf("AL: %X\n", (int)psg.AL);
				printf("DR: %X\n", (int)psg.DR);
				printf("RR: %X\n", (int)psg.RR);
			}
			else
			{
				for (int i = 1; i < off_e - off; ++i)
					printf(" %02X", (int)ym[i]);
			}
		}
		if (mode == 3)
		{
			printf("LFO: 0 0 0 0 0\n");
			printf("@:%d ",z);
			if (it < 4)
				printf("Patch %02X (%s)\n",inst[z],inst_type_name[it]);
			else
				printf("Patch %02X (%02X)\n",inst[z],it);
			GemsFM fm;
			fm.Set(ym);
			printf("CH: 64 %d %d %d %d 120 0\n",fm.FB,fm.ALG,fm.AMS,fm.FMS);
			char * opname[]={"M1","C1","M2","C2"};
			for (int i=0; i<4; ++i)
			{
				printf("%s: %d %d %d %d %d %d %d %d %d %d %d\n",
				opname[i],fm.OP[i].AR,fm.OP[i].DR,fm.OP[i].SDR,fm.OP[i].RR,fm.OP[i].SL,fm.IsOn(i)?fm.OP[i].TL:127,fm.OP[i].RS,fm.OP[i].MUL,fm.OP[i].DT,0,fm.OP[i].AM?128:0);
			}
		}
		if (mode == 4 && it == GEMSI_FM)
		{
			GemsFM fm;
			fm.Set(ym);
			unsigned char tyi[32];
			for (int i=0; i<4; ++i)
			{
				unsigned char *op = ym + 5 + i*6;
				tyi[ 0+i] = op[0]; // DT/MUL
				tyi[ 4+i] = op[1]; // TL
				if (!fm.IsOn(((i<<1)|(i>>1))&3))
					tyi[ 4+i] |= 0x7F; // TL
				tyi[ 8+i] = op[2]; // RS/AR
				tyi[12+i] = op[3]; // AM/DR
				tyi[16+i] = op[4]; // SR
				tyi[20+i] = op[5]; // SL/RR
				tyi[24+i] = 0 ; // SSG-EG
			}
			tyi[28] = ym[3]; // FB/ALG
			tyi[29] = ym[4]; // FMS/AMS
			tyi[30] = 'Y';
			tyi[31] = 'I';

			char tyi_name[20];
			sprintf(tyi_name,"%02X.tyi",inst[z]);
			FILE *tyi_f = fopen(tyi_name,"wb");
			fwrite(tyi,1,32,tyi_f);
			fclose(tyi_f);
		}
		if (mode == 5 && it == GEMSI_FM)
		{
			GemsFM fm;
			fm.Set(ym);
			BYTE tfi[42];
			tfi[0] = fm.ALG&7;
			tfi[1] = fm.FB&7;
			for (int i=0; i<4; ++i)
			{
				int j = ((i<<1)|(i>>1))&3;
				BYTE *op = tfi+2+i*10;
				op[0] = fm.OP[j].MUL;
				op[1] = 3+(fm.OP[j].DT&3)*(fm.OP[j].DT&4?-1:1);
				op[2] = fm.OP[j].TL;
				if (!fm.IsOn(j))
					op[2] = 0x7F; // TL
				op[3] = fm.OP[j].RS;
				op[4] = fm.OP[j].AR;
				op[5] = fm.OP[j].DR;
				op[6] = fm.OP[j].SDR;
				op[7] = fm.OP[j].RR;
				op[8] = fm.OP[j].SL;
				op[9] = 0;
			}

			char tfi_name[20];
			sprintf(tfi_name,"%02X.tfi",inst[z]);
			FILE *tfi_f = fopen(tfi_name,"wb");
			fwrite(tfi,1,42,tfi_f);
			fclose(tfi_f);
		}
		printf("\n");
		fflush(stdout);
	}

	return 0;
}