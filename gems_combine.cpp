#include <cstdio>
#include <vector>
#include <map>
#include <string>
#include "common.h"
#include "instruments.h"

void usage()
{
	printf(
"Usage: .exe cfg\n"
"\n"
"      Warning: all outputs will be overwrited automatically.\n"
"      All outputs will be in directory of \"out\" from params.\n"
"\n"
"      All configuration parameters placed in config\n"
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

// read line and split into tokens
bool readline(FILE *f, vector<BYTE> &line, vector<int> &start, vector<int> &end, vector<int> &type)
{
	static BYTE buff[1024];
	line.clear();
	start.clear();
	end.clear();
	type.clear();
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
				if (t == TOKEN_STR)
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

map<hashed_vector,int> patches;
map<hashed_vector,int> envelopes;
map<hashed_vector,int> samples;
int patches_count = 0;
int envelopes_count = 0;
int samples_count = 0;
int sequences_count = 0;

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
		patches[hv] = patches_count;
		return patches_count++;
	}
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

vector<BYTE> all_code;

void include_code(string path, map<hashed_vector,int> symbols)
{
	vector<BYTE> all;
	int err = readall(path,all);
	if (err)
	{
		printf("Error: can't %s file \"%s\"\n",readall_err[err],path.c_str());
		return;
	}
	for (int i=0; i<all.size(); ++i)
	{
		if (i>=14 && strcmp(all,i-14,i,"SECTION HEADER") == 0)
		{
			char buff[100];
			sprintf(buff,"\nsong_%03d:",sequences_count);
			for (int z = 0; buff[z]; ++z)
				all_code.push_back(buff[z]);
		}
		if (i+6<all.size() && strcmp(all,i,i+7,"channel") == 0)
		{
			char buff[100];
			sprintf(buff,"song_%03d_",sequences_count);
			for (int z = 0; buff[z]; ++z)
				all_code.push_back(buff[z]);
		}
		if (i+5<all.size() && strcmp(all,i,i+6,"patch_") == 0)
		{
			char buff[100];
			hashed_vector hv;
			hv.v.resize(5+3);
			memcpy(&hv.v[0],&all[i],5+3);
			hv.updatehash();
			sprintf(buff,"$%X",symbols[hv]);
			for (int z = 0; buff[z]; ++z)
				all_code.push_back(buff[z]);
			i+=8-1;
			continue;
		}
		if (i+10<all.size() && strcmp(all,i,i+11,"modulation_") == 0)
		{
			char buff[100];
			hashed_vector hv;
			hv.v.resize(11+2);
			memcpy(&hv.v[0],&all[i],11+2);
			hv.updatehash();
			sprintf(buff,"$%X",symbols[hv]);
			for (int z = 0; buff[z]; ++z)
				all_code.push_back(buff[z]);
			i+=11+2-1;
			continue;
		}
		if (i+6<all.size() && strcmp(all,i,i+7,"sample_") == 0)
		{
			char buff[100];
			hashed_vector hv;
			hv.v.resize(7+2);
			memcpy(&hv.v[0],&all[i],7+2);
			hv.updatehash();
			int sn = symbols[hv];
			int pn = (sn + 0x30)%0x60;
			sprintf(buff,"$%X",pn);
			for (int z = 0; buff[z]; ++z)
				all_code.push_back(buff[z]);
			i+=7+2-1;
			continue;
		}
		all_code.push_back(all[i]);
	}
	++sequences_count;
}

void include_sequence(string path, map<hashed_vector,int> names)
{
	string dir = getdir(path);
	string file = string(path).substr(dir.length());
	
	FILE *cfg = fopen(path.c_str(),"rb");
	if (!cfg)
	{
		printf("Can't open \"%s\"\n",path.c_str());
		return;
	}
	
	string codepath; 
	
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
		
		int cid = -1;
		
		char *operations[] = {"code","instrument","modulation","sample"};
		for (int i=0; i<sizeof(operations)/sizeof(operations[0]); ++i)
			if (strcmp(line,start[k],end[k],operations[i]) == 0)
			{
				cid = i;
				break;
			}
		
		// code
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
			
			codepath = dir + string((char*)&line[start[j]+1],plen);
		}
		
		string name;
		string ppath;
		if (cid > 0 && cid < 4)
		{
			if (start.size() < k+2)
			{
				printf("Syntax Error in %s(%d,%d): path or name expected\n", file.c_str(), l, end[k]);
				continue;
			}
			if (type[k+1] != TOKEN_STR && type[k+1] != TOKEN_NAME)
			{
				printf("Syntax Error in %s(%d,%d): path or name expected\n", file.c_str(), l, start[k+1]);
				continue;
			}
			int j = k+1;
			
			if (type[k+1] == TOKEN_NAME)
			{
				j=k+3;
				if (start.size() < k+3)
				{
					printf("Syntax Error in %s(%d,%d): \",\" expected\n", file.c_str(), l, end[k+1]);
					continue;
				}
				if (line[start[k+2]] != ',')
				{
					printf("Syntax Error in %s(%d,%d): \",\" expected\n", file.c_str(), l, start[k+2]);
					continue;
				}
				if (end[k+2]-start[k+2] != 1)
				{
					printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[k+2]+1);
					continue;
				}
				if (start.size() < k+4)
				{
					printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, end[k+2]);
					continue;
				}
				if (type[j] != TOKEN_STR)
				{
					printf("Syntax Error in %s(%d,%d): path expected\n", file.c_str(), l, start[k+3]);
					continue;
				}
				if (start.size() != k+4)
				{
					printf("Syntax Error in %s(%d,%d): extra parameters unexpected\n", file.c_str(), l, start[k+4]);
					continue;
				}
				int nlen = end[k+1]-start[k+1];
				name = string((char*)&line[start[k+1]],nlen);
			}
			int plen = end[j]-start[j]-2;
			
			ppath = dir + string((char*)&line[start[j]+1],plen);
		}
		// instrument
		if (cid == 1)
		{
			int id = include_instrument(ppath);
			if (name.length())
			{
				hashed_vector nv;
				nv.set(name);
				names[nv] = id;
			}
		}
		// modulation
		if (cid == 2)
		{
			int id = include_modulation(ppath);
			if (name.length())
			{
				hashed_vector nv;
				nv.set(name);
				names[nv] = id;
			}
		}
		// sample
		if (cid == 3)
		{
			int id = include_sample(ppath);
			if (name.length())
			{
				hashed_vector nv;
				nv.set(name);
				names[nv] = id;
			}
		}
		if (cid == -1)
		{
			printf("Error in %s(%d,%d): unknown command \"%s\"\n", file.c_str(), l, start[k],string((char*)&line[start[k]],end[k]-start[k]).c_str());
		}
	}
	fclose(cfg);
	/*for (map<hashed_vector,int>::iterator i=names.begin(); i!=names.end(); ++i)
		printf("%s=%d\n",string((char*)&((i->first).v[0]),(i->first).v.size()).c_str(),i->second);*/
	if (codepath.length())
	{
		include_code(codepath,names);
	}
}

int include_cfg(string path)
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
				
				include_sequence(path,map<hashed_vector,int>());
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

int main(int argc, char **args)
{
	if (argc != 2)
		usage();
	
	string dir = getdir(args[1]);
	
	int err = include_cfg(args[1]);
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
	
	spath = dir+"sequences.asm";
	FILE *s = fopen(spath.c_str(),"wb");
	if (!s)
		printf("Error: can't create file \"%s\"",spath.c_str());
	
	fwrite(&all_code[0],1,all_code.size(),s);
	fprintf(s,"\n SECTION PTRS\n");
	for (int i=0; i<sequences_count; ++i)
		fprintf(s," dc.w song_%03d\n",i);
	fclose(s);
	return 0;
}