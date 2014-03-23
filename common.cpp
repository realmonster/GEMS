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
	data[1] = value >> 8;
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
	data[0] = value >> 8;
}

// Get 3 bytes Little-Endian 01 23 45 = 0x452301
int GetTriple(const BYTE *data)
{
	return data[0]|(data[1]<<8)|(data[2]<<16);
}

// Set 3 bytes Little-Endian 01 23 45 = 0x452301
void SetTriple(BYTE *data, int value)
{
	data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
}

// Get Long Little-Endian 01 23 45 67 = 0x67452301
int GetLongLE(const BYTE *data)
{
	return
	  data[0]
	|(data[1]<<8)
	|(data[2]<<16)
	|(data[3]<<24);
}

// Set Long Little-Endian 01 23 45 67 = 0x67452301
void SetLongLE(BYTE *data, int value)
{
	data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
	data[3] = value >> 24;
}

// Get Long Big-Endian 01 23 45 67 = 0x1234567
int GetLongBE(const BYTE *data)
{
	return
	  data[3]
	|(data[2]<<8)
	|(data[1]<<16)
	|(data[0]<<24);
}

// Set Long Big-Endian 01 23 45 67 = 0x1234567
void SetLongBE(BYTE *data, int value)
{
	data[3] = value;
	data[2] = value >> 8;
	data[1] = value >> 16;
	data[0] = value >> 24;
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
