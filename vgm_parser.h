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
