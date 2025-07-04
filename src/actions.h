/*
 * actions.h
 *
 *  Created on: Dec 27, 2011
 *      Author: bob
 */

#ifndef ACTIONS_H_
#define ACTIONS_H_

#include <stddef.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#define INDEX 0
#define STICKY 1
#define WEIGHT 2
#define FILENAME 3
#define COLOR 4
#define FULLPATH 5

extern int update_volume;

struct sleep_time {
	int play;//minutes to play
	int fade;//minutes to fade
};

GtkWidget* create_weight_entry(void);
GtkWidget* create_sticky_but(void);
GtkWidget* create_pbar(void);
GtkWidget* create_sleep_label(void);
GtkWidget* create_art_entry(void);
GtkWidget* create_tit_entry(void);
GtkWidget* create_play_spin(void);
GtkWidget* create_fade_spin(void);
GtkWidget* create_playby_entry(void);
GtkWidget* create_time_entry(void);
GtkWidget* create_left_entry(void);
GtkWidget* create_vol(GtkAdjustment*);
GtkWidget* create_album_thumbnail(void);
void set_artist(char*);
void set_title(char*);
void set_thumbnail(void);
void clear_thumbnail(void);
void set_sticky(int);
void set_time(char*);
void set_left(char*);
void set_volume(int);
void change_weight_cursong(int);
void set_weight_and_sticky(char*, int);
void clear_weight_entry(void);
void clear_entry_fields(void);
void set_playby(char*);
void play_row_now(GtkTreeView*, GtkTreePath*);
void populate_tv(GtkWidget*, char*, int, int, int, int);

void update_progressbar(int);
void end_change_pos(GtkWidget*);
void sleepfade(void);
int read_sleep_config(void);
void save_sleep_config(void);
void send_sleep_config(void);

#endif /* ACTIONS_H_ */
