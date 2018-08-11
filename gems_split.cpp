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
#include <cstdlib>
#include <vector>
#include <cerrno>
#include <direct.h>
#include "common.h"
#include "instruments.h"

void usage()
{
	printf(
"Usage: .exe patches envelopes sequences samples out [flags]\n"
"\n"
"Flags: characters in any order\n"
"       3 - use 3 bytes ptr in sequences (example: comix zone)\n"
"\n"
"      Warning: all outputs will be overwrited automatically.\n"
"      All outputs will be in directory of \"out\" from params.\n"
"\n"
"      This program makes full configuration\n"
"      for further combination in new banks by gems_combine\n"
"      Makes all sequences independent between each other\n"
"      Makes all instruments independent between each other\n"
"      And so on...\n"
"\n"
"Author: r57shell@uralweb.ru\n");
	exit(0);
}

// decode sequence command
int gems_decode(const BYTE *code, char *text)
{
	if (code[0]&0x80) 
	{
		// delay or duration
		int len = 0, val = 0;
		for (len = 0; len < 10; ++len)
		{
			if ((code[len] & 0xC0) != (code[0] & 0xC0))
				break;
			val = (val << 6) | (code[len] & 0x3F);
		}
		sprintf(text,"%s %d",((code[0]&0xC0)==0xC0)?"delay":"duration", val);
		return len;
	}
	if (code[0]<0x60)
	{
		sprintf(text,"note $%02X", (int)code[0]);
		return 1;
	}
	switch (code[0])
	{
		case 0x60:
			sprintf(text,"eos");
			return 1;
		case 0x61:
			sprintf(text,"patch $%02X",(int)code[1]);
			return 2;
		case 0x62:
			sprintf(text,"modulation $%02X",(int)code[1]);
			return 2;
		case 0x63:
			sprintf(text,"nop");
			return 1;
		case 0x64:
			sprintf(text,"loop $%02X",(int)code[1]);
			return 2;
		case 0x65:
			sprintf(text,"loopend");
			return 1;
		case 0x66:
			sprintf(text,"retrigger %d",code[1]?1:0);
			return 2;
		case 0x67:
			sprintf(text,"sustain %d",code[1]?1:0);
			return 2;
		case 0x68:
			sprintf(text,"tempo %d",(int)code[1]+40);
			return 2;
		case 0x69:
			sprintf(text,"mute %d,%d",code[1]&0x10?1:0,(int)code[1]&0xF);
			return 2;
		case 0x6A:
			sprintf(text,"priority %d",(int)code[1]);
			return 2;
		case 0x6B:
			sprintf(text,"play %d",(int)code[1]);
			return 2;
		case 0x6C:
			sprintf(text,"pitch $%04X",GetWordLE(code+1));
			return 3;
		case 0x6D:
			sprintf(text,"sfx");
			return 1;
		case 0x6E:
			sprintf(text,"samplerate %d",(int)code[1]);
			return 2;
		case 0x6F:
			sprintf(text,"goto $%04X",GetWordLE(code+1));
			return 3;
		case 0x70:
			sprintf(text,"mailbox %d,%d",(int)code[1],(int)code[2]);
			return 3;
		case 0x71:
		{
			// real conditions, but used ELSE {"=","!=",">",">=","<","<="}
			const char *ifc[] = {"!=","=","<=","<",">=",">"};
			int c = code[2]; 
			if (c >= 6)
				c = 0;
			sprintf(text,"if %d%s%d,$%02X",(int)code[1],ifc[c],(int)code[3],(int)code[4]);
			return 5;
		}
		case 0x72:
			switch(code[1])
			{
				case 0:
					sprintf(text,"stop %d", (int)code[2]);
					break;
				case 1:
					sprintf(text,"pause %d", (int)code[2]);
					break;
				case 2:
					sprintf(text,"resume");
					break;
				case 3:
					sprintf(text,"pausel");
					break;
				case 4:
					sprintf(text,"mastervolume %d", (int)code[2]);
					break;
				case 5:
					sprintf(text,"volume %d", (int)code[2]);
					break;
				default:
					sprintf(text,"INVALIDEXTRA %d %d", (int)code[1], (int)code[2]);
			}
			return 3;
	}
	sprintf(text,"INVALID");
	return 0;
}

const char *readall_errtype[] = {"ok","open","read"};

int readall(const char *path, std::vector<BYTE> &result)
{
	FILE *f = fopen(path,"rb");
	if (!f)
		return 1;

	fseek(f,0,SEEK_END);
	int len = ftell(f);
	fseek(f,0,SEEK_SET);
	
	result.resize(len);
	int readed = fread(&result[0],1,len,f);
	if (readed != len)
	{
		fclose(f);
		return 2;
	}
	return 0;
}

std::vector<BYTE> patches;
std::vector<BYTE> envelopes;
std::vector<BYTE> sequences;
std::vector<bool> sequences_used;
std::vector<bool> sequence_used;
std::vector<BYTE> samples;

char buff[5000];
char dir[5000];
bool triple = true;

int main(int argc, char **args)
{
	if (argc != 6 && argc != 7)
		usage();
	
	bool ptr3 = false;

	if (argc == 7)
	{
		ptr3 = strchr(args[6],'3') != 0;
	}
	
	int err = readall(args[1],patches);
	if (err)
	{
		printf("Can't %s patches\n",readall_errtype[err]);
		usage();
	}
		
	err = readall(args[2],envelopes);
	if (err)
	{
		printf("Can't %s envelopes\n",readall_errtype[err]);
		usage();
	}
	
	err = readall(args[3],sequences);
	if (err)
	{
		printf("Can't %s sequences\n",readall_errtype[err]);
		usage();
	}
	
	err = readall(args[4],samples);
	if (err)
	{
		printf("Can't %s samples\n",readall_errtype[err]);
		usage();
	}
	
	if (sequences.size() < 2)
		return 0;

	sequences_used.resize(sequences.size());
	sequence_used.resize(sequences.size());
	
	{
		int i;
		for (i=0; args[5][i]; ++i)
			dir[i] = args[5][i];
		for (; i>=0; --i)
			if (dir[i] == '/'
			|| dir[i] == '\\')
				break;
		dir[i+1] = 0;
	}

	// get tracks count
	int count = GetWordLE(&sequences[0])/2;
	for (int i=0; i<count; ++i)
	{
		sprintf(buff,"%s%03d",dir,i);
		if (_mkdir(buff) != 0 && errno != EEXIST)
		{
			printf("Can't create dir \"%03d\"\n",i);
			return 0;
		}
		sprintf(buff,"%s%03d\\%03d.code",dir,i,i);
		FILE *f_seq = fopen(buff,"wb");
		if (!f_seq)
		{
			printf("Can't create \"%s\" file\n",buff);
			return 0;
		}
		
		// check that we can get sequence header offset
		if (i*2 + 1 >= sequences.size())
		{
			printf("Error: sequence %03d offset over end of file\n",i);
			return 0;
		}
		// get sequence header offset
		int seq_start = GetWordLE(&sequences[i*2]);
		if (seq_start >= sequences.size())
		{
			printf("Error: sequence %03d header over end of file\n",i);
			return 0;
		}
		// mark sequence offset bytes as "used"
		sequences_used[i*2  ] = true;
		sequences_used[i*2+1] = true;

		// get channels count
		int channels = sequences[seq_start];
		
		// calculate header length
		int header_len = channels*(ptr3?3:2) + 1;
		
		// check that whole header contains in file
		if (seq_start + header_len > sequences.size())
		{
			printf("Error: sequence %03d header over end of file\n",i);
			return 0;
		}
		
		// mark header offsets as "used"
		for (int j=0; j<header_len; ++j)
			sequences_used[seq_start+j] = true;
		
		// labels for "assembly"
		std::vector<int> labels; // where
		std::vector<int> labelswhere; // where required
		std::vector<BYTE> notepatch;
		notepatch.resize(sequences.size());
		
		// reserve labels for channel offsets
		labels.resize(channels);
		labelswhere.resize(channels);
		
		// mark all bytes not used for current sequence
		for (int j=0; j<sequences.size(); ++j)
			sequence_used[j] = 0;

		// mark all required bytes by sequence
		// and make labels
		for (int c=0; c<channels; ++c)
		{
			int ch_start;
			// get channel code offset
			if (ptr3)
				ch_start = GetTriple(&sequences[seq_start + c*3 + 1]);
			else
				ch_start = GetWordLE(&sequences[seq_start + c*2 + 1]);

			// set label (was reserved)
			labels[c] = ch_start;

			// check that channel start is in file
			if (ch_start >= sequences.size())
			{
				printf("Error: sequence %03d channel %d starts out of file\n",i,c);
				return 0;
			}
			// stack of addresses to scan
			std::vector<int> jmp_stack;
			std::vector<int> jmp_stackp; // patch
			// put channel start
			jmp_stack.push_back(ch_start);
			jmp_stackp.push_back(0);
			while (!jmp_stack.empty())
			{
				// we have address to scan!
				int j = jmp_stack.back();
				int pp = jmp_stackp.back(); // patch

				// remove from stack
				jmp_stack.pop_back();
				jmp_stackp.pop_back();
				for (;;)
				{
					// scanned?
					if (sequence_used[j])
						break; // stop
					// decode command, save length
					int len = gems_decode(&sequences[j], buff);
					if (!len)
					{
						// Unknown command detected
						printf("Warning: unknown command in sequence %03d, in channel %d, at global offset $%X\n",
								i,c,j);
						len = 1;
					}
					// check command end
					if (j + len > sequences.size())
					{
						printf("Warning: command start + len more than end of file "
								"in sequence %03d, in channel %d, at global offset $%X\n",
								i,c,j);
						break;
					}
					// is it "patch"?
					if (memcmp(buff,"patch",5) == 0) 
					{
						// yes, set pp

						// read patch index from text
						ScanNum(buff+5,pp);
					}
					// is it "goto"?
					if (memcmp(buff,"goto",4) == 0)
					{
						// yes, give label for offset
						
						// read offset from text
						int offs;
						ScanNum(buff+4,offs);
						
						// add label
						labels.push_back(j+len+(short)offs);
						// where needed
						labelswhere.push_back(j);
						
						// add offset into scan stack
						jmp_stack.push_back(j+len+(short)offs);
						jmp_stackp.push_back(pp); // patch
					}
					// is it "if"?
					if (memcmp(buff,"if",2) == 0)
					{
						// yes, give label for offset
						
						// find comma before offset
						char *tmp = strchr(buff,',');

						// read offset
						int offs;
						ScanNum(tmp+1,offs);
						
						// add label
						labels.push_back(j+len+offs);
						// where needed
						labelswhere.push_back(j);
						
						// add offset into scan stack
						jmp_stack.push_back(j+len+offs);
						jmp_stackp.push_back(pp); // patch
					}

					// mark command bytes as "being used"
					for (; len > 0; --len, ++j)
					{
						sequences_used[j] = true; // within whole file
						sequence_used[j] = true; // within current sequence
						notepatch[j] = pp;
					}

					// is it end of sequence?
					if (memcmp(buff,"eos",3) == 0)
						break; // stop scan
				}
			}
		}
		// write section
		fprintf(f_seq," SECTION HEADER\n");
		// write count of channels 
		fprintf(f_seq," dc.b %d\n",channels);
		// write channels offsets
		for (int c=0; c<channels; ++c)
			fprintf(f_seq," dc.t channel_%d\n",c);

		// write section
		fprintf(f_seq,"\n SECTION CODE\n");
		
		// indexes of patches, modulations, samples required for current sequence
		std::vector<int> patches_used;
		std::vector<int> modulations_used;
		std::vector<int> samples_used;
		
		// scan bytes again, but without jumps
		// all we need is to get marked commands
		for (int j=0; j<sequences.size(); )
		{
			for (int z=0; z<labels.size(); ++z)
				if (labels[z] == j)
					fprintf(f_seq,"%s_%d:\n",(z<channels)?"channel":"label",z);
			// Is this byte required by current sequence?
			if (!sequence_used[j])
			{
				// no, skip forward
				++j;
				continue;
			}
			int len = gems_decode(&sequences[j], buff);
			if (!len) // if it is unknown command
			{
				// then write plain byte
				fprintf(f_seq," dc.b $%02X",(int)sequences[j]);
				++j;
				continue; // resume to next byte
			}
			
			// is it "note"?
			if (memcmp(buff,"note",4) == 0)
			{
				// yes, then if patch is DAC, then use name
				int pp = notepatch[j];
				int poffs = -1;
				int ptype = -1;
				// if we can get patch offs, then get it
				if (pp*2 + 1 < patches.size())
					poffs = GetWordLE(&patches[pp*2]);
				
				// if we can get patch type, then get it
				if (poffs != -1
				 && poffs < patches.size())
					ptype = patches[poffs];

				// if patch type == DAC
				if (ptype == GEMSI_DAC)
				{
					// scan note id
					int sid;
					ScanNum(buff+4,sid);

					// translate to sample id
					sid = (sid + 0x30) % 0x60;
					
					// look in already found
					int z;
					for (z=0; z<samples_used.size(); ++z)
						if (samples_used[z] == sid)
							break;
					// add if not found
					if (z == samples_used.size())
						samples_used.push_back(sid);
					
					// replace id with name
					sprintf(buff+4," sample_%02X",sid);
				}
			}
			
			// is it "patch"?
			if (memcmp(buff,"patch",5) == 0) 
			{
				// yes, use name instead of index

				// read patch index from text
				int pn;
				ScanNum(buff+5,pn);

				// check that it is not allocated
				int z;
				for (z=0; z<patches_used.size(); ++z)
					if (patches_used[z] == pn)
						break;
				// if not exist, then add
				if (z == patches_used.size())
					patches_used.push_back(pn);
				// replace offset with name
				sprintf(buff+5," patch_%02X",pn);
			}
			
			// is it "modulation"?
			if (memcmp(buff,"modulation",10) == 0) 
			{
				// yes, use name instead of index
				
				// read modulation index from text
				int mn;
				ScanNum(buff+10,mn);
				
				// check that it is not allocated
				int z;
				for (z=0; z<modulations_used.size(); ++z)
					if (modulations_used[z] == mn)
						break;
				// if not exist, then add
				if (z == modulations_used.size())
					modulations_used.push_back(mn);
				
				// replace offset with name
				sprintf(buff+10," modulation_%02X",mn);
			}
			
			// is it "goto"?
			if (memcmp(buff,"goto",4) == 0)
			{
				// yes, set label instead of plain offset
				
				// find index of label given to this "goto"
				int z;
				for (z=0; z<labels.size(); ++z)
					if (labelswhere[z] == j)
						break;
				// check "not found"
				if (z == labels.size())
				{
					printf("Unexpected Error: label not allocated "
							"in %03d sequence at global offset $%X",i,j);
					--z;
				}
				// replace offset with label
				sprintf(buff+4," label_%d",z);
			}
			
			// is it "if"?
			if (memcmp(buff,"if",2) == 0)
			{
				// yes, set label instead of plain offset
				
				// find index of label given to this "if"
				int z;
				for (z=0; z<labels.size(); ++z)
					if (labelswhere[z] == j)
						break;
				// check "not found"
				if (z == labels.size())
				{
					printf("Unexpected Error: label not allocated "
							"in %03d sequence at global offset $%X",i,j);
					--z;
				}
				// find comma before offset
				char *tmp = strchr(buff,',');
				// replace offset with label
				sprintf(tmp+1,"label_%d",z);
			}
				
			// write command text
			fprintf(f_seq," %s\n",buff);

			// skip first byte, because it's label is correct
			++j; --len;
			// test for invalid labels
			for (; len>0; ++j, --len)
				for (int z=0; z<labels.size(); ++z)
					if (labels[z] == j)
						printf("Error: label %d at middle of command "
								"in sequence %03d, at global offset $%X\n",z,i,j);
		}
		fclose(f_seq);
		
		// dump used instruments
		for (int z=0; z<patches_used.size(); ++z)
		{
			int pn = patches_used[z];
			// check that we can get offset
			if (pn * 2 + 1 >= patches.size())
			{
				printf("Error: patch $%02X out of bounds of instruments files "
					"used in sequence %03d\n", pn, i);
				continue;
			}
			// get offset
			int poffs = GetWordLE(&patches[pn*2]);
			// check that we can get type
			if (poffs >= patches.size())
			{
				printf("Error: patch $%02X out of bounds of instruments files "
					"used in sequence %03d\n", pn, i);
				continue;
			}
			// get patch type
			int ptype = patches[poffs];
			// check patch type
			if (ptype > 3)
			{
				printf("Error: patch $%02X unknown type %d "
					"used in sequence %03d\n", pn, ptype, i);
				continue;
			}
			// patch dump sizes
			int psizes[]={0x27,2,7,7};
			// check that full patch dump contains in file
			if (poffs + psizes[ptype] > patches.size())
			{
				printf("Error: patch $%02X info not full, "
					"used in sequence %03d\n", pn, i);
				continue;
			}
			
			// create file for instrument info
			sprintf(buff,"%s%03d\\patch_%02X.ins",dir,i,pn);
			FILE *ins = fopen(buff,"w");
			if (!ins)
			{
				printf("Can't create file \"%s\"\n",buff);
				continue;
			}

			// write instrument info
			switch(ptype)
			{
				case GEMSI_FM:
				case GEMSI_PSG:
				case GEMSI_NOISE:
					fprintf(ins,"importraw 'patch_%02X.raw'\n",pn);
					break;
				case GEMSI_DAC: // DAC has only one byte param, so keep it in plain text
					fprintf(ins,"DAC %d\n",(int)patches[poffs+1]);
					break;
			}
			fclose(ins);
			
			// if patch type != DAC, write dump. DAC patch contains only 1 param,
			// so it will be in plain text right in instrument info
			if (ptype != GEMSI_DAC)
			{
				// create dump file
				sprintf(buff,"%s%03d\\patch_%02X.raw",dir,i,pn);
				ins = fopen(buff,"wb");
				if (!ins)
				{
					printf("Can't create file \"%s\"\n",buff);
					continue;
				}
				// write dump
				int writed = fwrite(&patches[poffs],1,psizes[ptype],ins);
				// check that it was writen
				if (writed != psizes[ptype])
					printf("Error: can't write \"%s\" file\n",buff);
				fclose(ins);
			}
		}
		// write dumps of used modulations
		for (int z=0; z<modulations_used.size(); ++z)
		{
			int mn = modulations_used[z];
			// check that we can get offset
			if (mn*2 + 1 > envelopes.size())
			{
				printf("Error: modulation $%02X out of file bounds, "
						"used in sequence %03d\n",mn,i);
				continue;
			}
			// get offset
			int moffs = GetWordLE(&envelopes[mn*2]);
			// check that file contains this offset
			if (moffs >= envelopes.size())
			{
				printf("Error: modulation $%02X out of file bounds, "
						"used in sequence %03d\n",mn,i);
				continue;
			}
			
			// calculate end of envelope
			int mend;
			// start from delay at offset moffs + 2
			for (mend = moffs + 2; ; mend += 3)
			{
				// check that we can test delay
				if (mend >= envelopes.size())
				{
					printf("Error: modulation $%02X has no end, "
							"used in sequence %03d\n",mn,i);
					mend = 0;
					break;
				}
				// test delay
				if (envelopes[mend] == 0)
					break; // we found END
			}
			// check that we found mend
			if (!mend)
				continue;
			// set end after last delay ( = 0)
			++mend;

			// create mod file
			sprintf(buff,"%s%03d\\modulation_%02X.mod",dir,i,mn);
			FILE *mod = fopen(buff,"wb");
			if (!mod)
			{
				printf("Can't create file \"%s\"\n",buff);
				continue;
			}
			// write dump
			int writed = fwrite(&envelopes[moffs],1,mend-moffs,mod);
			// check that it was writen
			if (writed != mend - moffs)
				printf("Error: can't write \"%s\" file\n",buff);
			fclose(mod);
		}
		
		// write dumps of used samples
		for (int z=0; z<samples_used.size(); ++z)
		{
			int sn = samples_used[z];
			int sh = sn*12;
			// check than we can read sample header
			if (sh + 12 > samples.size())
			{
				printf("Error: sample $%02X header out of file, "
						"used in %03d sequence\n",sn,i);
				continue;
			}
			int sflags = samples[sh];
			int soff   = GetTriple(&samples[sh+1]);
			int sskip  = GetWordLE(&samples[sh+4]);
			int sfirst = GetWordLE(&samples[sh+6]);
			int sloop  = (short)GetWordLE(&samples[sh+8]);
			int send   = (short)GetWordLE(&samples[sh+10]);
			int s_all  = sskip + sfirst + send;
			
			if (soff+s_all > samples.size())
			{
				printf("Error: sample $%02X ending is out of file, "
						"used in sequence %03d",sn,i);
				continue;
			}
			
			// create file for sample cfg
			sprintf(buff,"%s%03d\\sample_%02X.sfx",dir,i,sn);
			FILE *s = fopen(buff,"w");
			if (!s)
			{
				printf("Can't create file \"%s\"\n",buff);
				continue;
			}
			fprintf(s,
				"RAW 'sample_%02X.snd'\n"
				"FLAGS =$%02X\n"
				"SKIP  =$%04X\n"
				"FIRST =$%04X\n"
				"LOOP  =$%04X\n"
				"END   =$%04X\n",
				sn,
				sflags,
				sskip,
				sfirst,
				sloop,
				send);
			fclose(s);
			
			sprintf(buff,"%s%03d\\sample_%02X.snd",dir,i,sn);
			s = fopen(buff,"wb");
			if (!s)
			{
				printf("Can't create file \"%s\"\n",buff);
				continue;
			}
			int writed = fwrite(&samples[soff],1,s_all,s);
			if (writed != s_all)
				printf("Can't write file \"%s\"\n",buff);
			fclose(s);
		}
		
		// write sequence config
		sprintf(buff,"%s%03d\\%03d.cfg",dir,i,i);
		f_seq = fopen(buff,"w");
		
		// include code
		fprintf(f_seq,"code '%03d.code'\n\n",i);
		
		// include instruments
		for (int z=0; z<patches_used.size(); ++z)
			fprintf(f_seq,"instrument patch_%02X,'patch_%02X.ins'\n",patches_used[z],patches_used[z]);
		fprintf(f_seq,"\n");

		// include modulations
		for (int z=0; z<modulations_used.size(); ++z)
			fprintf(f_seq,"modulation modulation_%02X,'modulation_%02X.mod'\n",modulations_used[z],modulations_used[z]);
		fprintf(f_seq,"\n");
		
		// include samples
		for (int z=0; z<samples_used.size(); ++z)
			fprintf(f_seq,"sample sample_%02X,'sample_%02X.sfx'\n",samples_used[z],samples_used[z]);
		
		// end of cfg
		fclose(f_seq);
	}
	// create main cfg
	FILE *cfg = fopen(args[5],"w");
	if (!cfg)
	{
		printf("Can't create \"%s\"\n",args[5]);
		return 0;
	}
	// include sequences
	for (int i=0; i<count; ++i)
	{
		fprintf(cfg,"sequence '%03d\\%03d.cfg'\n",i,i);
	}
	fclose(cfg);
	
	bool _mid = true;
	for (int i=0; i<sequences.size(); ++i)
		if (sequences_used[i] != _mid)
		{
			if (_mid)
				printf("Sequences not used bytes: $%X",i);
			else
				printf("-$%X\n",i);
			_mid = sequences_used[i];
		}
	if (!_mid)
		printf("-$%X\n",sequences.size());
	return 0;
}