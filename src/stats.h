/*
 * history.h
 *
 *  Created on: Jun 30, 2011
 *      Author: bob
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
#include <cairo.h>

int launch_stats(void);
void populate_hist_tv(char*, int, int, int);
void update_hist_tv(char*, int, int);
void add_update_data(int, char*);

#endif /* HISTORY_H_ */
