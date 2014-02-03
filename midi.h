#ifndef MIDI_H
#define MIDI_H
#include <vector>

void track_delta(std::vector<unsigned char> &track, int delta);
void track_text_event(std::vector<unsigned char> &track, int delta, int event, const char * text);
void track_name(std::vector<unsigned char> &track, const char * name);
void track_instrument_name(std::vector<unsigned char> &track, const char * name);
void track_pitch_sens(std::vector<unsigned char> &track, int channel, int semitones);
void track_end(std::vector<unsigned char> &track, int delta);

#endif