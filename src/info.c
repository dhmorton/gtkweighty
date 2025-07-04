/*
 * info.c
 *
 *  Created on: Jul 10, 2011
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
#define _GNU_SOURCE

#include "info.h"
#include "models.h"
#include "myutils.h"
#include "playing.h"
#include "actions.h"

static int streaming = 0;
static int first_time = 0;

static GtkWidget *info_win, *track_tv, *nb, *top_hbox, *info_tag_vbox;
static GtkWidget *lyrics_title_entry, *lyrics_artist_entry, *playby_entry, *lyrics_label, *lyrics_frame, *lyrics_vbox;
static GtkWidget *image_frame, *album_cover, *next_button, *prev_button;
static GtkWidget *track_up_but, *track_down_but;
static GtkWidget *image_vbox;
static GtkWidget *album_image_search_entry = NULL;
static GtkWidget *artist_image_search_entry = NULL;
static GtkWidget *album_search_frame = NULL;
static GtkWidget *artist_search_frame = NULL;
static char** image;//an array of images to display
static int image_count = -1;
static int image_index = -1;
char* discogs_token;

static void build_info_tag_vbox(int);
static GtkTreeModel* create_streaming_model(GtkWidget*);
static void change_info_weight(GtkButton*, GdkEventButton*);
static int lyrics_search(void);
static int discogs_image_search(void);
static size_t write_curl_data(void*, size_t, size_t, void*);
static void save_tags(void);
static void discogs_image_save(void);
static void clear_amazon_image_search(void);
static void playby_changed(GtkComboBox*, gpointer*);
static int change_image(GtkButton*);
static int resize_image(char*);
static void thumbnail_image(char*);
static int check_image(char*);
static void clear_images(void);
static void switch_page(GtkWidget*, gpointer, guint);
static void close_info_win(void);

int launch_info()
{
	if (info_win == NULL)
	{
		info_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		g_signal_connect(G_OBJECT (info_win), "destroy", G_CALLBACK (close_info_win), NULL);
	}

	char title[1024];
	memset(title, 0, 1024);
	char *playing_title = get_playing_title();
	if (playing_title != NULL)
		memcpy(title, playing_title, strlen(playing_title));
	char *playing_artist = get_playing_artist();
	if (playing_artist != NULL)
	{
		strcat(title, " by ");
		strncat(title, playing_artist, strlen(playing_artist) + 1);
	}
	if (*title == '\0')
		memcpy(title, "None", 4);
	int num_tags = get_num_tags();
	gtk_window_set_title(GTK_WINDOW(info_win), title);
	gtk_widget_set_size_request(info_win, -1, -1);

	GtkWidget *info_tag_tab_vbox, *info_lyrics_tab_vbox, *info_list_tab_vbox;
	if (first_time == 0)
	{
	//create the notebook and the various tabs
		nb = gtk_notebook_new();
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_TOP);

		GtkWidget *info_tag_tab_image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_MENU);
		GtkWidget *info_tag_tab_label = gtk_label_new("Tags");
		info_tag_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(info_tag_tab_vbox), info_tag_tab_image);
		gtk_container_add(GTK_CONTAINER(info_tag_tab_vbox), info_tag_tab_label);
		gtk_widget_show_all(info_tag_tab_vbox);

		GtkWidget *info_lyrics_tab_image = gtk_image_new_from_icon_name("format-text-bold", GTK_ICON_SIZE_MENU);
		GtkWidget *info_lyrics_tab_label = gtk_label_new("Lyrics");
		info_lyrics_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(info_lyrics_tab_vbox), info_lyrics_tab_image);
		gtk_container_add(GTK_CONTAINER(info_lyrics_tab_vbox), info_lyrics_tab_label);
		gtk_widget_show_all(info_lyrics_tab_vbox);

		GtkWidget *info_list_tab_image = gtk_image_new_from_icon_name("media-optical", GTK_ICON_SIZE_MENU);
		GtkWidget *info_list_tab_label = gtk_label_new("Tracks");
		info_list_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(info_list_tab_vbox), info_list_tab_image);
		gtk_container_add(GTK_CONTAINER(info_list_tab_vbox), info_list_tab_label);
		gtk_widget_show_all(info_list_tab_vbox);
	}
/*
 * TAGS TAB
 */
	//create the page of tags
	if (first_time == 0)
	{
		info_tag_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_set_homogeneous(GTK_BOX(info_tag_vbox), FALSE);
		gtk_widget_set_size_request(info_tag_vbox, 500, -1);
	}

	//set up possible tag values from the last three directories
	char *playing_file = get_playing_file();
	//printf("playing %s\n", playing_file);
	if (playing_file != NULL)
	{
		int active = 0;
		if(get_playby() == ALBUM)
			active = 1;
		else if (get_playby() == ARTIST)
			active = 2;
		else if (get_playby() == GENRE)
			active = 3;
		GtkWidget *playby_frame = gtk_frame_new("Edit tags by");
		playby_entry = gtk_combo_box_text_new();
		gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(playby_entry), 0, "song");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(playby_entry), "album");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(playby_entry), "artist");
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(playby_entry), "genre");
		gtk_combo_box_set_active(GTK_COMBO_BOX(playby_entry), active);
		gtk_container_add(GTK_CONTAINER(playby_frame), playby_entry);
		gtk_box_pack_start(GTK_BOX(info_tag_vbox), playby_frame, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(info_tag_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
		g_signal_connect(playby_entry, "changed", G_CALLBACK(playby_changed), NULL);
		build_info_tag_vbox(active);
	}
	else if (streaming)//streaming
	{
		char *basefields[] = { "Artist", "Title", "Genre"};
		char *basetags[] = { playing_artist, playing_title, get_playing_genre()};
		GtkWidget *frame[3 + num_tags];
		GtkWidget *entry[3 + num_tags];
		int i;
		for (i = 0; i < 3; i++)
		{
			frame[i] = gtk_frame_new(basefields[i]);
			entry[i] = gtk_entry_new();
			if (basetags[i] != NULL)
				gtk_entry_set_text(GTK_ENTRY(entry[i]), basetags[i]);
			gtk_container_add(GTK_CONTAINER(frame[i]), entry[i]);
			gtk_widget_show_all(frame[i]);
			gtk_box_pack_start(GTK_BOX(info_tag_vbox), frame[i], FALSE, FALSE, 2);
		}
		for (i = 0; i < num_tags; i++)
		{
			frame[i+3] = gtk_frame_new(get_playing_field(i));
			entry[i+3] = gtk_entry_new();
			char *tag = get_playing_tag(i);
			if (tag != NULL)
				gtk_entry_set_text(GTK_ENTRY(entry[i+3]), tag);
			gtk_container_add(GTK_CONTAINER(frame[i+3]), entry[i+3]);
			gtk_widget_show_all(frame[i+3]);
			gtk_box_pack_start(GTK_BOX(info_tag_vbox), frame[i+3], FALSE, FALSE, 2);
		}
	}
	//save and close buttons
	GtkWidget *info_tag_but_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *quit_but = gtk_button_new();
	GtkWidget *quit_img = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image((GtkButton*) quit_but, quit_img);
	GtkWidget *save_but = gtk_button_new();
	GtkWidget *save_img = gtk_image_new_from_icon_name("media-floppy", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image((GtkButton*) save_but, save_img);
	gtk_box_pack_start(GTK_BOX(info_tag_but_hbox), save_but, TRUE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(info_tag_but_hbox), quit_but, TRUE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(info_tag_vbox), info_tag_but_hbox, TRUE, FALSE, 2);

	//signals for the tags tab
	g_signal_connect(G_OBJECT(save_but), "clicked", G_CALLBACK (save_tags), NULL);
	g_signal_connect(G_OBJECT(quit_but), "clicked", G_CALLBACK (close_info_win), NULL);

/*
 * LYRICS TAB
 */
	lyrics_frame = gtk_frame_new(title);
	gtk_widget_set_size_request(lyrics_frame, 500, -1);
	GtkWidget *lyrics_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(lyrics_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(lyrics_frame), lyrics_scroll);
	lyrics_label = gtk_label_new("");
	gtk_label_set_selectable(GTK_LABEL(lyrics_label), TRUE);
	gtk_container_add(GTK_CONTAINER(lyrics_scroll), lyrics_label);
	//gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(lyrics_scroll), lyrics_label);

	lyrics_title_entry = gtk_entry_new();
	if (playing_title != NULL)
		gtk_entry_set_text(GTK_ENTRY(lyrics_title_entry), playing_title);
	gtk_editable_set_editable(GTK_EDITABLE(lyrics_title_entry), TRUE);
	GtkWidget *lyrics_title_frame = gtk_frame_new("Title");
	gtk_container_add(GTK_CONTAINER(lyrics_title_frame), lyrics_title_entry);

	lyrics_artist_entry = gtk_entry_new();
	if (playing_artist != NULL)
		gtk_entry_set_text(GTK_ENTRY(lyrics_artist_entry), playing_artist);
	gtk_editable_set_editable(GTK_EDITABLE(lyrics_artist_entry), TRUE);
	GtkWidget *lyrics_artist_frame = gtk_frame_new("Artist");
	gtk_container_add(GTK_CONTAINER(lyrics_artist_frame), lyrics_artist_entry);

	GtkWidget *lyrics_quit_but = gtk_button_new();
	GtkWidget *lyrics_quit_img = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image((GtkButton*) lyrics_quit_but, lyrics_quit_img);
	GtkWidget *lyrics_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(lyrics_button_hbox), lyrics_quit_but, FALSE, FALSE, 2);

	if (first_time == 0)
		lyrics_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(lyrics_vbox), lyrics_frame, TRUE, TRUE, 2);
	gtk_box_pack_end(GTK_BOX(lyrics_vbox), lyrics_button_hbox, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(lyrics_vbox), lyrics_artist_frame, FALSE, FALSE, 2);
	gtk_box_pack_end(GTK_BOX(lyrics_vbox), lyrics_title_frame, FALSE, FALSE, 2);

	//signals for the lyrics tab
	g_signal_connect(G_OBJECT(lyrics_quit_but), "clicked", G_CALLBACK (close_info_win), NULL);
	g_signal_connect(G_OBJECT(lyrics_artist_entry), "activate", G_CALLBACK (lyrics_search), NULL);
	g_signal_connect(G_OBJECT(lyrics_title_entry), "activate", G_CALLBACK (lyrics_search), NULL);
/*
 * TRACKS TAB
 */
	GtkTreeModel *track_model;
	GtkWidget *track_scroll, *info_list_vbox;
	GtkWidget *track_but_hbox, *track_play_but, *track_quit_but;
	if (first_time == 0)
	{
		track_scroll = gtk_scrolled_window_new(NULL, NULL);
		track_tv = gtk_tree_view_new();
		if (streaming == 0)
			track_model = create_standard_model(track_tv);
		else
			track_model = create_streaming_model(track_tv);
		gtk_tree_view_set_model(GTK_TREE_VIEW(track_tv), track_model);
		gtk_container_add(GTK_CONTAINER(track_scroll), track_tv);

		track_but_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		track_play_but = gtk_button_new();
		GtkWidget *track_play_img = gtk_image_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image((GtkButton*) track_play_but, track_play_img);
		track_quit_but = gtk_button_new();
		GtkWidget* track_quit_img = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image((GtkButton*) track_quit_but, track_quit_img);
		track_up_but = gtk_button_new();
		GtkWidget *track_up_image = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
		gtk_container_add(GTK_CONTAINER(track_up_but), track_up_image);
		track_down_but = gtk_button_new();
		GtkWidget *track_down_image = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
		gtk_container_add(GTK_CONTAINER(track_down_but), track_down_image);

		GtkWidget *up_down_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
		gtk_box_pack_start(GTK_BOX(up_down_hbox), track_up_but, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(up_down_hbox), track_down_but, FALSE, FALSE, 2);

		gtk_box_pack_start(GTK_BOX(track_but_hbox), track_play_but, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(track_but_hbox), track_quit_but, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(track_but_hbox), up_down_hbox, FALSE, FALSE, 2);

		info_list_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start(GTK_BOX(info_list_vbox), track_scroll, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(info_list_vbox), track_but_hbox, FALSE, TRUE, 0);
	//signals for the tracks tab
		//g_signal_connect(G_OBJECT(track_play_but), "clicked", G_CALLBACK (play_album_now), NULL);
		g_signal_connect(G_OBJECT(track_quit_but), "clicked", G_CALLBACK (close_info_win), NULL);
		g_signal_connect(track_tv, "row-activated", G_CALLBACK(play_row_now), NULL);
		g_signal_connect(G_OBJECT(track_up_but), "button_press_event", G_CALLBACK (change_info_weight), NULL);
		g_signal_connect(G_OBJECT(track_down_but), "button_press_event", G_CALLBACK (change_info_weight), NULL);
	}
/*
 * IMAGE PANE/ALBUM ART
 */
	if (first_time == 0)
	{
		image_frame = gtk_frame_new(NULL);
		album_cover = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_DIALOG);
		gtk_container_add(GTK_CONTAINER(image_frame), album_cover);

		next_button = gtk_button_new();
		GtkWidget *next_image = gtk_image_new_from_icon_name("media-skip-forward", GTK_ICON_SIZE_MENU);
		gtk_container_add(GTK_CONTAINER(next_button), next_image);

		prev_button = gtk_button_new();
		GtkWidget *prev_image = gtk_image_new_from_icon_name("media-skip-backward", GTK_ICON_SIZE_MENU);
		gtk_container_add(GTK_CONTAINER(prev_button), prev_image);

		GtkWidget *search_amazon_button = gtk_button_new();
		GtkWidget *search_amazon_img = gtk_image_new_from_icon_name("edit-find", GTK_ICON_SIZE_MENU);
		gtk_container_add(GTK_CONTAINER(search_amazon_button), search_amazon_img);
		GtkWidget *image_save_button = gtk_button_new();
		GtkWidget *image_save_image = gtk_image_new_from_icon_name("object-select-symbolic", GTK_ICON_SIZE_MENU);
		gtk_container_add(GTK_CONTAINER(image_save_button), image_save_image);

		//GtkWidget *cancel_button = gtk_button_new();
		//GtkWidget *cancel_image = gtk_image_new_from_stock("gtk-cancel", GTK_ICON_SIZE_MENU);
		GtkWidget *cancel_button = gtk_button_new_with_mnemonic("_Cancel");
		//gtk_container_add(GTK_CONTAINER(cancel_button), cancel_image);

		GtkWidget *image_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
		gtk_box_pack_start(GTK_BOX(image_button_hbox), cancel_button, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(image_button_hbox), image_save_button, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(image_button_hbox), search_amazon_button, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(image_button_hbox), next_button, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(image_button_hbox), prev_button, FALSE, FALSE, 2);

		image_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start(GTK_BOX(image_vbox), image_frame, TRUE, TRUE, 2);
		gtk_box_pack_start(GTK_BOX(image_vbox), image_button_hbox, FALSE, FALSE, 2);

		//image tab signals
		g_signal_connect(G_OBJECT(search_amazon_button), "button_press_event", G_CALLBACK(discogs_image_search), NULL);
		g_signal_connect(G_OBJECT(image_save_button), "button_press_event", G_CALLBACK(discogs_image_save), NULL);
		g_signal_connect(G_OBJECT(cancel_button), "button_press_event", G_CALLBACK(clear_amazon_image_search), NULL);
		g_signal_connect(G_OBJECT(next_button), "button_press_event", G_CALLBACK (change_image), NULL);
		g_signal_connect(G_OBJECT(prev_button), "button_press_event", G_CALLBACK (change_image), NULL);
	}

	//put the notebook together
	if (first_time == 0)
	{
		gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), info_tag_vbox, info_tag_tab_vbox, NULL);
		gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), lyrics_vbox, info_lyrics_tab_vbox, NULL);
		gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), info_list_vbox, info_list_tab_vbox, NULL);
		//notebook signals
		g_signal_connect(G_OBJECT(nb), "switch-page", G_CALLBACK (switch_page), NULL);

		top_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		if (streaming == 0)
			gtk_box_pack_start(GTK_BOX(top_hbox), image_vbox, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(top_hbox), nb, TRUE, TRUE, 0);

		gtk_container_add(GTK_CONTAINER(info_win), top_hbox);
	}

	gtk_widget_show_all(info_win);
	if (streaming && (first_time == 0))
		send_command("MH", 2);
	else
		send_command("DI", 2);
	get_images_by_dir();
	first_time = 1;
	return 0;
}
void build_info_tag_vbox(int active)
{
	char *album_guess = NULL;
	char *artist_guess = NULL;
	char *last_guess = NULL;
	char *segments[16 * sizeof(char)];

	int guesses = 0;
	char *playing_file = get_playing_file();
	char *playing_title = get_playing_title();
	char *playing_artist = get_playing_artist();
	int num_tags = get_num_tags();

	char file_copy[strlen(playing_file) + 1];
	memcpy(file_copy, playing_file, strlen(playing_file) + 1);
	char *p = strrchr(file_copy, '/');//strip off the file name
	*p = '\0';
	p = strrchr(file_copy, '/');//strip the / and point to the char after it
	*p = '\0';
	p++;
	album_guess = malloc(strlen(p) + 1);
	memcpy(album_guess, p, strlen(p) + 1);

	p = strrchr(file_copy, '/');//strip the next one &c.
	if (p != NULL)//no longer guaranteed to exist (unless people put music in their / directories?
	{
		*p = '\0';
		p++;
		artist_guess = malloc(strlen(p) + 1);
		memcpy(artist_guess, p, strlen(p) + 1);
	}
	p = strrchr(file_copy, '/');//strip the last one
	if (p != NULL)
	{
		*p = '\0';
		p++;
		last_guess = malloc(strlen(p) + 1);
		memcpy(last_guess, p, strlen(p) + 1);
	}
	//set up some more by splitting the filename at '-' and switching '_' to ' '
	memcpy(file_copy, playing_file, strlen(playing_file) + 1);//set up the file again
	p = strrchr(file_copy, '.');
	*p = '\0';
	p = strrchr(file_copy, '/');
	p++;

	char temp[256];
	int t = 0;
	while (*p != '\0')
	{
		if (*p == '_')
			*p = ' ';
		if (*p == '-')
		{
			temp[t] = '\0';
			segments[guesses] = malloc(256);
			memset(segments[guesses], 0, 256);
			memcpy(segments[guesses], temp, strlen(temp) + 1);
			guesses++;
			t = 0;
			if (guesses > 15)
				break;
		}
		else
		{
			temp[t++] = *p;
		}
		p++;
	}
	temp[t] = '\0';
	segments[guesses] = malloc(256);
	memset(segments[guesses], 0, 256);
	memcpy(segments[guesses], temp, strlen(temp) + 1);
	guesses++;

	//The first ones are always Artist, Title, Album, Genre, Track Number, and Year in that order even if blank
	char *basefields[] = { "Artist", "Title", "Album", "Genre", "Track Number", "Year" };
	char *basetags[] = { playing_artist, playing_title, get_playing_album(), get_playing_genre(), get_playing_track(), get_playing_year()};
	GtkWidget *frame[6 + num_tags];
	GtkWidget *entry[6 + num_tags];
	int i;
	for (i = 0; i < 6; i++)
	{
		frame[i] = gtk_frame_new(basefields[i]);
		if(active == 0 || (active == 1 && i == 2) || (active == 2 && i == 0) || (active == 3 && i == 3))
		{
			entry[i] = gtk_combo_box_text_new_with_entry();
			if (basetags[i] != NULL)
				gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(entry[i]), 0, basetags[i]);
			if (i == 0)
			{
				if (artist_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), artist_guess);
				if (album_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), album_guess);
				if (last_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), last_guess);
			}
			else if (i == 2)
			{
				if (album_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), album_guess);
				if (artist_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), artist_guess);
				if (last_guess != NULL)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), last_guess);
			}
			if ((i == 0) || (i == 1) || (i == 2))
			{
				int g;
				for (g = 0; g < guesses; g++)
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry[i]), segments[g]);
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(entry[i]), 0);
		}

		else
		{
			entry[i] = gtk_label_new(basetags[i]);
			gtk_label_set_xalign(GTK_LABEL(entry[i]), 0.03);
		}
		gtk_container_add(GTK_CONTAINER(frame[i]), entry[i]);
		gtk_widget_show_all(frame[i]);
		gtk_box_pack_start(GTK_BOX(info_tag_vbox), frame[i], FALSE, FALSE, 2);
	}
	for (i = 0; i < num_tags; i++)
	{
		char *field = get_playing_field(i);
		char *tag = get_playing_tag(i);
		frame[i+6] = gtk_frame_new(field);
		entry[i+6] = gtk_combo_box_text_new_with_entry();
		if (tag != NULL)
			gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(entry[i+6]), 0, tag);
		if(active != 0)
		{
			gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(entry[i+6]), GTK_SENSITIVITY_OFF);
			gtk_widget_set_focus_on_click(entry[i], FALSE);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(entry[i+6]), 0);
		gtk_container_add(GTK_CONTAINER(frame[i+6]), entry[i+6]);
		gtk_widget_show_all(frame[i+6]);
		gtk_box_pack_start(GTK_BOX(info_tag_vbox), frame[i+6], FALSE, FALSE, 2);
	}
}
GtkTreeModel* create_streaming_model(GtkWidget* tv)
{
	GtkListStore *model;
	model = gtk_list_store_new
	(		5,
			G_TYPE_STRING, 	//time
			G_TYPE_STRING, 	//artist
			G_TYPE_STRING,	//title
			G_TYPE_STRING, //stream/album
			GDK_TYPE_RGBA //color
	);
	GtkTreeViewColumn* time_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(time_col, "Time");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), time_col, 0);
	GtkCellRenderer* time_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(time_col, time_rend, TRUE);
	gtk_tree_view_column_add_attribute(time_col, time_rend, "text", 0);
	gtk_tree_view_column_add_attribute(time_col, time_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(time_col, 0);
	gtk_tree_view_column_set_sort_indicator(time_col, TRUE);

	GtkTreeViewColumn* artist_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(artist_col, "Artist");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), artist_col, 1);
	GtkCellRenderer* artist_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(artist_col, artist_rend, TRUE);
	gtk_tree_view_column_add_attribute(artist_col, artist_rend, "text", 1);
	gtk_tree_view_column_add_attribute(artist_col, artist_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(artist_col, 1);
	gtk_tree_view_column_set_sort_indicator(artist_col, TRUE);
	gtk_tree_view_column_set_resizable(artist_col, TRUE);

	GtkTreeViewColumn* title_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(title_col, "Title");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), title_col, 2);
	GtkCellRenderer* title_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(title_col, title_rend, TRUE);
	gtk_tree_view_column_add_attribute(title_col, title_rend, "text", 2);
	gtk_tree_view_column_add_attribute(title_col, title_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(title_col, 2);
	gtk_tree_view_column_set_sort_indicator(title_col, TRUE);
	gtk_tree_view_column_set_resizable(title_col, TRUE);

	GtkTreeViewColumn* stream_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(stream_col, "Stream/Album");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), stream_col, 3);
	GtkCellRenderer* stream_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(stream_col, stream_rend, TRUE);
	gtk_tree_view_column_add_attribute(stream_col, stream_rend, "text", 3);
	gtk_tree_view_column_add_attribute(stream_col, stream_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(stream_col, 3);
	gtk_tree_view_column_set_sort_indicator(stream_col, TRUE);

	//gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), TRUE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(model));
	return GTK_TREE_MODEL(model);
}
int populate_stream_tv(time_t t, char *artist, char *title, char *stream)
{
	if (info_win == NULL)
		return -1;
	struct tm *now;
	now = localtime(&t);
	char time[6];
	strftime(time, sizeof(time), "%H:%M", now);
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(track_tv));
	GtkTreeIter iter;
	gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			0, time,
			1, artist,
			2, title,
			3, stream, -1);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(track_tv), gtk_tree_model_get_path(model, &iter), NULL, TRUE, 0.5, 0.0);
	gtk_widget_show_all(track_tv);
	return 0;
}
void change_info_weight(GtkButton *button, GdkEventButton *event)
{
	int change = 0;
	if (button == GTK_BUTTON(track_up_but))
	{
		if (event->button == 1)
			change = 1;
		else if (event->button == 3)
			change = 10;
	}
	else if (button == GTK_BUTTON(track_down_but))
	{
		if (event->button == 1)
			change = -1;
		else if (event->button == 3)
			change = -10;
	}
	if (change)
	{
		char *playing = get_playing_file();
		GtkTreeModel *model;
		GList *list;
		list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(track_tv)), &model));
		if (list == NULL)//change currently playing song
		{
			//change it on the player
			change_weight_cursong(change);
			//find it and change it in the track_tv and the browser tvs
			change_info_tv_weight(playing, change);
//FIXME			change_browser_tv_weight(playing.file, change);
		}
		while (list != NULL)
		{
			GtkTreePath* path = list->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);
			gchar *file;
			int weight;
			gtk_tree_model_get(model, &iter, WEIGHT, &weight, FULLPATH, &file, -1);
			weight += change;
			if (weight > 100)
				weight = 100;
			else if (weight < 0)
				weight = 0;
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, WEIGHT, weight, -1);
			if (! strcmp(file, playing))
				change_weight_cursong(change);
//FIXME			change_browser_tv_weight(file, change);

			char com[4096];
			memset(com, '\0', 4096);
			memcpy(com, "SS", 2);
			char *pcom = &com[2];
			char w[4];
			memset(w, 0, 4);
			itoa(weight, w);
			memcpy(pcom, w, strlen(w) + 1);
			pcom += strlen(w) + 1;
			memcpy(pcom, file, strlen(file) + 1);
			send_command(com, strlen(w) + strlen(file) + 4);
			list = list->next;
			g_free(file);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
		gtk_widget_show_all(GTK_WIDGET(track_tv));
	}
}
//changes the weight of file by change if it matches anything in the tv
void change_info_tv_weight(char* file, int change)
{
	if (track_tv != NULL)
	{
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(track_tv));
		GtkTreeIter iter;
		int weight;
		if (gtk_tree_model_get_iter_first(model, &iter))
		{
			do
			{
				gchar *filepath;
				gtk_tree_model_get(model, &iter, WEIGHT, &weight, FULLPATH, &filepath, -1);
				if (! strcmp(file, filepath))
				{
					weight += change;
					if (weight > 100)
						weight = 100;
					else if (weight < 0)
						weight = 0;
					gtk_list_store_set(GTK_LIST_STORE(model), &iter, WEIGHT, weight, -1);
					break;
				}
				g_free(filepath);
			} while (gtk_tree_model_iter_next(model, &iter));
			gtk_widget_show_all(GTK_WIDGET(track_tv));
		}
	}
}
void set_lyrics(char *lyrics)
{
	gtk_label_set_text(GTK_LABEL(lyrics_label), lyrics);
	gtk_widget_show_all(lyrics_label);
}
/*
 * CALLBACKS
 */
void playby_changed(GtkComboBox *combo, gpointer *data)
{
	GList *iter;
	GList *children = gtk_container_get_children(GTK_CONTAINER(info_tag_vbox));
	//want to leave the combo box and the button box and just delete the tag boxes
	for(iter = g_list_next(children); iter->next != NULL; iter = g_list_next(iter))
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_object_ref(iter->data);
	gtk_container_remove(GTK_CONTAINER(info_tag_vbox), GTK_WIDGET(iter->data));
	build_info_tag_vbox((int) gtk_combo_box_get_active(combo));
	gtk_box_pack_end(GTK_BOX(info_tag_vbox), GTK_WIDGET(iter->data), TRUE, FALSE, 2);
	g_list_free(children);
}
//save new tags for the currently playing song
void save_tags()
{
	printf("\t\t***SAVE TAGS\n");
	//look through all of the combo/entry boxes in the tag tab
	int i = 0;
	char *playing_file = get_playing_file();
	int active = (int) gtk_combo_box_get_active(GTK_COMBO_BOX(playby_entry));

	if(active == 0)
	{
		GList *list = gtk_container_get_children(GTK_CONTAINER(info_tag_vbox));
		int len = 0;
		char com[32767];
		memset(com, 0, 32767);
		memcpy(com, "ST", 2);
		char *pcom = &com[2];
		int tag_count = 6 + get_num_tags();//the standard 6 plus whatever else the particular file has
		char anum_tags[3];
		memset(anum_tags, 0, 3);
		itoa(tag_count, anum_tags);
		memcpy(pcom, anum_tags, strlen(anum_tags) + 1);
		pcom += strlen(anum_tags) + 1;
		len += strlen(anum_tags) + 1;
		memcpy(pcom, playing_file, strlen(playing_file) + 1);
		pcom += strlen(playing_file) + 1;
		len += strlen(playing_file) + 1 + 2;
		while (list != NULL)
		{
			GtkWidget *frame = list->data;
			if(! GTK_IS_FRAME(frame))
			{
				list = list->next;
				continue;
			}
			const gchar* field;
			const gchar *tag = NULL;
			if ((field = gtk_frame_get_label(GTK_FRAME(frame))))
			{
				printf("field = %s\n", field);
				GList *taglist = gtk_container_get_children(GTK_CONTAINER(frame));
				while (taglist != NULL)
				{
					tag = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(taglist->data));
					printf("tag = %s\n", tag);
					break;
				}
				g_list_free(taglist);
			}
			//change the artist/title window and update the playing struct
			if (! strcasecmp(field, "artist"))
			{
				set_artist((char*)tag);
				set_playing_artist((char*)tag);
			}
			else if (! strcasecmp(field, "title"))
			{
				set_title((char*)tag);
				set_playing_title((char*)tag);
			}
			else if (! strcasecmp(field, "album"))
				set_playing_album((char*)tag);
			else if (! strcasecmp(field, "genre"))
				set_playing_genre((char*)tag);
			else if (! strcasecmp(field, "track number"))
				set_playing_track((char*)tag);
			else if (! strcasecmp(field, "year"))
				set_playing_year((char*)tag);
			char *trfield = malloc(sizeof(char) * 5);
			back_translate((char*)field, &trfield);
			printf("trfield = %s\n", trfield);
			if (trfield[0] == '\0')
			{
				memcpy(pcom, field, strlen(field) + 1);
				pcom += strlen(field) + 1;
				len += strlen(field) + 1;
				printf("\nno back translate for %s\n\n", field);
			}
			else
			{
				memcpy(pcom, trfield, strlen(trfield) + 1);
				pcom += strlen(trfield) + 1;
				len += strlen(trfield) + 1;
			}
			if (tag != NULL)
			{
				memcpy(pcom, tag, strlen(tag) + 1);
				pcom += strlen(tag) + 1;
				len += strlen(tag) + 1;
			}
			else
			{
				memcpy(pcom, "\0", 1);
				pcom++;
				len++;
			}
			//printf("len = %d\n", len);
			list = list->next;
			if (++i == tag_count)
				break;
			free(trfield);
		}
		g_list_free(list);
		send_command(com, len);
	}
	else
	{
		char *match = "album";
		if(active == 2)
			match = "artist";
		else if (active == 3)
			match = "genre";
		GList *list = gtk_container_get_children(GTK_CONTAINER(info_tag_vbox));
		GtkWidget *frame = list->data;
		const gchar *tag = NULL;
		while (frame != NULL)
		{
			const gchar* field;
			if(GTK_IS_FRAME(frame))
			{
				field = gtk_frame_get_label(GTK_FRAME(frame));
				if (! strcasecmp(field, match))
				{
					GList *taglist = gtk_container_get_children(GTK_CONTAINER(frame));
					while (taglist != NULL)
					{
						if(! GTK_IS_COMBO_BOX_TEXT(taglist->data))
						{
							taglist = taglist->next;
							continue;
						}
						tag = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(taglist->data));
						break;
					}
					g_list_free(taglist);
					if(tag == NULL)
					{
						printf("NULL tag\n");
						break;
					}
					printf("tag = %s\n", tag);
					if(active == 1)
					{
						set_playing_album((char*) tag);
					}
					else if (active == 2)
					{
						set_title((char*)tag);
						set_playing_title((char*)tag);
					}
					else
					{
						set_playing_genre((char*)tag);
					}

					break;
				}
			}
			list = list->next;
			frame = list->data;
		}
		g_list_free(list);
		GtkTreeIter iter;
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(track_tv));
		gtk_tree_model_get_iter_first(model, &iter);
		do {
			gchar *filepath;
			gtk_tree_model_get(model, &iter, FULLPATH, &filepath, -1);
			printf("song = %s\n", filepath);
			char com[32767];
			memset(com, 0, 32767);
			memcpy(com, "ST", 2);
			char *pcom = &com[2];
			int len = 2;
			memcpy(pcom, "1", 2);
			pcom += 2;
			len += 2;
			memcpy(pcom, filepath, strlen(filepath) + 1);
			pcom += strlen(filepath) + 1;
			len += strlen(filepath) + 1;
			char *trfield = malloc(sizeof(char) * 5);
			back_translate((char*)match, &trfield);
			printf("trfield = %s\n", trfield);
			if (trfield[0] == '\0')
			{
				memcpy(pcom, match, strlen(match) + 1);
				pcom += strlen(match) + 1;
				len += strlen(match) + 1;
				printf("\nno back translate for %s\n\n", match);
			}
			else
			{
				memcpy(pcom, trfield, strlen(trfield) + 1);
				pcom += strlen(trfield) + 1;
				len += strlen(trfield) + 1;
			}
			if (tag != NULL)
			{
				memcpy(pcom, tag, strlen(tag) + 1);
				pcom += strlen(tag) + 1;
				len += strlen(tag) + 1;
			}
			else
			{
				memcpy(pcom, "\0", 1);
				pcom++;
				len++;
			}
			print_data(com, len);
			send_command(com, len);
			g_free(filepath);
			free(trfield);
		} while(gtk_tree_model_iter_next(model, &iter));
	}
}

int clear_info()
{
	if (info_win == NULL)
		return -1;
	GList *list = gtk_container_get_children(GTK_CONTAINER(info_tag_vbox));
	while (list != NULL)
	{
		GtkWidget *widget = list->data;
		gtk_widget_destroy(widget);
		list = list->next;
	}
	list = gtk_container_get_children(GTK_CONTAINER(lyrics_vbox));
	while (list != NULL)
	{
		GtkWidget *widget = list->data;
		gtk_widget_destroy(widget);
		list = list->next;
	}
	return 0;
}
int lyrics_search()
{
	gtk_label_set_text(GTK_LABEL(lyrics_label), "");
	const gchar *artist = gtk_entry_get_text(GTK_ENTRY(lyrics_artist_entry));
	const gchar *title = gtk_entry_get_text(GTK_ENTRY(lyrics_title_entry));
	if ((artist == NULL) || (title == NULL))
		return 1;
	char frame_title[256];
	memset(frame_title, 0, 256);
	memcpy(frame_title, title, strlen(title));
	strncat(frame_title, " by ", 5);
	strncat(frame_title, artist, strlen(artist) + 1);
	gtk_frame_set_label(GTK_FRAME(lyrics_frame), frame_title);
	char com[1024];
	memset(com, 0, 1024);
	memcpy(com, "DY", 2);
	char *pcom = &com[2];
	memcpy(pcom, artist, strlen(artist) + 1);
	pcom += strlen(artist) + 1;
	memcpy(pcom, title, strlen(title) + 1);
	send_command(com, strlen(artist) + strlen(title) + 4);
	return 0;
}
int discogs_image_search()
{
	const char *title = NULL;
	const char *artist = NULL;
    struct curl_slist *header = NULL;
    int res = 0;
    struct curl_data chunk;
    chunk.data = malloc(1);
    chunk.size = 0;

	if(album_image_search_entry == NULL)
		title = get_playing_album();
	else
		title = gtk_entry_get_text(GTK_ENTRY(album_image_search_entry));
	if (artist_image_search_entry == NULL)
		artist = get_playing_artist();
	else
		artist = gtk_entry_get_text(GTK_ENTRY(artist_image_search_entry));

	if (artist == NULL && title == NULL) {
		printf("fail\n");
		return -1;
	}
	//set up the album/title fields if at least one is non-empty
	char frame_string[64];
	memset(frame_string, 0, 64);
	memcpy(frame_string, "Searching", 9);
	gtk_frame_set_label(GTK_FRAME(image_frame), frame_string);
	if (album_image_search_entry == NULL && artist_image_search_entry == NULL) {
		album_image_search_entry = gtk_entry_new();
		artist_image_search_entry = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(album_image_search_entry), title);
		gtk_entry_set_text(GTK_ENTRY(artist_image_search_entry), artist);
		album_search_frame = gtk_frame_new("Album");
		artist_search_frame = gtk_frame_new("Artist");
		gtk_container_add(GTK_CONTAINER(album_search_frame), album_image_search_entry);
		gtk_container_add(GTK_CONTAINER(artist_search_frame), artist_image_search_entry);
		gtk_box_pack_end(GTK_BOX(image_vbox), artist_search_frame, FALSE, FALSE, 2);
		gtk_box_pack_end(GTK_BOX(image_vbox), album_search_frame, FALSE, FALSE, 2);
	}
	gtk_widget_show_all(info_win);

	//bout 1 - get the search results for title and/or artist
	CURL *curl = curl_easy_init();
	char *cartist = curl_easy_escape(curl, artist, strlen(artist));
	char *ctitle;
	if (title != NULL)
		ctitle = curl_easy_escape(curl, title, strlen(title));

	char head[70] = "Authorization: Discogs token=";
	char discogs_header[70];
	snprintf(discogs_header, sizeof(discogs_header), "%s%s", head, discogs_token);
	discogs_header[69] = 0;
	printf("discogs header = %s\n", discogs_header);

	header = curl_slist_append(header, discogs_header);
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "weighty/4.0.3 +https://github.com/dhmorton/weighty");
    if(res != CURLE_OK)
    {
    	printf("failed to set header\n");
    	return -1;
    }
    int req_length = 1024;
    unsigned char *request = malloc(sizeof(char) * req_length);
    memset(request, 0, req_length);
    unsigned char *pr = &request[0];
    memcpy(pr, "https://api.discogs.com/database/search?", 40);
    pr += 40;
    if(title != NULL)
    {
    	memcpy(pr, "release_title=", 14);
    	pr += 14;
    	memcpy(pr, ctitle, strlen(ctitle));
    	pr += strlen(ctitle);
    	memcpy(pr, "&", 1);
    	pr += 1;
    }
    if(artist != NULL)
    {
    	memcpy(pr, "artist=", 7);
    	pr += 7;
    	memcpy(pr, cartist, strlen(cartist));
    	pr += strlen(cartist);
    	memcpy(pr, "&", 1);
    	pr += 1;
    }
    memcpy(pr, "page=1", 7);
    pr += 7;
    printf("%s\n", request);

	curl_easy_setopt(curl, CURLOPT_URL, request);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		printf("curl failed %d\n", res);
		curl_easy_strerror(res);
		return -1;
	}
	curl_easy_cleanup(curl);
	//printf("\n%s\n\n", chunk.data);

	char* pdata = &(chunk.data[0]);
	int pos = 0;
	unsigned char* resource_url = malloc(sizeof(char) * 300);
	memset(resource_url, 0, 300);
	while(pos <= chunk.size - 12)
	{
		if(strncmp(pdata, "resource_url", 12) == 0)
		{
			pdata += 16;//skip ": "
			pos += 16;
			int count = 0;
			while(*pdata != '"')
			{
				pdata++;
				count++;
			}
			pdata -= count;
			memcpy(resource_url, pdata, count);
			break;
		}
		pos++;
		pdata++;
	}
	printf("RESOURCE URL = %s\n", resource_url);
	free(chunk.data);
	chunk.data = malloc(1);
	chunk.size = 0;
	if(strlen((char*)resource_url) == 0)
	{
		printf("not found at discogs\n");
		return -1;
	}

	//bout 2 - get the image url from the resource_url from the search result
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "weighty/4.0.3 +https://github.com/dhmorton/weighty");
	curl_easy_setopt(curl, CURLOPT_URL, resource_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		printf("curl failed %d\n", res);
		curl_easy_strerror(res);
		return -1;
	}
	curl_easy_cleanup(curl);
	//printf("\n%s\n\n", chunk.data);

	pdata = &(chunk.data[0]);
	pos = 0;
	memset(resource_url, 0, 300);
	while(pos <= chunk.size - 20)
	{
		if(strncmp(pdata, "\"images\": [{\"uri\": \"", 20) == 0)
		{
			pdata += 20;
			pos += 20;
			int count = 0;
			while(*pdata != '"')
			{
				pdata++;
				count++;
			}
			pdata -= count;
			memcpy(resource_url, pdata, count);
			break;
		}
		pos++;
		pdata++;
	}
	printf("IMAGE URL = %s\n", resource_url);
	if(strlen((char*)resource_url) == 0)
	{
		printf("No image found at discogs\n");
		return -1;
	}

	//bout 3 - get the image from the images uri tag
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, resource_url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "weighty/4.0.3 +https://github.com/dhmorton/weighty");
	FILE* image_file = fopen("/tmp/discogs-image-search-result.jpg", "w");
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, image_file);
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		printf("curl_easy_perform failed %s\n", curl_easy_strerror(res));
	fclose(image_file);


	//add the image to the image frame
	gtk_container_remove(GTK_CONTAINER(image_frame), album_cover);
	album_cover = gtk_image_new_from_file("/tmp/discogs-image-search-result.jpg");
	gtk_container_add(GTK_CONTAINER(image_frame), album_cover);
	gtk_frame_set_label(GTK_FRAME(image_frame), "Discog Image Search Result");
	gtk_widget_show_all(info_win);

	//cleanup
	curl_easy_cleanup(curl);
	free(chunk.data);
	free(resource_url);

	return 0;
}
size_t write_curl_data(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct curl_data *data = (struct curl_data *)userp;

	data->data = realloc(data->data, data->size + realsize + 1);
	if(data->data == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(data->data[data->size]), contents, realsize);
	data->size += realsize;
	data->data[data->size] = 0;

	return realsize;
}
void discogs_image_save()
{
	char *playing_file = get_playing_file();
	if(playing_file == NULL)
		return;
	char *playing_album = get_playing_album();
	char *dir = malloc(strlen(playing_file) + 1);
	memcpy(dir, playing_file, strlen(playing_file) + 1);
	char *p = strrchr(dir, '/');
	p++;
	*p = '\0';//truncate the path after the last slash to get the directory

	//for the case where the album name has a / in it
	char *palbum = &playing_album[0];
	if (strrchr(playing_album, '/'))
		palbum++;

	char *new_filename = malloc(sizeof(char)*256);
	memset(new_filename, 0, sizeof(char)*256);
	memcpy(new_filename, dir, strlen(dir));
	strcat(new_filename, palbum);
	strcat(new_filename, "-Front-Cover.jpg");
	printf("move \t%s\n \t %s\n","/tmp/discogs-image-search-result.jpg", new_filename);
	char com[512];
	sprintf(com, "cp \"%s\" \"%s\"", "/tmp/discogs-image-search-result.jpg", new_filename);
	system(com);
	free(dir);
	free(new_filename);
	clear_amazon_image_search();
}
void clear_amazon_image_search()
{
	int i;
	for (i = 0; i < image_count; i++) {
		if (strncmp(image[i], "/tmp/", 5) == 0)
			unlink(image[i]);
	}
	gtk_container_remove(GTK_CONTAINER(image_vbox), album_search_frame);
	gtk_container_remove(GTK_CONTAINER(image_vbox), artist_search_frame);
	album_search_frame = NULL;
	artist_search_frame = NULL;
	album_image_search_entry = NULL;
	artist_image_search_entry = NULL;
	printf("Get images by dir from clear_amazon_image_search()\n");
	get_images_by_dir();
}
/*
 * IMAGE FUNCTIONS
 */
int get_images_by_dir()
{
	char *playing_file = get_playing_file();
	if ((info_win == NULL) || (playing_file == NULL) || (streaming))
		return -1;
	clear_images();

	struct stat sbuf;

	//first check to see if there is an image from the tags waiting for us
	if (stat("/tmp/weighty-tag-album-art.jpg", &sbuf) == -1)
	{
		//printf("no tag album art\n");
	}
	else if (S_ISREG(sbuf.st_mode))
	{
		//printf("*FOUND tag album art\n");
		image = malloc(128 * sizeof(char*));
		if (image_count < 0)
			image_count = 0;
		image[0] = malloc(32);
		memset(image[0], 0, 32);
		memcpy(image[0], "/tmp/weighty-tag-album-art.jpg\0", 31);
		image_count++;
	}

	char *dir = malloc(strlen(playing_file) + 1);
	memcpy(dir, playing_file, strlen(playing_file) + 1);
	char *p = strrchr(dir, '/');
	*p = '\0';//truncate the path at the last slash to get the directory

	if (stat(dir, &sbuf) == -1)
		printf("lstat failed on %s\n", dir);
	else if (S_ISDIR(sbuf.st_mode))
	{
		DIR *ddir;
		struct dirent *pdir;
		ddir = opendir(dir);
		if (image_count != 1)
			image = malloc(128 * sizeof(char*));

		while ((pdir = readdir(ddir)) != NULL)
		{
			int ret = check_image(pdir->d_name);
			if (ret >= 0)
			{
				if (image_count < 0)
					image_count = 0;
				int len = strlen(dir) + strlen(pdir->d_name) + 2;
				image[image_count] = malloc(len);
				memset(image[image_count], 0, len);
				memcpy(image[image_count], dir, strlen(dir));
				strcat(image[image_count], "/");
				strcat(image[image_count], pdir->d_name);
				strcat(image[image_count], "\0");
				image_count++;
			}
		}
		closedir(ddir);
		//move image with name that matches "front" to the beginning
		int i;
		for(i = 0; i < image_count; i++) {
			//printf("%d %s\n", i, image[i]);
			if(i > 0 && strcasestr(image[i], "front") != NULL) {
				printf("moving %s to spot 0\n", image[i]);
				char temp[strlen(image[0]) + 1];
				strcpy(temp, image[0]);
				free(image[0]);
				image[0] = malloc(strlen(image[i]) + 1);
				strcpy(image[0], image[i]);
				free(image[i]);
				image[i] = malloc(strlen(temp) + 1);
				strcpy(image[i], temp);
			}
		}
	}
	if (image_count > 0)
	{
		thumbnail_image(image[0]);
		gtk_container_remove(GTK_CONTAINER(image_frame), album_cover);
		if (resize_image(image[0]))
			album_cover = gtk_image_new_from_file("/tmp/weighty-scaled-album-art.png");
		else
			album_cover = gtk_image_new_from_file(image[0]);
		gtk_container_add(GTK_CONTAINER(image_frame), album_cover);
		char frame_string[128];
		memset(frame_string, 0, 128);
		memcpy(frame_string, "Album Art 1/", 12);
		char c[4];
		itoa(image_count, c);
		strcat(frame_string, c);
		strcat(frame_string, "\0");
		gtk_frame_set_label(GTK_FRAME(image_frame), frame_string);
		gtk_widget_show_all(info_win);
		image_index = 0;
	}
	else
	{
		gtk_frame_set_label(GTK_FRAME(image_frame), "No Cover Art Available");
		image_index = -1;
		image_count = -1;
	}
	return 0;
}
int change_image(GtkButton *button)
{
	//no images so bail
	if(image_count <= 0)
		return 0;
	if (button == GTK_BUTTON(next_button))
	{
		if (image_index == (image_count - 1))
			return -1;
		image_index++;
	}
	else if (button == GTK_BUTTON(prev_button))
	{
		if (image_index == 0)
			return -1;
		image_index--;
	}
	gtk_container_remove(GTK_CONTAINER(image_frame), album_cover);
	if (resize_image(image[image_index]))
		album_cover = gtk_image_new_from_file("/tmp/weighty-scaled-album-art.png");
	else
		album_cover = gtk_image_new_from_file(image[image_index]);
	gtk_window_resize(GTK_WINDOW(info_win), 200, 200);
	gtk_container_add(GTK_CONTAINER(image_frame), album_cover);
	char frame_string[128];
	memset(frame_string, 0, 128);
	memcpy(frame_string, "Album Art ", 10);
	char s[4];
	memset(s, 0, 4);
	itoa(image_index + 1, s);
	strcat(frame_string, s);
	strcat(frame_string, "/");
	char c[4];
	itoa(image_count, c);
	strcat(frame_string, c);
	strcat(frame_string, "\0");
	gtk_frame_set_label(GTK_FRAME(image_frame), frame_string);
	gtk_widget_show_all(info_win);

	return 0;
}
int check_image(char *file)
{
	char *s = strrchr(file, '.');
	if (s == NULL)//no extension
		return -1;
	s++;//step past the .
	if ((! strcasecmp(s, "jpg")) || (! strcasecmp(s, "jpeg")))
		return 0;
	else if (! strcasecmp(s, "png"))
		return 1;
	else if (! strcasecmp(s, "bmp"))
		return 2;
	return -1;
}
int resize_image(char *file)
{
	Imlib_Image image;
	image = imlib_load_image(file);
	if (image != NULL)
	{
		imlib_context_set_image(image);
		int w = imlib_image_get_width();
		int h = imlib_image_get_height();
		//printf("width = %d\theight = %d\n", w, h);
		float ratio = ((float) w) / h;
		//printf("scale = %.2f\n", ratio);
		if ((h > 960) || (w > 1150))
		{
			//printf("\tRESIZED\n");
			float f_w, f_h = 0;
			if (w > 1150)//scale by width
			{
				f_w = 1150.0;
				f_h = f_w / ratio;
				//printf("scaled h = %.2f\tscaled w = %.2f\n", f_h, f_w);
			}
			if (h > 960)//if height is still too big scale again
			{
				f_h = 960.0;
				f_w = f_h * ratio;
				//printf("scaled h = %.2f\tscaled w = %.2f\n", f_h, f_w);
			}
			Imlib_Image scaled;
			scaled = imlib_create_image((int) f_w, (int) f_h);
			imlib_context_set_blend(1);
			imlib_context_set_image(scaled);
			imlib_blend_image_onto_image(image, 0, 0, 0, w, h, 0, 0, (int) f_w, (int) f_h);
			imlib_save_image("/tmp/weighty-scaled-album-art.png");
			imlib_free_image_and_decache();
			imlib_context_set_image(image);
			imlib_free_image_and_decache();
			return 1;
		}
	}
	else
		printf("failed to load image %s\n", file);
	return 0;
}
void thumbnail_image(char *file)
{
	Imlib_Image image;
	image = imlib_load_image(file);
	if(image == NULL)
	{
		printf("failed to load image for thumbnail %s\n", file);
		return;
	}
	imlib_context_set_image(image);
	int w = imlib_image_get_width();
	int h = imlib_image_get_height();
	float f_h = 60.0;
	float f_w = ((float) w * f_h) / h;	
	Imlib_Image scaled;
	scaled = imlib_create_image((int) f_w, (int) f_h);
	imlib_context_set_blend(1);
	imlib_context_set_image(scaled);
	imlib_blend_image_onto_image(image, 0, 0, 0, w, h, 0, 0, (int) f_w, (int) f_h);
	imlib_save_image("/tmp/weighty-thumbnail-album-art.png");
	imlib_free_image_and_decache();
	imlib_context_set_image(image);
	imlib_free_image_and_decache();
	set_thumbnail();
}
//free the memory for all of the album art and remove it from the gui
void clear_images()
{
	int i;
	for (i = 0; i < image_count; i++)
	{
		if (image[i] != NULL)
		{
			free(image[i]);
			image[i] = NULL;
		}
	}
	if (image != NULL)
		free(image);
	image_count = -1;
	image_index = -1;
	gtk_container_remove(GTK_CONTAINER(image_frame), album_cover);
	album_cover = gtk_image_new_from_icon_name("image-missing", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(image_frame), album_cover);
	gtk_widget_show_all(image_frame);
	clear_thumbnail();
}
void update_info_win()
{
	if (info_win != NULL)
	{
		if (streaming == 0)
		{
			gtk_window_resize(GTK_WINDOW(info_win), 100, 100);
			launch_info();
			if (gtk_notebook_get_current_page(GTK_NOTEBOOK(nb)) == 1)
				lyrics_search();
		}
	}
}
void update_streaming_win()
{
	if (info_win != NULL)
	{
		if (streaming)
		{
			launch_info();
			if (gtk_notebook_get_current_page(GTK_NOTEBOOK(nb)) == 1)
				lyrics_search();
		}
	}
}
void switch_page(GtkWidget *nb, gpointer page, guint page_num)
{
	char *playing_artist = get_playing_artist();
	char *playing_title = get_playing_title();
	if ((page_num == 1) && (playing_artist != NULL) && (playing_title != NULL))
	{
		char com[1024];
		memset(com, 0, 1024);
		memcpy(com, "DY", 2);
		char *pcom = &com[2];
		memcpy(pcom, playing_artist, strlen(playing_artist) + 1);
		pcom += strlen(playing_artist) + 1;
		memcpy(pcom, playing_title, strlen(playing_title) + 1);
		pcom += strlen(playing_title) + 1;
		send_command(com, strlen(playing_artist) + strlen(playing_title) + 4);
	}
}
int highlight_track()
{
	char *playing_file = get_playing_file();
	if (playing_file == NULL)
		return -1;
	//printf("highlight track\n");
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(track_tv));
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(model, &iter);
	do
	{
		gchar *filepath;
		gtk_tree_model_get(model, &iter, FULLPATH, &filepath, -1);
		if (! strcmp(playing_file, filepath))
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLOR, &color_red, -1);
			GtkTreePath *path;
			path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(track_tv), path, NULL, TRUE, 0.5, 0.0);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(track_tv), path, NULL, FALSE);
		}
		else
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLOR, &color_black, -1);
		g_free(filepath);
	} while (gtk_tree_model_iter_next(model, &iter));
	gtk_widget_show_all(GTK_WIDGET(track_tv));
	return 0;
}
void populate_track_tv(char *file, int weight, int sticky, int i)
{
	if(streaming == 0)
		populate_tv(track_tv, file, weight, sticky, i, 0);
}
void set_streaming(int flag)
{
	if (flag != streaming)
	{
		streaming = (flag == 0) ? 0 : 1;
		destroy_track_tv();
	}
}
void clear_track_tv()
{
	if ((info_win != NULL) && (streaming == 0))
	{
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(track_tv));
		gtk_list_store_clear(GTK_LIST_STORE(model));
	}
}
void destroy_track_tv()
{
	if (info_win != NULL)
	{
		printf("DESTROY\n");
		gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(nb));
		gtk_widget_destroy(top_hbox);
		first_time = 0;
		gtk_window_resize(GTK_WINDOW(info_win), 100, 100);
		launch_info();
		gtk_notebook_set_current_page(GTK_NOTEBOOK(nb), page);
	}
}
void close_info_win()
{
	gtk_widget_destroy(info_win);
	first_time = 0;
	info_win = NULL;
}
