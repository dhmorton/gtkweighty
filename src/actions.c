/*
 * actions.c
 *
 *  Created on: Dec 27, 2011
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

#include "actions.h"
#include "myutils.h"

static GtkWidget *weight_entry, *sticky_but, *pbar, *sleep_label, *art_entry, *tit_entry, *fade_spin, *play_spin, *playby_entry, *time_entry, *left_entry, *vol;
static int old_val = 0;//a bad hack for dragging the play position around
static int sleeping = 0;//keep track of sleep countdown
struct sleep_time sleep_fade;
int update_volume = 1;

GtkWidget* create_weight_entry()
{
	weight_entry = gtk_entry_new();
	//gtk_widget_set_size_request(weight_entry, -1, 14);
	gtk_entry_set_width_chars(GTK_ENTRY (weight_entry), 3);
	gtk_editable_set_editable(GTK_EDITABLE (weight_entry), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY (weight_entry), 1);
	gtk_entry_set_has_frame(GTK_ENTRY (weight_entry), FALSE);


	return weight_entry;
}
GtkWidget* create_sticky_but()
{
	sticky_but = gtk_check_button_new();
	//gtk_widget_set_size_request(sticky_but, -1, 14);

	return sticky_but;
}
GtkWidget* create_pbar()
{
	GtkAdjustment *padj = gtk_adjustment_new(0, 0, 1000, 1, 10, 10);
	pbar = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (padj));
	gtk_scale_set_draw_value((GtkScale *) pbar, FALSE);
	//gtk_widget_set_size_request(pbar, 400, 14);

	return pbar;
}
GtkWidget* create_sleep_label()
{
	sleep_label = gtk_label_new("");

	return sleep_label;
}
GtkWidget* create_art_entry()
{
	art_entry = gtk_entry_new();
	gtk_widget_set_size_request(art_entry, 400, 14);
	gtk_editable_set_editable(GTK_EDITABLE (art_entry), FALSE);
	gtk_entry_set_has_frame(GTK_ENTRY (art_entry), FALSE);

	return art_entry;
}
GtkWidget* create_tit_entry()
{
	tit_entry = gtk_entry_new();
	//gtk_widget_set_size_request(tit_entry, 400, 14);
	gtk_editable_set_editable(GTK_EDITABLE (tit_entry), FALSE);
	gtk_entry_set_has_frame(GTK_ENTRY (tit_entry), FALSE);


	return tit_entry;
}
GtkWidget* create_play_spin()
{
	play_spin = gtk_spin_button_new_with_range(1, 999, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(play_spin), sleep_fade.play);

	return play_spin;
}
GtkWidget* create_fade_spin()
{
	fade_spin = gtk_spin_button_new_with_range(1, 999, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fade_spin), sleep_fade.fade);

	return fade_spin;
}
GtkWidget* create_playby_entry()
{
	playby_entry = gtk_entry_new();
	gtk_widget_set_size_request(playby_entry, -1, 14);
	gtk_entry_set_width_chars(GTK_ENTRY(playby_entry), 8);
	gtk_editable_set_editable(GTK_EDITABLE (playby_entry), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY (playby_entry), 1);
	gtk_entry_set_has_frame(GTK_ENTRY (playby_entry), FALSE);

	return playby_entry;
}
GtkWidget* create_time_entry()
{
	time_entry = gtk_entry_new();
	//gtk_widget_set_size_request(time_entry, -1, 14);
	gtk_entry_set_width_chars(GTK_ENTRY (time_entry), 6);
	gtk_editable_set_editable(GTK_EDITABLE (time_entry), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY (time_entry), 0.5);
	gtk_entry_set_has_frame(GTK_ENTRY (time_entry), FALSE);

	return time_entry;
}
GtkWidget* create_left_entry()
{
	left_entry = gtk_entry_new();
	//gtk_widget_set_size_request(left_entry, -1, 14);
	gtk_entry_set_width_chars(GTK_ENTRY (left_entry), 12);
	gtk_editable_set_editable(GTK_EDITABLE (left_entry), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY (left_entry), 0.5);
	gtk_entry_set_has_frame(GTK_ENTRY (left_entry), FALSE);

	return left_entry;
}
GtkWidget* create_vol(GtkAdjustment *vadj)
{
	vol = gtk_scale_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (vadj));
	gtk_scale_set_digits(GTK_SCALE(vol), 0);
	gtk_range_set_inverted(GTK_RANGE(vol), TRUE);

	return vol;
}
void set_artist(char* artist)
{
	gtk_entry_set_text(GTK_ENTRY(art_entry), artist);
	gtk_widget_show_all(art_entry);
}
void set_title(char* title)
{
	gtk_entry_set_text((GtkEntry*) tit_entry, title);
	gtk_widget_show_all(tit_entry);
}
void set_sticky(int sticky)
{
	if (sticky)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_but), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_but), FALSE);
}
void change_weight_cursong(int change)
{
	if (change)
	{
		char com[6];
		memset(com, 0, 6);
		char w[4];
		memset(w, 0, 4);
		snprintf(w, 4, "%d", change);
		memcpy(com, "SW", 2);
		strcat(com, w);
		strcat(com, "\0");
		send_command(com, strlen(w) + 3);
		const char *weight_s = gtk_entry_get_text(GTK_ENTRY(weight_entry));
		int weight = atoi(weight_s);
		stats.count[weight]--;
		weight += change;
		if (weight > 100)
			weight = 100;
		else if (weight < 0)
			weight = 0;
		stats.count[weight]++;
		snprintf(w, 4, "%d", weight);
		set_weight_and_sticky(w, -1);
	}
}
void set_weight_and_sticky(char* weight, int sticky)
{
	gtk_entry_set_text(GTK_ENTRY(weight_entry), weight);
	if (sticky > 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_but), TRUE);
	}
	else if (sticky == 0)
	{
		gtk_widget_show_now(sticky_but);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sticky_but), FALSE);
	}
	old_val = 0;
	gtk_range_set_value(GTK_RANGE(pbar), 0);
}
void clear_weight_entry()
{
	gtk_entry_set_text(GTK_ENTRY(weight_entry), "");
}
void clear_entry_fields()
{
	gtk_entry_set_text(GTK_ENTRY(weight_entry), "");
	gtk_entry_set_text(GTK_ENTRY(art_entry), "");
	gtk_entry_set_text(GTK_ENTRY(tit_entry), "");
	gtk_entry_set_text(GTK_ENTRY(time_entry), "");
	gtk_entry_set_text(GTK_ENTRY(left_entry), "");
}
void set_playby(char *playby)
{
	gtk_entry_set_text(GTK_ENTRY(playby_entry), playby);
	gtk_widget_show_all(playby_entry);
}
void set_time(char* time)
{
	gtk_entry_set_text(GTK_ENTRY(time_entry), time);
	gtk_widget_show_all(time_entry);
}
void set_left(char *left)
{
	gtk_entry_set_text(GTK_ENTRY(left_entry), left);
	gtk_widget_show_all(left_entry);
}
void set_volume(int volume)
{
	update_volume = 0;
	gtk_range_set_value(GTK_RANGE(vol), volume);
	gtk_widget_show_all(vol);
}

void play_row_now(GtkTreeView *tv, GtkTreePath *path)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tv);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	gchar *file;
	gtk_tree_model_get(model, &iter, FULLPATH, &file, -1);
	char s[strlen(file) + 3];
	memset(s, '\0', strlen(file) + 3);
	memcpy(s, "PL", 2);
	strcat(s, file);
	strcat(s, "\0");
	send_command(s, strlen(file) + 3);
	g_free(file);
}
void populate_tv(GtkWidget* tv, char *song, int weight, int sticky, int i, int flag)//the flag is how to truncate the displayed path
{
	GtkTreeModel *model;
	if(song == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
	GtkTreeIter iter;
	gtk_list_store_insert(GTK_LIST_STORE(model), &iter, (gint) i + 1);
	char* file = malloc(strlen(song) + 1);
	memset(file, 0, strlen(song) + 1);
	strncpy(file, song, strlen(song) );
	if (flag == 0)
	{
		song = strrchr(song, '/');
		song++;
	}
	else if (flag == 1)
		song += strlen(val.musicdir);
	else if (flag == 2)//for the list tv
	{
		if (strchr(song, '/') != NULL)
		{
			song += strlen(val.musicdir);
			song++;
		}
		//else don't truncate, it's not a song
	}
	//else if (flag == 3)
	//	;//do nothing
	//printf("set list store %s | %s\n", song, file);

	/*
	 * Some strings will not deign to be converted to utf8 for some reason
	 * If they won't, just put in what's there and ignore the warnings.
	 */
	GError *error = NULL;
	gchar *gsong = g_locale_to_utf8(song, -1, NULL, NULL, &error);
	if(gsong == NULL)
	{
		//printf("%s\n", error->message);
		g_error_free(error);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
						INDEX, i+1,
						STICKY, sticky,
						WEIGHT, weight,
						FILENAME, song,
						FULLPATH, file, -1);
	}
	else
	{
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
						INDEX, i+1,
						STICKY, sticky,
						WEIGHT, weight,
						FILENAME, gsong,
						FULLPATH, file, -1);
	}
	free(file);
}
void update_progressbar(int permil)
{
	int val = gtk_range_get_value(GTK_RANGE(pbar));
	if ((val - old_val) == 0)
	{
		gtk_range_set_value(GTK_RANGE(pbar), permil);
		old_val = permil;
	}
	if (sleeping)
	{
		time_t now = time(NULL);
		int left = sleeping - now;
		if (left <= 0)
		{
			sleeping = 0;
			gtk_label_set_text(GTK_LABEL(sleep_label), "");
		}
		else
		{
			char time_s[6];
			int sec = left % 60;
			int min = (int)(left / 60);
			char mm[3];
			char ss[3];
			snprintf(mm, 3, "%d", min);
			snprintf(ss, 3, "%d", sec);
			int i = 0;
			time_s[i] = mm[i];
			i++;
			if (min < 10)
				time_s[i] = ':';
			else
			{
				time_s[i] = mm[1];
				i++;
				time_s[i] = ':';
			}
			i++;
			if (sec < 10)
			{
				time_s[i] = '0';
				i++;
			}
			time_s[i] = ss[0];
			i++;
			time_s[i] = ss[1];
			i++;
			time_s[i] = '\0';
			gtk_label_set_text(GTK_LABEL(sleep_label), time_s);
		}
	}
}
void end_change_pos(GtkWidget *widget)
{
	int pos;
	int val = gtk_range_get_value(GTK_RANGE(widget));
	if (abs(val - old_val) < 20)
	{
		gint x, y;
		gdk_window_get_device_position(gtk_widget_get_parent_window(widget), GDK_SOURCE_MOUSE, &x, &y, NULL);
		pos = (65535 * x) / gtk_widget_get_allocated_width(widget);
		//x goes from 0 to 400; val goes from 0 to 1000; ratio is 2.5
		val = (5 * x) / 2;
		old_val = val;
		gtk_range_set_value(GTK_RANGE(widget), val);
	}
	else
	{
		old_val = val;
		pos = (val * 65535) / 1000;
	}
	char com[10];
	memset(com, 0, 10);
	memcpy(com, "PJ", 2);
	char p[8];
	memset(p, 0, 8);
	snprintf(p, 8, "%d", pos);
	memcpy(&com[2], p, strlen(p) + 1);
	send_command(com, strlen(p) + 3);
}
void sleepfade()
{
	if (sleeping)
	{
		send_command("PO", 2);
		sleeping = 0;
		gtk_label_set_text(GTK_LABEL(sleep_label), "");
	}
	else
	{
		send_command("PP", 2);
		time_t now = time(NULL);
		sleeping = now + (sleep_fade.play + sleep_fade.fade) * 60;
	}
}
int read_sleep_config()
{
	FILE *fp;
	char *homedir = NULL;
	char *sleepconfig = NULL;
	homedir = getenv("HOME");
	sleepconfig = malloc(strlen(homedir) + 14);
	memset(sleepconfig, 0, strlen(homedir) + 14);
	memcpy(sleepconfig, homedir, strlen(homedir));
	strcat(sleepconfig, "/sleep.config");
	if ((fp = fopen(sleepconfig, "r")) == NULL)
		printf("config file doesn't exist %s\n", sleepconfig);
	else
	{
		char *line = NULL;
		size_t len = 0;
		ssize_t read;

		if ((read = getline(&line, &len, fp)) != -1)
			sleep_fade.play = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			sleep_fade.fade = atoi(line);
		if (line)
			free(line);
	}
	fclose(fp);
	return 0;
}
void save_sleep_config()
{
	send_sleep_config();
	char line[256];
	memset(line, 0, 256);
	char *homedir = NULL;
	char *sleepconfig = NULL;
	homedir = getenv("HOME");
	strcat(homedir, "/.weighty-new");
	sleepconfig = malloc(strlen(homedir) + 14);
	memset(sleepconfig, 0, strlen(homedir) + 14);
	memcpy(sleepconfig, homedir, strlen(homedir));
	strcat(sleepconfig, "/sleep.config");
	FILE *fp;
	if ((fp = fopen(sleepconfig, "w")) == NULL)
		printf("can't open config file for writing %s\n", sleepconfig);
	else
	{
		fprintf(fp, "%d\n", sleep_fade.play);
		fprintf(fp, "%d\n", sleep_fade.fade);
	}
	fclose(fp);
}
void send_sleep_config()
{
	char com[1024];
	memset(com, 0, 1024);
	memcpy(com, "SL", 2);
	char *pcom = &com[2];

	sleep_fade.play = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(play_spin));
	char play_s[6];
	memset(play_s, 0, 6);
	snprintf(play_s, 6, "%d", sleep_fade.play);
	memcpy(pcom, play_s, strlen(play_s) + 1);
	pcom += strlen(play_s) + 1;

	sleep_fade.fade = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fade_spin));
	char fade_s[6];
	memset(fade_s, 0, 6);
	snprintf(fade_s, 6, "%d", sleep_fade.fade);
	memcpy(pcom, fade_s, strlen(fade_s) + 1);
	pcom += strlen(fade_s) + 1;

	send_command(com, strlen(fade_s) + strlen(play_s) + 4);
}
