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

// Get Long Little-Endian 01 23 45 67 = 0x67452301
int GetLongLE(const BYTE *data);
void SetLongLE(BYTE *data, int value);

// Get Long Big-Endian 01 23 45 67 = 0x01234567
int GetLongBE(const BYTE *data);
void SetLongBE(BYTE *data, int value);

// Scan number
// input: spaces + number in format:
// 0123 for decimal
// $12 for hex
// 0x12 for hex
int ScanNum(const char *str, int &num);

#endif
