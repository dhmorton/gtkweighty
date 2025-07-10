/*
 * gtkmain.c
 *
 *  Created on: Aug 10, 2008
*  Copyright: 2008-2017 David Morton
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

#include "gtkmain.h"
#include "browser.h"
#include "stats.h"
#include "info.h"
#include "config.h"
#include "actions.h"
#include "myutils.h"
#include "comm.h"
#include "playing.h"

static char *homedir = NULL;
static char *configfile = NULL;
static char *musicdir = NULL;

static GtkWidget *win;
static GtkWidget *up_but, *down_but, *album_but, *play_now_but;//right side info fields

static void build_main_window(void);
static int daemonize(void);
//callbacks
static void back(void);
static void play(void);
static void pausetoggle(void);
static void stop(void);
static void next(GtkButton*, GdkEventButton*);
static void toggle_sticky(GtkToggleButton*);
static void change_weight(GtkButton*, GdkEventButton*, gpointer);
static void change_volume(GtkWidget*);
static void find_in_browser(GtkButton*, GdkEventButton*);
static void play_full_album(GtkButton*, GdkEventButton*);
static void browser(void);
static void config(void);
static void info(GtkButton*, GdkEventButton*);
static int read_config(void);
static int read_alarm_config(void);

int main(int argc, char **argv)
{
	homedir = getenv("HOME");
	strcat(homedir, "/.weighty-new");
	printf("homedir = %s\n", homedir);
	configfile = malloc(strlen(homedir) + 8);
	memset(configfile, 0, strlen(homedir) + 8);
	memcpy(configfile, homedir, strlen(homedir));
	strcat(configfile, "/config");
	alarmconfig = malloc(strlen(homedir) + 14);
	memset(alarmconfig, 0, strlen(homedir) + 14);
	memcpy(alarmconfig, homedir, strlen(homedir));
	strcat(alarmconfig, "/alarm.config");

	//read the configs and set up the socket before we do any gtk stuff
	if (read_config() < 0)
		error("can't read config");
	if (read_alarm_config() < 0)
		error("can't read alarm.config");
	if (read_sleep_config() < 0)
		error("can't read sleep.config");
	if (g_connect_to_server() < 0)
		error("can't connect to server");

	//gtk_set_locale();
	gtk_init(&argc, &argv);

	GtkCssProvider *provider;
	GdkDisplay *display;
	GdkScreen *screen;
	provider = gtk_css_provider_new ();
	display = gdk_display_get_default ();
	screen = gdk_display_get_default_screen (display);
	gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gsize bytes_written, bytes_read;

	const gchar* home[strlen(homedir) + 9];
	memset(home, 0, strlen(homedir) + 9);
	memcpy(home, homedir, strlen(homedir));
	strcat((char*)home, "/gtk.css");

	GError *error = 0;

	gtk_css_provider_load_from_path (provider,g_filename_to_utf8((char*)home, strlen((char*)home), &bytes_read, &bytes_written, &error), NULL);

	if(error != NULL) {
		printf("ERROR: %s\n", error->message);
		g_error_free(error);
	}

	g_object_unref (provider);

	build_main_window();
	send_command("DC", 2);//get the values from the config file
	send_command("DV", 2);//get the volume
	send_command("DD", 2);//get the discogs token

	gtk_main();

	return 0;
}
int daemonize()
{
	//printf("daemonizing\n");
	//doesn't work probably due to threading with gtk_main()
	//daemon(1, 1);
	//TODO
	//open stderr to errorlog
	//char *errorlog = "/home/bob/log/gtkweighty-error.log";
	//freopen(errorlog, "w", stderr);
	//open stdout to out.log
	//char *logfile = "/home/bob/log/gtkweighty.log";
	//freopen(logfile, "w", stdout);
	//printf("test log");
	//open stdin to /dev/null

	return 0;
}
int read_config()
{
	FILE *fp;

	if ((fp = fopen(configfile, "r")) == NULL)
	{
		printf("can't open config file\n");
		return -1;
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
//	char *fields[] = {"musicdir", "noskip", "random", "threshhold", "type", "var"};
	char *p;
	while ((read = getline(&line, &len, fp)) != -1)
	{
		char buf[16];
		for(p = line; *p != '='; p++)
			;
		strncpy(buf, line, p - line);
		if (*(p-1) == ' ')
			buf[p - line - 1] = '\0';
		else
			buf[p - line] = '\0';
		char value[256];
		if(*(++p) == ' ')
			p++;
		strncpy(value, p, 256);
		value[strlen(value) - 1] = '\0';
		if (strcmp("musicdir", buf) == 0)
		{
			musicdir = malloc(strlen(value) + 1);
			memcpy(musicdir, value, strlen(value) + 1);
			printf("musicdir = %s\n", musicdir);
			break;
		}
		if (line != NULL)
			free(line);
	}
	fclose(fp);
	return 0;
}
int read_alarm_config()
{
	FILE *fp;

	if ((fp = fopen(alarmconfig, "r")) == NULL)
	{
		printf("config file doesn't exist\n");
		return -1;
	}
	else
	{
		wake.file = NULL;
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		int i;
		for(i = 0; i < 7; i++)
		{
			if ((read = getline(&line, &len, fp)) != -1)
				if (atoi(line) != i)
					printf("bad parsing!!!\n");
			if ((read = getline(&line, &len, fp)) != -1)
				wake.set[i] = atoi(line);
			if ((read = getline(&line, &len, fp)) != -1)
				wake.hr[i] = atoi(line);
			if ((read = getline(&line, &len, fp)) != -1)
				wake.min[i] = atoi(line);
		}
		if ((read = getline(&line, &len, fp)) != -1)
			wake.start_vol = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			wake.end_vol = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
			wake.fade = atoi(line);
		if ((read = getline(&line, &len, fp)) != -1)
		{
			//print_data(line, len);
			if (len == 1)
				wake.file = NULL;
			else
			{
				wake.file = malloc(len);
				memcpy(wake.file, line, len);
			}
		}
		if (line)
			free(line);
	}
	fclose(fp);
	return 0;
}
void build_main_window()
{
	GtkWidget *table, *left_vbox, *button_hbox, *close_hbox, *weight_hbox, *right_vbox;//main window containers
	GtkWidget *album_thumbnail;
	GtkWidget *info_but, *config_but, *pl_but;//left side buttons
	GtkWidget *info_img, *config_img, *pl_img;//images for said buttons
	static GtkAdjustment *vadj; //adjustment object attached to the volume widget
	GtkWidget *back_but, *play_but, *pause_but, *stop_but, *next_but, *sleep_but; //control buttons
	GtkWidget *back_img, *play_img, *pause_img, *stop_img, *next_img, *sleep_img, *up_img, *down_img, *album_img, *play_now_img; //button images
	GtkWidget *hsep1, *hsep2; //horizontal seperator between the next and sleep buttons
	GtkWidget *close_but, *close_img;

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_name(win, "weighty-main");
	gtk_window_set_title(GTK_WINDOW (win), "GtkWeighty");
	gtk_window_set_default_size(GTK_WINDOW (win), 610, 50);
	g_signal_connect(G_OBJECT (win), "destroy", G_CALLBACK (quit), NULL);

	table = create_grid();
	gtk_container_add(GTK_CONTAINER (win), table);

	//thumbnail album art
	album_thumbnail = create_album_thumbnail();

	//three buttons on the left, info, config and playlist
	left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_size_request(left_vbox, 28, 50);
	info_but = gtk_button_new();
	info_img = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (info_but), info_img);
	config_but = gtk_button_new();
	config_img = gtk_image_new_from_icon_name("preferences-system", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (config_but), config_img);
	pl_but = gtk_button_new();
	pl_img = gtk_image_new_from_icon_name("folder-music-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (pl_but), pl_img);
	gtk_box_pack_start(GTK_BOX (left_vbox), info_but, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (left_vbox), config_but, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX (left_vbox), pl_but, TRUE, TRUE, 0);

	//the volume control
//	vol = gtk_volume_button_new();
	vadj = gtk_adjustment_new(50, 0, 100, 1, 1, 2);
	GtkWidget *vol = create_vol(vadj);

//	gtk_scale_button_set_adjustment(GTK_SCALE_BUTTON (vol), GTK_ADJUSTMENT (vadj));

	//the artist and title entry fields
	GtkWidget *art_entry = create_art_entry();
	GtkWidget *tit_entry = create_tit_entry();
	//the progress bar
	GtkWidget *pbar = create_pbar();

	//the bottom button bar
	back_but = gtk_button_new();
	back_img = gtk_image_new_from_icon_name("media-skip-backward", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (back_but), back_img);
	play_but = gtk_button_new();
	play_img = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (play_but), play_img);
	pause_but = gtk_toggle_button_new();
	pause_img = gtk_image_new_from_icon_name("media-playback-pause", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (pause_but), pause_img);
	stop_but = gtk_button_new();
	stop_img = gtk_image_new_from_icon_name("media-playback-stop", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (stop_but), stop_img);
	next_but = gtk_button_new();
	next_img = gtk_image_new_from_icon_name("media-seek-forward", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (next_but), next_img);

	hsep1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	hsep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

	sleep_but = gtk_button_new();
	sleep_img = gtk_image_new_from_icon_name("weather-few-clouds-night", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (sleep_but), sleep_img);
	GtkWidget *sleep_label = create_sleep_label();

	up_but = gtk_button_new();
	up_img = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (up_but), up_img);
	down_but = gtk_button_new();
	down_img = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (down_but), down_img);
	album_but = gtk_button_new();
	album_img = gtk_image_new_from_icon_name("media-optical", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(album_but), album_img);
	play_now_but = gtk_button_new();
	play_now_img = gtk_image_new_from_icon_name("go-jump", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(play_now_but), play_now_img);

	button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_spacing(GTK_BOX (button_hbox), 2);
	gtk_box_pack_start(GTK_BOX (button_hbox), back_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), play_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), pause_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), stop_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), next_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), hsep1, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX (button_hbox), sleep_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (button_hbox), sleep_label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (button_hbox), down_but, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (button_hbox), up_but, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (button_hbox), hsep2, FALSE, FALSE, 5);
	gtk_box_pack_end(GTK_BOX (button_hbox), album_but, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX (button_hbox), play_now_but, FALSE, FALSE, 0);
	//the right hand displays
	GtkWidget *left_entry = create_left_entry();

	close_but = gtk_button_new();
	close_img = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON (close_but), close_img);
	g_signal_connect(G_OBJECT (close_but), "clicked", G_CALLBACK (quit), NULL);

	close_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX (close_hbox), left_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (close_hbox), close_but, FALSE, FALSE, 0);

	GtkWidget *weight_entry = create_weight_entry();
	GtkWidget *sticky_but = create_sticky_but();
	GtkWidget *playby_entry = create_playby_entry();

	weight_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_size_request(weight_hbox, -1, 14);
	gtk_box_pack_start(GTK_BOX (weight_hbox), weight_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (weight_hbox), sticky_but, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (weight_hbox), playby_entry, FALSE, FALSE, 0);

	GtkWidget *time_entry = create_time_entry();

	right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX (right_vbox), close_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (right_vbox), weight_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (right_vbox), time_entry, FALSE, FALSE, 0);

	//attach everything to the table
	gtk_grid_attach(GTK_GRID (table), album_thumbnail, 0, 0, 1, 4);
	gtk_grid_attach(GTK_GRID (table), left_vbox, 1, 0, 1, 4);
	gtk_grid_attach(GTK_GRID (table), vol, 2, 0, 1, 4);
	gtk_grid_attach(GTK_GRID (table), art_entry, 3, 0, 1, 1);
	gtk_grid_attach(GTK_GRID (table), tit_entry, 3, 1, 1, 1);
	gtk_grid_attach(GTK_GRID (table), pbar, 3, 2, 1, 1);
	gtk_grid_attach(GTK_GRID (table), button_hbox, 3, 3, 1, 1);
	gtk_grid_attach(GTK_GRID (table), right_vbox, 4, 0, 1, 4);

	//add the signal handling for the widgets
	g_signal_connect(G_OBJECT (back_but), "clicked", G_CALLBACK (back), NULL);
	g_signal_connect(G_OBJECT (play_but), "clicked", G_CALLBACK (play), NULL);
	g_signal_connect(G_OBJECT (pause_but), "toggled", G_CALLBACK (pausetoggle), NULL);
	g_signal_connect(G_OBJECT (stop_but), "clicked", G_CALLBACK (stop), NULL);
	g_signal_connect(G_OBJECT (next_but), "button_press_event", G_CALLBACK (next), NULL);
	g_signal_connect(G_OBJECT (sleep_but), "clicked", G_CALLBACK (sleepfade), NULL);
	g_signal_connect(G_OBJECT (up_but), "button_press_event", G_CALLBACK (change_weight), NULL);
	g_signal_connect(G_OBJECT (down_but), "button_press_event", G_CALLBACK (change_weight), NULL);
	g_signal_connect(G_OBJECT (album_but), "button_press_event", G_CALLBACK (find_in_browser), NULL);
	g_signal_connect(G_OBJECT (play_now_but), "button_press_event", G_CALLBACK (play_full_album), NULL);
	g_signal_connect(G_OBJECT (pl_but), "clicked", G_CALLBACK (browser), NULL);
	g_signal_connect(G_OBJECT (config_but), "clicked", G_CALLBACK (config), NULL);
	g_signal_connect(G_OBJECT (info_but), "button_press_event", G_CALLBACK (info), NULL);
	g_signal_connect(G_OBJECT (sticky_but), "toggled", G_CALLBACK (toggle_sticky), NULL);
	g_signal_connect(vol, "value-changed", G_CALLBACK(change_volume), NULL);
	g_signal_connect(G_OBJECT(pbar), "button-release-event", G_CALLBACK(end_change_pos), NULL);


	gtk_widget_show_all(win);
	// printf("artist height = %d\n", gtk_widget_get_allocated_height(art_entry));
	// printf("title height = %d\n", gtk_widget_get_allocated_height(tit_entry));
	// printf("pbar height = %d\n", gtk_widget_get_allocated_height(pbar));
	// printf("button height = %d\n", gtk_widget_get_allocated_height(button_hbox));
}
/*
 * CALLBACKS
 */
void back() { send_command("PB", 2); }
void play()
{
	clear_entry_fields();
	send_command("PL\0", 3);
}
void pausetoggle(){	send_command("PA", 2); }
void stop() { send_command("PS", 2); }
void next(GtkButton *button, GdkEventButton *event)
{

	if (event->button == 1)
		send_command("PN", 2);
	else if (event->button == 3)
		send_command("PG", 2);
}
void toggle_sticky(GtkToggleButton *tb)
{
	if (gtk_toggle_button_get_active(tb))
		send_command("SN1\0", 4);
	else
		send_command("SN0\0", 4);
}
void change_weight(GtkButton *button, GdkEventButton *event, gpointer func_data)
{
	GdkWindow *check = gtk_widget_get_window(GTK_WIDGET(button));
	if (check == NULL)
		printf("null button !\n");
	int change = 0;
	if (button == GTK_BUTTON(up_but))
	{
		if (event->button == 1)
			change = 1;
		else if (event->button == 3)
			change = 10;
	}
	else if (button == GTK_BUTTON(down_but))
	{
		if (event->button == 1)
			change = -1;
		else if (event->button == 3)
			change = -10;
	}
	char *playing = get_playing_file();
	if (playing != NULL)
	{
		change_weight_cursong(change);
		change_browser_tv_weight(playing, change);
		change_info_tv_weight(playing, change);
	}
}
void find_in_browser(GtkButton *button, GdkEventButton *event)
{
	if (event->button == 3)
		find_in_list(musicdir);
	else if (event->button == 1)
		find_in_dir(musicdir);
}
void play_full_album(GtkButton *button, GdkEventButton *event)
{
	printf("Playing album %s\n", get_playing_album());
	send_command("PC", 2);
}
void config() { launch_config(); }
void browser() { launch_browser(musicdir, 0); }
void info(GtkButton *button, GdkEventButton *event)
{
	if (event->button == 3)
		launch_stats();
	else if (event->button == 1)
		launch_info();
}
void change_volume(GtkWidget* widget)
{
	//System volume can be changed by another process which causes the server to send a volume update
	//Don't trigger the callback when the server sends an update or it fights back
	if(update_volume == 0) {
		update_volume = 1;
		return;
	}
	int volume = gtk_range_get_value(GTK_RANGE(widget));
	char v[4];
	itoa(volume, v);
	char com[6];
	memset(com, 0, 6);
	memcpy(com, "SV", 2);
	strcat(com, v);
	send_command(com, strlen(v) + 3);
}
