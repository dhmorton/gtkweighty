/*
 * utils.h
 *
 *  Created on: Aug 10, 2008
 *      Author: bob
 */
#ifndef MYUTILS_H_
#define MYUTILS_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern GdkColor color_black;
extern GdkColor color_white;
extern GdkColor color_red;
extern GdkColor color_blue;
extern GdkColor color_yellow;
extern GdkColor color_green;
extern GdkColor color_cyan;
extern GdkColor color_gray;

struct counts {
	int mp3;
	int ogg;
	int mpc;
	int wav;
	int flac;
	int m4a;
	int wma;
	int other;
	int zero;
	int hund;
	int sticky;
	int count[101];
	int total;
	int max;
	int albums;
	int artists;
	int genres;
};
typedef struct {
	char *file; //full path to song
	int weight;
	int sticky;
} song;

struct config {
	char musicdir[255];
	int threshhold;
	char type[16];
	char playby[16];
	int song_rand;
	int song_skip;
	int album_rand;
	int album_skip;
	int artist_rand;
	int artist_skip;
	int genre_rand;
	int genre_skip;
	int var;
};
enum {
	SONG,
	ALBUM,
	ARTIST,
	GENRE
};

extern struct counts stats;
extern struct config val;

int get_playby();
void itoa(int, char*);
void translate_field(char*, char*);
void back_translate(char*, char**);
void print_data(char*, int);
void set_sock(GSocket*);
void send_command(char*, int);
void error(char*);
void quit(void);

#endif /* MYUTILS_H_ */
