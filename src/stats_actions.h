/*
 * stats_actions.h
 *
 *  Created on: Dec 28, 2011
 *      Author: bob
 */

#ifndef STATS_ACTIONS_H_
#define STATS_ACTIONS_H_

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

GtkWidget* create_stats_win(void);
GtkWidget* create_stats_label(void);
void redraw_canvas(void);
int update_stats_label();
void close_stats_win(void);


#endif /* STATS_ACTIONS_H_ */
