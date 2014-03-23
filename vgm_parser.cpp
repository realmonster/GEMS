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

#include "vgm_parser.h"
#include "common.h"
#include <cstdlib>

static int cmd_size[] = {
1,1,1,1,1,1,1,1, // 0x30-0x37 reserved
1,1,1,1,1,1,1,1, // 0x38-0x3F reserved
2,2,2,2,2,2,2,2, // 0x40-0x47 reserved
2,2,2,2,2,2,2,   // 0x48-0x4E reserved
1,1, // 0x4F, 0x50
2,2,2,2,2, // 0x51-0x55
2,2,2,2,2, // 0x56-0x5A
2,2,2,2,2, // 0x5B-0x5F
-1, // 0x60
2,  // 0x61
0,0, // 0x62,0x63
3,  // 0x64
-1, // 0x65
0,  // 0x66
0,0, // 0x67, 0x68 (variable)
-1,-1,-1,-1,-1,-1,-1, // 0x69-0x6F
0,0,0,0,0,0,0,0, // 0x70-0x77
0,0,0,0,0,0,0,0, // 0x78-0x7F
0,0,0,0,0,0,0,0, // 0x80-0x87
0,0,0,0,0,0,0,0, // 0x88-0x8F
0,0,0,0,0,0, // 0x90-0x95
-1,-1,-1,-1,-1, // 0x96-0x9A
-1,-1,-1,-1,-1, // 0x9B-0x9F
2,2,2,2,2,2,2,2, // 0xA0-0xA7
2,2,2,2,2,2,2,2, // 0xA8-0xAF
2,2,2,2,2,2,2,2, // 0xB0-0xB7
2,2,2,2,2,2,2,2, // 0xB8-0xBF
3,3,3,3,3,3,3,3, // 0xC0-0xC7
3,3,3,3,3,3,3,3, // 0xC7-0xCF
3,3,3,3,3,3,3,3, // 0xD0-0xD7
3,3,3,3,3,3,3,3, // 0xD8-0xDF
4,4,4,4,4,4,4,4, // 0xE0-0xE7
4,4,4,4,4,4,4,4, // 0xE8-0xEF
4,4,4,4,4,4,4,4, // 0xF0-0xF7
4,4,4,4,4,4,4,4, // 0xF8-0xFF
};

vgmParser::vgmParser()
{
	version = 0;
	psgClock = 3579545;
	ymClock = 7670454;
	dataBlocks = 0;
	dataBlocksCount = 0;
}

vgmParser::~vgmParser()
{
	for (int i=0; i<dataBlocksCount; ++i)
		free(dataBlocks[i]);
	if (dataBlocks)
		free(dataBlocks);
}

int vgmParser::getVersion() const
{
	return version;
}

int vgmParser::clockSN76489() const
{
	return psgClock;
}

int vgmParser::clockYM2612() const
{
	return ymClock;
}

int vgmParser::getDataBlockCount() const
{
	return dataBlocksCount;
}

int vgmParser::getDataBlockType(int id) const
{
	if (id >=0 && id < dataBlocksCount)
		return dataBlocks[id][0];
	else
		return -1;
}

int vgmParser::getDataBlockSize(int id) const
{
	if (id >=0 && id < dataBlocksCount)
		return *(int*)(dataBlocks[id]+1);
	else
		return -1;
}

const unsigned char * vgmParser::getDataBlock(int id) const
{
	if (id >= 0 && id < dataBlocksCount)
		return dataBlocks[id]+1+sizeof(int);
	else
		return 0;
}

unsigned char * vgmParser::createDataBlock(int type, int size)
{
	unsigned char **nd = (unsigned char **)realloc(dataBlocks, (dataBlocksCount + 1)*sizeof(unsigned char*));
	if (!nd)
		return 0;
	dataBlocks = nd;
	unsigned char *d = (unsigned char *)malloc(size+1+sizeof(int)); 
	if (!d)
		return 0;
	nd[dataBlocksCount] = d;
	*d = type;
	*(int*)(d+1) = size;
	++dataBlocksCount;
	return d+1+sizeof(int);
}

int vgmParser::readHeader(FILE *f)
{
	unsigned char buff[0xC0];
	if (fread(buff,1,0x38,f) != 0x38)
		return 1;
	if (GetLongBE(buff) != 0x56676D20) // "Vgm "
		return 2;

	version = GetLongLE(buff+8);

	int v = GetLongLE(buff+12);
	if (v != 0)
		psgClock = v;

	if (version < 0x110)
		v = GetLongLE(buff+0x10);
	else
		v = GetLongLE(buff+0x2C);
	if (v != 0)
		ymClock = v;

	// vgm offset
	if (version < 0x150)
		v = 0xC;
	else
		v = GetLongLE(buff+0x34);
	v -= 4;
	if (v < 0)
		return 3;

	while (v != 0)
	{
		int r = 0;
		r = fread(buff,1,v>sizeof(buff)?sizeof(buff):v,f);
		if (!r)
			return 4;
		v -= r;
	}
	return 0;
}

int vgmParser::parse(FILE *f)
{
	unsigned char buff[0x40];
	for (;;)
	{
		int r = fread(buff,1,1,f);
		if (!r)
			return 1; // unexpected end
		int cmd = buff[0];
		if (cmd<0x30)
			return 2; // invalid command

		int cmdsize = cmd_size[cmd-0x30];
		if (cmdsize<0)
			return 2; // invalid command

		if (cmd == 0x67) // data block
		{
			r = fread(buff,1,6,f);
			if (r != 6)
				return 1; // unexpected end
			if (buff[0] != 0x66)
				return 2; // invalid command

			int size = GetLongLE(buff+2);
			unsigned char *data = createDataBlock(buff[1], size);
			fread(data,1,size,f);
			continue;
		}

		if (cmd == 0x68) // PCM RAM write
		{
			r = fread(buff,1,11,f);
			if (r != 11)
				return 1; // unexpected end
			if (buff[0] != 0x66)
				return 2; // invalid command

			if (process)
			{
				if (process(this, custom, cmd, buff+1))
					return 0;
			}
			continue;
		}

		r = fread(buff,1,cmdsize,f);
		if (r != cmdsize)
			return 1; // unexpected end

		if (process)
		{
			if (process(this, custom, cmd, buff))
				return 0;
		}

		if (cmd == 0x66) // End of sound
			return 0;
	}
	return 0;
}