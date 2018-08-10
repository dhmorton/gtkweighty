/*
 * playing.h
 *
 *  Created on: Dec 31, 2011
 *      Author: bob
 * contains the data about the currently playing song
 */


#ifndef PLAYING_H_
#define PLAYING_H_

#include <stdlib.h>
#include <string.h>

typedef struct {
	char *field;
	char *tag;
} tag_data;

struct current {
	char *file;
	char *artist;
	char *title;
	char *album;
	char *genre;
	char *track;
	char *length;
	char *year;
	int weight;
	int sticky;
	int time;
	int num_tags;
	tag_data tags[64];
};
void set_playing_file(char*);
void set_playing_artist(char*);
void set_playing_title(char*);
void set_playing_album(char*);
void set_playing_genre(char*);
void set_playing_track(char*);
void set_playing_length(char*);
void set_playing_tag(char*, char*, int, int);
void set_playing_year(char*);
void set_playing_weight(int);
void set_playing_sticky(int);
void set_playing_time_data(int);
void set_num_tags(int);
char* get_playing_file(void);
char* get_playing_artist(void);
char* get_playing_title(void);
char* get_playing_album(void);
char* get_playing_genre(void);
char* get_playing_track(void);
char* get_playing_length(void);
char* get_playing_year(void);
char* get_playing_field(int);
char* get_playing_tag(int);
int get_playing_weight(void);
int get_playing_sticky(void);
int get_playing_time_data(void);
int get_num_tags(void);
void clear_playing(void);
int clear_tag_data(tag_data**, int);

#endif /* PLAYING_H_ */
