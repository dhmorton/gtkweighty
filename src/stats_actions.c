/*
 * stats_actions.c
 *
 *  Created on: Dec 28, 2011
*  Copyright: 2011-2017 David Morton
 *
 *   This file is part of weighty.
 *
 *   Weighty is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Weighty is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with weighty.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stats_actions.h"
#include "myutils.h"

static GtkWidget *stats_win = NULL;
static GtkWidget *stats_label;

GtkWidget* create_stats_win()
{
	if (stats_win != NULL)
		return NULL;
	stats_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (stats_win), "Statistics");
	gtk_window_set_default_size(GTK_WINDOW (stats_win), 1024, 780);
	g_signal_connect(G_OBJECT (stats_win), "destroy", G_CALLBACK (close_stats_win), NULL);

	return stats_win;
}
GtkWidget* create_stats_label()
{
	stats_label = gtk_label_new("");
	gtk_label_set_line_wrap(GTK_LABEL(stats_label), TRUE);

	return stats_label;
}
void redraw_canvas()
{
	//printf("redraw canvas\n");
	if (stats_win != NULL)
	{
		//printf("getting window size\n");
		gint w = 0;
		gint h = 0;
		gtk_window_get_size(GTK_WINDOW(stats_win), &w, &h);
		//printf("w = %d\th = %d\n", w, h);
		gtk_widget_queue_draw_area(stats_win, 0, 0, w, h);
	}
}
int update_stats_label()
{
	int i;
	float mean = 0.0;
	int median = 0, mode = 0, total = 0;
	for(i = 0; i <= 100; i++)
	{
		mean += i * stats.count[i];
		total += stats.count[i];
		if (total >= stats.total/2 && median == 0)
			median = i;
		if (stats.count[i] == stats.max)
			mode = i;
	}
	mean /= stats.total;
	if (stats_win == NULL)
		return 1;
	char *markup;
	markup = g_markup_printf_escaped(
			"<span weight = \"bold\">Total number of songs = %d\n</span>"
			"Number of albums = %d\n"
			"Number of artists = %d\n"
			"Number of genres = %d\n"
			"\tnumber of mp3s = %d\n"
			"\tnumber of flacs = %d\n"
			"\tnumber of oggs = %d\n"
			"\tnumber of mpcs = %d\n"
			"\tnumber of wavs = %d\n"
			"\tnumber of m4as = %d\n"
			"\tnumber of wmas = %d\n"
			"\tnumber of others = %d\n"
			"Number with weight zero = %d\n"
			"Number with weight 100 = %d\n"
			"Number with sticky flag = %d\n"
			"Mean = %.2f\n"
			"Median = %d\n"
			"Mode = %d\n"
			, stats.total, stats.albums, stats.artists, stats.genres, stats.mp3, stats.flac, stats.ogg, stats.mpc, stats.wav, stats.m4a, stats.wma, stats.other, stats.zero, stats.hund, stats.sticky
			, mean, median, mode
	);
	gtk_label_set_markup(GTK_LABEL(stats_label), markup);
	return 0;
}
void close_stats_win()
{
	gtk_widget_destroy(stats_win);
	stats_win = NULL;
}
