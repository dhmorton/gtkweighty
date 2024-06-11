/*
 * browser.h
 *
 *  Created on: Jun 20, 2011
 *      Author: bob
 */

#ifndef BROWSER_H_
#define BROWSER_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#include "playing.h"

#define BIGBUF 900000
#define QUERY 1024

#define INDEX 0
#define STICKY 1
#define WEIGHT 2
#define FILENAME 3
#define COLOR 4
#define FULLPATH 5

enum {
	STREAM_NAME,
	STREAM_GENRE,
	STREAM_BITRATE,
	STREAM_DESCRIPTION,
	STREAM_URL
};

typedef struct {
	GtkWidget *scroll;
	GtkWidget *tv;
	int col_count;
	char *tv_cols[1024];
} tag_tv_columns;

int launch_browser(char*, int);
//populate tree view functions
int set_cursor_on_playing(void);
void set_cursor_on_playing_file(int);
void populate_file_tv(char*, int, int, int);
void populate_pl_tv(char*, int, int, int);
void populate_search_tv(char*, int, int, int);
void populate_list_tv(char*, int, int, int);
void populate_list_file_tv(char*, int, int, int);
void populate_tag_tv(tag_data**, int, int);
void populate_tag_tv_func_new(tag_data**, int, int, tag_tv_columns*);
void populate_tag_tv_func(tag_data**, int, int, tag_tv_columns*);
void change_browser_tv_weight(char*, int);
void add_stream_data(char*, char*, char*, char*);
void clear_search_fields();
void create_tag_model_new(char, int, char**);

//utilities
int highlight_playlist(int);
void find_in_list(char*);
void find_in_dir(char*);

#endif /* BROWSER_H_ */
