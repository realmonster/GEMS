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
#include <vector>
#include <map>
#include <string>
#include "common.h"
#include "instruments.h"

bool ptr3 = true;

void usage()
{
	printf(
"Usage: .exe cfg [2]\n"
"\n"
"      Warning: all outputs will be overwrited automatically.\n"
"      All outputs will be in directory of \"cfg\" from params.\n"
"\n"
"      All configuration parameters placed in config\n"
"      2 - optional. use 2 bytes ptr in sequences. default is 3.\n"
"\n"
"Author: r57shell@uralweb.ru\n");
	exit(0);
}

using namespace std;

enum {TOKEN_SP, TOKEN_NUM, TOKEN_EQ, TOKEN_DEL, TOKEN_NAME, TOKEN_STR};
char * token_names[] = {"SP","NUM","EQ","DEL","NAME","STR"};

int gettoken(char b)
{
	if (b == ' ' || b == '\t')
		return TOKEN_SP;
	if (b == '$' || (b >= '0' && b <= '9'))
		return TOKEN_NUM;
	if (strchr("<=>!",b) != 0)
		return TOKEN_EQ;
	if (b == ',' || b == '(' || b == ')' || b == ':')
		return TOKEN_DEL;
	if (b == '"' || b == '\'')
		return TOKEN_STR;
	return TOKEN_NAME;
}

// split into tokens
void parseline(const vector<BYTE> &line, vector<int> &start, vector<int> &end, vector<int> &type)
{
	start.clear();
	end.clear();
	type.clear();

	int pt = -1;
	bool _mid = false;
	int i;
	for (i=0; i<line.size(); ++i)
	{
		bool _new = false;
		if (pt != TOKEN_STR && line[i] == ';')
			break;

		int t = gettoken(line[i]);
		switch(pt)
		{
		case -1:
			if (t == TOKEN_SP)
			{
				type.push_back(t);
				start.push_back(i);
				_mid = true;
				pt = t;
			}
			else
				_new = true;
			break;

		case TOKEN_SP:
			if (t != TOKEN_SP)
				_new = true;
			break;
		
		case TOKEN_NUM:
			if (line[i] == '$')
				_new = true;
			else if (line[i] == 'x' && start.back() == i-1 && line[i-1] == '0')
			{ 	}
			else if (line[i] >= 'a' && line[i] <= 'f')
			{	}
			else if (line[i] >= 'A' && line[i] <= 'F')
			{	}
			else if (t != TOKEN_NUM)
				_new = true;
			break;

		case TOKEN_EQ:
			if (t != TOKEN_EQ)
				_new = true;
			break;
		
		case TOKEN_DEL:
			_new = true;
			break;

		case TOKEN_NAME:
			if (t == TOKEN_NUM)
			{
				if (line[i] == '$')
					_new = true;
			}
			else if (t != TOKEN_NAME)
				_new = true;
			break;
		
		case TOKEN_STR:
			if (_mid)
			{			
				if (t == TOKEN_STR && line[start.back()] == line[i])
				{
					end.push_back(i+1);
					_mid = false;
				}
			}
			else
				_new = true;
			break;
		}
		if (_new)
		{
			if (_mid)
			{
				end.push_back(i);
				_mid = false;
			}
			pt = t;
			if (t != TOKEN_SP)
			{
				type.push_back(t);
				start.push_back(i);
				_mid = true;
			}
		}
	}
	if (_mid)
		end.push_back(i);
	if (type.size() == 1 && type[0] == TOKEN_SP)
	{
		// empty line
		start.clear();
		end.clear();
		type.clear();
	}
}

// read line and split into tokens
bool readline(FILE *f, vector<BYTE> &line, vector<int> &start, vector<int> &end, vector<int> &type)
{
	line.clear();
	static BYTE buff[1024];
	// read line
	for (;;)
	{
		int readed = fread(buff,1,1024,f);
		if (!readed)
		{
			if (line.size())
				break; // readed at some previous iteration
			else
				return false; // nothing readed at all
		}
		int i;
		for (i=0; i<readed; ++i)
		{
			if (buff[i]=='\n')
				break; // end line found
			line.push_back(buff[i]);
		}
		if (buff[i]=='\n')
		{
			// end line found, move back
			fseek(f,i+1-readed,SEEK_CUR);
			break;
		}
	}
	// trim \r
	while (line.size())
		if (line.back() == '\r')
			line.pop_back();
		else
			break;
	
	parseline(line,start,end,type);
	return true;
}

int strcmp(const std::vector<BYTE> &line, int start, int end, char *str)
{
	for (; start < end && start < line.size(); ++start, ++str)
		if (line[start]!= *str)
			return (int)line[start]-*str;
	return (int)0-*str;
}

string getdir(string path)
{
	int j=-1;
	for (int i=0; i<path.length(); ++i)
		if (path[i] == '/' || path[i] == '\\')
			j = i;
	return path.substr(0,j+1);
}

char *readall_err[]={"ok", "open", "read", "create", "write"};

int readall(string path, vector<BYTE> &r)
{
	FILE *f = fopen(path.c_str(),"rb");
	if (!f)
		return 1;
	
	fseek(f,0,SEEK_END);
	
	int len = ftell(f);
	r.resize(len);
	
	fseek(f,0,SEEK_SET);
	int readed = fread(&r[0],1,len,f);
	if (readed != len)
	{
		fclose(f);
		return 2;
	}

	fclose(f);
	return 0;
}

struct Parser
{
	int line_n;  // line number
	string path; // file path
	string file; // file name
	string dir;  // file dir
	vector<BYTE> line; // line bytes
	vector<int> index; // index of byte in original line
	vector<int> start, end, type; // token start, end, type
	int current; // current token
	int origin;  // origin in code

	// helper, to output parser state
	void printstate() const
	{
		printf("%s(%d) State:", path.c_str(), line_n);
		for (int i=0; i<start.size(); ++i)
		{
			printf("\"");
			for (int j=start[i]; j<end[i]; ++j)
				printf("%c", line[j]);
			printf("\"=%s ", token_names[type[i]]);
		}
		printf("\n");
	}

	// set path to current file
	void setPath(string _path)
	{
		path = _path;
		dir = getdir(path);
		file = string(path).substr(dir.length());
	}
	
	// cmp token wit text
	int cmp(int i, char *text) const
	{
		return strcmp(line,start[i+current],end[i+current],text);
	}
	
	// get tokens left
	int getLeft() const
	{
		return start.size() - current;
	}
	
	// print error at current line, at certain collumn
	void printError(int collumn, char *text) const
	{
		if (collumn > line.size())
			collumn = line.size();
		printf("%s|%s;%d\n",string((char*)&line[0],collumn).c_str(),string((char*)&line[collumn],line.size()-collumn).c_str(),getLeft());
		printf("%s(%d, %d): Error : %s\n", path.c_str(), line_n, collumn+1, text);
	}
	
	// get start index of token
	int getStart(int id) const
	{
		return start[id+current];
	}

	// get end index of token
	int getEnd(int id) const
	{
		return end[id+current];
	}
	
	// get type of token
	int getType(int id) const
	{
		return type[id+current];
	}
	
	// get string from token
	string getStr(int id) const
	{
		return string((const char *)&line[start[current+id]], end[current+id] - start[current+id]);
	}

	// get expression result
	// TODO: make full version
	int getExpression(int from, int to, int &readed) const
	{
		int r;
		int len = ScanNum(getStr(from).c_str(), r);
		readed = 0;
		if (len == end[current+from] - start[current+from])
			readed = 1;
		//printf("Expression: \"%s\" %d %d %d\n",getStr(from).c_str(),end[current+from] - start[current+from],len,readed);
		return r;
	}
	
	// append path to current dir
	string appendPath(int id)
	{
		string t = getStr(id);
		int start = 0;
		int end = t.length();
		if (start != end && (t[start] == '\'' || t[start] == '"'))
			++start;
		if (start != end && (t[end-1] == '\'' || t[end-1] == '"'))
			--end;
		
		// check for drive first, but if linux - I don't know
		if (end-start > 1 && t[start+1] == ':')
		{
			return t.substr(start, end-start);
		}
		return dir + t.substr(start, end-start);
	}
};

int gems_encode(const Parser &parser, vector<BYTE> &code, int pass, int &op)
{	
	static char * opcodes[] = {
	"note",        // 0  1
	"delay",       // 1  3
	"duration",    // 2  3
	"eos",         // 3  1
	"patch",       // 4  2
	"modulation",  // 5  2
	"nop",         // 6  1
	"loop",        // 7  2
	"loopend",     // 8  1
	"retrigger",   // 9  2
	"sustain",     // 10 2
	"tempo",       // 11 2
	"mute",        // 12 2
	"priority",    // 13 2
	"play",        // 14 2
	"pitch",       // 15 3
	"sfx",         // 16 1
	"samplerate",  // 17 2
	"goto",        // 18 3
	"mailbox",     // 19 3
	"if",          // 20 5
	"stop",        // 21 3
	"pause",       // 22 3
	"resume",      // 23 3
	"pausel",      // 24 3
	"mastervolume",// 25 3
	"volume",      // 26 3
	"INVALIDEXTRA",// 27 3
	"dc.b",        // 28 v
	"dc.w",        // 29 v
	"dc.l",        // 30 v
	"dc.t"         // 31 v
	};
	int opcode_sizes[] = {1,   3,   3,   1,   2,   2,   1,   2,   1,   2,   2,   2,   2,   2,
						  2,   3,   1,   2,   3,   3,   5,   3,   3,   3,   3,   3,   3,   3};
	int opcode_first[] = {0,0xC0,0x80,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
					   0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0x72,0x72,0x72,0x72,0x72,0x72};
	
	if (op < 0)
		for (int i=0; i<sizeof(opcodes)/sizeof(opcodes[0]); ++i)
			if (parser.cmp(0, opcodes[i]) == 0)
				op = i;

	if (op < 0)
		return -1;
	
	// variable length
	// dc.
	if (op >= 28 && op <= 31)
	{
		int len = 1;
		int strings = 0;
		for (int i=0; i<parser.getLeft(); ++i)
			if (parser.getType(i) == TOKEN_DEL && parser.cmp(i,",") == 0)
				++len;
			else if (parser.getType(i) == TOKEN_STR)
			{
				// if string, find string len
				++strings;
				int start = parser.getStart(i);
				int end = parser.getEnd(i);
				if (parser.line[i] == '\''
				 || parser.line[i] == '"')
					++start;
				if (start != end
				 &&(parser.line[end-1] == '\''
				 || parser.line[end-1] == '"'))
					--end;
				// add string len sub one for assuming one element before
				len += end - start - 1;
			}

		len -= strings;
		if (op == 29)
			len *= 2;
		if (op == 30)
			len *= 4;
		if (op == 31)
			len *= (ptr3 ? 3 : 2);
		code.resize(len);
	}
	else
	{
		// resize code
		code.resize(opcode_sizes[op]);
	}
	// if it is pass just for get size
	if (pass == 0)
		return 0; // return success

	// dc.
	if (op >= 28 && op <= 31)
	{
		int cur = 1;
		int p = 0;
		for (;;)
		{
			if (parser.getLeft() == cur)
			{
				parser.printError(parser.getStart(cur),"Expression expected");
				return 2;
			}
			int del = -1;

			if (parser.getType(cur) == TOKEN_STR)
			{
				del = cur + 1;
				if (parser.getLeft() > del
				 &&(parser.getType(del) != TOKEN_DEL
				 || parser.cmp(del, ",") != 0))
				{
					parser.printError(parser.getEnd(cur), "\",\" expected");
					return 6;
				}
			}
			int s_start = parser.getStart(cur);
			int s_end = parser.getEnd(cur);
			for (int j=s_start; j<s_end; ++j)
			{
				int val;
				if (parser.getType(cur) == TOKEN_STR)
				{
					// if string, then get symbol
					if((j == s_start
					 || j == s_end-1)
					 &&(parser.line[j] == '\''
					 || parser.line[j] == '\"'))
						continue;
					val = parser.line[j];
				}
				else
				{
					// else, get expression
					del = -1;
					for (int i=cur; i<parser.getLeft(); ++i)
						if (parser.getType(i) == TOKEN_DEL
						 && parser.cmp(i, ",") == 0)
						{
							del = i;
							break;
						};
					int end = del;
					if (end == -1)
						end = parser.getLeft();
					int readed;
					val = parser.getExpression(cur, end, readed);
					if (readed != end - cur)
					{
						printf("%d %d\n", cur, end);
						if (readed == 0)
							readed = 1;
						parser.printError(parser.getEnd(cur+readed-1), "Invalid expression");
						return 5;
					}
				}
				// dc.b
				if (op == 28)
				{
					if (val < -0x80 || val > 0xFF)
					{
						parser.printError(parser.getStart(cur),"Invalid value");
						return 3;
					}
					if (p >= code.size())
					{
						parser.printError(parser.getStart(cur),"Allocated code size overflow occured");
						return 6;
					}
					code[p] = val;
				}
				// dc.w
				if (op == 29 || (op == 31 && !ptr3)) // and dc.t
				{
					if (val < -0x8000 || val > 0xFFFF)
					{
						parser.printError(parser.getStart(cur),"Invalid value");
						return 3;
					}
					if (p*2+1 >= code.size())
					{
						parser.printError(parser.getStart(cur),"Allocated code size overflow occured");
						return 6;
					}
					SetWordLE(&code[p*2],val);
				}
				// dc.l
				if (op == 30)
				{
					if (p*4+3 >= code.size())
					{
						parser.printError(parser.getStart(cur),"Allocated code size overflow occured");
						return 6;
					}
					for (int l=0; l<4; ++l)
						code[p*4+l] = (unsigned char)(val>>(l*8));
				}
				// dc.t
				if (op == 31 && ptr3)
				{
					if (val < -0x800000 || val > 0xFFFFFF)
					{
						parser.printError(parser.getStart(cur),"Invalid value");
						return 3;
					}
					if (p*3+2 >= code.size())
					{
						parser.printError(parser.getStart(cur),"Allocated code size overflow occured");
						return 6;
					}
					SetTriple(&code[p*3],val);
				}
				++p;

				// if not string, then break loop for string
				if (parser.getType(cur) != TOKEN_STR)
					break;
			}
			if (del == -1)
				return 0;
			else
				cur = del + 1;
		}
	}
	
	code[0] = opcode_first[op];
	
	// without parameters
	if (op == 3  // eos
	 || op == 6  // nop
	 || op == 8  // loopend
	 || op == 16 // sfx
	 || op == 23 // resume
	 || op == 24)// pausel
	{
		if (parser.getLeft() != 1)
		{
			parser.printError(parser.getStart(1), "Unexpected additional parameters");
			return 1;
		}
		// resume
		else if (op == 23)
		{
			code[1] = 2;
			code[2] = 0;
		}
		// pausel
		else if (op == 24)
		{
			code[1] = 3;
			code[2] = 0;
		}
		// return success
		return 0;
	}
	
	// one parameter
	if (op != 12 // mute
	 && op != 19 // mailbox
	 && op != 20 // if
	 && op != 27)// INVALIDEXTRA
	{
		int end = parser.getLeft();
		if (end == 1)
		{
			parser.printError(parser.getEnd(0), "Expression expected");
			return 2;
		}
		int readed;
		int val = parser.getExpression(1,end,readed);
		if (readed != end - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getStart(1+readed), "Unexpected additional parameters");
			return 1;
		}
		// note
		if (op == 0)
		{
			if (val < 0 || val > 0x5F)
			{
				parser.printError(parser.getStart(1),"Invalid value");
				return 3;
			}
			code[0] = val;
			return 0;
		}
		// delay and duration
		if (op == 1 || op == 2)
		{
			code.clear();
			for (int i=12; i>=0; i-=6)
				if (i == 0 || (val >> i) !=0 || pass == 2)
					code.push_back(((val >> i) & 0x3F)
								   |(op == 1? 0xC0 : 0x80));
			return 0;
		}

		// one raw byte param
		if (op != 11 // tempo
		 && op != 15 // pitch
		 && op != 18)// goto
		{
			if (val < -0x80 || val > 0xFF)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			code[1] = val;
			// stop
			if (op == 21)
			{
				code[1] = 0;
				code[2] = val;
			}
			// pause
			if (op == 22)
			{
				code[1] = 1;
				code[2] = val;
			}
			// mastervolume
			if (op == 25)
			{
				code[1] = 4;
				code[2] = val;
			}
			// volume
			if (op == 26)
			{
				code[1] = 5;
				code[2] = val;
			}
			return 0;
		}
		// tempo
		if (op == 11)
		{
			if (val < 40 || val > 0xFF+40)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			code[1] = val-40;
			return 0;
		}
		// pitch
		if (op == 15)
		{
			if (val < -0x8000 || val > 0xFFFF)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			SetWordLE(&code[1], val);
			return 0;
		}
		// goto
		if (op == 18)
		{
			val -= parser.origin+3;
			if (val < -0x8000 || val > 0x7FFF)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			SetWordLE(&code[1], val);
			return 0;
		}
		// unreachable
		parser.printError(parser.getStart(1), "Unexpected error");
		return 4;
	}

	// simple two parameters
	if (op == 12 // mute
	 || op == 19 // mailbox
	 || op == 27)// INVALIDEXTRA
	{
		if (parser.getLeft() == 1)
		{
			parser.printError(parser.getEnd(0), "Expression expected");
			return 2;
		}
		int del = -1;
		for (int i=0; i<parser.getLeft(); ++i)
			if (parser.getType(i) == TOKEN_DEL
			 && parser.cmp(i, ",") == 0)
			{
				del = i;
				break;
			}
		int end = del;
		if (end == -1)
			end = parser.getLeft();
		int readed;
		int val1 = parser.getExpression(1, end, readed);
		if (readed != end - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getEnd(readed), "Invalid expression");
			return 5;
		}
		if (del == -1)
		{
			parser.printError(parser.getEnd(parser.getLeft() - 1), "\",\" expected");
			return 6;
		}
		if (parser.getLeft() == del + 1)
		{
			parser.printError(parser.getEnd(del), "Expression expected");
			return 2;
		}
		end = parser.getLeft();
		int val2 = parser.getExpression(del + 1, end, readed);
		if (readed != end - del - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getEnd(del + readed), "Invalid expression");
			return 5;
		}
		// mute
		if (op == 12)
		{
			if (val1 < 0 || val1 > 1)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			if (val2 < 0 || val2 > 0xF)
			{
				parser.printError(parser.getStart(del + 1), "Invalid value");
				return 3;
			}
			code[1] = (val1<<4)|val2;
			return 0;
		}
		// mailbox
		if (op == 19)
		{
			if (val1 < 0 || val1 >= 30)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			if (val2 < -0x80 || val2 > 0xFF)
			{
				parser.printError(parser.getStart(del + 1), "Invalid value");
				return 3;
			}
			code[1] = val1;
			code[2] = val2;
			return 0;
		}
		// INVALIDEXTRA
		if (op == 27)
		{
			if (val1 < -0x80 || val1 >= 0xFF)
			{
				parser.printError(parser.getStart(1), "Invalid value");
				return 3;
			}
			if (val2 < -0x80 || val2 > 0xFF)
			{
				parser.printError(parser.getStart(del + 1), "Invalid value");
				return 3;
			}
			code[1] = val1;
			code[2] = val2;
			return 0;
		}
		parser.printError(parser.getStart(1),"Unexpected error");
		return 4;
	}
	// if
	if (op == 20)
	{
		int cond = -1;
		int del = -1;
		for (int i=0; i<parser.getLeft(); ++i)
			if (parser.getType(i) == TOKEN_EQ)
				cond = i;
		for (int i=0; i<parser.getLeft(); ++i)
			if (parser.getType(i) == TOKEN_DEL
			 && parser.cmp(i, ",") == 0)
			{
				del = i;
				break;
			}
		int end = cond;
		if (end == -1)
			end = parser.getLeft();
		int readed;
		int val1 = parser.getExpression(1, end, readed);
		if (readed != end - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getEnd(readed), "Invalid expression");
			return 5;
		}
		if (cond == -1)
		{
			parser.printError(parser.getEnd(parser.getLeft() - 1), "condition expected");
			return 2;
		}
		// real conditions, but used ELSE {"=","!=",">",">=","<","<="}
		static char *ifc[] = {"!=","=","<=","<",">=",">"};
		int ifc_id = -1;
		for (int i=0; i<sizeof(ifc)/sizeof(ifc); ++i)
			if (parser.cmp(cond, ifc[i]) == 0)
				ifc_id = i;
		if (ifc_id == -1)
		{
			parser.printError(parser.getStart(cond), "Unknown condition");
			return 2;
		}
		if (parser.getLeft() == cond + 1)
		{
			parser.printError(parser.getEnd(cond), "Expression expected");
			return 2;
		}
		if (del == -1)
			end = parser.getLeft();
		else
			end = del;
		int val2 = parser.getExpression(cond + 1, end, readed);
		if (readed != end - cond - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getEnd(cond + readed), "Invalid expression");
			return 5;
		}
		if (del == -1)
		{
			parser.printError(parser.getEnd(parser.getLeft() - 1), "\",\" expected");
			return 2;
		}
		end = parser.getLeft();
		int val3 = parser.getExpression(del + 1, end, readed);
		if (readed != end - del - 1)
		{
			if (readed == 0)
				readed = 1;
			parser.printError(parser.getEnd(del + readed), "Invalid expression");
			return 5;
		}
		code[1] = val1;
		code[2] = ifc_id;
		code[3] = val2;
		val3 -= parser.origin+5;
		if (val3 < 0 || val3 > 0xFF)
		{
			parser.printError(parser.getStart(del+1), "Invalid value");
			return 3;
		}
		code[4] = val3;
		return 0;
	}

	parser.printError(parser.getStart(1),"Unexpected error");
	return 4;
}

struct hashed_vector
{
	vector<BYTE> v;
	void updatehash()
	{
		hash = 0;
		for (int i=0; i<v.size(); ++i)
			hash = (hash << 1) + hash>>28 + v[i];
	}
	void set(string s)
	{
		v.resize(s.length());
		for (int i=0; i<v.size(); ++i)
			v[i] = s[i];
		updatehash();
	}
	hashed_vector substr(int from, int to)
	{
		hashed_vector r;
		r.v.resize(to-from);
		for (int i=from; i<to; ++i)
			r.v[i-from] = v[i];
		return r;
	}
	unsigned int hash;
	bool operator < (const hashed_vector &a) const
	{
		if (hash != a.hash)
			return hash < a.hash;
		if (v.size() != a.v.size())
			return v.size() < a.v.size();
		return memcmp(&v[0],&a.v[0],v.size()) < 0;
	}
};

vector<BYTE> substr(const vector<BYTE> &line, int from, int to)
{
	return vector<BYTE>(line.begin()+from,line.begin()+to);
}

// expand names
int expand_names(Parser &parser, const map<hashed_vector,int> &symbols, vector<int> &not_found)
{
	/*printf("EXPAND\n%s\nSYMBOLS\n",string((char *)&parser.line[0],parser.line.size()).c_str());
	for (map<hashed_vector,int>::const_iterator ii=symbols.begin(); ii!=symbols.end(); ++ii)
	{
		printf("%s %d\n",string((const char*)&ii->first.v[0],ii->first.v.size()).c_str(),ii->second);
	}*/
	// vector for searching
	hashed_vector hv;

	// iterator for searching
	map<hashed_vector, int>::const_iterator f;
	
	// new line, new index
	vector<BYTE> new_line;
	vector<int> new_index;
	
	// mark that we done some replacing
	bool found = true;

	// while we found something and expanded it
	while(found)
	{
		// make new empty line
		new_line.clear();
		new_index.clear();

		// clear not_found collection
		not_found.clear();
		
		// not found replacement yet
		found = false;

		// previous position
		int pp = 0;
		for (int i=0; i<parser.start.size(); ++i)
		{
			if (parser.type[i] != TOKEN_NAME)
				continue;

			int start = parser.start[i];
			int len = parser.end[i] - start;
			
			hv.v.resize(len);
			for (int j=0; j<len; ++j)
				hv.v[j] = parser.line[j+start];
			hv.updatehash();
			
			f = symbols.find(hv);
			if (f == symbols.end())
			{
				// not found, add index
				not_found.push_back(i);
			}
			else
			{
				// found, do replace.
				int v = f->second;
				
				// make string
				char buff[22];
				sprintf(buff,"%d", v);
				
				// add all bytes skiped after previous iteration
				while (pp < start)
				{
					new_line.push_back(parser.line[pp]);
					new_index.push_back(parser.index[pp]);
					++pp;
				}
				
				// write replacement
				for (int j=0; buff[j]; ++j)
				{
					new_line.push_back(buff[j]);
					new_index.push_back(parser.index[start]);
				}
				
				pp = parser.end[i];
				found = true;
			}
		}
		if (found)
		{
			// replace line and index
			parser.line = new_line;
			parser.index = new_index;

			// parse line again
			parseline(parser.line, parser.start, parser.end, parser.type);
		}
	}
	return 0; // return success
}

map<hashed_vector,int> patches;
map<hashed_vector,int> envelopes;
map<hashed_vector,int> samples;
int patches_count = 0;
int envelopes_count = 0;
int samples_count = 0;
int sequences_count = 0;

// last imported dac patch
int dac_patch = 0;

map<hashed_vector,int> export_strings;
vector<int> sequence_title;
vector<int> sequence_game;
vector<int> sequence_author;

int exportString(const hashed_vector &hv)
{
	map<hashed_vector,int>::iterator f;
	f = export_strings.find(hv);
	if (f != export_strings.end())
		return f->second;
	int r = export_strings.size();
	export_strings[hv] = r;
	return r;
}

void exportString_init()
{
	exportString(hashed_vector());
}

int include_instrument(string path)
{
	string dir = getdir(path);
	string file = string(path).substr(dir.length());
	
	FILE *cfg = fopen(path.c_str(),"rb");
	if (!cfg)
	{
		printf("Can't open \"%s\"\n",path.c_str());
		return -1;
	}

	GemsFM fm;
	vector<BYTE> psg;
	int ptype = GEMSI_FM; // FM default;
	int dac_param = 4; // override samplerate
	
	std::vector<BYTE> line;
	std::vector<int> start,end,type;
	int l = 0; // line number
	while(readline(cfg,line,start,end,type))
	{
		// inc line number
		++l;
		// skip empty line
		if (!start.size())
			continue;
		
		int k = 0;
		if (type[0] == TOKEN_SP)
			k = 1;
		if (type[k] != TOKEN_NAME)
		{
			printf("Syntax Error in %s(%d,%d): name or instruction expected\n", file.c_str(), l, start[k]+1);
			continue;
		}
		
		int cid = -1;
		
		char *operations[] = {"importraw","DAC"};
		for (int i=0; i<sizeof(operations)/sizeof(operations[0]); ++i)
			if (strcmp(line,start[k],end[k],operations[i]) == 0)
			{
				cid = i;
				break;
			}
		
		string ppath;
		if (cid == 0)
		{
			if (start.size() < k+2)
			{
				printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, end[k]);
				continue;
			}
			if (type[k+1] != TOKEN_STR)
			{
				printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[k+1]);
				continue;
			}
			if (start.size() != k+2)
			{
				printf("Syntax Error in %s(%d,%d): extra parameters unexpected\n", file.c_str(), l, start[k+2]);
				continue;
			}
			int j = k+1;
			// path len
			int plen = end[j]-start[j]-2;
			
			ppath = dir + string((char*)&line[start[j]+1],plen);
		}
		// importraw
		if (cid == 0)
		{
			vector<BYTE> v;
			int err = readall(ppath, v);
			if (err)
			{
				printf("Error: can't %s file \"%s\"\n",readall_err[err],ppath.c_str());
				continue;
			}
			switch(v[0])
			{
				case GEMSI_FM:
					fm.Set(&v[0]);
					ptype = GEMSI_FM;
					break;

				case GEMSI_DAC:
					ptype = GEMSI_DAC;
					dac_param = v[1];
					break;

				case GEMSI_PSG:
				case GEMSI_NOISE:
					ptype = v[0];
					psg = v;
					break;
			}
		}
		
		// DAC
		if (cid == 1)
		{
			if (start.size() < k+2)
			{
				printf("Syntax Error in %s(%d,%d): number expected\n", file.c_str(), l, end[k]);
				continue;
			}
			if (type[k+1] != TOKEN_NUM)
			{
				printf("Syntax Error in %s(%d,%d): number expected\n", file.c_str(), l, start[k+1]);
				continue;
			}
			if (start.size() != k+2)
			{
				printf("Syntax Error in %s(%d,%d): extra parameters unexpected\n", file.c_str(), l, start[k+2]);
				continue;
			}
			ptype = GEMSI_DAC;
			int tmp;
			int len = ScanNum((char*)&line[start[k+1]],tmp);
			if (len != end[k+1]-start[k+1])
			{
				printf("Syntax Error in %s(%d,%d): invalid number\n", file.c_str(), l, start[k+2]);
				continue;
			}
			dac_param = tmp;
		}
		
		if (cid == -1)
		{
			printf("Error in %s(%d,%d): unknown command \"%s\"\n", file.c_str(), l, start[k],string((char*)&line[start[k]],end[k]-start[k]).c_str());
		}
	}
	fclose(cfg);

	// make BYTE dump of instrument
	hashed_vector hv;
	switch(ptype)
	{
	case GEMSI_FM:
		hv.v.resize(39);
		fm.Write(&hv.v[0]);
		break;
	case GEMSI_DAC:
		hv.v.resize(2);
		hv.v[1] = dac_param;
		break;
	case GEMSI_PSG:
	case GEMSI_NOISE:
		hv.v = psg;
		break;
	}
	hv.v[0] = ptype;
	// update dump hash
	hv.updatehash();

	// check existing copy
	map<hashed_vector,int>::iterator f;
	f = patches.find(hv);
	if (f == patches.end())
	{
		if (ptype == GEMSI_DAC)
			dac_patch = patches_count;
		patches[hv] = patches_count;
		return patches_count++;
	}
	if (ptype == GEMSI_DAC)
		dac_patch = f->second;
	return f->second;
}

int include_modulation(string path)
{
	hashed_vector hv;
	int err = readall(path,hv.v);
	if (err)
	{
		printf("Error: can't %s file \"%s\"\n",readall_err[err],path.c_str());
		return -1;
	}
	hv.updatehash();

	// check existing copy
	map<hashed_vector,int>::iterator f;
	f = envelopes.find(hv);
	if (f == envelopes.end())
	{
		envelopes[hv] = envelopes_count;
		return envelopes_count++;
	}
	return f->second;
}

int include_sample(string path)
{
	string dir = getdir(path);
	string file = string(path).substr(dir.length());
	
	FILE *cfg = fopen(path.c_str(),"rb");
	if (!cfg)
	{
		printf("Can't open \"%s\"\n",path.c_str());
		return -1;
	}

	vector<BYTE> snd;
	hashed_vector hv;
	hv.v.resize(12);
	
	std::vector<BYTE> line;
	std::vector<int> start,end,type;
	int l = 0; // line number
	while(readline(cfg,line,start,end,type))
	{
		// inc line number
		++l;
		// skip empty line
		if (!start.size())
			continue;
		
		int k = 0;
		if (type[0] == TOKEN_SP)
			k = 1;
		if (type[k] != TOKEN_NAME)
		{
			printf("Syntax Error in %s(%d,%d): instruction or expected\n", file.c_str(), l, start[k]);
			continue;
		}
		
		int cid = -1;
		int val = 0;
		
		char *operations[] = {"FLAGS","SKIP","FIRST","LOOP","END","RAW"};
		for (int i=0; i<sizeof(operations)/sizeof(operations[0]); ++i)
			if (strcmp(line,start[k],end[k],operations[i]) == 0)
			{
				cid = i;
				break;
			}
		if (cid >= 0 && cid < 5)
		{
			if (start.size() < k+2)
			{
				printf("Syntax Error in %s(%d,%d): \"=\" expected\n", file.c_str(), l, start[k]);
				continue;
			}
			if (line[start[k+1]] != '=')
			{
				printf("Syntax Error in %s(%d,%d): \"=\" expected\n", file.c_str(), l, start[k+1]);
				continue;
			}
			if (end[k+1]-start[k+1] != 1)
			{
				printf("Syntax Error in %s(%d,%d): number expected\n", file.c_str(), l, start[k+1]+1);
				continue;
			}
			if (start.size() < k+3)
			{
				printf("Syntax Error in %s(%d,%d): number expected\n", file.c_str(), l, end[k+1]);
				continue;
			}
			if (type[k+2] != TOKEN_NUM)
			{
				printf("Syntax Error in %s(%d,%d): number expected\n", file.c_str(), l, start[k+2]);
				continue;
			}
			if (start.size() != k+3)
			{
				printf("Syntax Error in %s(%d,%d): extra parameters unexpected\n", file.c_str(), l, start[k+3]);
				printf("%s\n",string((char*)&line[start[k+3]],end[k+3]-start[k+3]).c_str());
				continue;
			}
			int len = ScanNum((char*)&line[start[k+2]],val);
			if (len != end[k+2]-start[k+2])
			{
				printf("Syntax Error in %s(%d,%d): invalid number\n", file.c_str(), l, start[k+2]);
				continue;
			}
		}
		// FLAGS
		if (cid == 0)
			hv.v[0] = val;
		
		// SKIP
		if (cid == 1)
			SetWordLE(&hv.v[4],val);
		
		// FIRST
		if (cid == 2)
			SetWordLE(&hv.v[6],val);
		
		// LOOP
		if (cid == 3)
			SetWordLE(&hv.v[8],val);
		
		// END
		if (cid == 4)
			SetWordLE(&hv.v[10],val);
		
		if (cid == 5)
		{
			if (start.size() < k+2)
			{
				printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, end[k]);
				continue;
			}
			if (type[k+1] != TOKEN_STR)
			{
				printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[k+1]);
				continue;
			}
			if (start.size() != k+2)
			{
				printf("Syntax Error in %s(%d,%d): extra parameters unexpected\n", file.c_str(), l, start[k+2]);
				continue;
			}
			int j = k+1;
			// path len
			int plen = end[j]-start[j]-2;
			
			string ppath = dir + string((char*)&line[start[j]+1],plen);
			int err = readall(ppath,snd);
			if (err)
			{
				printf("Error: can't %s file \"%s\"\n",readall_err[err],ppath.c_str());
			}
		}
		
		if (cid == -1)
		{
			printf("Error in %s(%d,%d): unknown command \"%s\"\n", file.c_str(), l, start[k],string((char*)&line[start[k]],end[k]-start[k]).c_str());
		}
	}
	fclose(cfg);
	
	hv.v.resize(12 + snd.size());
	for (int i=0; i<snd.size(); ++i)
		hv.v[i+12] = snd[i];
	hv.updatehash();
	
	// check existing copy
	map<hashed_vector,int>::iterator f;
	f = samples.find(hv);
	if (f == samples.end())
	{
		samples[hv] = samples_count;
		return samples_count++;
	}
	return f->second;
}

struct Section
{
	int id, start, origin, current;
	vector<BYTE> code;
};

struct Pass2Fix
{
	Parser parser;
	int section;
	int sequence;
};

struct label
{
	int section;
	int offset;
	int sequence;
	hashed_vector name;
};

Section *section; // current section
vector<Section> sections;
vector<int> sequence_offset;

vector<Pass2Fix> pass2_fixes;
vector<label> labels;
vector< map<hashed_vector,int> > symbols;

void include_code(string path, int sequence)
{
	Parser parser;
	
	FILE *f = fopen(path.c_str(),"rb");
	if (!f)
	{
		printf("Error : can't open \"%s\"\n",path.c_str());
		//return 1;
	}
	
	parser.setPath(path);
	
	// encoded gems opcode
	vector<BYTE> code;

	// not found symbols
	vector<int> not_found;

	parser.line_n = 0;
	parser.origin = section->current;
	while (readline(f, parser.line, parser.start, parser.end, parser.type))
	{
		// setup size of indexes. additional one for index of end of line
		parser.index.resize(parser.line.size()+1);
		for (int i=0; i<parser.index.size(); ++i)
			parser.index[i] = i; // plain map i = i.

		++parser.line_n;
		parser.current = 0;
		
		// check for empty line
		if (parser.getLeft() == 0)
			continue;
		
		int err = expand_names(parser, symbols[sequence], not_found);
		int ex = not_found.size(); // count of not found symbols
		
		if (parser.getType(0) == TOKEN_SP)
			++parser.current;
		
		if (parser.getType(0) != TOKEN_NAME)
		{
			parser.printError(parser.getStart(0), "Label or name or instruction expected");
			continue;
		}

		int op = -1;
		parser.origin = section->current;
		err = gems_encode(parser, code, 0, op);
		if (op != -1)
		{
			// opcode detected
			if (ex == 1)
			{
				// all expanded
				gems_encode(parser, code, 1, op);
			}
			else
			{
				// need to fix later
				Pass2Fix pf;
				pf.parser = parser;
				pf.section = section->id;
				pf.sequence = sequence;
				pass2_fixes.push_back(pf);
			}
			for (int i=0; i<code.size(); ++i)
			{
				section->code.push_back(code[i]);
				++section->current;
			}
			continue;
		}
		
		
		int cid = -1;
		
		char *operations[] = {"SECTION", "include"};
		for (int i=0; i<sizeof(operations)/sizeof(operations[0]); ++i)
			if (parser.cmp(0,operations[i]) == 0)
			{
				cid = i;
				break;
			}
		
		// SECTION
		if (cid == 0)
		{
			if (ex != 1)
			{
				parser.printError(parser.getStart(not_found[1]), "Undefined name");
				continue;
			}
			int readed;
			int val = parser.getExpression(1, parser.getLeft(), readed);
			if (readed != parser.getLeft() - 1)
			{
				parser.printError(parser.getStart(1),"Invalid expression");
				continue;
			}
			if (val < 0 || val > 2)
			{
				parser.printError(parser.getStart(1),"Invalid value");
				continue;
			}
			section = &sections[val];
			continue;
		}
		
		// include
		if (cid == 1)
		{
			if (parser.getLeft() == 1)
			{
				parser.printError(parser.getEnd(0), "Path expected");
				continue;
			}
			if (parser.getType(1) != TOKEN_STR)
			{
				parser.printError(parser.getStart(1), "Path expected");
				continue;
			}
			if (parser.getLeft() > 2)
			{
				parser.printError(parser.getStart(2), "Unexpected additional parameters");
				continue;
			}
			include_code(parser.appendPath(1), sequence);
			continue;
		}
		
		// label check
		if (parser.getLeft() > 1 && parser.getType(1) == TOKEN_DEL && parser.cmp(1,":") == 0)
		{
			label l;
			l.offset = section->current;
			l.section = section->id;
			l.sequence = sequence;
	
			int s = parser.getStart(0);
			int e = parser.getEnd(0);
			
			// setup label size
			l.name.v.resize(e-s);
			// copy label name
			for (int j=0; j<e-s; ++j)
				l.name.v[j] = parser.line[j+s];
			l.name.updatehash();
			// add label
			labels.push_back(l);
			continue;
		}
		
		parser.printError(parser.getStart(0), "Unknown instruction");
	}
	fclose(f);
}

void make_pass2_fixes()
{
	// section PTR
	section = &sections[0];
	
	// setup section PTRS
	section->origin = 0;
	section->code.resize(sequences_count*2);

	// write section PTRS
	for (int i=0; i<sequences_count; ++i)
		SetWordLE(&section->code[i*2], sequence_offset[i] + sequences_count * 2);
	
	// move section HEADER to its position
	section = &sections[1];
	section->origin = sections[0].code.size();
	
	// move section CODE to its position
	sections[2].origin = section->origin + section->code.size();
	
	for (int i=0; i<3; ++i)
	{
		printf("SECTION AT %X SIZE %X\n",sections[i].origin, sections[i].code.size());
	}
	
	// add labels symbols
	for (int i=0; i<labels.size(); ++i)
	{
		label &l = labels[i];
		symbols[l.sequence][l.name] = sections[l.section].origin + l.offset; 
	}
	
	// not found symbols
	vector<int> not_found;

	// encoded gems opcode
	vector<BYTE> code;

	for (int i=0; i<pass2_fixes.size(); ++i)
	{
		Parser &parser = pass2_fixes[i].parser;
		parser.origin += sections[pass2_fixes[i].section].origin;

		int err = expand_names(parser, symbols[pass2_fixes[i].sequence], not_found);
		int ex = not_found.size(); // count of not found symbols
		
		if (parser.getType(0) == TOKEN_SP)
			++parser.current;

		int op = -1;
		err = gems_encode(parser, code, 0, op);
		if (op == -1)
		{
			parser.printError(parser.getStart(not_found[1]),"Vanished opcode with overwrited name");
			continue;
		}
		
		// opcode detected
		if (ex == 1)
		{
			// all expanded, pass 2
			gems_encode(parser, code, 2, op);
		}
		else
		{
			// symbol not found
			parser.printError(parser.getStart(not_found[1]),"Undefined name");
		}

		// set section
		section = &sections[pass2_fixes[i].section];
		
		// position where rewrite
		int origin = parser.origin - sections[pass2_fixes[i].section].origin;
		
		// check index overflow
		if (origin + code.size() > section->code.size())
		{
			parser.printError(parser.getStart(0),"Unexpected code size overflow");
			continue;
		}

		// ovewrite code
		for (int i=0; i<code.size(); ++i)
		{
			section->code[origin+i] = code[i];
			++section->current;
		}
	}
}

void include_sequence(string path, map<hashed_vector,int> names)
{
	Parser parser;
	parser.setPath(path);

	// reset section to header
	section = &sections[1];
	
	// align
	/*if ((section->current)&1)
	{
		++section->current;
		section->code.push_back(0);
	}
	
	if ((sections[2].current)&1)
	{
		++sections[2].current;
		sections[2].code.push_back(0);
	}*/

	// add section header offset
	sequence_offset.push_back(section->current);
	
	// add symbols
	symbols.push_back(names);
	
	// add title, game, author
	sequence_title.push_back(0);
	sequence_game.push_back(0);
	sequence_author.push_back(0);
	
	// add sections count;
	++sequences_count;
	
	FILE *cfg = fopen(path.c_str(),"rb");
	if (!cfg)
	{
		printf("Can't open \"%s\"\n",path.c_str());
		return;
	}
	
	parser.line_n = 0; // line number
	while(readline(cfg,parser.line,parser.start,parser.end,parser.type))
	{
		// inc line number
		++parser.line_n;
		
		// set start
		parser.current = 0;

		// skip empty line
		if (!parser.getLeft())
			continue;
		
		if (parser.getType(0) == TOKEN_SP)
			++parser.current;
		
		if (parser.getType(0) != TOKEN_NAME)
		{
			parser.printError(parser.getStart(0), "Instruction expected");
			continue;
		}
		
		int cid = -1;
		
		char *operations[] = {"code","instrument","modulation","sample","title","game","author"};
		for (int i=0; i<sizeof(operations)/sizeof(operations[0]); ++i)
			if (parser.cmp(0,operations[i]) == 0)
			{
				cid = i;
				break;
			}
		
		// code
		if (cid == 0) 
		{
			if (parser.getLeft() == 1)
			{
				parser.printError(parser.getEnd(0), "Path expected");
				continue;
			}
			if (parser.getType(1) != TOKEN_STR)
			{
				parser.printError(parser.getStart(1), "Path expected");
				continue;
			}
			if (parser.getLeft() != 2)
			{
				parser.printError(parser.getStart(2), "Extra parameters unexpected");
				continue;
			}
			// code file

			include_code(parser.appendPath(1), sequences_count-1);
		}
		
		hashed_vector name;
		string ppath;
		if (cid > 0 && cid < 4)
		{
			if (parser.getLeft() == 1)
			{
				parser.printError(parser.getEnd(0), "Path or name expected");
				continue;
			}
			if (parser.getType(1) != TOKEN_STR && parser.getType(1) != TOKEN_NAME)
			{
				parser.printError(parser.getStart(1), "Path or name expected");
				continue;
			}
			
			// name
			if (parser.getType(1) == TOKEN_NAME)
			{
				if (parser.getLeft() == 2)
				{
					parser.printError(parser.getEnd(1), "\",\" expected");
					continue;
				}
				if (parser.cmp(2,",") != 0)
				{
					parser.printError(parser.getStart(2), "\",\" expected");
					continue;
				}
				if (parser.getLeft() == 3)
				{
					parser.printError(parser.getEnd(2),"Path expected");
					continue;
				}
				if (parser.getType(3) != TOKEN_STR)
				{
					parser.printError(parser.getStart(3),"Path expected\n");
					continue;
				}
				if (parser.getLeft() != 4)
				{
					parser.printError(parser.getStart(4),"Extra parameters unexpected");
					continue;
				}
				
				name.v = substr(parser.line, parser.getStart(1), parser.getEnd(1));
				name.updatehash();

				parser.current += 2;
			}
			ppath = parser.appendPath(1);
		}
		// instrument
		if (cid == 1)
		{
			int id = include_instrument(ppath);
			if (name.v.size())
				symbols[sequences_count-1][name] = id;
		}
		// modulation
		if (cid == 2)
		{
			int id = include_modulation(ppath);
			if (name.v.size())
				symbols[sequences_count-1][name] = id;
		}
		// sample
		if (cid == 3)
		{
			int id = include_sample(ppath);
			if (name.v.size())
				symbols[sequences_count-1][name] = (id+0x30)%0x60;
		}
		
		if (cid == 4 // title
		 || cid == 5 // game
		 || cid == 6)// author
		{
			if (parser.getLeft() == 1)
			{
				parser.printError(parser.getEnd(0), "\"=\" expected");
				continue;
			}
			if (parser.cmp(1,"=") != 0)
			{
				parser.printError(parser.getStart(1), "\"=\" expected");
				continue;
			}
			if (parser.getLeft() == 2)
			{
				parser.printError(parser.getEnd(1), "String expected");
				continue;
			}
			if (parser.getType(2) != TOKEN_STR)
			{
				parser.printError(parser.getStart(2), "String expected");
				continue;
			}
			if (parser.getLeft() != 3)
			{
				parser.printError(parser.getStart(3), "Extra parameters unexpected");
				continue;
			}
			
			// trim quotes
			int start = parser.getStart(2);
			int end =  parser.getEnd(2);
			if (start != end && (parser.line[start]=='\'' || parser.line[start]=='"'))
				++start;
			if (start != end && (parser.line[end-1]=='\'' || parser.line[end-1]=='"'))
				--end;
			
			name.v = substr(parser.line, start, end);
			name.updatehash();
			int nameid = exportString(name);
			if (cid == 4)
				sequence_title[sequences_count-1] = nameid;

			if (cid == 5)
				sequence_game[sequences_count-1] = nameid;

			if (cid == 6)
				sequence_author[sequences_count-1] = nameid;
		}
		if (cid == -1)
		{
			parser.printError(parser.getStart(0), "Unknown command");
		}
	}
	fclose(cfg);
}

int include_cfg(string path, map<hashed_vector,int> symbols)
{
	string dir = getdir(path);
	string file = path.substr(dir.length());
	
	FILE *cfg = fopen(path.c_str(),"rb");
	if (!cfg)
	{
		printf("Can't open \"%s\"\n",path.c_str());
		return 1;
	}
	
	std::vector<BYTE> line;
	std::vector<int> start,end,type;
	int l = 0; // line number
	while(readline(cfg,line,start,end,type))
	{
		// inc line number
		++l;
		// skip empty line
		if (!start.size())
			continue;

		int k = 0;
		if (type[0] == TOKEN_SP)
			k = 1;
		if (type[k] != TOKEN_NAME)
		{
			printf("Syntax Error in %s(%d,%d): instruction expected\n", file.c_str(), l, start[k]+1);
			continue;
		}
		if (strcmp(line,start[k],end[k],"sequence")==0)
		{
			if (start.size() == k+2 || start.size() == k+4)
			{
				int j = k+1;
				if ( start.size() == k+4)
				{
					j = k+3;
					if (type[k+1] != TOKEN_NAME)
					{
						printf("Syntax Error in %s(%d,%d): name expected\n", file.c_str(), l, start[k+1]);
						continue;
					}
					else if (line[start[k+2]] != ',')
					{
						printf("Syntax Error in %s(%d,%d): \",\" expected\n", file.c_str(), l, start[k+2]);
						continue;
					}
					else if (end[k+2]-start[k+2] != 1)
					{
						printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[k+2]+1);
						continue;
					}
				}
				// opname 'path'
				if (type[j] != TOKEN_STR)
					printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[j]);
				
				// path len
				int plen = end[j]-start[j]-2;
				
				string path = dir + string((char*)&line[start[j]+1],plen);
				
				include_sequence(path,symbols);
			}
		}
	}
	fclose(cfg);
	return 0;
}

int write_bank(string path, map<hashed_vector,int> &bank)
{
	int count = bank.size();

	// compute pointers (additional one for last entry size)
	vector<int> ptrs(count+1);

	// first, set sizes
	for (map<hashed_vector,int>::iterator i = bank.begin();
			i != bank.end();++i)
		ptrs[i->second+1] = i->first.v.size();
	
	// first ptr = size of ptrs = count*2
	ptrs[0] = count*2;
	// now compute final ptrs
	// current = previous + size of current
	for (int i=1; i<count; ++i)
		ptrs[i] += ptrs[i-1];
	
	// create file
	FILE *b = fopen(path.c_str(),"wb");
	if (!b)
		return 3;

	// write ptrs
	for (int i=0; i<count; ++i)
	{
		BYTE buff[2];
		SetWordLE(buff,ptrs[i]);
		if (fwrite(buff,1,2,b) != 2)
		{
			fclose(b);
			return 4;
		}
	}
	
	// write entries
	for (map<hashed_vector,int>::iterator i = bank.begin();
			i != bank.end();++i)
	{
		fseek(b,ptrs[i->second],SEEK_SET);
		int writed = fwrite(&i->first.v[0],1,i->first.v.size(),b);
		if (writed != i->first.v.size())
		{
			fclose(b);
			return 4;
		}
	}
	fclose(b);
	return 0;
}

int write_samples(string path, map<hashed_vector,int> &bank)
{
	int count = bank.size();

	// compute pointers (additional one for last entry size)
	vector<int> ptrs(count+1);

	// first, set sizes
	for (map<hashed_vector,int>::iterator i = bank.begin();
			i != bank.end();++i)
		ptrs[i->second+1] = i->first.v.size() - 12;
	
	// first ptr = size of ptrs = count*12
	ptrs[0] = count*12;
	// now compute final ptrs
	// current = previous + size of current
	for (int i=1; i<count; ++i)
		ptrs[i] += ptrs[i-1];
	
	// create file
	FILE *b = fopen(path.c_str(),"wb");
	if (!b)
		return 3;

	// write samples
	for (map<hashed_vector,int>::iterator i = bank.begin();
			i != bank.end();++i)
	{
		// write header
		fseek(b,i->second*12,SEEK_SET);
		int writed = fwrite(&i->first.v[0],1,12,b);
		if (writed != 12)
		{
			fclose(b);
			return 4;
		}
		// write data
		fseek(b,ptrs[i->second],SEEK_SET);
		writed = fwrite(&i->first.v[12],1,i->first.v.size()-12,b);
		if (writed != i->first.v.size()-12)
		{
			fclose(b);
			return 4;
		}
	}
	
	// fix ptrs in headers
	for (int i=0; i<count; ++i)
	{
		BYTE buff[3];
		SetTriple(buff,ptrs[i]);

		fseek(b,i*12+1,SEEK_SET);
		if (fwrite(buff,1,3,b) != 3)
		{
			fclose(b);
			return 4;
		}
	}
	
	fclose(b);
	return 0;
}

int write_sequences(string path)
{	
	// now make all fixes of labels
	make_pass2_fixes();

	// create file
	FILE *b = fopen(path.c_str(),"wb");
	if (!b)
		return 3;

	// write sections
	for (int i=0; i<3; ++i)
	{
		// move to origin
		section = &sections[i];
		fseek(b,section->origin,SEEK_SET);
		
		// write bytes
		int writed = fwrite(&section->code[0],1,section->code.size(),b);
		if (writed != section->code.size())
		{
			fclose(b);
			return 4;
		}
	}

	fclose(b);
	return 0;
}

int write_sequences_info(string path)
{	
	// create file
	FILE *b = fopen(path.c_str(),"wb");
	if (!b)
		return 3;
	
	// define sequences count
	fprintf(b,"_sequences_count equ %d\n",sequences_count);
	fprintf(b,"_samples_count equ %d\n",samples_count);
	fprintf(b,"_dac_patch equ %d\n\n",dac_patch);
	
	// write pointers
	for (int j=0; j<sequences_count; ++j)
	{
		fprintf(b,"; %d\n",j);
		fprintf(b," dc.l _export_string_%d\n",sequence_title[j]);
		fprintf(b," dc.l _export_string_%d\n",sequence_game[j]);
		fprintf(b," dc.l _export_string_%d\n",sequence_author[j]);
	}
	fprintf(b,"\n");

	map<hashed_vector,int>::iterator i;
	// write strings
	for (i=export_strings.begin(); i!=export_strings.end(); ++i)
	{
		fprintf(b,"_export_string_%d:\n dc.b ", i->second);
		const vector<BYTE> &v = i->first.v;
		for (int j=0; j<v.size(); ++j)
			fprintf(b, "%d,", v[j]);
		fprintf(b,"0\n");
	}

	fclose(b);
	return 0;
}

void sections_init(map<hashed_vector,int> &symbols)
{
	sections.resize(3);
	for (int i=0; i<sections.size(); ++i)
	{
		sections[i].current = 0;
		sections[i].id = i;
	}

	static char *section_names[]={"PTRS","HEADER","CODE"};
	hashed_vector hv;
	for (int i=0; i<3; ++i)
	{
		hv.v.resize(strlen(section_names[i]));
		memcpy(&hv.v[0], section_names[i], hv.v.size());
		hv.updatehash();
		symbols[hv] = i;
	}
}

int main(int argc, char **args)
{
	if (argc == 3)
	{
		if (strcmp(args[2], "2"))
			usage();
		ptr3 = false;
	}
	else if (argc != 2)
		usage();
	
	map<hashed_vector,int> symbols;

	sections_init(symbols);
	
	exportString_init();
	
	string dir = getdir(args[1]);
	
	sections.resize(3);
	
	int err = include_cfg(args[1], symbols);
	if (err)
	{
		printf("Error: can't open file \"%s\"\n",args[1]);
		usage();
	}
	
	string ppath = dir+"patches.bin";
	err = write_bank(ppath, patches);
	if (err)
	{
		printf("Error: can't %s file \"%s\"\n",readall_err[err],ppath.c_str());
	}
	
	string epath = dir+"envelopes.bin";
	err = write_bank(epath, envelopes);
	if (err)
	{
		printf("Error: can't %s file \"%s\"\n",readall_err[err],epath.c_str());
	}
	
	string spath = dir+"samples.bin";
	err = write_samples(spath, samples);
	if (err)
	{
		printf("Error: can't %s file \"%s\"",readall_err[err],spath.c_str());
	}
	
	spath = dir+"sequences.bin";
	err = write_sequences(spath.c_str());
	if (err)
	{
		printf("Error: can't %s file \"%s\"",readall_err[err],spath.c_str());
	}
	
	spath = dir+"sequences_info.asm";
	err = write_sequences_info(spath.c_str());
	if (err)
	{
		printf("Error: can't %s file \"%s\"",readall_err[err],spath.c_str());
	}
	
	// print statistic
	printf("Patches: %d\n",patches_count);
	printf("Envelopes: %d\n",envelopes_count);
	printf("Sequences: %d\n",sequences_count);
	printf("Samples: %d\n",samples_count);
	return 0;
}
