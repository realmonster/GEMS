#include "common.h"

// Get Word Little-Endian 01 23 = 0x2301
int GetWordLE(const BYTE *data)
{
	return data[0]|(data[1]<<8);
}

// Set Word Little-Endian 01 23 = 0x2301
void SetWordLE(BYTE *data, int value)
{
	data[0] = value;
	data[1]	= value >> 8;
}

// Get Word Big-Endian 01 23 = 0x123
int GetWordBE(const BYTE *data)
{
	return data[1]|(data[0]<<8);
}

// Set Word Big-Endian 01 23 = 0x123
void SetWordBE(BYTE *data, int value)
{
	data[1] = value;
	data[0]	= value >> 8;
}

// Get 3 bytes Little-Endian 01 23 45 = 0x452301
int GetTriple(const BYTE *data)
{
	return 	data[0]|(data[1]<<8)|(data[2]<<16);
}

// Set 3 bytes Little-Endian 01 23 45 = 0x452301
void SetTriple(BYTE *data, int value)
{
	data[0] = value;
	data[1]	= value >> 8;
	data[2]	= value >> 16;
}
