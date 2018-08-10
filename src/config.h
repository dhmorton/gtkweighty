/*
 * config.h
 *
 *  Created on: Jun 29, 2011
 *      Author: bob
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <cairo.h>

struct alarm_time {
	int set[7];
	int hr[7];
	int min[7];
	int start_vol;
	int end_vol;
	int fade;
	char *file;//song to play
};

extern struct alarm_time wake;
extern char* alarmconfig;

int launch_config(void);
void set_config_data(void);
void draw_distribution(GtkWidget*);
void da_expose_event(GtkWidget*);
void da_configure_event(GtkWidget*);
float linear(int, int, int);
float exponential(int, int, int);
float gaussian(int, int, int);
float flat(int, int, int);
float get_constant(int);
void change_type(GtkWidget*);
void change_phone_type(GtkWidget*);
void change_playby(GtkWidget*);
void change_rand(GtkWidget*);
void change_skip(GtkWidget*);
void change_thresh(GtkSpinButton*);
void change_var(GtkSpinButton*);
void send_config(void);
void save_config(void);
void send_alarm_config(void);
void save_alarm_config(void);
void send_phone_data(void);
void close_config_win(void);

#endif /* CONFIG_H_ */
