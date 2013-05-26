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

// Scan number
// input: spaces + number in format:
// 0123 for decimal
// $12 for hex
// 0x12 for hex
int ScanNum(const char *str, int &num)
{
	int l=0, r=0;
	while (*str == ' '
		|| *str == '\r'
		|| *str == '\n'
		|| *str == '\t')
	{
		++l,++str;
	}
	bool was = false;
	bool neg = false;
	if (*str == '-')
	{
		neg = true;
		++l,++str;
	}
	else if (*str == '+')
	{
		++l,++str;
	}
	if (*str == '$' || (str[0] == '0' && str[1] == 'x'))
	{
		if (*str == '$')
			++l,++str;
		else
			l+=2,str+=2;
		for (;;)
		{
			if (*str >= '0' && *str <= '9')
				r = (r<<4) | (*str - '0');
			else if (*str >= 'a' && *str <= 'f')
				r = (r<<4) | (*str - 'a' + 10);
			else if (*str >= 'A' && *str <= 'F')
				r = (r<<4) | (*str - 'A' + 10);
			else
			{
				if (!was)
					return 0;
				if (neg)
					r = -r;
				num = r;
				return l;
			}
			++l,++str;
			was = true;
		}
	}
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			r = (r*10) + (*str - '0');
		else
		{
			if (!was)
				return 0;
			if (neg)
				r = -r;
			num = r;
			return l;
		}
		++l,++str;
		was = true;
	}
	// unreachable
	return 0;	
}
