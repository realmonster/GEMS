#ifndef COMMON_H
#define COMMON_H

typedef unsigned char BYTE;

// Get Word Little-Endian 01 23 = 0x2301
int GetWordLE(const BYTE *data);
void SetWordLE(BYTE *data, int value);

// Get Word Big-Endian 01 23 = 0x0123
int GetWordBE(const BYTE *data);
void SetWordBE(BYTE *data, int value);

// Get 3 bytes Little-Endian 01 23 45 = 0x452301
int GetTriple(const BYTE *data);
void SetTriple(BYTE *data, int value);

// Scan number
// input: spaces + number in format:
// 0123 for decimal
// $12 for hex
// 0x12 for hex
int ScanNum(const char *str, int &num);

#endif
