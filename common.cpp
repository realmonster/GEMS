#include "common.h"

// Get Word Little-Endian 01 23 = 0x2301
int GetWordLE(BYTE *data)
{
	return data[0]|(data[1]<<8);
}

// Get Word Big-Endian 01 23 = 0x123
int GetWordBE(BYTE *data)
{
	return data[1]|(data[0]<<8);
}

// Get 3 bytes Little-Endian 01 23 45 = 0x452301
int GetTriple(BYTE *data)
{
	return 	data[0]|(data[1]<<8)|(data[2]<<16);
}
