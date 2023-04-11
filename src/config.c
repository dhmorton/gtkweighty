/*
 * config.c
 *
 *  Created on: Jun 29, 2011
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

#include "config.h"
#include "models.h"
#include "myutils.h"
#include "actions.h"

static GtkWidget *config_win = NULL;
static GtkWidget *nb;
static GtkWidget *var_spin, *type_combo, *thresh_spin;
static GtkWidget *phone_var_spin, *phone_type_combo, *phone_thresh_spin;
static GtkWidget *phone_time_spin, *phone_time_combo, *phone_data_spin, *phone_data_combo;
static GtkWidget *song_radio, *song_rand_check, *song_skip_check;
static GtkWidget *album_radio, *album_rand_check, *album_skip_check;
static GtkWidget *artist_radio, *artist_rand_check, *artist_skip_check;
static GtkWidget *genre_radio, *genre_rand_check, *genre_skip_check;
static GtkWidget *sleep_tv;
static GtkWidget *day_check[7];//alarm on or off by day
static GtkWidget *day_hspin[7];//hour spinner
static GtkWidget *day_mspin[7];//minute spinner
static GtkWidget *fade_in_spin, *wake_song_entry;
static GtkAdjustment *start_adj, *end_adj;

//struct sleep_time sleep_fade;
struct alarm_time wake;
struct config temp;

char *alarmconfig;


int launch_config(void)
{
	printf("Config opened\n");
	if (config_win != NULL)
		return 1;

	send_command("DC", 2);
	config_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (config_win), "Config");
	gtk_window_set_default_size(GTK_WINDOW (config_win), 404, 480);
	g_signal_connect(G_OBJECT (config_win), "destroy", G_CALLBACK (close_config_win), NULL);
	//create the notebook and the various tabs
	nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_TOP);
	gtk_container_add(GTK_CONTAINER(config_win), nb);

	//set up the first tab, CONFIG
	GtkWidget *config_image = gtk_image_new_from_icon_name("preferences-system", GTK_ICON_SIZE_MENU);
	GtkWidget *config_label = gtk_label_new("Config");
	GtkWidget *config_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_container_add(GTK_CONTAINER(config_vbox_tab), config_image);
	gtk_container_add(GTK_CONTAINER(config_vbox_tab), config_label);
	gtk_widget_show_all(config_vbox_tab);

	//Sleep tab
	GtkWidget *sleep_image = gtk_image_new_from_icon_name("weather-few-clouds-night", GTK_ICON_SIZE_MENU);
	GtkWidget *sleep_label = gtk_label_new("Sleep");
	GtkWidget *sleep_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_container_add(GTK_CONTAINER(sleep_vbox_tab), sleep_image);
	gtk_container_add(GTK_CONTAINER(sleep_vbox_tab), sleep_label);
	gtk_widget_show_all(sleep_vbox_tab);

	//Alarm tab
	GtkWidget *alarm_image = gtk_image_new_from_icon_name("alarm-symbolic", GTK_ICON_SIZE_MENU);
	GtkWidget *alarm_label = gtk_label_new("Alarm");
	GtkWidget *alarm_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_container_add(GTK_CONTAINER(alarm_vbox_tab), alarm_image);
	gtk_container_add(GTK_CONTAINER(alarm_vbox_tab), alarm_label);
	gtk_widget_show_all(alarm_vbox_tab);

	//Phone tab
	GtkWidget *phone_image = gtk_image_new_from_icon_name("call-start-symbolic", GTK_ICON_SIZE_MENU);
	GtkWidget *phone_label = gtk_label_new("Phone");
	GtkWidget *phone_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_container_add(GTK_CONTAINER(phone_vbox_tab), phone_image);
	gtk_container_add(GTK_CONTAINER(phone_vbox_tab), phone_label);
	gtk_widget_show_all(phone_vbox_tab);
/*
 * CONFIG TAB
 */
	//the first hbox contains the distribution choices
	GtkWidget *config_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	GtkWidget *da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 404, 250);
	g_signal_connect(da, "draw", G_CALLBACK(da_expose_event), NULL);
	g_signal_connect(da, "configure-event", G_CALLBACK(da_configure_event), NULL);

	var_spin = gtk_spin_button_new_with_range(1, 50, 1);
	GtkWidget *var_frame = gtk_frame_new("var");
	gtk_container_add(GTK_CONTAINER(var_frame), var_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(var_spin), 2);
	gtk_editable_set_editable(GTK_EDITABLE(var_spin), FALSE);
	if (strcmp(val.type, "gaussian") == 0)
		gtk_widget_set_sensitive(GTK_WIDGET(var_spin), TRUE);
	else
		gtk_widget_set_sensitive(GTK_WIDGET(var_spin), FALSE);
	g_signal_connect(var_spin, "changed", G_CALLBACK(change_var), NULL);
	temp.var = val.var;

	type_combo = gtk_combo_box_text_new();
	GtkWidget *type_frame = gtk_frame_new("Distribution type");
	gtk_container_add(GTK_CONTAINER(type_frame), type_combo);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "exponential");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "linear");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "gaussian");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(type_combo), "flat");
//	gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
	g_signal_connect(type_combo, "changed", G_CALLBACK(change_type), NULL);
	memcpy(temp.type, val.type, strlen(val.type) + 1);

	thresh_spin = gtk_spin_button_new_with_range(1, 99, 1);
	GtkWidget *thresh_frame = gtk_frame_new("Threshold");
	gtk_container_add(GTK_CONTAINER(thresh_frame), thresh_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(thresh_spin), 3);
	gtk_editable_set_editable(GTK_EDITABLE(thresh_spin), TRUE);
	g_signal_connect(thresh_spin, "changed", G_CALLBACK(change_thresh), NULL);
	temp.threshhold = val.threshhold;

	gtk_box_pack_start(GTK_BOX(config_hbox1), type_frame, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(config_hbox1), thresh_frame, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(config_hbox1), var_frame, TRUE, FALSE, 1);

	//playby playback choices
	GtkWidget *playby_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	GtkWidget *song_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	song_radio = gtk_radio_button_new_with_label(NULL, "Song ");
	song_rand_check = gtk_check_button_new_with_label("Random order");
	song_skip_check = gtk_check_button_new_with_label("Skip low weights");
	gtk_box_pack_start(GTK_BOX(song_hbox), song_radio, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(song_hbox), song_rand_check, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(song_hbox), song_skip_check, TRUE, FALSE, 1);

	GtkWidget *album_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	album_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(song_radio), "Album");
	album_rand_check = gtk_check_button_new_with_label("Random order");
	album_skip_check = gtk_check_button_new_with_label("Skip low weights");
	gtk_box_pack_start(GTK_BOX(album_hbox), album_radio, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(album_hbox), album_rand_check, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(album_hbox), album_skip_check, TRUE, FALSE, 1);

	GtkWidget *artist_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	artist_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(album_radio), "Artist ");
	artist_rand_check = gtk_check_button_new_with_label("Random order");
	artist_skip_check = gtk_check_button_new_with_label("Skip low weights");
	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_radio, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_rand_check, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_skip_check, TRUE, FALSE, 1);

	GtkWidget *genre_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	genre_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(artist_radio), "Genre");
	genre_rand_check = gtk_check_button_new_with_label("Random order");
	genre_skip_check = gtk_check_button_new_with_label("Skip low weights");
	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_radio, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_rand_check, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_skip_check, TRUE, FALSE, 1);
	memcpy(temp.playby, val.playby, strlen(val.playby) + 1);


	gtk_box_pack_start(GTK_BOX(playby_vbox), song_hbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(playby_vbox), album_hbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(playby_vbox), artist_hbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(playby_vbox), genre_hbox, TRUE, FALSE, 0);

	GtkWidget *playby_frame = gtk_frame_new("Playback by");
	gtk_container_add(GTK_CONTAINER(playby_frame), playby_vbox);

	g_signal_connect(G_OBJECT(song_radio), "toggled", G_CALLBACK(change_playby), NULL);
	g_signal_connect(G_OBJECT(album_radio), "toggled", G_CALLBACK(change_playby), NULL);
	g_signal_connect(G_OBJECT(artist_radio), "toggled", G_CALLBACK(change_playby), NULL);
	g_signal_connect(G_OBJECT(genre_radio), "toggled", G_CALLBACK(change_playby), NULL);
	g_signal_connect(G_OBJECT(song_rand_check), "toggled", G_CALLBACK(change_rand), NULL);
	g_signal_connect(G_OBJECT(album_rand_check), "toggled", G_CALLBACK(change_rand), NULL);
	g_signal_connect(G_OBJECT(artist_rand_check), "toggled", G_CALLBACK(change_rand), NULL);
	g_signal_connect(G_OBJECT(genre_rand_check), "toggled", G_CALLBACK(change_rand), NULL);
	g_signal_connect(G_OBJECT(song_skip_check), "toggled", G_CALLBACK(change_skip), NULL);
	g_signal_connect(G_OBJECT(album_skip_check), "toggled", G_CALLBACK(change_skip), NULL);
	g_signal_connect(G_OBJECT(artist_skip_check), "toggled", G_CALLBACK(change_skip), NULL);
	g_signal_connect(G_OBJECT(genre_skip_check), "toggled", G_CALLBACK(change_skip), NULL);

	//the bottom hbox contains the standard apply, save, quit buttons
	GtkWidget *config_hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	GtkWidget *config_apply_but = gtk_button_new_with_mnemonic("_Apply");
	GtkWidget *config_save_but = gtk_button_new_from_icon_name("drive-multidisk", GTK_ICON_SIZE_MENU);
	GtkWidget *config_quit_but = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
	g_signal_connect(config_apply_but, "clicked", G_CALLBACK(send_config), NULL);
	g_signal_connect(config_quit_but, "clicked", G_CALLBACK(close_config_win), NULL);
	g_signal_connect(config_save_but, "clicked", G_CALLBACK(save_config), NULL);

	gtk_box_pack_start(GTK_BOX(config_hbox3), config_apply_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(config_hbox3), config_save_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(config_hbox3), config_quit_but, TRUE, FALSE, 1);

	GtkWidget *config_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(config_vbox), config_hbox1, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(config_vbox), playby_frame, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(config_vbox), da, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(config_vbox), config_hbox3, FALSE, FALSE, 2);

/*
 * SLEEP TAB
 */
	GtkWidget *sleep_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	GtkWidget *play_spin = create_play_spin();
	GtkWidget *play_frame = gtk_frame_new("Play");
	gtk_container_add(GTK_CONTAINER(play_frame), play_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(play_spin), 3);
	gtk_editable_set_editable(GTK_EDITABLE(play_spin), FALSE);

	GtkWidget *fade_spin = create_fade_spin();
	GtkWidget *fade_frame = gtk_frame_new("Fade");
	gtk_container_add(GTK_CONTAINER(fade_frame), fade_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(fade_spin), 3);
	gtk_editable_set_editable(GTK_EDITABLE(fade_spin), FALSE);

	gtk_box_pack_start(GTK_BOX(sleep_hbox1), play_frame, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(sleep_hbox1), fade_frame, TRUE, FALSE, 1);

	GtkTreeModel *sleep_model;
	sleep_tv = gtk_tree_view_new();
	sleep_model = create_standard_model(sleep_tv);
	GtkWidget *sleep_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(sleep_tv), sleep_model);
	gtk_container_add(GTK_CONTAINER(sleep_scroll), sleep_tv);

	GtkWidget *sleep_hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	GtkWidget *sleep_apply_but = gtk_button_new_with_mnemonic("_Apply");
	GtkWidget *sleep_save_but = gtk_button_new_from_icon_name("drive-multidisk", GTK_ICON_SIZE_MENU);
	GtkWidget *sleep_quit_but = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
	g_signal_connect(sleep_apply_but, "clicked", G_CALLBACK(send_sleep_config), NULL);
	g_signal_connect(sleep_quit_but, "clicked", G_CALLBACK(close_config_win), NULL);
	g_signal_connect(sleep_save_but, "clicked", G_CALLBACK(save_sleep_config), NULL);

	gtk_box_pack_start(GTK_BOX(sleep_hbox3), sleep_apply_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(sleep_hbox3), sleep_save_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(sleep_hbox3), sleep_quit_but, TRUE, FALSE, 1);

	GtkWidget *sleep_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(sleep_vbox), sleep_hbox1, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(sleep_vbox), sleep_scroll, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(sleep_vbox), sleep_hbox3, FALSE, FALSE, 1);

/*
 * ALARM TAB
 */
	GtkWidget *day_widget_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	char *days[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
	GtkWidget *day_hbox[7];//container for each of the days and their settings

	int i;
	for (i = 0; i < 7; i++)
	{
		day_check[i] = gtk_check_button_new_with_label(days[i]);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(day_check[i]), wake.set[i]);
		day_hspin[i] = gtk_spin_button_new_with_range(0, 23, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(day_hspin[i]), wake.hr[i]);
		gtk_entry_set_width_chars(GTK_ENTRY(day_hspin[i]), 2);
		day_mspin[i] = gtk_spin_button_new_with_range(0, 59, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(day_mspin[i]), wake.min[i]);
		gtk_entry_set_width_chars(GTK_ENTRY(day_mspin[i]), 2);
		day_hbox[i] = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
		gtk_box_pack_start(GTK_BOX(day_hbox[i]), day_check[i], TRUE, TRUE, 1);
		gtk_box_pack_start(GTK_BOX(day_hbox[i]), day_hspin[i], TRUE, TRUE, 1);
		gtk_box_pack_start(GTK_BOX(day_hbox[i]), day_mspin[i], TRUE, TRUE, 1);
		gtk_box_pack_start(GTK_BOX(day_widget_vbox), day_hbox[i], TRUE, TRUE, 1);
	}

	GtkWidget *day_widget_frame = gtk_frame_new("Set the alarm per day");
	gtk_container_add(GTK_CONTAINER(day_widget_frame), day_widget_vbox);

	//the fade in hbox
	GtkWidget *wake_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
//	GtkWidget *fade_in_check = gtk_check_button_new_with_label("Fade in music");
	fade_in_spin = gtk_spin_button_new_with_range(0, 999, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(fade_in_spin), wake.fade);
	GtkWidget *fade_in_label = gtk_label_new("Number of seconds to fade in");
//	gtk_box_pack_start(GTK_BOX(wake_hbox1), fade_in_check, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(wake_hbox1), fade_in_spin, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(wake_hbox1), fade_in_label, TRUE, FALSE, 1);

	//starting volume
	GtkWidget *start_vol_frame = gtk_frame_new("Starting volume");
	start_adj = gtk_adjustment_new(wake.start_vol, 0, 100, 1, 1, 2);
	GtkWidget *start_vol = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (start_adj));
	gtk_scale_set_digits(GTK_SCALE(start_vol), 0);
	gtk_container_add(GTK_CONTAINER(start_vol_frame), start_vol);

	GtkWidget *end_vol_frame = gtk_frame_new("Ending volume");
	end_adj = gtk_adjustment_new(wake.end_vol, 0, 100, 1, 1, 2);
	GtkWidget *end_vol = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (end_adj));
	gtk_scale_set_digits(GTK_SCALE(end_vol), 0);
	gtk_container_add(GTK_CONTAINER(end_vol_frame), end_vol);

	//entry for wake song
	GtkWidget *wake_song_frame = gtk_frame_new("Song to play");
	wake_song_entry = gtk_entry_new();
	if (wake.file != NULL)
		gtk_entry_set_text(GTK_ENTRY(wake_song_entry), wake.file);
	gtk_editable_set_editable(GTK_EDITABLE (wake_song_entry), TRUE);
	gtk_container_add(GTK_CONTAINER(wake_song_frame), wake_song_entry);

	//apply/save/quit
	GtkWidget *wake_hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	GtkWidget *wake_apply_but = gtk_button_new_with_mnemonic("_Apply");
	GtkWidget *wake_save_but = gtk_button_new_from_icon_name("drive-multidisk", GTK_ICON_SIZE_MENU);
	GtkWidget *wake_quit_but = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
	g_signal_connect(wake_apply_but, "clicked", G_CALLBACK(send_alarm_config), NULL);
	g_signal_connect(wake_quit_but, "clicked", G_CALLBACK(close_config_win), NULL);
	g_signal_connect(wake_save_but, "clicked", G_CALLBACK(save_alarm_config), NULL);

	gtk_box_pack_start(GTK_BOX(wake_hbox3), wake_apply_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(wake_hbox3), wake_save_but, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(wake_hbox3), wake_quit_but, TRUE, FALSE, 1);

	GtkWidget *alarm_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), day_widget_frame, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), wake_hbox1, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), start_vol_frame, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), end_vol_frame, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), wake_song_frame, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(alarm_vbox), wake_hbox3, FALSE, FALSE, 1);

	/*
	 * PHONE TAB
	 */
	//selections for how to slice weights for songs to put on phone
	GtkWidget *phone_hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	phone_var_spin = gtk_spin_button_new_with_range(1, 50, 1);
	GtkWidget *phone_var_frame = gtk_frame_new("var");
	gtk_container_add(GTK_CONTAINER(phone_var_frame), phone_var_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(phone_var_spin), 2);
	gtk_editable_set_editable(GTK_EDITABLE(phone_var_spin), TRUE);
	gtk_widget_set_sensitive(phone_var_spin, FALSE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(phone_var_spin), 1);
	//temp.var = val.var;

	phone_type_combo = gtk_combo_box_text_new();
	GtkWidget *phone_type_frame = gtk_frame_new("Distribution type");
	gtk_container_add(GTK_CONTAINER(phone_type_frame), phone_type_combo);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_type_combo), "exponential");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_type_combo), "linear");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_type_combo), "gaussian");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_type_combo), "flat");
	gtk_combo_box_set_active(GTK_COMBO_BOX(phone_type_combo), 1);
	g_signal_connect(phone_type_combo, "changed", G_CALLBACK(change_phone_type), NULL);

	phone_thresh_spin = gtk_spin_button_new_with_range(1, 99, 1);
	GtkWidget *phone_thresh_frame = gtk_frame_new("Threshold");
	gtk_container_add(GTK_CONTAINER(phone_thresh_frame), phone_thresh_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(phone_thresh_spin), 3);
	gtk_editable_set_editable(GTK_EDITABLE(phone_thresh_spin), TRUE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(phone_thresh_spin), 90);
	//g_signal_connect(phone_thresh_spin, "changed", G_CALLBACK(change_thresh), NULL);
	//temp.threshhold = val.threshhold;

	gtk_box_pack_start(GTK_BOX(phone_hbox1), phone_type_frame, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(phone_hbox1), phone_thresh_frame, TRUE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(phone_hbox1), phone_var_frame, TRUE, FALSE, 1);

	//selection for how far back to go to get songs
	GtkWidget *phone_hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	phone_time_spin = gtk_spin_button_new_with_range(1, 20, 1);
	GtkWidget *phone_time_frame = gtk_frame_new("Time frame");
	gtk_container_add(GTK_CONTAINER(phone_hbox2), phone_time_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(phone_time_spin), 3);
	gtk_editable_set_editable(GTK_EDITABLE(phone_time_spin), TRUE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(phone_time_spin), 2);

	phone_time_combo = gtk_combo_box_text_new();
	//GtkWidget *phone_time_combo_frame = gtk_frame_new("Units of time");
	gtk_container_add(GTK_CONTAINER(phone_hbox2), phone_time_combo);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_time_combo), "days");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_time_combo), "months");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_time_combo), "years");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_time_combo), "all times");
	gtk_combo_box_set_active(GTK_COMBO_BOX(phone_time_combo), 2);

	gtk_container_add(GTK_CONTAINER(phone_time_frame), phone_hbox2);
	//gtk_box_pack_start(GTK_BOX(phone_hbox2), phone_time_frame, TRUE, FALSE, 1);
	//gtk_box_pack_start(GTK_BOX(phone_hbox2), phone_time_combo_frame, TRUE, FALSE, 1);

	//select how much music to transfer
	GtkWidget *phone_hbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	phone_data_spin = gtk_spin_button_new_with_range(1, 1024, 1);
	GtkWidget *phone_data_frame = gtk_frame_new("Data amount");
	gtk_container_add(GTK_CONTAINER(phone_hbox3), phone_data_spin);
	gtk_entry_set_width_chars(GTK_ENTRY(phone_data_spin), 4);
	gtk_editable_set_editable(GTK_EDITABLE(phone_data_spin), TRUE);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(phone_data_spin), 10);

	phone_data_combo = gtk_combo_box_text_new();
	gtk_container_add(GTK_CONTAINER(phone_hbox3), phone_data_combo);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_data_combo), "Mb");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_data_combo), "Gb");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(phone_data_combo), "Tb");
	gtk_combo_box_set_active(GTK_COMBO_BOX(phone_data_combo), 1);

	gtk_container_add(GTK_CONTAINER(phone_data_frame), phone_hbox3);

	//Transfer button
	GtkWidget *phone_hbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	GtkWidget *phone_send_but = gtk_button_new_from_icon_name("send-to-symbolic", GTK_ICON_SIZE_MENU);
	g_signal_connect(phone_send_but, "clicked", G_CALLBACK(send_phone_data), NULL);
	gtk_box_pack_start(GTK_BOX(phone_hbox4), phone_send_but, TRUE, FALSE, 1);

	//pack it all up into the phone vbox
	GtkWidget *phone_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_box_pack_start(GTK_BOX(phone_vbox), phone_hbox1, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(phone_vbox), phone_time_frame, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(phone_vbox), phone_data_frame, FALSE, FALSE, 1);
	gtk_box_pack_end(GTK_BOX(phone_vbox), phone_hbox4, FALSE, FALSE, 1);

	//put it all in the notebook
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), config_vbox, config_vbox_tab, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), sleep_vbox, sleep_vbox_tab, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), alarm_vbox, alarm_vbox_tab, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), phone_vbox, phone_vbox_tab, NULL);

//	draw_distribution(da);
	set_config_data();
	gtk_widget_show_all(config_win);
	return 0;
}
void draw_distribution(GtkWidget* da)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(da));
	GtkAllocation al;
	gtk_widget_get_allocation(da, &al);
	int w = al.width;
	int h = al.height;
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.2, 0.8);
	cairo_rectangle(cr, 0, 0, w, h);
	float (*f)(int, int, int);
	if (strcmp(val.type, "linear") == 0)
		f = &linear;
	else if (strcmp(val.type, "exponential"))
		f = &exponential;
	else if (strcmp(val.type, "gaussian"))
		f = &gaussian;
	else if (strcmp(val.type, "flat"))
		f = &flat;
	int i;
	for (i = 0; i <= 100; i++)
	{
		float yval = h*f(i, val.threshhold, val.var);
		if (i % 10)
			cairo_set_source_rgba(cr, 1, 0, 0, 0.9);
		else
			cairo_set_source_rgba(cr, 0, 1, 0, 0.6);
		cairo_rectangle(cr, i*4, h - yval, 4, 5);
		cairo_fill(cr);
	}
	cairo_destroy(cr);
}
//FIXME not used anymore?
void da_expose_event(GtkWidget* da)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(da));
	GtkAllocation al;
	gtk_widget_get_allocation(da, &al);
	int w = al.width;
	int h = al.height;
	cairo_set_line_width(cr, 0.5);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.8);
	cairo_fill(cr);
	float (*f)(int, int, int);
	if (strcmp(temp.type, "linear") == 0)
		f = &linear;
	else if (strcmp(temp.type, "exponential") == 0)
		f = &exponential;
	else if (strcmp(temp.type, "gaussian") == 0)
		f = &gaussian;
	else if (strcmp(temp.type, "flat") == 0)
		f = &flat;
	else
		printf("doesn't match anything\n");
	int i;
	for (i = 0; i <= 100; i++)
	{
		float yval = h * f(i, temp.threshhold, temp.var);
		if (i % 10)
			cairo_set_source_rgba(cr, 1, 0, 0, 0.9);
		else
			cairo_set_source_rgba(cr, 0, 1, 0, 0.6);
		cairo_rectangle(cr, i*4, h - yval, 4, 5);
		cairo_fill(cr);
	}
	cairo_destroy(cr);
}
void da_configure_event(GtkWidget* da)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(da));
	GtkAllocation al;
	gtk_widget_get_allocation(da, &al);
	cairo_set_line_width(cr, 0.5);
	cairo_rectangle(cr, 0, 0, al.width, al.height);
	cairo_set_source_rgba(cr, 1, 0, 0, 0.8);
	cairo_fill(cr);
	cairo_destroy(cr);
}
//the next four functions take a weight, threshhold and var
//they each return a float between 0 and 1 inclusive (hopefully)
//for the linear function thresh is the weight at which the probality is 0.5, var is ignored
//for the exponential function thresh is where p = 0.5, var is ignored
//for the gaussian thresh is where the bell curve is centered and var is the variance
float linear(int weight, int thresh, int var)
{
	if (weight == 100)
		return 1.0;
	float numerator = weight + 100 - 2 * thresh;
	if (numerator < 0)
		return 0.0;
	float denominator = 2 * (100 - thresh);
	if (denominator == 0)//shouldn't ever happen
		return 1.0;
	float p = numerator / denominator;
	return p;
}
float exponential(int weight, int thresh, int var)
{
	float c = get_constant(thresh);
	return ((pow(2,(c * weight/100)) -1)/(pow(2, c) - 1) );
}
float gaussian(int weight, int thresh, int var)
{
	return (exp((-pow(weight - thresh, 2))/(2*pow(var, 2))));
}
float flat(int weight, int thresh, int var)
{
	return (weight > thresh ? 1 : 0);
}
//this is a constant used to calculate the exponential curve
//it calculates some value c that forces exponential() to return 0.5 at thresh
float get_constant(int thresh)
{
	float t = (float) thresh / 100;
	float c = 5.0;
	float delta = 1.0;

	float prob = (pow(2, c * t) - 1) / (pow(2, c) - 1);
	float diff = 0.5 - prob;

	while ((diff < -0.02) || (diff > 0.02))
	{
		if (diff < 0)
			c += delta;
		else if (diff > 0)
			c -= delta;
		prob = (pow(2, c * t) - 1) / (pow(2, c) - 1);
		diff = 0.5 - prob;
		if (prob < 1)
			delta += diff;
		else
			delta -= diff;
	}
	return c;
}
void set_config_data()
{
	if (config_win != NULL)
	{
	//	gtk_entry_set_text(GTK_ENTRY(type_combo), val.type);
		if (strcmp(val.type, "exponential") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 0);
		else if (strcmp(val.type, "linear") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 1);
		else if (strcmp(val.type, "gaussian") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 2);
		else if (strcmp(val.type, "flat") == 0)
			gtk_combo_box_set_active(GTK_COMBO_BOX(type_combo), 3);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(thresh_spin), val.threshhold);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(var_spin), val.var);
/*		if (! strcasecmp(val.playby, "song"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(play_by_combo), 0);
		else if (! strcasecmp(val.playby, "album"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(play_by_combo), 1);
		else if (! strcasecmp(val.playby, "artist"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(play_by_combo), 2);
		else if (! strcasecmp(val.playby, "genre"))
			gtk_combo_box_set_active(GTK_COMBO_BOX(play_by_combo), 3);
*/
		if (! strcasecmp(val.playby, "song"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(song_radio), TRUE);
		else if (! strcasecmp(val.playby, "album"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(album_radio), TRUE);
		else if (! strcasecmp(val.playby, "artist"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(artist_radio), TRUE);
		else if (! strcasecmp(val.playby, "genre"))
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_radio), TRUE);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(song_rand_check), val.song_rand);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(song_skip_check), val.song_skip);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(album_rand_check), val.album_rand);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(album_skip_check), val.album_skip);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(artist_rand_check), val.artist_rand);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(artist_skip_check), val.artist_skip);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_rand_check), val.genre_rand);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_skip_check), val.genre_skip);
	}

}
void change_var(GtkSpinButton *spin)
{
	temp.var = gtk_spin_button_get_value_as_int(spin);
	gtk_widget_queue_draw_area(config_win, 0, 0, 404, 480);
}
void change_thresh(GtkSpinButton *spin)
{
	temp.threshhold = gtk_spin_button_get_value_as_int(spin);
	gtk_widget_queue_draw_area(config_win, 0, 0, 404, 480);
}
void change_type(GtkWidget *combo)
{
	const gchar *type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
	memset(temp.type, 0, 16);
	memcpy(temp.type, type, strlen(type) + 1);
	if (strcmp(temp.type, "gaussian") == 0)
		gtk_widget_set_sensitive(var_spin, TRUE);
	else
		gtk_widget_set_sensitive(var_spin, FALSE);
	gtk_widget_queue_draw_area(config_win, 0, 0, 404, 480);
}
void change_phone_type(GtkWidget *combo)
{
	const gchar *type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
	if (strcmp(type, "gaussian") == 0)
		gtk_widget_set_sensitive(phone_var_spin, TRUE);
	else
		gtk_widget_set_sensitive(phone_var_spin, FALSE);
}
void change_playby(GtkWidget *widget)
{
	memset(temp.playby, 0, 16);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(song_radio)))
	{
		memcpy(temp.playby, "song", 4);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(album_radio)))
	{
		memcpy(temp.playby, "album", 5);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(artist_radio)))
	{
		memcpy(temp.playby, "artist", 6);
	}
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(genre_radio)))
	{
		memcpy(temp.playby, "genre", 5);
	}
	printf("playby = %s\n", temp.playby);
}
void change_rand (GtkWidget *widget)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(song_rand_check)))
		temp.song_rand = 1;
	else
		temp.song_rand = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(album_rand_check)))
		temp.album_rand = 1;
	else
		temp.album_rand = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(artist_rand_check)))
		temp.artist_rand = 1;
	else
		temp.artist_rand = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(genre_rand_check)))
		temp.genre_rand = 1;
	else
		temp.genre_rand = 0;
}
void change_skip (GtkWidget *widget)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(song_skip_check)))
		temp.song_skip = 1;
	else
		temp.song_skip = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(album_skip_check)))
		temp.album_skip = 1;
	else
		temp.album_skip = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(artist_skip_check)))
		temp.artist_skip = 1;
	else
		temp.artist_skip = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(genre_skip_check)))
		temp.genre_skip = 1;
	else
		temp.genre_skip = 0;
}
void send_config()
{
	memcpy(val.type, temp.type, strlen(temp.type) + 1);
	val.threshhold = temp.threshhold;
	val.var = temp.var;
	memcpy(val.playby, temp.playby, strlen(temp.playby) + 1);
	val.song_rand = temp.song_rand;
	val.song_skip = temp.song_skip;
	val.album_rand = temp.album_rand;
	val.album_skip = temp.album_skip;
	val.artist_rand = temp.artist_rand;
	val.artist_skip = temp.artist_skip;
	val.genre_rand = temp.genre_rand;
	val.genre_skip = temp.genre_skip;

	char com[32767];
	memcpy(com, "SC", 2);
	char *p = &com[2];
	int len = 2;
	memcpy(p, val.musicdir, strlen(val.musicdir) + 1);
	p += strlen(val.musicdir) + 1;
	len += strlen(val.musicdir) + 1;

	char t[4];
	memset(t, 0, 4);
	itoa(val.threshhold, t);
	memcpy(p, t, strlen(t) + 1);
	p += strlen(t) + 1;
	len += strlen(t) + 1;
	memcpy(p, val.type, strlen(val.type) + 1);
	p += strlen(val.type) + 1;
	len += strlen(val.type) + 1;
	char v[3];
	memset(v, 0, 3);
	itoa(val.var, v);
	memcpy(p, v, strlen(v) + 1);
	p += strlen(v) + 1;
	len += strlen(v) + 1;
	memcpy(p, val.playby, strlen(val.playby) + 1);
	p += strlen(val.playby) + 1;
	len += strlen(val.playby) + 1;

	if (val.song_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	if (val.song_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	len += 4;

	if (val.album_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	if (val.album_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	len += 4;

	if (val.artist_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	if (val.artist_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	len += 4;

	if (val.genre_rand)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	if (val.genre_skip)
		memcpy(p++, "1", 1);
	else
		memcpy(p++, "0", 1);
	memcpy(p++, "\0", 1);
	len += 4;

	send_command(com, len);
	set_playby(val.playby);
}
void save_config()
{
	send_config();
	send_command("SD", 2);
}
//com is constructed as follows:
//0/1|hh|mm| x 7 the hours and mins might be 1 digit, | = '\0'
//starting vol|ending vol|seconds to fade|file to play|
void send_alarm_config()
{
	char com[2048];
	memset(com, 0, 2048);
	memcpy(com, "SA", 2);
	char *pcom = &com[2];
	int len = 2;
	int i;
	for (i = 0; i < 7; i++)
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(day_check[i])))
		{
			memcpy(pcom, "1\0", 2);
			wake.set[i] = 1;
		}
		else
		{
			memcpy(pcom, "0\0", 2);
			wake.set[i] = 0;
		}
		pcom += 2;
		gint hr = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(day_hspin[i]));
		wake.hr[i] = (int) hr;
		char hr_s[3];
		memset(hr_s, 0, 3);
		itoa(hr, hr_s);
		memcpy(pcom, hr_s, strlen(hr_s) + 1);
		pcom += strlen(hr_s) + 1;
		gint min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(day_mspin[i]));
		wake.min[i] = (int) min;
		char min_s[3];
		memset(min_s, 0, 3);
		itoa(min, min_s);
		memcpy(pcom, min_s, strlen(min_s) + 1);
		pcom += strlen(min_s) + 1;
		len += strlen(hr_s) + strlen(min_s) + 4;
	}
	wake.start_vol = gtk_adjustment_get_value(GTK_ADJUSTMENT(start_adj));
	char start_s[4];
	memset(start_s, 0, 4);
	itoa(wake.start_vol, start_s);
	memcpy(pcom, start_s, strlen(start_s) + 1);
	pcom += strlen(start_s) + 1;

	wake.end_vol = gtk_adjustment_get_value(GTK_ADJUSTMENT(end_adj));
	char end_s[4];
	memset(end_s, 0, 4);
	itoa(wake.end_vol, end_s);
	memcpy(pcom, end_s, strlen(end_s) + 1);
	pcom += strlen(end_s) + 1;

	wake.fade = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(fade_in_spin));
	char fade_s[4];
	memset(fade_s, 0, 4);
	itoa(wake.fade, fade_s);
	memcpy(pcom, fade_s, strlen(fade_s) + 1);
	pcom += strlen(fade_s) + 1;

	const gchar *song = gtk_entry_get_text(GTK_ENTRY(wake_song_entry));
	memcpy(pcom, song, strlen(song) + 1);
	len += strlen(start_s) + strlen(end_s) + strlen(fade_s) + strlen(song) + 4;
	if (wake.file != NULL)
		free(wake.file);
	wake.file = malloc(strlen(song) + 1);
	memcpy(wake.file, song, strlen(song) + 1);

	send_command(com, len);
}
void save_alarm_config()
{
	send_alarm_config();
	char line[256];
	memset(line, 0, 256);
	FILE *fp;
	if ((fp = fopen(alarmconfig, "w")) == NULL)
		printf("can't open config file for writing\n");
	else
	{
		int i;
		for(i = 0; i < 7; i++)
		{
			fprintf(fp, "%d\n", i);
			fprintf(fp, "%d\n", wake.set[i]);
			fprintf(fp, "%d\n", wake.hr[i]);
			fprintf(fp, "%d\n", wake.min[i]);
		}
		fprintf(fp, "%d\n", wake.start_vol);
		fprintf(fp, "%d\n", wake.end_vol);
		fprintf(fp, "%d\n", wake.fade);
		if (wake.file == NULL)
			printf("No wake file\n");
		else
			fprintf(fp, "%s", wake.file);
	}
	fclose(fp);
}
void send_phone_data()
{
	char com[2048];
	memset(com, 0, 2048);
	memcpy(com, "DP", 2);
	char *pcom = &com[2];
	int len = 2;
	const gchar *type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(phone_type_combo));
	memcpy(pcom, type, strlen(type) + 1);
	pcom += strlen(type) + 1;
	len += strlen(type) + 1;

	gint thresh = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(phone_thresh_spin));
	char thresh_s[4];
	memset(thresh_s, 0, 4);
	snprintf(thresh_s, 3, "%d", thresh);
	memcpy(pcom, thresh_s, strlen(thresh_s) + 1);
	pcom += strlen(thresh_s) + 1;
	len += strlen(thresh_s) + 1;

	gint var = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(phone_var_spin));
	char var_s[4];
	memset(var_s, 0, 4);
	snprintf(var_s, 3, "%d", var);
	memcpy(pcom, var_s, strlen(var_s) + 1);
	pcom += strlen(var_s) + 1;
	len += strlen(var_s) + 1;

	gint time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(phone_time_spin));
	char time_s[4];
	memset(time_s, 0, 4);
	snprintf(time_s, 3, "%d", time);
	memcpy(pcom, time_s, strlen(time_s) + 1);
	pcom += strlen(time_s) + 1;
	len += strlen(time_s) + 1;

	const gchar *t_scale = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(phone_time_combo));
	memcpy(pcom, t_scale, strlen(t_scale) + 1);
	pcom += strlen(t_scale) + 1;
	len += strlen(t_scale) + 1;

	gint data = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(phone_data_spin));
	char data_s[5];
	memset(data_s, 0, 5);
	snprintf(data_s, 4, "%d", data);
	memcpy(pcom, data_s, strlen(data_s) + 1);
	pcom += strlen(data_s) + 1;
	len += strlen(data_s) + 1;

	const gchar *d_scale = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(phone_data_combo));
	memcpy(pcom, d_scale, strlen(d_scale) + 1);
	pcom += strlen(d_scale) + 1;
	len += strlen(d_scale) + 1;

	//print_data(com, len);
	send_command(com, len);
}
void close_config_win()
{
	gtk_widget_destroy(config_win);
	config_win = NULL;
}
