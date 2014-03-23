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

/*

Converter from SEGA Genesis/Megadrive GEMS sound driver into MIDI.

Use At Own Risk. Overwrites all existed files without confirmation.

Features:
Track/Instruments name
Loop marks
Pitch bend support (if your MIDI instrument/software supports pitch bend sensitivity)

Last update: 11.05.2013
Author: r57shell@uralweb.ru 

*/

#include <cstdio>
#include <algorithm>
#include <vector>

void usage()
{
	printf(
"Usage: .exe sequences instruments outdir [flags]\n"
"\n"
"Overwrites all existed files without confirmation\n"
"\n"
"Flags: characters in any order\n"
"       3 - use 3 bytes ptr in sequences (example: comix zone)\n"
"       p - pal tempo scale (example: donald in maui mallard)\n"
"       s - play another songs (example: doom troopers)\n"
"       c - use CTR for loop calculation\n"
"       l - write logs\n"
"\n"
"Last update: 11.05.2013\n"
"Author: r57shell@uralweb.ru\n");
	exit(0);
}

unsigned char buff[1000];
unsigned char instruments_file[0x10100];
FILE *f,*out;

struct note
{
	int x,t,c,v,p,z,pitch,id; // note, time, channel, volume, patch, priority, pitch, id;
	bool on, update; // note on, pitch update;
	note(int _c, int _x, int _t, bool _on, int _v, int _p, int _z, int _pitch) : c(_c), x(_x), t(_t), on(_on), v(_v), p(_p), z(_z), pitch(_pitch){ update = false;}
	
	// comparator by time
	bool operator < (const note &n)
	{
		if (t != n.t)
			return t < n.t;
		if ((on?1:0)!=(n.on?1:0))
			return (on?1:0)<(n.on?1:0);
		return id<n.id;
	}
};

// comparator by channel, and then by time
bool chan_time(const note &a, const note &b)
{
	if (a.c != b.c)
		return a.c < b.c;
	if (a.t != b.t)
		return a.t < b.t;
	if ((a.on?1:0)!=(b.on?1:0))
		return (a.on?1:0)<(b.on?1:0);
	return a.id<b.id;
}

std::vector<note> notes;      // all notes
bool play_songs = false;      // play another songs
std::vector<int> play_song;   // schedule play song
std::vector<int> play_time;   // schedule play song time
std::vector<int> loop_start;  // song contains loop start
std::vector<int> loop_period; // song contains loop period

// equations x == a (mod b) for CRT
std::vector<int> loop_eqa;    // loop eqa
std::vector<int> loop_eqb;    // loop eqb
std::vector<unsigned char> track; // MIDI file

// Chinese Remainder Theorem
// all eqb must be distinct, and none of them divides another
void CRT(int &a, int &b)
{
	b = 1;
	for (int i=0; i<loop_eqb.size(); ++i)
		b *= loop_eqb[i];
	a = 0;
	for (int i=0; i<loop_eqb.size(); ++i)
	{
		int n = b / loop_eqb[i];
		int j;
		for (j=0; j<loop_eqb[i]; ++j)
			if ((j * n) % loop_eqb[i] == loop_eqa[i])
				break;
		a += n * j;
	}
}

int delay, duration, time, maxtime, channel, tempo, patch, volume, priority, pitch;
bool wasdelay, wasduration;
bool write_log = false;

#define lprintf if(write_log) printf
#define lfprintf if(write_log) fprintf

void press(int c, int x, int at)
{
	notes.push_back(note(c,x,at,true,volume,patch,priority,pitch));
}

void release(int c, int x, int at)
{
	notes.push_back(note(c,x,at,false,volume,patch,priority,pitch));
}

void change_pitch()
{
	notes.push_back(note(channel,0,time,true,volume,patch,priority,pitch));
	notes[notes.size()-1].update = true;
}

void delayadd()
{
	time += delay;
	wasduration = false;
	wasdelay = false;
	lprintf("wasdelay = false\n");
}

bool was[100][0x80];

void parseseq(int offs)
{
	fseek(f, offs, SEEK_SET);
	for (;;)
	{
		if (time < 0 || (maxtime != 0 && time > maxtime))
			break;
		fread(buff,1,1,f);
		if (buff[0]>=0xC0)
		{
			if (wasdelay)
				delay = delay*0x40+buff[0]-0xC0;
			else
				delay = buff[0]-0xC0;
			wasdelay = true;
			wasduration = false;
			lprintf("delay = %X\n", delay);
			continue;
		}
		if (buff[0]>=0x80)
		{
			if (wasduration)
				duration = duration*0x40+buff[0]-0x80;
			else
				duration = buff[0]-0x80;
			wasduration = true;
			wasdelay = false;
			lprintf("duration = %X\n", duration);
			continue;
		}
		if (buff[0]<0x60)
		{
			lprintf("note = %X t = %X offs = %X\n", (int)buff[0], time, ftell(f));
			press(channel,buff[0],time);
			release(channel,buff[0],time+duration);
			delayadd();
			continue;
		}
		if (buff[0] == 0x60)
		{
			lprintf("eos\n");
			break; // end sequence
		}
		if (buff[0] == 0x65)
		{
			lprintf("eol\n");
			wasdelay = false;
			wasduration = false;
			break; // end loop
		}
		if (buff[0]>0x72)
		{
			lprintf("Unknown command %X\n",(int)buff[0]);
			continue;
		}
		switch(buff[0])
		{
			case 0x61:
				fread(buff,1,1,f);
				lprintf("patch %X\n",(int)buff[0]);
				patch = buff[0];
				delayadd();
				break;
			case 0x62:
				fread(buff,1,1,f);
				lprintf("modulation %X\n",(int)buff[0]);
				delayadd();
				break;
			case 0x6D: //set song to use SFX timebase.'
				lprintf("sfx\n");
				tempo = 150;
				delayadd();
				break;
				// RESUME
			case 0x63: //nop
				lprintf("nop\n");
				delayadd();
				break;
			case 0x64:
				fread(buff,1,1,f);
				lprintf("loop %X times\n", (int)buff[0]);
				{
					int pos = ftell(f);
					int times = buff[0];
					if (times == 0x7F) // infinity
					{
						if (maxtime == 0) // if we don't know "full" period of whole track
						{
							int tmp = time; // remember loop start time
							parseseq(pos); // do one loop iteration to calculate loop period

							// add new loop
							loop_start.push_back(tmp);
							loop_period.push_back(time-tmp);
						}
						else // if we know "full" period of whole track
							while (time < maxtime) // repeat loop 'til end of period
								parseseq(pos);
					}
				}
				wasdelay = false;
				wasduration = false;
				break;
			case 0x68: // tempo
				fread(buff,1,1,f);
				lprintf("tempo %d+40=%d (dec)\n", (int)buff[0], ((int)buff[0])+40);
				tempo = (int)buff[0]+40;
				delayadd();
				break;
			case 0x6A: // channel priority
				fread(buff,1,1,f);
				lprintf("priority %d (dec)\n", (int)buff[0]);
				priority = (int)buff[0];
				delayadd();
				break;
			case 0x6B: // start another song
				fread(buff,1,1,f);
				lprintf("play %02X\n",(int)buff[0]);
				if (play_songs)
				{
					play_song.push_back(buff[0]);
					play_time.push_back(time);
				}
				delayadd();
				break;
			case 0x66: // toggle retriger mode
			case 0x67: // toggle sustain mode
			case 0x69: // mute
			case 0x6E: // set DAC playback rate
				fread(buff,1,1,f);
				lprintf("param %02X\n",(int)buff[0]);
				delayadd();
				break;
			case 0x6C: // pitch bend
				fread(buff,1,2,f);
				lprintf("pitch %02X %02X\n",(int)buff[0],(int)buff[1]);
				pitch = buff[0]|(buff[1]<<8);
				if (pitch&0x8000)
					pitch |= 0xFFFF0000;
				change_pitch();
				delayadd();
				break;
			case 0x6F:
			{
				fread(buff,1,2,f);
				int k = buff[0]+(buff[1]<<8);
				lprintf("jmp %X\n",k);
				int pos = ftell(f);
				//if (k<0x8000)
				//	parseseq(f,pos+k);
				fseek(f,pos,SEEK_SET);
				wasdelay = false;
				wasduration = false;
			}
				break;
			case 0x70:
				fread(buff,1,2,f);
				lprintf("set _%X = %X\n",(int)buff[0],(int)buff[1]);
				delayadd();
				break;
			case 0x71:
			{
				fread(buff,1,4,f);
				lprintf("jmp ? %X %X %X %X\n",(int)buff[0],(int)buff[1],(int)buff[2],(int)buff[3]);
				//int pos = ftell(f);
				//if (buff[3]<0x80)
				//	parseseq(f,pos+buff[3]);
				//fseek(f,pos,SEEK_SET);
				delayadd();
			}
				break;
			case 0x72:
				fread(buff,1,2,f);
				if (buff[0]>5)
				{
					lprintf("$72 unk %X %X\n",(int)buff[0],(int)buff[1]);
				}
				if (buff[0]==4)
				{
					lprintf("mastervolume %d\n",(int)buff[0]);
				}
				if (buff[0]==5)
				{
					lprintf("volume %d\n",(int)buff[0]);
					volume = buff[0];
				}
				delayadd();
				break;
			default:
				break;
		}
	}
}

// get MIDI delta time
std::vector<unsigned char> gett(int t)
{
	std::vector<unsigned char> res;
	if (t == 0)
	{
		res.push_back(0);
		return res;
	}
	for (int i=0; t>=(1<<(i*7)); ++i)
		res.push_back((i?0x80:0)|((t>>(i*7))&0x7F));
	res[0] &= 0x7F;
	std::reverse(res.begin(),res.end());
	return res;
}

void track_text_event(std::vector<unsigned char> &track, int delta, int event, const char * text)
{
	std::vector<unsigned char> tmp = gett(delta);
	for (int i=0; i<tmp.size(); ++i)
		track.push_back(tmp[i]);

	track.push_back(0xFF);  // META
	track.push_back(event); // text event
	track.push_back(strlen(text)); // text length
	for (int i=0; text[i]; ++i)
		track.push_back(text[i]);
}

// add MIDI META track name
void track_name(std::vector<unsigned char> &track, const char * name)
{
	track_text_event(track,0,3,name);
}

// add MIDI META instrument name
void track_instrument_name(std::vector<unsigned char> &track, const char * name)
{
	track_text_event(track,0,4,name);
}

// setup MIDI pitch sensivity
void track_pitch_sens(std::vector<unsigned char> &track, int channel, int semitones)
{
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(100); // CC#100
	track.push_back(0);   // value

	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(101); // CC#101
	track.push_back(0);   // value
	
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(6); // CC#6
	track.push_back(semitones);   // value
	
	track.push_back(0); // delta time
	track.push_back(0xB0|(channel&0xF)); // control change
	track.push_back(100);  // CC#100
	track.push_back(0x7F); // value
}

// add MIDI META end
void track_end(std::vector<unsigned char> &track, int delta)
{
	std::vector<unsigned char> tmp = gett(delta);
	for (int i=0; i<tmp.size(); ++i)
		track.push_back(tmp[i]);
	track.push_back(0xFF); // META
	track.push_back(0x2F); // end
	track.push_back(0);
}

char *inst_type_name[] = {"FM", "DAC", "PSG", "NOISE"};
std::vector<int> inst;
std::vector<int> inst_pan;
std::vector<int> inst_volume;
std::vector<int> dac;

int inst_type(int i)
{
	int off = instruments_file[2*i]+(instruments_file[2*i+1]<<8);
	if (off>=sizeof(instruments_file))
		return -1;
	return instruments_file[off];
}

int getinst(int p)
{
	for (int i=0; i<inst.size(); ++i)
		if (inst[i] == p)
			return i;
	inst.push_back(p);
	inst_volume.push_back(0x7F);
	return inst.size()-1;
}

int getdac(int p, int n)
{
	int x = ((p<<8)+n);
	for (int i=0; i<dac.size(); ++i)
		if (dac[i] == x)
			return i;
	dac.push_back(x);
	return dac.size()-1;
}

int pt;

void process_note(note & n, FILE *txt, int midi_channel)
{
	lfprintf(txt,"time=%X channel=%X note=%X on=%X\n",n.t,n.c,n.x,n.on?1:0);
	if (n.t < 0 || n.t >= maxtime)
		return;

	std::vector<unsigned char> tmp = gett(n.t - pt);
	for (int tmp1=0; tmp1<tmp.size(); ++tmp1)
		track.push_back(tmp[tmp1]);

	if (n.update)
	{
		track.push_back(0xE0|midi_channel);
		int pv=0x2000+n.pitch*(0x2000/double(0xC00));
		if (pv>0x4000)
			pv=0x4000;
		if (pv<0)
			pv=0;
		track.push_back(pv&0x7F);
		track.push_back((pv>>7)&0x7F);
		pt = n.t;
		return;
	}
	getinst(n.p);
	int type = inst_type(n.p);
	if (was[n.c&0xFF][n.x]==n.on)
	{
		track.push_back((!n.on?0x90:0x80)|midi_channel);
		if (type == 1)
			track.push_back(getdac(n.p, n.x)+0x30);
		else
			track.push_back(n.x);
		track.push_back(0x7F-n.v);
		pt = n.t;
		track.push_back(0);
	}
	track.push_back((n.on?0x90:0x80)|midi_channel);
	if (type == 1)
		track.push_back(getdac(n.p, n.x)+0x30);
	else
		track.push_back(n.x);

	track.push_back(0x7F-n.v);
	was[n.c&0xFF][n.x]=n.on;
	pt = n.t;
}

int main(int argc, char **args)
{
	if (argc != 4 && argc != 5)
		usage();

	bool ptr3 = false;
	bool use_crt = false;
	bool pal_scale = false;

	if (argc == 5)
	{
		ptr3 = strchr(args[4],'3') != 0;
		pal_scale = strchr(args[4],'p') != 0;
		play_songs = strchr(args[4],'s') != 0;
		use_crt = strchr(args[4],'c') != 0;
		write_log = strchr(args[4],'l') != 0;
	}

	f = fopen(args[2],"rb");
	if (!f)
	{
		printf("Can't open instruments\n");
		usage();
	}
	fread(instruments_file,1,0x10100,f);
	fclose(f);
	
	f = fopen(args[1],"rb");
	if (!f)
	{
		printf("Can't open sequences\n");
		usage();
	}

	fseek(f,0,SEEK_SET);
	fread(buff,1,2,f);
	int lend = (buff[1]<<8)+buff[0];

	int i;
	for (i=0; i*2<lend; ++i)
	{	
		sprintf((char*)buff,"%s\\%03d.mid",args[3],i);
		out = fopen((char*)buff,"wb");
		
		sprintf((char*)buff,"%s\\%03d.txt",args[3],i);
		FILE *txt = fopen((char*)buff,"w+");

		inst.clear();
		inst_pan.clear();
		inst_volume.clear();
		dac.clear();
		loop_start.clear();
		loop_period.clear();
		loop_eqa.clear();
		loop_eqb.clear();
		
		bool err_crt = !use_crt; // CRT error
		
		for (maxtime=0;;)
		{
			notes.clear();
			play_song.clear();
			play_song.push_back(i);
			play_time.clear();
			play_time.push_back(0);
		
			for (int song=0; song<play_song.size(); ++song)
			{
				if (maxtime == 0)
				{
					int jj;
					for (jj=song-1; jj>=0; --jj)
						if (play_song[jj]==play_song[song])
							break;
					if (jj != -1)
						continue;
				}
				fseek(f,play_song[song]*2,SEEK_SET);
				fread(buff,1,2,f);
				int s = (buff[1]<<8)+buff[0];

				fseek(f,s,SEEK_SET);
				fread(buff,1,1,f);
				int c = buff[0];
				if (!c)
					continue;
			
				for (int j=0; j<c; ++j)
				{
					int k;
					if (ptr3) // 3 bytes ptr
					{
						fseek(f,s+1+j*3,SEEK_SET);
						fread(buff,1,3,f);
						
						k = buff[0]+(buff[1]<<8)+(buff[2]<<16);
					}
					else
					{
						fseek(f,s+1+j*2,SEEK_SET);
						fread(buff,1,2,f);
						
						k = buff[0]+(buff[1]<<8);
					}

					patch = -1;
					pitch = 0;
					volume = 0; // loudest
					priority = 0; // lowest
					delay = 0;
					duration = 0;
					time = play_time[song];
					wasdelay = false;
					wasduration = false;
					channel = j+(play_song[song]<<8);
					lprintf("parseseq %X %X\n",i,channel);
					parseseq(k);
				}
			}
			if (maxtime != 0)
				break;
			if (loop_start.size() == 0)
				err_crt = true;
			std::sort(notes.begin(),notes.end());
			
			// Make arrays of eqa, eqb
			for (int j=0; j<loop_start.size(); ++j)
			{
				int pp = loop_period[j];
				for (int p=2; p<=pp; ++p)
				{
					if (err_crt)
						break;
					if (pp%p != 0)
						continue;
					int q = p;
					while (pp%(q*p) == 0)
						q *= p;
					int z;
					for (z=0; z<loop_eqa.size(); ++z)
					{
						if ((loop_eqb[z] % q) == 0)
						{
							if ((loop_eqa[z] % q) != (loop_start[j] % q))
								err_crt = true;
							break;
						}
						if ((q % loop_eqb[z]) == 0)
						{
							if ((loop_start[j] % loop_eqb[z]) != (loop_eqa[z] % loop_eqb[z]))
								err_crt = true;
							loop_eqa[z] = loop_start[j] % q;
							loop_eqb[z] = q;
							break;
						}
					}
					if (err_crt)
						break;
					pp /= q;
					if (z != loop_eqa.size())
						continue;
					loop_eqa.push_back(loop_start[j]%q);
					loop_eqb.push_back(q);
				}
			}
			if (err_crt) // if CRT error found, get maxtime from last event
			{
				if (notes.size() != 0)
					maxtime = notes[notes.size()-1].t;
				else
					maxtime = 1;
			}
			else
			{
				int a,b,c;
				if (notes.size() != 0)
					c = notes[notes.size()-1].t;
				else
					c = 1;
				CRT(a, b);
				
				// repeat "full" period until all loops done at least once.
				for (maxtime = a; maxtime < c; maxtime += b);
			}
			lprintf("maxtime = %d\n",maxtime);
		}
		lprintf("output\n");
		
		// MIDI header
		strcpy((char*)buff,"MThd");
		buff[4]=buff[5]=buff[6]=0;
		buff[7]=6;
		buff[8]=buff[10]=0;
		buff[9]=1; // format multiple sync tracks
		buff[11]=1; // tracks count
		buff[12]=0;
		buff[13]=24;
		
		// write MIDI header
		fwrite(buff,1,14,out);

		// add MIDI META tempo
		double pal = 1;
		if (pal_scale)
			pal = 6/5.0;
		int microseconds = (int)(double(60*1000*1000)*pal/tempo);
		buff[0]=0;
		buff[1]=0xFF; // meta
		buff[2]=0x51; // set tempo
		buff[3]=3;
		buff[4]=microseconds>>16;
		buff[5]=microseconds>>8;
		buff[6]=microseconds;
		for (int tmp1=0; tmp1<7; ++tmp1)
			track.push_back(buff[tmp1]);
		
		// find loop start time
		int loop_start_time = 1<<30;
		for (int z = 0; z<loop_start.size(); ++z)
			if (loop_start_time > loop_start[z])
				loop_start_time = loop_start[z];

		if (!err_crt)
		{
			lprintf("loop_start:%d\n",loop_start_time);
			
			track_text_event(track,loop_start_time,6,"loop start");
			track_text_event(track,maxtime-loop_start_time,6,"loop end");
		}
		else
		{
			lprintf("track_end:%d\n",maxtime);
			track_text_event(track,maxtime,6,"end");
		}
		track_end(track,0);
		
		// Write MIDI track with loops and tempo.
		strcpy((char*)buff,"MTrk");
		buff[4] = track.size()>>24;
		buff[5] = track.size()>>16;
		buff[6] = track.size()>>8;
		buff[7] = track.size();
		fwrite(buff,1,8,out);
		fwrite(&track[0],1,track.size(),out);

		track.clear();
		
		int pc = -1;
		int tracks_count = 0;

		std::sort(notes.begin(),notes.end(),chan_time);

		for (int z = 0; z<notes.size(); ++z)
		{
			if (pc != notes[z].c)
			{
				if (track.size())
				{
					track_end(track,maxtime-pt);
					strcpy((char*)buff,"MTrk");
					buff[4] = track.size()>>24;
					buff[5] = track.size()>>16;
					buff[6] = track.size()>>8;
					buff[7] = track.size();
					fwrite(buff,1,8,out);
					fwrite(&track[0],1,track.size(),out);
					++tracks_count;
				}
				track.clear();
				// init new track;
				sprintf((char*)buff,"Patch %02X(%s)",notes[z].p,inst_type_name[inst_type(notes[z].p)]);
				track_name(track,(char*)buff);
				track_instrument_name(track,(char*)buff);
				track_pitch_sens(track, tracks_count, 12);
				memset(was,0,sizeof(was));
				pt = 0;
				pc = notes[z].c;
			}
			process_note(notes[z],txt,tracks_count&0xF);
		}
		if (track.size())
		{
			track_end(track,maxtime-pt);
			strcpy((char*)buff,"MTrk");
			buff[4] = track.size()>>24;
			buff[5] = track.size()>>16;
			buff[6] = track.size()>>8;
			buff[7] = track.size();
			fwrite(buff,1,8,out);
			fwrite(&track[0],1,track.size(),out);
			++tracks_count;
		}
		track.clear();
		
		// fix tracks count int MIDI Header
		fseek(out,11,SEEK_SET);
		buff[0]=tracks_count+1;
		fwrite(buff,1,1,out);
		
		// write instruments
		for (int z=0; z<inst.size(); ++z)
		{
			int off = instruments_file[2*inst[z]]+(instruments_file[2*inst[z]+1]<<8);
			if (off>=sizeof(instruments_file))
				off=0;
			unsigned char *ym = instruments_file+off;
			
			fprintf(txt,"Patch %02X (%s):",inst[z],inst_type_name[inst_type(inst[z])]);
			for (int zz=0; zz<5; ++zz)
				fprintf(txt," %02X", (int)ym[zz]);
			fprintf(txt,"\n");
		}
		// write DAC
		for (int z=0; z<dac.size(); ++z)
			fprintf(txt,"DAC (%d) %02X %02X\n",z,dac[z]>>8,dac[z]&0xFF);

		fclose(out);
		fclose(txt);
	}
	
	fclose(f);
	return 0;
}