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

#ifndef VGM_PARSER_H
#define VGM_PARSER_H

#include <cstdio>

class vgmParser
{
public:
	vgmParser();
	~vgmParser();
	int readHeader(FILE *f);
	int parse(FILE *f);

	// custom callback
	// return true to stop parsing
	void *custom;
	bool (*process)(vgmParser *parser, void *custom, int cmd, const unsigned char *args);

	int getVersion() const;
	int clockSN76489() const;
	int clockYM2612() const;

	int getDataBlockCount() const;
	int getDataBlockType(int id) const;
	int getDataBlockSize(int id) const;
	const unsigned char * getDataBlock(int id) const;

private:
	int version;
	int psgClock;
	int ymClock;
	int dataBlocksCount;
	unsigned char **dataBlocks;

	unsigned char * createDataBlock(int type, int size);
};

#endif
