/*
 * browser.c
 *
 *  Created on: Jun 20, 2011
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

#include "browser.h"
#include "info.h"
#include "actions.h"
#include "stats_actions.h"
#include "models.h"
#include "myutils.h"
#include "config.h"
#include "playing.h"

static GtkWidget *b_win = NULL;
static GtkWidget *nb, *b_tree_scroll, *b_pl_scroll;
static GtkWidget *tv, *file_tv, *pl_tv, *search_file_tv, *list_file_tv, *list_tv, *list_tree_tv, *stream_tv;
static GtkWidget *up_but_sel, *down_but_sel, *list_song_up_but, *list_song_down_but, *list_item_up_but, *list_item_down_but;
static GtkWidget *add_all_but, *add_sel_but, *search_addall_but, *search_addsel_but;
static GtkWidget *search_fields, *url_entry;//the combo box with the selected search field, also the text entry with the stream url to add
static int static_i = 0;
static int dircount = 0;
static char displayed_dir[1024];//the currently displayed directory
//lists of the columns being displayed in the respective tag_tv windows
tag_tv_columns *tag_col = NULL;
tag_tv_columns *search_tag_col = NULL;
tag_tv_columns *list_tag_col = NULL;

static int playlist_index = -1;
static int tag_tv_flag = 0;//0 for tag_tv, 1 for search_tag_tv
static char *musicdir = NULL;

static int show_current = 0;//for setting the cursor on the list panes as data comes in
static int show_current_file = 0;
static char displayed_list[1024];



//model creation functions
static void create_browser_model(void);
static GtkTreeModel* create_list_model(GtkWidget*);
static GtkTreeModel* create_stream_model(GtkWidget*);
//browser callbacks
static void open_dir(void);
static void play_list_now(GtkWidget*);
static void add_selected(GtkButton*);
static void add_all(GtkButton*);
static void clear_queue(GtkButton*, GdkEventButton*);
static void change_weight_selected(GtkButton*, GdkEventButton*);
static void change_tv_weight(GtkTreeView*, char*, int);
static void change_weight_selected_item(GtkButton*, GdkEventButton*);
static int check_field(char*, tag_tv_columns*);
static void clear_list_fields();
static void clear_fields();
//search callbacks
static void search(GtkEntry*);
//list callbacks
static void open_list(GtkWidget*);
static int open_list_item(void);
//stream callbacks
static void add_stream(void);
static void play_stream(void);
static void remove_stream(void);
static void populate_stream_model(char*, char*, char*, char*, char*);
static void write_stream_config(void);

static int read_stream_config(void);
static void open_path(char*, GtkTreeIter*);
static int get_tree_path(GtkTreeModel*, GtkTreeIter*, gchar*);
static void create_dir_structure(GtkTreeStore*, GtkTreeIter*, char**, int);
static void build_dir_tree(GtkTreeStore*);
static int get_dirlist(const char*, char**);
static void get_song_list(const char*);
static void initialize_colors(void);
static int cmpstringp(const void*, const void*);
static void close_win(void);


int launch_browser(char *mdir, int flag)//if the flag is nonzero file fetching is suppressed - fixes a problem with launching the browser then highlighting a song
{
	printf("Browser opened\n");
	if (b_win != NULL)
		return 1;
	musicdir = malloc(strlen(mdir) + 1);
	memcpy(musicdir,mdir, strlen(mdir) + 1);
	strcpy(displayed_dir, musicdir);
	initialize_colors();

	b_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (b_win), "Browser");
	gtk_window_set_default_size(GTK_WINDOW (b_win), 1280, 948);
	g_signal_connect(G_OBJECT (b_win), "destroy", G_CALLBACK (close_win), NULL);
	//create the notebook and the various tabs
	nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_LEFT);

	GtkWidget *b_image = gtk_image_new_from_icon_name("emblem-music-symbolic", GTK_ICON_SIZE_MENU);
	GtkWidget *b_label = gtk_label_new("Browse");
	GtkWidget *b_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(b_vbox), b_image);
	gtk_container_add(GTK_CONTAINER(b_vbox), b_label);
	gtk_widget_show_all(b_vbox);

	GtkWidget *search_image = gtk_image_new_from_icon_name("edit-find", GTK_ICON_SIZE_MENU);
	GtkWidget *search_label = gtk_label_new("Search");
	GtkWidget *search_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(search_tab_vbox), search_image);
	gtk_container_add(GTK_CONTAINER(search_tab_vbox), search_label);
	gtk_widget_show_all(search_tab_vbox);

	GtkWidget *list_image = gtk_image_new_from_icon_name("format-justify-left", GTK_ICON_SIZE_MENU);
	GtkWidget *list_label = gtk_label_new("List");
	GtkWidget *list_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(list_tab_vbox), list_image);
	gtk_container_add(GTK_CONTAINER(list_tab_vbox), list_label);
	gtk_widget_show_all(list_tab_vbox);

	GtkWidget *stream_image = gtk_image_new_from_icon_name("network-workgroup", GTK_ICON_SIZE_MENU);
	GtkWidget *stream_label = gtk_label_new("Stream");
	GtkWidget *stream_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(stream_tab_vbox), stream_image);
	gtk_container_add(GTK_CONTAINER(stream_tab_vbox), stream_label);
	gtk_widget_show_all(stream_tab_vbox);
/*
 * BROWSER TAB
 */
	//start at the upper left with the widgets on top of the left tree view
/*	GtkWidget *b_dir_filter_but = gtk_check_button_new_with_label("tag");
	GtkWidget *b_tree_filter_combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Artist/Album");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Artist/Genre");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Artist");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Genre/Artist");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Genre/Album");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Genre");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Album/Genre");
	gtk_combo_box_text_append_text((GtkComboBoxText*) b_tree_filter_combo, "Album");
	gtk_combo_box_set_active((GtkComboBox*) b_tree_filter_combo, 0);

	GtkWidget *refresh_tree_but = gtk_button_new();
	GtkWidget *refresh_tree_image = gtk_image_new_from_stock("gtk-refresh", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image((GtkButton*) refresh_tree_but, refresh_tree_image);

	GtkWidget *b_tree_filter_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start((GtkBox*) b_tree_filter_hbox, b_dir_filter_but, FALSE, FALSE, 4);
	gtk_box_pack_start((GtkBox*) b_tree_filter_hbox, b_tree_filter_combo, FALSE, FALSE, 4);
	gtk_box_pack_start((GtkBox*) b_tree_filter_hbox, refresh_tree_but, FALSE, FALSE, 4);
*/
	//create the scrolled windows for the browser tab
	tag_col = malloc(sizeof(tag_tv_columns));
	tag_col->tv = NULL;
	tag_col->col_count = 0;
	b_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(b_tree_scroll, 230, 0);

	GtkWidget *b_file_nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos((GtkNotebook*) b_file_nb, GTK_POS_TOP);
	GtkWidget *b_file_scroll = gtk_scrolled_window_new(NULL, NULL);
	tag_col->scroll = gtk_scrolled_window_new(NULL, NULL);
	b_pl_scroll = gtk_scrolled_window_new(NULL, NULL);

	//the add-to-playlist button bar
	GtkWidget *pl_but_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	add_all_but = gtk_button_new_from_icon_name("edit-select-all", GTK_ICON_SIZE_BUTTON);

	add_sel_but = gtk_button_new();
	GtkWidget *add_sel_image = gtk_image_new_from_icon_name("go-down", GTK_ICON_SIZE_BUTTON);
	gtk_container_add((GtkContainer*) add_sel_but, add_sel_image);

	up_but_sel = gtk_button_new();
	GtkWidget *up_image = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_container_add((GtkContainer*) up_but_sel, up_image);

	down_but_sel = gtk_button_new();
	GtkWidget *down_image = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_container_add((GtkContainer*) down_but_sel, down_image);

	gtk_box_pack_start((GtkBox*) pl_but_hbox, add_all_but, FALSE, TRUE, 2);
	gtk_box_pack_start((GtkBox*) pl_but_hbox, add_sel_but, FALSE, TRUE, 2);
	gtk_box_pack_start((GtkBox*) pl_but_hbox, up_but_sel, FALSE, TRUE, 2);
	gtk_box_pack_start((GtkBox*) pl_but_hbox, down_but_sel, FALSE, TRUE, 2);

	//create the bottom bar of buttons
	GtkWidget *b_but_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	GtkWidget *clear_pl_but = gtk_button_new_from_icon_name("edit-clear", GTK_ICON_SIZE_BUTTON);

	GtkWidget *rand_but = gtk_toggle_button_new_with_label("Random");
	GtkWidget *repeat_but = gtk_toggle_button_new_with_label("Repeat");

	GtkWidget *pl_mode_check = gtk_check_button_new_with_label("pl mode");
	GtkWidget *ignore_weights_check = gtk_check_button_new_with_label("ignore weights");

	gtk_box_pack_start((GtkBox*) b_but_hbox, clear_pl_but, FALSE, FALSE, 2);
	gtk_box_pack_start((GtkBox*) b_but_hbox, rand_but, FALSE, FALSE, 2);
	gtk_box_pack_start((GtkBox*) b_but_hbox, repeat_but, FALSE, FALSE, 2);
	gtk_box_pack_start((GtkBox*) b_but_hbox, pl_mode_check, FALSE, FALSE, 2);
	gtk_box_pack_start((GtkBox*) b_but_hbox, ignore_weights_check, FALSE, FALSE, 2);

	//put it all in a table
	GtkWidget *b_table = gtk_grid_new();
	gtk_widget_set_name(b_pl_scroll, "weighty-b-pl-scroll");
	gtk_grid_attach(GTK_GRID(b_table), b_file_nb, 1, 0, 39, 1);
	gtk_grid_attach(GTK_GRID(b_table), pl_but_hbox, 1, 1, 39, 1);
	gtk_grid_attach(GTK_GRID(b_table), b_pl_scroll, 1, 2, 39, 1);
	gtk_grid_attach(GTK_GRID(b_table), b_but_hbox, 1, 3, 39, 1);
	gtk_widget_set_vexpand(b_tree_scroll, TRUE);
	gtk_widget_set_hexpand(b_pl_scroll, TRUE);
	g_object_set(b_file_nb, "expand", TRUE, NULL);
	//g_object_set(b_pl_scroll, "expand", TRUE, NULL);

	//create an adjustable split pane
	GtkWidget *b_pane = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_add1(GTK_PANED(b_pane), b_tree_scroll);
	gtk_paned_add2(GTK_PANED(b_pane), b_table);

	//create the file pane
	GtkWidget *file_image = gtk_image_new_from_icon_name("text-x-generic", GTK_ICON_SIZE_MENU);
	GtkWidget *file_label = gtk_label_new("Files");
	GtkWidget *file_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) file_vbox, file_image);
	gtk_container_add((GtkContainer*) file_vbox, file_label);
	gtk_widget_show_all(file_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) b_file_nb, b_file_scroll, file_vbox, NULL);

	GtkWidget *tag_image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_MENU);
	GtkWidget *tag_label = gtk_label_new("Tags");
	GtkWidget *tag_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) tag_vbox, tag_image);
	gtk_container_add((GtkContainer*) tag_vbox, tag_label);
	gtk_widget_show_all(tag_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) b_file_nb, tag_col->scroll, tag_vbox, NULL);

	//fill everything up with data
	create_browser_model();

	//set up the file display window
	GtkTreeModel *file_model;
	file_tv = gtk_tree_view_new();
	file_model = create_standard_model(file_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(file_tv), file_model);
	gtk_container_add(GTK_CONTAINER(b_file_scroll), file_tv);
	if (flag == 0)
		get_song_list(musicdir);

	send_command("DMF", 3);

	//set up the playlist window
	GtkTreeModel *pl_model;
	pl_tv = gtk_tree_view_new();
	pl_model = create_standard_model(pl_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(pl_tv), pl_model);
	gtk_container_add(GTK_CONTAINER(b_pl_scroll), pl_tv);
	send_command("QG", 2);//show any playlist that's still around	gtk_notebook_append_page_menu((GtkNotebook*) b_file_nb, b_file_scroll, file_vbox, NULL);ne


	//create all the signals
	g_signal_connect(tv, "cursor-changed", G_CALLBACK(open_dir), NULL);
	g_signal_connect(file_tv, "row-activated", G_CALLBACK(play_row_now), NULL);
	g_signal_connect(add_sel_but, "clicked", G_CALLBACK(add_selected), NULL);
	g_signal_connect(add_all_but, "clicked", G_CALLBACK(add_all), NULL);
	g_signal_connect(clear_pl_but, "button_press_event", G_CALLBACK(clear_queue), NULL);
	g_signal_connect(up_but_sel, "button_press_event", G_CALLBACK(change_weight_selected), NULL);
	g_signal_connect(down_but_sel, "button_press_event", G_CALLBACK(change_weight_selected), NULL);
/*
 * SEARCH TAB
 */
	search_tag_col = malloc(sizeof(tag_tv_columns));
	search_tag_col->col_count = 0;
	search_tag_col->tv = NULL;
	GtkWidget *s_file_nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos((GtkNotebook*) s_file_nb, GTK_POS_TOP);
	GtkWidget *s_file_scroll = gtk_scrolled_window_new(NULL, NULL);
	search_tag_col->scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *search_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	GtkWidget *search_file_image = gtk_image_new_from_icon_name("text-x-generic", GTK_ICON_SIZE_MENU);
	GtkWidget *search_file_label = gtk_label_new("Files");
	GtkWidget *search_file_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) search_file_vbox, search_file_image);
	gtk_container_add((GtkContainer*) search_file_vbox, search_file_label);
	gtk_widget_show_all(search_file_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) s_file_nb, s_file_scroll, search_file_vbox, NULL);

	GtkWidget *search_tag_image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_MENU);
	GtkWidget *search_tag_label = gtk_label_new("Tags");
	GtkWidget *search_tag_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) search_tag_vbox, search_tag_image);
	gtk_container_add((GtkContainer*) search_tag_vbox, search_tag_label);
	gtk_widget_show_all(search_tag_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) s_file_nb, search_tag_col->scroll, search_tag_vbox, NULL);

	GtkTreeModel *search_file_model;
	search_file_tv = gtk_tree_view_new();
	search_file_model = create_standard_model(search_file_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(search_file_tv), search_file_model);
	gtk_container_add(GTK_CONTAINER(s_file_scroll), search_file_tv);

/*	GtkListStore *search_tag_model;
	search_tag_col->tv = gtk_tree_view_new();
	search_tag_model = gtk_list_store_new(1, G_TYPE_UINT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(search_tag_col->tv), GTK_TREE_MODEL(search_tag_model));
	gtk_container_add(GTK_CONTAINER(search_tag_col->scroll), search_tag_col->tv);
	search_tag_col->col_count = 0;
*/
	send_command("DMS", 3);

	search_addall_but = gtk_button_new_from_icon_name("edit-select-all", GTK_ICON_SIZE_BUTTON);
	search_addsel_but = gtk_button_new_from_icon_name("go-down", GTK_ICON_SIZE_BUTTON);

	GtkWidget *search_frame = gtk_frame_new("Search");
	GtkWidget *search_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_add(GTK_CONTAINER(search_frame), search_hbox);

	search_fields = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "All tags");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Album");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Artist");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Genre");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Title");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Full path");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(search_fields), "Year");
	gtk_combo_box_set_active(GTK_COMBO_BOX(search_fields), 0);

	GtkWidget *search_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(search_entry), QUERY);
	gtk_editable_set_editable(GTK_EDITABLE(search_entry), TRUE);

	gtk_box_pack_start(GTK_BOX(search_hbox), search_fields, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(search_hbox), search_entry, TRUE, TRUE, 2);

	GtkWidget *search_table = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(search_table), search_frame, 0, 0, 13, 1);
	gtk_grid_attach(GTK_GRID(search_table), search_addall_but, 0, 6, 1, 1);
	gtk_grid_attach(GTK_GRID(search_table), search_addsel_but, 0, 8, 1, 1);
	gtk_grid_attach(GTK_GRID(search_table), s_file_nb, 1, 1, 12, 12);
	g_object_set(s_file_nb, "expand", TRUE, NULL);

	gtk_box_pack_start(GTK_BOX(search_vbox), search_table, TRUE, TRUE, 0);

	g_signal_connect(search_entry, "activate", G_CALLBACK(search), NULL);
	g_signal_connect(search_file_tv, "row-activated", G_CALLBACK(play_row_now), NULL);
	g_signal_connect(search_addsel_but, "clicked", G_CALLBACK(add_selected), NULL);
	g_signal_connect(search_addall_but, "clicked", G_CALLBACK(add_all), NULL);
/*
 * LIST TAB
 */
	list_tag_col = malloc(sizeof(tag_tv_columns));
	//set up the file and tag notebook first
	GtkWidget *l_file_nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos((GtkNotebook*) l_file_nb, GTK_POS_TOP);
	GtkWidget *l_file_scroll = gtk_scrolled_window_new(NULL, NULL);
	list_tag_col->scroll = gtk_scrolled_window_new(NULL, NULL);
	list_tag_col->col_count = 0;

	GtkWidget *list_file_image = gtk_image_new_from_icon_name("text-x-generic", GTK_ICON_SIZE_MENU);
	GtkWidget *list_file_label = gtk_label_new("Files");
	GtkWidget *list_file_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) list_file_vbox, list_file_image);
	gtk_container_add((GtkContainer*) list_file_vbox, list_file_label);
	gtk_widget_show_all(list_file_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) l_file_nb, l_file_scroll, list_file_vbox, NULL);

	GtkWidget *list_tag_image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_MENU);
	GtkWidget *list_tag_label = gtk_label_new("Tags");
	GtkWidget *list_tag_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add((GtkContainer*) list_tag_vbox, list_tag_image);
	gtk_container_add((GtkContainer*) list_tag_vbox, list_tag_label);
	gtk_widget_show_all(list_tag_vbox);
	gtk_notebook_append_page_menu((GtkNotebook*) l_file_nb, list_tag_col->scroll, list_tag_vbox, NULL);

	GtkTreeModel *list_file_model;
	list_file_tv = gtk_tree_view_new();
	list_file_model = create_standard_model(list_file_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list_file_tv), list_file_model);
	gtk_container_add(GTK_CONTAINER(l_file_scroll), list_file_tv);

	//FIXME why was this commented out?
	//ANSWER: I don't know but it breaks the list_tag_model for some reason
	/*GtkListStore *list_tag_model;
	list_tag_col->tv = gtk_tree_view_new();
	list_tag_model = gtk_list_store_new(1, G_TYPE_UINT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list_tag_col->tv), GTK_TREE_MODEL(list_tag_model));
	gtk_container_add(GTK_CONTAINER(list_tag_col->scroll), list_tag_col->tv);
	list_tag_col->col_count = 0;*/

	send_command("DML", 3);

	GtkWidget *list_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(list_tree_scroll, 180, 0);
	GtkWidget *list_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(list_scroll, 450, 0);

	GtkTreeModel *list_tree_model;
	list_tree_tv = create_list_tree_tv();
	list_tree_model = create_list_model(list_tree_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list_tree_tv), list_tree_model);
	gtk_container_add(GTK_CONTAINER(list_tree_scroll), list_tree_tv);

	GtkTreeModel *list_model;
	list_tv = gtk_tree_view_new();
	list_model = create_standard_model(list_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list_tv), list_model);
	gtk_container_add(GTK_CONTAINER(list_scroll), list_tv);

	list_song_up_but = gtk_button_new();
	GtkWidget *list_up_image = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(list_song_up_but), list_up_image);

	list_song_down_but = gtk_button_new();
	GtkWidget *list_down_image = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(list_song_down_but), list_down_image);

	list_item_up_but = gtk_button_new();
	GtkWidget *list_item_up_image = gtk_image_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(list_item_up_but), list_item_up_image);

	list_item_down_but = gtk_button_new();
	GtkWidget *list_item_down_image = gtk_image_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(list_item_down_but), list_item_down_image);

	GtkWidget *list_song_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(list_song_button_hbox), list_song_down_but, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(list_song_button_hbox), list_song_up_but, FALSE, TRUE, 2);

	GtkWidget *list_item_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(list_item_button_hbox), list_item_down_but, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(list_item_button_hbox), list_item_up_but, FALSE, TRUE, 2);

	GtkWidget *list_table = gtk_grid_new();
	gtk_widget_set_name(list_scroll, "weighty-list-scroll");
	gtk_grid_attach(GTK_GRID(list_table), list_tree_scroll, 0, 0, 1, 2);
	gtk_grid_attach(GTK_GRID(list_table), list_scroll, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(list_table), l_file_nb, 2, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(list_table), list_item_button_hbox, 1, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(list_table), list_song_button_hbox, 2, 1, 1, 1);
	gtk_widget_set_vexpand(list_tree_scroll, TRUE);
	gtk_widget_set_vexpand(list_scroll, TRUE);
	//gtk_widget_set_vexpand(l_file_nb, TRUE);
	//g_object_set(list_scroll, "expand", TRUE, NULL);
	g_object_set(l_file_nb, "expand", TRUE, NULL);

	//List signals
	g_signal_connect(list_tree_tv, "cursor-changed", G_CALLBACK(open_list), NULL);
	g_signal_connect(list_tv, "cursor-changed", G_CALLBACK(open_list_item), NULL);
	g_signal_connect(list_tv, "row-activated", G_CALLBACK(play_list_now), NULL);
	g_signal_connect(list_file_tv, "row-activated", G_CALLBACK(play_row_now), NULL);
	g_signal_connect(list_song_up_but, "button_press_event", G_CALLBACK(change_weight_selected), NULL);
	g_signal_connect(list_song_down_but, "button_press_event", G_CALLBACK(change_weight_selected), NULL);
	g_signal_connect(list_item_up_but, "button_press_event", G_CALLBACK(change_weight_selected_item), NULL);
	g_signal_connect(list_item_down_but, "button_press_event", G_CALLBACK(change_weight_selected_item), NULL);

/*
 * STREAM TAB
 */
	GtkWidget *stream_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

	GtkWidget *url_frame = gtk_frame_new("URL");
	url_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(url_entry), 1024);
	gtk_container_add(GTK_CONTAINER(url_frame), url_entry);

	GtkWidget *stream_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *stream_add_but = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_BUTTON);
	GtkWidget *stream_rem_but = gtk_button_new_from_icon_name("list-remove", GTK_ICON_SIZE_BUTTON);
	GtkWidget *stream_play_but = gtk_button_new_from_icon_name("media-playback-start", GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(stream_button_hbox), stream_add_but, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(stream_button_hbox), stream_rem_but, FALSE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(stream_button_hbox), stream_play_but, FALSE, TRUE, 2);

	GtkWidget *stream_scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkTreeModel *stream_model;
	stream_tv = gtk_tree_view_new();
	stream_model = create_stream_model(stream_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(stream_tv), stream_model);
	gtk_container_add(GTK_CONTAINER(stream_scroll), stream_tv);

	//pack it all up in the vbox
	gtk_box_pack_start(GTK_BOX(stream_vbox), url_frame, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(stream_vbox), stream_button_hbox, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(stream_vbox), stream_scroll, TRUE, TRUE, 2);

	//stream signals
	g_signal_connect(G_OBJECT(stream_add_but), "clicked", G_CALLBACK(add_stream), NULL);
	g_signal_connect(G_OBJECT(url_entry), "activate", G_CALLBACK(add_stream), NULL);
	g_signal_connect(G_OBJECT(stream_play_but), "clicked", G_CALLBACK(play_stream), NULL);
	g_signal_connect(G_OBJECT(stream_rem_but), "clicked", G_CALLBACK(remove_stream), NULL);
	//put it all together and show it
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), b_pane, b_vbox, NULL);
//	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), b_table, b_vbox, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), search_vbox, search_tab_vbox, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), list_table, list_tab_vbox, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), stream_vbox, stream_tab_vbox, NULL);
	gtk_container_add((GtkContainer*) b_win, nb);

	gtk_widget_show_all(b_win);

	g_object_unref(file_model);
//	g_object_unref(tag_model);
	g_object_unref(list_model);
	g_object_unref(list_file_model);
//	g_object_unref(list_tag_model);
	read_stream_config();

	printf("FINISHED BUILDING BROWSER\n");
	return 0;
}
/*
 * MODELS
 */
void create_tag_model_new(char flag, int num_fields, char **fields)
{
	printf("%c\tCREATE TAG MODEL NEW %d\n", flag, num_fields);
	tag_tv_columns *col = NULL;
	if (flag == 'F')
		col = tag_col;
	else if (flag == 'S')
		col = search_tag_col;
	else if (flag == 'L')
		col = list_tag_col;
	col->tv = gtk_tree_view_new();
	//gtk_tree_view_set_rules_hint(GTK_TREE_VIEW((col)->tv), TRUE);
	(col)->col_count = 0;

	GType type_list[num_fields * sizeof(GType)];
	type_list[0] = G_TYPE_UINT;
	int i;
	for (i = 1; i < num_fields; i++)
		type_list[i] = G_TYPE_STRING;
	GtkListStore *model;
	model = gtk_list_store_newv(num_fields, type_list);

	GtkTreeViewColumn* index_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(index_col, "#");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(col->tv), index_col, 0);
	GtkCellRenderer* index_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(index_col, index_rend, TRUE);
	gtk_tree_view_column_add_attribute(index_col, index_rend, "text", 0);
	gtk_tree_view_column_set_sort_column_id(index_col, 0);
	gtk_tree_view_column_set_sort_indicator(index_col, TRUE);

	char *basefields[] = { "Track #", "Artist", "Title", "Album", "Genre", "Year" };

	for (i = 0; i < 6; i++)
	{
		GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(tvcol, basefields[i]);
		gtk_tree_view_append_column(GTK_TREE_VIEW(col->tv), tvcol);
		GtkCellRenderer *rend = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(tvcol, rend, TRUE);
		gtk_tree_view_column_add_attribute(tvcol, rend, "text", col->col_count + 1);
		gtk_tree_view_column_set_sort_column_id(tvcol, col->col_count + 1);
		gtk_tree_view_column_set_sort_indicator(tvcol, TRUE);
		gtk_tree_view_column_set_visible(tvcol, TRUE);
		gtk_tree_view_column_set_sizing(tvcol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

		(col)->tv_cols[col->col_count] = malloc(strlen(basefields[i]) + 1);
		memcpy((col)->tv_cols[col->col_count], basefields[i], strlen(basefields[i]) + 1);
		(col)->col_count++;
	}
	for (i = 6; i < num_fields; i++)
	{
		char trfield[64];
		GError *error = NULL;
		g_locale_to_utf8(fields[i], -1, NULL, NULL, &error);
		if(error)
		{
			//printf("%s\n", error->message);
			g_error_free(error);
			continue;
		}
		else
		{
			translate_field(fields[i], trfield);
		}
		if (	(strcasecmp(trfield, "Track #") == 0) ||
				(strcasecmp(trfield, "Artist") == 0) ||
				(strcasecmp(trfield, "Title") == 0) ||
				(strcasecmp(trfield, "Album") == 0) ||
				(strcasecmp(trfield, "Genre") == 0) ||
				(strcasecmp(trfield, "Year") == 0))
			printf(" ");//to keep eclipse from complaining
		if ((strncmp(trfield, "cue_track", 9)) && (strncasecmp(trfield, "replaygain_", 10)))
		{
			GtkTreeViewColumn *tvcol = gtk_tree_view_column_new();
			gtk_tree_view_column_set_title(tvcol, trfield);
			gtk_tree_view_append_column(GTK_TREE_VIEW((col)->tv), tvcol);
			GtkCellRenderer *rend = gtk_cell_renderer_text_new();
			gtk_tree_view_column_pack_start(tvcol, rend, TRUE);
			gtk_tree_view_column_add_attribute(tvcol, rend, "text", col->col_count + 1);
			gtk_tree_view_column_set_sort_column_id(tvcol, col->col_count + 1);
			gtk_tree_view_column_set_sort_indicator(tvcol, TRUE);
			gtk_tree_view_column_set_visible(tvcol, FALSE);
			gtk_tree_view_column_set_sizing(tvcol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

			(col)->tv_cols[col->col_count] = malloc(strlen(trfield) + 1);
			memcpy((col)->tv_cols[col->col_count], trfield, strlen(trfield) + 1);
			(col)->col_count++;
		}
		free(fields[i]);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(col->tv), GTK_TREE_MODEL(model));
	gtk_container_add(GTK_CONTAINER((col)->scroll), (col)->tv);
	gtk_widget_show_all((col)->tv);
}
void create_browser_model()
{
	printf("create browser model\n");
	GtkTreeStore *ts;
	ts = gtk_tree_store_new(2, G_TYPE_STRING, GDK_TYPE_RGBA);
	build_dir_tree(ts);
	tv = gtk_tree_view_new_with_model((GtkTreeModel*)ts);
	//gtk_tree_view_set_rules_hint((GtkTreeView*) tv, TRUE);
	gtk_container_add((GtkContainer*) b_tree_scroll, tv);
	GtkTreeViewColumn *col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Dir");
	gtk_tree_view_append_column((GtkTreeView*)tv, col);
	GtkCellRenderer *rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, rend, TRUE);
	gtk_tree_view_column_add_attribute(col, rend, "text", 0);
	gtk_tree_view_set_model((GtkTreeView*)tv, (GtkTreeModel*) ts);
	g_object_unref(ts);
}
GtkTreeModel* create_list_model(GtkWidget *tv)
{
	GtkTreeStore *model;
	model = gtk_tree_store_new
	(		2,
			G_TYPE_STRING,	//full path of song, as a reference
			GDK_TYPE_RGBA //color
	);

	GtkTreeViewColumn* list_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(list_col, "List");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), list_col, 0);
	GtkCellRenderer* list_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(list_col, list_rend, TRUE);
	gtk_tree_view_column_add_attribute(list_col, list_rend, "text", 0);
	gtk_tree_view_column_add_attribute(list_col, list_rend, "foreground-rgba", 1);
	gtk_tree_view_column_set_sort_column_id(list_col, 0);
	gtk_tree_view_column_set_sort_indicator(list_col, TRUE);

	GtkTreeIter iter;
	GtkTreeIter iter2;
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Album", -1);
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Artist", -1);
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Genre", -1);
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Recent Albums", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Week", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Month", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last 3 Months", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Year", -1);
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Recent Artists", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Week", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Month", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last 3 Months", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Year", -1);
	gtk_tree_store_append(model, &iter, NULL);
	gtk_tree_store_set(model, &iter, 0, "Recent Songs", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Week", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Month", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last 3 Months", -1);
	gtk_tree_store_append(model, &iter2, &iter);
	gtk_tree_store_set(model, &iter2, 0, "Last Year", -1);

	return GTK_TREE_MODEL(model);
}
GtkTreeModel* create_stream_model(GtkWidget *tv)
{
	GtkListStore *model;
	model = gtk_list_store_new
	(		7,
			G_TYPE_STRING, 	//parsed name for the stream
			G_TYPE_STRING, //genre
			G_TYPE_STRING, 	//site url
			G_TYPE_STRING, //stream name (from metadata)
			G_TYPE_STRING,	//stream url
			G_TYPE_STRING, //bitrate
			GDK_TYPE_RGBA //color - just in case
	);

	GtkTreeViewColumn* name_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(name_col, "Name");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), name_col, 0);
	GtkCellRenderer* name_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(name_col, name_rend, TRUE);
	gtk_tree_view_column_add_attribute(name_col, name_rend, "text", 0);
	gtk_tree_view_column_add_attribute(name_col, name_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(name_col, 0);
	gtk_tree_view_column_set_sort_indicator(name_col, TRUE);

	GtkTreeViewColumn* genre_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(genre_col, "Genre");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), genre_col, 1);
	GtkCellRenderer* genre_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(genre_col, genre_rend, TRUE);
	gtk_tree_view_column_add_attribute(genre_col, genre_rend, "text", 1);
	gtk_tree_view_column_add_attribute(genre_col, genre_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(genre_col, 1);
	gtk_tree_view_column_set_sort_indicator(genre_col, TRUE);

/*	GtkTreeViewColumn* site_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(site_col, "Website");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), site_col, 3);
	GtkCellRenderer* site_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(site_col, site_rend, TRUE);
	gtk_tree_view_column_add_attribute(site_col, site_rend, "text", 3);
	gtk_tree_view_column_add_attribute(site_col, site_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(site_col, 0);
	gtk_tree_view_column_set_sort_indicator(site_col, TRUE);
*/
	GtkTreeViewColumn* stream_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(stream_col, "Description");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), stream_col, 3);
	GtkCellRenderer* stream_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(stream_col, stream_rend, TRUE);
	gtk_tree_view_column_add_attribute(stream_col, stream_rend, "text", 3);
	gtk_tree_view_column_add_attribute(stream_col, stream_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(stream_col, 3);
	gtk_tree_view_column_set_sort_indicator(stream_col, TRUE);

	GtkTreeViewColumn* url_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(url_col, "URL");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), url_col, 4);
	GtkCellRenderer* url_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(url_col, url_rend, TRUE);
	gtk_tree_view_column_add_attribute(url_col, url_rend, "text", 4);
	gtk_tree_view_column_add_attribute(url_col, url_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(url_col, 4);
	gtk_tree_view_column_set_sort_indicator(url_col, TRUE);

	GtkTreeViewColumn* bitrate_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(bitrate_col, "Bitrate");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), bitrate_col, 2);
	GtkCellRenderer* bitrate_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(bitrate_col, bitrate_rend, TRUE);
	gtk_tree_view_column_add_attribute(bitrate_col, bitrate_rend, "text", 2);
	gtk_tree_view_column_add_attribute(bitrate_col, bitrate_rend, "foreground-rgba", 6);
	gtk_tree_view_column_set_sort_column_id(bitrate_col, 2);
	gtk_tree_view_column_set_sort_indicator(bitrate_col, TRUE);

	//gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), TRUE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(model));
	return GTK_TREE_MODEL(model);
}
/*
 * TREE VIEW FUNCTIONS
 */
int set_cursor_on_playing()
{
	if (show_current)//now we have data in the list_tv column and we can check to see if we should move the cursor there.
	{
		char *match = NULL;
		if (! strcasecmp(val.playby, "song"))
			match = get_playing_album();
		else if (! strcasecmp(val.playby, "album"))
			match = get_playing_album();
		else if (! strcasecmp(val.playby, "artist"))
			match = get_playing_artist();
		else if (! strcasecmp(val.playby, "genre"))
			match = get_playing_genre();
		if (match == NULL)
			return -1;
		printf("match = %s\n", match);

		GtkTreeModel *model;
		if(list_tv == NULL)
			return -1;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_tv));
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first(model, &iter))
		{
			do
			{
				gchar *playby;
				gtk_tree_model_get(model, &iter, FILENAME, &playby, -1);
				if (! strcmp(match, playby))
					break;
				g_free(playby);
			} while (gtk_tree_model_iter_next(model, &iter));
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tv), path, NULL, FALSE);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list_tv), path, NULL, TRUE, 0.5, 0.0);
		}
		else
			printf("no data in list_tv\n");
	}
	show_current = 0;
	show_current_file = 1;

	return 0;
}
void set_cursor_on_playing_file(int flag)
{
	if (show_current_file)
	{
		char *playing_file = get_playing_file();
		GtkTreeModel *model;
		if (flag == 0 && file_tv != NULL)
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(file_tv));
		else if (flag == 2 && list_file_tv != NULL)
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_file_tv));
		else {
			printf("set_cursor_on_playing_file no model error\n");
			return;
		}
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter_first(model, &iter) && (playing_file != NULL))
		{
			do
			{
				gchar *file;
				gtk_tree_model_get(model, &iter, FULLPATH, &file, -1);
				if (! strcmp(playing_file, file))
					break;
				g_free(file);
			} while (gtk_tree_model_iter_next(model, &iter));
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			if (flag == 0)
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(file_tv), path, NULL, FALSE);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(file_tv), path, NULL, TRUE, 0.5, 0.0);
			}
			else if (flag == 2)
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_file_tv), path, NULL, FALSE);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list_file_tv), path, NULL, TRUE, 0.5, 0.0);
			}
		}
		else
			printf("no data in list_file_tv\n");
	}
	show_current_file = 0;//
}
//takes a song struct and an index and populates that row of the file tv with the data
void populate_file_tv(char *file, int weight, int sticky, int i) {	populate_tv(file_tv, file, weight, sticky, i, 0); }
void populate_pl_tv(char *file, int weight, int sticky, int i)
{
	populate_tv(pl_tv, file, weight, sticky, i, 1);
	highlight_playlist(playlist_index);
}
void populate_search_tv(char *file, int weight, int sticky, int i) { populate_tv(search_file_tv, file, weight, sticky, i, 1); }
void populate_list_tv(char *file, int weight, int sticky, int i)
{
	populate_tv(list_tv, file, weight, sticky, i, 3);
}
void populate_list_file_tv(char *file, int weight, int sticky, int i)
{
	populate_tv(list_file_tv, file, weight, sticky, i, 2);
}
void populate_tag_tv(tag_data **data, int i, int num_fields)
{
	if (tag_tv_flag == 0)
		populate_tag_tv_func_new(data, i, num_fields, tag_col);
	else if (tag_tv_flag == 1)
		populate_tag_tv_func_new(data, i, num_fields, search_tag_col);
	else if (tag_tv_flag == 2)
		populate_tag_tv_func_new(data, i, num_fields, list_tag_col);
}
void populate_tag_tv_func_new(tag_data** data, int i, int num_fields, tag_tv_columns *col)
{
	if(col->tv == NULL)
		return;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(col->tv));
	if(model == NULL)
		return;

	GtkTreeIter iter;
	gtk_list_store_insert(GTK_LIST_STORE(model), &iter, (gint) i);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, i + 1, -1);
	int field;
	for (field = 0; field < num_fields; field++)
	{
		//int colnum = check_field((*data)[field].field, list_tag_col);FIXME
		int colnum = check_field((*data)[field].field, col);
		if (colnum != -1)
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, colnum + 1, (*data)[field].tag, -1);
			gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(col->tv), colnum + 1), TRUE);
		}
		else
			printf("couldn't find column %s\n", (*data)[field].field);
	}
	gtk_widget_show_all(col->tv);
	clear_tag_data(data, num_fields);
}
int check_field(char* field, tag_tv_columns *col)
{
	int i;
	for (i = 0; i < col->col_count; i++)
		if (strcmp(col->tv_cols[i], field) == 0)
			return i;
	return -1;
}
void clear_list_fields()
{
	int i;
	for (i = 6; i < list_tag_col->col_count; i++)
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(list_tag_col->tv), i + 1), FALSE);
}
void clear_fields()
{
	int i;
	for (i = 6; i < tag_col->col_count; i++)
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tag_col->tv), i + 1), FALSE);
}
void clear_search_fields()
{
	int i;
	for (i = 6; i < search_tag_col->col_count; i++)
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(search_tag_col->tv), i + 1), FALSE);
}
/*
 * BROWSER CALLBACKS
 */
void open_dir()
{
	tag_tv_flag = 0;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_tree_selection_get_selected(sel, &model, &iter);
	gchar updir[256];
	memset(updir, 0, 256);
	get_tree_path(model, &iter, updir);
	char full_path[1024];
	memset(full_path, 0, 1024);
	memcpy(full_path, musicdir, strlen(musicdir));
	strcat(full_path, updir);
	strcat(full_path, "\0");
	bzero(displayed_dir, 1024);
	memcpy(displayed_dir, full_path, strlen(full_path) + 1);
	//clear the file_tv pane
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(file_tv));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	//clear out the tag_tv pane
	clear_fields();
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tag_col->tv));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	get_song_list((const char*) full_path);
	gtk_widget_show_all(file_tv);
}
//copies the full paths of every song on the list
//and sends them to weighty to be pushed onto the playnow list
//since the playlist plays from the head of the list
//the list should be pushed onto the stack in reverse
void play_list_now(GtkWidget *tv)
{
	GtkTreeModel *lmodel;
	GtkTreeIter liter;
	gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tree_tv)), &lmodel, &liter);
	gchar *list;
	gtk_tree_model_get(lmodel, &liter, 0, &list, -1);

	GtkTreeModel *model;
	GList *glist = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tv)), &model));
	if (glist != NULL)
	{
		char com[2048];
		memset(com, 0, 2048);
		memcpy(com, "PF", 2);//play_playlist()
		char *pcom = &com[2];
		int len = 2;
		GtkTreePath* path = glist->data;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gchar *item;
		gtk_tree_model_get(model, &iter, FILENAME, &item, -1);
//		printf("list = %s\titem = %s\n", list, item);
		if ((strcmp(list, "Album") == 0) || (strcmp(list, "Recent Albums") == 0)) {
			memcpy(pcom, "TALB", 4);
			pcom += 5;
			len += 5;
		}
		else if ((strcmp(list, "Artist") == 0) || (strcmp(list, "Recent Artists") == 0)) {
			memcpy(pcom, "TPE1", 4);
			pcom += 5;
			len += 5;
		}
		else if ((strcmp(list, "Genre") == 0) || (strcmp(list, "Recent Genres") == 0)) {
			memcpy(pcom, "TCON", 4);
			pcom += 5;
			len += 5;
		}
		else//it's a subiter of one of the "Recent" lists
		{
			GtkTreeIter topiter;
			if (gtk_tree_model_iter_parent(lmodel, &topiter, &liter))
			{
				gchar *uplist;
				gtk_tree_model_get(lmodel, &topiter, 0, &uplist, -1);
				printf("uplist = %s\n", uplist);
				if (strcmp(uplist, "Recent Albums") == 0) {
					memcpy(pcom, "TALB", 4);
					pcom += 5;
					len += 5;
				}
				else if (strcmp(uplist, "Recent Artists") == 0) {
					memcpy(pcom, "TPE1", 4);
					pcom += 5;
					len += 5;
				}
				else if (strcmp(uplist, "Recent Genres") == 0) {
					memcpy(pcom, "TCON", 4);
					pcom += 5;
					len += 5;
				}
				else
					printf("no match for |%s|\n", uplist);
			}
			else
				printf("iter has no parent\n");
		}
		memcpy(pcom, item, strlen(item) + 1);
		len += strlen(item) + 1;
		send_command(com, len);
		//clean up what's there now
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_file_tv))));
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_tag_col->tv))));
		send_command(com, strlen(item) + 4);
		g_free(item);
		g_list_free(glist);
	}
}
void add_all(GtkButton *button)
{
	if (button == GTK_BUTTON(add_sel_but) || button == GTK_BUTTON(add_all_but))
		gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(file_tv)));
	else if (button == GTK_BUTTON(search_addsel_but) || button == GTK_BUTTON(search_addall_but))
		gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(search_file_tv)));
	add_selected(button);
}
void add_selected(GtkButton *button)
{
	printf("add selected\n");
	GtkTreeModel *model;
	GList *list = NULL;
	if (button == GTK_BUTTON(add_sel_but) || button == GTK_BUTTON(add_all_but))
		list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(file_tv)), &model));
	else if (button == GTK_BUTTON(search_addsel_but) || button == GTK_BUTTON(search_addall_but))
		list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(search_file_tv)), &model));
	while (list != NULL)
	{
		GtkTreePath* path = list->data;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gchar *file;
		gtk_tree_model_get(model, &iter, FULLPATH, &file, -1);
		char com[4096];
		memset(com, '\0', 4096);
		memcpy(com, "QQ", 2);
		strcat(com, file);
		strcat(com, "\0");
		send_command(com, strlen(file) + 3);
		list = list->next;
		g_free(file);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(pl_tv))));
	send_command("QG", 2);//get queue
}
void change_weight_selected(GtkButton *button, GdkEventButton *event)
{
	int change = 0;
	if ((button == GTK_BUTTON(up_but_sel)) || (button == GTK_BUTTON(list_song_up_but)))
	{
		if (event->button == 1)
			change = 1;
		else if (event->button == 3)
			change = 10;
	}
	else if ((button == GTK_BUTTON(down_but_sel)) || (button == GTK_BUTTON(list_song_down_but)))
	{
		if (event->button == 1)
			change = -1;
		else if (event->button == 3)
			change = -10;
	}
	if (change)
	{
		GtkTreeModel *model;
		GList *list = NULL;
		if (button == GTK_BUTTON(up_but_sel) || button == GTK_BUTTON(down_but_sel))
			list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(file_tv)), &model));
		else if (button == GTK_BUTTON(list_song_up_but) || button == GTK_BUTTON(list_song_down_but))
			list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_file_tv)), &model));
		while (list != NULL)
		{
			GtkTreePath* path = list->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);
			gchar *file;
			int weight;
			gtk_tree_model_get(model, &iter, WEIGHT, &weight, FULLPATH, &file, -1);
			printf("change weight %d %s\n", change, file);
			stats.count[weight]--;
			weight += change;
			if (weight > 100)
				weight = 100;
			else if (weight < 0)
				weight = 0;
			stats.count[weight]++;
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, WEIGHT, weight, -1);
			//change the weight of the song in the main window if it matches
			char *playing_file = get_playing_file();
			if ((playing_file != NULL) && (strcmp(file, playing_file) == 0))
				change_weight_cursong(change);
			change_info_tv_weight(file, change);

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
			//print_data(com, strlen(w) + strlen(file) + 4);
			send_command(com, strlen(w) + strlen(file) + 4);
			list = list->next;
			g_free(file);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
		gtk_widget_show_all(GTK_WIDGET(file_tv));
	}
	redraw_canvas();
}
//just a wrapper to check for all the tvs in case they happen to be displaying the song whose weight is being changed
void change_browser_tv_weight(char* file, int change)
{
	change_tv_weight(GTK_TREE_VIEW(file_tv), file, change);
	change_tv_weight(GTK_TREE_VIEW(pl_tv), file, change);
	change_tv_weight(GTK_TREE_VIEW(search_file_tv), file, change);
	change_tv_weight(GTK_TREE_VIEW(list_file_tv), file, change);
}
void change_tv_weight(GtkTreeView *tv, char *file, int change)
{
	if (b_win != NULL)
	{
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
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
					stats.count[weight]--;
					weight += change;
					if (weight > 100)
						weight = 100;
					else if (weight < 0)
						weight = 0;
					stats.count[weight]++;
					gtk_list_store_set(GTK_LIST_STORE(model), &iter, WEIGHT, weight, -1);
					break;
				}
				g_free(filepath);
			} while (gtk_tree_model_iter_next(model, &iter));
			gtk_widget_show_all(GTK_WIDGET(tv));
		}
	}
	redraw_canvas();
}
void change_weight_selected_item(GtkButton *button, GdkEventButton *event)
{
	int change = 0;
	if (button == GTK_BUTTON(list_item_up_but))
	{
		if (event->button == 1)
			change = 1;
		else if (event->button == 3)
			change = 10;
	}
	else if (button == GTK_BUTTON(list_item_down_but))
	{
		if (event->button == 1)
			change = -1;
		else if (event->button == 3)
			change = -10;
	}
	if (change)
	{
		//first get the type of list we're looking at
		GtkTreeModel *lmodel;
		GtkTreeIter liter;
		gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tree_tv)), &lmodel, &liter);
		gchar *listtype;
		gtk_tree_model_get(lmodel, &liter, 0, &listtype, -1);
		if (! strncasecmp(listtype, "Last", 4)) {
			gtk_tree_model_get_iter_first(lmodel, &liter);
			gtk_tree_model_get(lmodel, &liter, 0, &listtype, -1);
		}

		GtkTreeModel *model;
		GList *list;
		list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tv)), &model));
		while (list != NULL)
		{
			GtkTreePath* path = list->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);
			gchar *item;
			int weight;
			gtk_tree_model_get(model, &iter, WEIGHT, &weight, FILENAME, &item, -1);
			weight += change;
			if (weight > 100)
				weight = 100;
			else if (weight < 0)
				weight = 0;
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, WEIGHT, weight, -1);

			char com[4096];
			memset(com, '\0', 4096);
			if ((strcmp(listtype, "Album") == 0) || (strcmp(listtype, "Recent Albums") == 0))
				memcpy(com, "SIA", 3);
			else if ((strcmp(listtype, "Artist") == 0) || (strcmp(listtype, "Recent Artists") == 0))
				memcpy(com, "SIT", 3);
			else if (strcmp(listtype, "Genre") == 0)
				memcpy(com, "SIG", 3);
			char *pcom = &com[3];
			char w[4];
			memset(w, 0, 4);
			itoa(weight, w);
			memcpy(pcom, w, strlen(w) + 1);
			pcom += strlen(w) + 1;
			memcpy(pcom, item, strlen(item) + 1);
			send_command(com, strlen(w) + strlen(item) + 5);
			list = list->next;
			g_free(item);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);
		gtk_widget_show_all(GTK_WIDGET(file_tv));
	}
}
int highlight_playlist(int index)
{
	//printf("highlight %d\n", index);
	playlist_index = index;
	if (index < 0)
		return 1;
	else if (b_win == NULL)
		return 2;
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl_tv));
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(model, &iter);
	int i;
	do
	{
		gtk_tree_model_get(model, &iter, INDEX, &i, -1);
		if (i == index)
		{
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLOR, &color_red, -1);
			GtkTreePath *path;
			path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl_tv), path, NULL, TRUE, 0.5, 0.0);
		}
		else
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLOR, &color_gray, -1);
	} while (gtk_tree_model_iter_next(model, &iter));
	gtk_widget_show_all(GTK_WIDGET(pl_tv));
	return 0;
}
void clear_queue(GtkButton *button, GdkEventButton *event)
{
	if (event->button == 3)//clear the entire queue
	{
		send_command("QC", 2);
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl_tv));
		gtk_list_store_clear(GTK_LIST_STORE(model));
	}
	else if (event->button == 1)
	{
		GtkTreeModel *model;
		GList *list = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl_tv)), &model));
		int count = 0;
		char clear[32767*4];//average 4 chars for a very long list?
		char *pclear = &clear[0];
		int len = 0;
		while (list != NULL)
		{
			GtkTreePath* path = list->data;
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);
			int index;
			gtk_tree_model_get(model, &iter, INDEX, &index, -1);
			char i[6];
			memset(i, 0, 6);
			itoa((index - 1), i);
			memcpy(pclear, i, strlen(i) + 1);
			printf("%s\n", pclear);
			pclear += strlen(i) + 1;
			len += strlen(i) + 1;
			count++;
			list = list->next;
		}
		char com[32767*4];
		memcpy(com, "QN", 2);
		char count_s[10];
		itoa(count, count_s);
		char *pcom = &com[2];
		memcpy(pcom, count_s, strlen(count_s));
		pcom += strlen(count_s);
		memcpy(pcom++, "\0", 1);
		memcpy(pcom, clear, len);
		//print_data(com, len);
		send_command(com, len + strlen(count_s) + 3);
		gtk_list_store_clear(GTK_LIST_STORE(model));
	}
}
void get_song_list(const char* dir)
{
	char s[4096];
	bzero(s, 4096);
	memcpy(s, "DL", 2);
	strcat(s, dir);
	strcat(s, "\0");
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(file_tv))));
	send_command(s, strlen(dir) + 3);
}

/*
 * SEARCH CALLBACKS
 */
void search(GtkEntry *entry)
{
	tag_tv_flag = 1;
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(search_file_tv));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(search_tag_col->tv));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	const gchar *query;
	query = gtk_entry_get_text(GTK_ENTRY(entry));
	const gchar *field;
	field = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(search_fields));
	//printf("field = %s\tsearch = %s\n", field, query);
	char com[1100];
	char *pcom = &com[0];
	memcpy(pcom, "DQ", 2);
	pcom += 2;
	memcpy(pcom, field, strlen(field) + 1);
	pcom += strlen(field) + 1;
	memcpy(pcom, query, strlen(query) + 1);
	pcom += strlen(query) + 1;
	send_command(com, strlen(field) + strlen(query) + 4);
}
/*
 * LIST CALLBACKS
 */
//callback when a list element in the left pane is clicked
void open_list(GtkWidget *tv)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tv)), &model, &iter);
	gchar *list;
	gtk_tree_model_get(model, &iter, 0, &list, -1);
	printf("List = %s\n", list);
	if (strcmp(list, "Album") == 0)
		send_command("DFA", 3);
	else if (strcmp(list, "Artist") == 0)
		send_command("DFT", 3);
	else if (strcmp(list, "Genre") == 0)
		send_command("DFG", 3);
	else if (strcmp(list, "Recent Albums") == 0)
		send_command("DRA86400\0", 9);//one day
	else if (strcmp(list, "Recent Artists") == 0)
		send_command("DRT86400\0", 9);//one day
	else if (strcmp(list, "Recent Songs") == 0)
		send_command("DRS86400\0", 9);//one day
	else if (strcmp(list, "Last Week") == 0)
	{
		GtkTreeIter upiter;
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_up(path);
		gtk_tree_model_get_iter(model, &upiter, path);
		gchar *uplist;
		gtk_tree_model_get(model, &upiter, 0, &uplist, -1);
		if (strcmp(uplist, "Recent Albums") == 0)
			send_command("DRA604800\0", 10);//1 week = 3600*24*7 = 604800
		else if (strcmp(uplist, "Recent Artists") == 0)
			send_command("DRT604800\0", 10);//1 week = 3600*24*7 = 604800
		else if (strcmp(uplist, "Recent Songs") == 0)
			send_command("DRS604800\0", 10);//1 week = 3600*24*7 = 604800
	}
	else if (strcmp(list, "Last Month") == 0)
	{
		GtkTreeIter upiter;
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_up(path);
		gtk_tree_model_get_iter(model, &upiter, path);
		gchar *uplist;
		gtk_tree_model_get(model, &upiter, 0, &uplist, -1);
		if (strcmp(uplist, "Recent Albums") == 0)
			send_command("DRA2592000\0", 11);//1 month = 3600*24*30 = 2592000
		else if (strcmp(uplist, "Recent Artists") == 0)
			send_command("DRT2592000\0", 11);//1 month = 3600*24*30 = 2592000
		else if (strcmp(uplist, "Recent Songs") == 0)
			send_command("DRS2592000\0", 11);//1 month = 3600*24*30 = 2592000
	}
	else if (strcmp(list, "Last 3 Months") == 0)
	{
		GtkTreeIter upiter;
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_up(path);
		gtk_tree_model_get_iter(model, &upiter, path);
		gchar *uplist;
		gtk_tree_model_get(model, &upiter, 0, &uplist, -1);
		if (strcmp(uplist, "Recent Albums") == 0)
			send_command("DRA7862400\0", 11);//1 month = 3600*24*91 = 7862400
		else if (strcmp(uplist, "Recent Artists") == 0)
			send_command("DRT7862400\0", 11);//1 month = 3600*24*91 = 7862400
		else if (strcmp(uplist, "Recent Songs") == 0)
			send_command("DRS7862400\0", 11);//1 month = 3600*24*91 = 7862400
	}
	else if (strcmp(list, "Last Year") == 0)
	{
		GtkTreeIter upiter;
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_up(path);
		gtk_tree_model_get_iter(model, &upiter, path);
		gchar *uplist;
		gtk_tree_model_get(model, &upiter, 0, &uplist, -1);
		if (strcmp(uplist, "Recent Albums") == 0)
			send_command("DRA31557600\0", 12);//1 Year = 3600*24*365.25 = 31557600
		else if (strcmp(uplist, "Recent Artists") == 0)
			send_command("DRT31557600\0", 12);//1 Year = 3600*24*365.25 = 31557600
		else if (strcmp(uplist, "Recent Songs") == 0)
			send_command("DRS31557600\0", 12);//1 Year = 3600*24*365.25 = 31557600
	}
	gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(list_tv), 3), list);
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_file_tv))));
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_tv))));
	//gtk_tree_view_set_model(GTK_TREE_VIEW(list_tv), NULL);
	//gtk_tree_view_set_model(GTK_TREE_VIEW(list_tv), create_standard_model(list_tv));
	if (list_tag_col != NULL)
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_tag_col->tv))));

	memset(displayed_list, 0, 1024);
}
int open_list_item()
{
	//first get the type of list we're looking at
	tag_tv_flag = 2;
	clear_list_fields();
	GtkTreeModel *lmodel;
	GtkTreeIter liter;
	gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tree_tv)), &lmodel, &liter);
	gchar *list;
	gtk_tree_model_get(lmodel, &liter, 0, &list, -1);

	GtkTreeModel *model;
	GList *glist = g_list_first(gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tv)), &model));
	if (glist != NULL)
	{
		GtkTreePath* path = glist->data;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gchar *item;
		gtk_tree_model_get(model, &iter, FILENAME, &item, -1);
		char com[1024];
		printf("list = %s\n", list);
		if ((strcmp(list, "Album") == 0) || (strcmp(list, "Recent Albums") == 0))
			memcpy(com, "DTA", 3);
		else if ((strcmp(list, "Artist") == 0) || (strcmp(list, "Recent Artists") == 0))
			memcpy(com, "DTT", 3);
		else if ((strcmp(list, "Genre") == 0) || (strcmp(list, "Recent Genres") == 0))
			memcpy(com, "DTG", 3);
		else//it's a subiter of one of the "Recent" lists
		{
			GtkTreeIter topiter;
			if (gtk_tree_model_iter_parent(lmodel, &topiter, &liter))
			{
				gchar *uplist;
				gtk_tree_model_get(lmodel, &topiter, 0, &uplist, -1);
				printf("uplist = %s\n", uplist);
				if (strcmp(uplist, "Recent Albums") == 0)
					memcpy(com, "DTA", 3);
				else if (strcmp(uplist, "Recent Artists") == 0)
					memcpy(com, "DTT", 3);
				else if (strcmp(uplist, "Recent Genres") == 0)
					memcpy(com, "DTG", 3);
				else
					printf("no match for |%s|\n", uplist);
			}
			else
				printf("iter has no parent\n");
		}
		memcpy(&com[3], item, strlen(item) + 1);
		if (! strcmp(item, displayed_list))
			return 1;
		else
			memcpy(displayed_list, item, strlen(item) + 1);
		//clean up what's there now
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_file_tv))));
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list_tag_col->tv))));
		send_command(com, strlen(item) + 4);
		g_free(item);
		g_list_free(glist);
	}
	return 0;
}
/*
 * STREAM CALLBACKS
 */
void add_stream()
{
	const gchar *url = gtk_entry_get_text(GTK_ENTRY(url_entry));
	char com[strlen(url) + 3];
	memset(com, 0, strlen(url) + 3);
	memcpy(com, "MA", 2);
	memcpy(&com[2], url, strlen(url) + 1);
	send_command(com, strlen(url) + 3);
}
void play_stream()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stream_tv)), &model, &iter);
	gchar *url;
	gtk_tree_model_get(model, &iter, STREAM_URL, &url, -1);
	char com[strlen(url) + 3];
	memcpy(com, "MP", 2);
	memcpy(&com[2], url, strlen(url) + 1);
	send_command(com, strlen(url) + 3);
//	set_playby("");
}
void remove_stream()
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stream_tv)), &model, &iter);
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	write_stream_config();
	gtk_widget_show_now(stream_tv);
}
void add_stream_data(char *name, char *genre, char *desc, char *bitrate)
{
	const gchar *url = gtk_entry_get_text(GTK_ENTRY(url_entry));
	if(url == NULL)
		return;
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(stream_tv));
	if (model == NULL)
		printf("NULL model wtf\n");
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			STREAM_NAME, name,
			STREAM_GENRE, genre,
			STREAM_BITRATE, bitrate,
			STREAM_DESCRIPTION, desc,
			STREAM_URL, url, -1);
	gtk_widget_show_all(stream_tv);
	printf("NAME = %s\nGENRE = %s\nDESC = %s\nbitrate = %s\n", name, genre, desc, bitrate);
	write_stream_config();
}
void populate_stream_model(char *name, char *genre, char *bitrate, char *desc, char *url)
{
	if (genre == NULL)
	{
		genre = malloc(2);
		memcpy(genre, " \0", 2);
	}
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(stream_tv));
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			STREAM_NAME, name,
			STREAM_GENRE, genre,
			STREAM_BITRATE, bitrate,
			STREAM_DESCRIPTION, desc,
			STREAM_URL, url, -1);
	gtk_widget_show_all(stream_tv);
}
void write_stream_config()
{
	char *streamconfig = NULL;
	char *homedir = getenv("HOME");
	streamconfig = malloc(strlen(homedir) + 15);
	memset(streamconfig, 0, strlen(homedir) + 15);
	memcpy(streamconfig, homedir, strlen(homedir));
	strcat(streamconfig, "/stream.config");
	FILE *fp;
	if ((fp = fopen(streamconfig, "w")) == NULL)
		printf("can't open config file for writing here: %s\n", streamconfig);
	else
	{
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stream_tv));
		GtkTreeIter iter;
		gtk_tree_model_get_iter_first(model, &iter);
		gchar *name, *genre, *bitrate, *desc, *url;
		do
		{
			gtk_tree_model_get(model, &iter, 0, &name, 1, &genre, 2, &bitrate, 3, &desc, 4, &url, -1);
			fprintf(fp, "name=%s\n", name);
			fprintf(fp, "genre=%s\n", genre);
			fprintf(fp, "bitrate=%s\n", bitrate);
			fprintf(fp, "description=%s\n", desc);
			fprintf(fp, "url=%s\n", url);
		} while (gtk_tree_model_iter_next(model, &iter));
		g_free(name);
		g_free(genre);
		g_free(bitrate);
		g_free(desc);
		g_free(url);
	}
	fclose(fp);
}
/*
 * UTILITIES
 */
int read_stream_config()
{
	char *streamconfig = NULL;
	char *homedir = NULL;
	homedir = getenv("HOME");
	printf("homedir %s\n", homedir);
	streamconfig = malloc(strlen(homedir) + 15);
	memset(streamconfig, 0, strlen(homedir) + 15);
	memcpy(streamconfig, homedir, strlen(homedir));
	strcat(streamconfig, "/stream.config");
	printf("streamconfig %s\n", streamconfig);
	FILE *fp;
	if ((fp = fopen(streamconfig, "r")) == NULL)
		printf("config file doesn't exist\n");
	else
	{
		char *line = NULL;
		line = malloc(1024);
		memset(line, 0, 1024);
		size_t len = 1024;
		ssize_t read;
		int set = 0;
		char *name, *genre, *bitrate, *desc, *url;
		while ((read = getline(&line, &len, fp)) != -1)
		{
			char *p = strrchr(line, '\n');
			*p = '\0';
			if (! strncmp(line, "name=", 5))
			{
				line += 5;
				name = malloc(strlen(line) + 1);
				memcpy(name, line, strlen(line) + 1);
			}
			else if (! strncmp(line, "genre=", 6))
			{
				line += 6;
				if (*line == '\0')
					genre = NULL;
				else
				{
					genre = malloc(strlen(line) + 1);
					memcpy(genre, line, strlen(line) + 1);
				}
			}
			else if (! strncmp(line, "bitrate=", 8))
			{
				line += 8;
				bitrate = malloc(strlen(line) + 1);
				memcpy(bitrate, line, strlen(line) + 1);
			}
			else if (! strncmp(line, "description=", 12))
			{
				line += 12;
				desc = malloc(strlen(line) + 1);
				memcpy(desc, line, strlen(line) + 1);
			}
			else if (! strncmp(line, "url=", 4))
			{
				line += 4;
				url = malloc(strlen(line) + 1);
				memcpy(url, line, strlen(line) + 1);
				set = 1;
			}
			if (set)
			{
//				printf("name = %s\ngenre = %s\nbitrate = %s\ndesc = %s\nurl = %s\n", name, genre, bitrate, desc, url);
				populate_stream_model(name, genre, bitrate, desc, url);
				if (name != NULL)
					free(name);
				name = NULL;
				if (genre != NULL)
					free(genre);
				genre = NULL;
				if (bitrate != NULL)
					free(bitrate);
				bitrate = NULL;
				if (desc != NULL)
					free(desc);
				desc = NULL;
				if (url != NULL)
					free(url);
				url = NULL;
				set = 0;
			}
		}
	}
	fclose(fp);
	printf("Done reading\n");
	return 0;
}
void find_in_list(char *dir)
{
	if (b_win == NULL)
		launch_browser(dir, 1);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(nb), 2);
	if (! strcasecmp(val.playby, "song"))
	{
		GtkTreePath *path;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(list_tree_tv), &path, NULL);
		if (path != NULL)
		{
			show_current = 1;
			if (strcmp(gtk_tree_path_to_string(path), "0"))//if they're different
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("0"), NULL, FALSE);
				show_current = 1;
			}
			else
			{
				show_current = 1;
				set_cursor_on_playing();
			}
		}
		else
		{
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("0"), NULL, FALSE);
			show_current = 1;
		}
	}
	else if (! strcasecmp(val.playby, "album"))
	{
		GtkTreePath *path;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(list_tree_tv), &path, NULL);
		if (path != NULL)
		{
			if (strcmp(gtk_tree_path_to_string(path), "0"))//if they're different
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("0"), NULL, FALSE);
				show_current = 1;
			}
			else
			{
				show_current = 1;
				set_cursor_on_playing();
			}
		}
		else
		{
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("0"), NULL, FALSE);
			show_current = 1;
		}
	}
	else if (! strcasecmp(val.playby, "artist"))
	{
		printf("playby artist\n");
		GtkTreePath *path;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(list_tree_tv), &path, NULL);
		printf("got cursor\n");
		if (path != NULL)
		{
			if (strcmp(gtk_tree_path_to_string(path), "1"))//if they're different
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("1"), NULL, FALSE);
				printf("set cursor\n");
				show_current = 1;
			}
			else
			{
				show_current = 1;
				printf("set cursor on playing\n");
				set_cursor_on_playing();
			}
		}
		else
		{
			printf("null path\n");
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("1"), NULL, FALSE);
			show_current = 1;
		}
	}
	else if (! strcasecmp(val.playby, "genre"))
	{
		GtkTreePath *path;
		gtk_tree_view_get_cursor(GTK_TREE_VIEW(list_tree_tv), &path, NULL);
		if (path != NULL)
		{
			if (strcmp(gtk_tree_path_to_string(path), "2"))//if they're different
			{
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("2"), NULL, FALSE);
				show_current = 1;
			}
			else
			{
				show_current = 1;
				set_cursor_on_playing();
			}
		}
		else
		{
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(list_tree_tv), gtk_tree_path_new_from_string("2"), NULL, FALSE);
			show_current = 1;
		}

	}
}
void find_in_dir(char *dir)
{
	if (b_win == NULL)
		launch_browser(dir, 1);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(nb), 0);

	char *playing_file = get_playing_file();
	if (playing_file != NULL)
	{
		char *path = malloc(strlen(playing_file) + 1);
		memcpy(path, playing_file, strlen(playing_file) + 1);
		char *temp = strrchr(dir, '/');
		path += (strlen(dir) - strlen(temp));//strip off the beginning path but leave the last dir
		path++;//the leading /
		char *p = strrchr(path, '/');
		*p = '\0';//terminate the string at the end of the last dir
		GtkTreeIter iter;
		GtkTreeModel *model;
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
		if (gtk_tree_model_get_iter_first(model, &iter))
			open_path(path, &iter);
		else
			printf("no data in tv\n");
		show_current_file = 1;
	}
}
void open_path(char *path, GtkTreeIter *iter)//just a directory name, not a full path
{
	char dir[256];
	int i = 0;
	while ((*path != '/') && (*path != '\0'))
		dir[i++] = *path++;
	dir[i++] = '\0';
	GtkTreeModel *model;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tv));
	do
	{
		gchar *item;
		gtk_tree_model_get(model, iter, 0, &item, -1);
		if (! strcmp(dir, item))
			break;
		g_free(item);
	} while (gtk_tree_model_iter_next(model, iter));

	if (*path != '\0')
	{
		if (gtk_tree_model_iter_has_child(model, iter))
		{
			GtkTreeIter iter2;
			gtk_tree_model_iter_children(model, &iter2, iter);
			open_path(++path, &iter2);
		}
	}
	else
	{
		GtkTreePath *tp = gtk_tree_model_get_path(model, iter);
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tv), tp);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), tp, NULL, FALSE);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tv), tp, NULL, TRUE, 0.5, 0.0);
	}
}
int get_tree_path(GtkTreeModel *model, GtkTreeIter *iter, gchar *updir)
{
	gchar *dir;
	gtk_tree_model_get(model, iter, 0, &dir, -1);

	GtkTreeIter iter2;
	if (gtk_tree_model_iter_parent(model, &iter2, iter))
	{
		gchar temp[1024];
		memset(temp, '\0', 1024);
		memcpy(temp, updir, strlen(updir));
		memset(updir , '\0', strlen(updir));
		strcat(updir, "/");
		strcat(updir, dir);
		strcat(updir, temp);
		get_tree_path(model, &iter2, updir);
	}
	else
		strcat(updir, "\0");
	return 0;
}
void build_dir_tree(GtkTreeStore *ts)
{
	//get a list of all unique paths
	char **dirlist;
	dirlist = malloc(30000 * sizeof(char*));
	static_i = get_dirlist(musicdir, dirlist);
	qsort(dirlist, static_i, sizeof(char*), cmpstringp);
	static_i = 0;//reset our counter - bad design probably
	GtkTreeIter iter;
	create_dir_structure(ts, &iter, dirlist, 0);
	free(dirlist);
}
int get_dirlist(const char *dir, char **list)
{
	struct stat sbuf;
	static int i;
	//if we're starting over then reset i = 0
	if(strcmp(musicdir, dir) == 0)
		i = 0;
	if (stat(dir, &sbuf) == -1)
		printf("lstat failed on %s\n", dir);
	/* If it's a directory increment dircount and open the directory.
	 * If it succeeds then read everything in the directory and recursively call get_dirlist on it.
	 * If it fails then print an error message.
	 */
	if (S_ISDIR(sbuf.st_mode))
	{
		dircount++;
		DIR *ddir;
		struct dirent *pdir;
		ddir = opendir(dir);
		if(ddir != NULL)
		{
			list[i] = malloc(strlen(dir) + 1);
			memcpy(list[i], dir, strlen(dir) + 1);
			i++;

			while ((pdir = readdir(ddir)) != NULL)
			{
				if (*(pdir->d_name) != '.')
				{
					char s[strlen(dir) + strlen(pdir->d_name) + 2];//1 for the / and one for the \0
					strcpy(s, dir);
					strcat(s, "/");
					strcat(s, pdir->d_name);
					s[strlen(dir) + strlen(pdir->d_name) + 1] = '\0';
					if(i >= 6860)
						print_data(s, strlen(dir) + strlen(pdir->d_name) + 2);
					get_dirlist(s, list);
				}
			}
			closedir(ddir);
		}
		else {
			printf("Open dir failed %s\n", dir);
			dircount--;
			return 1;
		}
	}

	return i;
}
//iter2 is the child and iter is the parent
void create_dir_structure(GtkTreeStore *ts, GtkTreeIter *iter, char **list, int slashes)
{
	if (static_i == 0)//first time through, reusing our static counter
	{
		char *p = list[static_i];
		p = strrchr(p, '/');
		p++;
		gtk_tree_store_append(ts, iter, NULL);
		gtk_tree_store_set(ts, iter, 0, p, 1, color_black.pixel, -1);
		if (list[static_i] != NULL)
			free(list[static_i]);
		static_i++;
		create_dir_structure(ts, iter, list, 0);
	}
	else
	{
		int c = 0;
		char *p = list[static_i];
		if(p == NULL)
			return;
		//printf("%s\n", p);
		char *s = strrchr(p, '/');
		s++;
		while (*p++ != '\0')
		{
			if (*p == '/')
				c++;
		}
		if (c == slashes)//add to the same level
		{
			GtkTreePath *path = gtk_tree_model_get_path((GtkTreeModel*)ts, iter);
			if (gtk_tree_path_up(path))
				gtk_tree_model_get_iter((GtkTreeModel*)ts, iter, path);
			else
				iter = NULL;
		}
		else if (c < slashes)
		{
			int dif = slashes - c;
			while (dif-- != -1)
			{
				GtkTreePath *path = gtk_tree_model_get_path((GtkTreeModel*)ts, iter);
				if (gtk_tree_path_up(path))
					gtk_tree_model_get_iter((GtkTreeModel*)ts, iter, path);
				else
					iter = NULL;
			}
		}
		GtkTreeIter iter2;
		gtk_tree_store_append(ts, &iter2, iter);
		gtk_tree_store_set(ts, &iter2, 0, s, 1, color_black.pixel, -1);
		if (list[static_i] != NULL)
			free(list[static_i]);
		if (++static_i < dircount)
			create_dir_structure(ts, &iter2, list, c);
	}
}
void initialize_colors()
{
	//FIXME
	/*gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_black, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_white, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_red, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_blue, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_green, TRUE, TRUE);
	gdk_colormap_alloc_color(gdk_colormap_get_system (), &color_gray, TRUE, TRUE);
	*/
}
int cmpstringp(const void *p1, const void *p2)
{
	const char *s1 = * (char * const *) p1;
	const char *s2 = * (char * const *) p2;
	unsigned char c1, c2;
	do
	{
	   c1 = (unsigned char) *s1++;
	   c2 = (unsigned char) *s2++;
	   if (c1 == '\0')
	     return c1 - c2;
	}
	while (c1 == c2);

	/* since I'm just using this for qsort for a list of directories
	 * this fixes the problem where "foo/bar" is sorted after "foo foo/baz"
	 * otherwise subdirs show up under the dir with the space if the first word is exactly the same
	 */
	if((c1 == '/' && c2 == ' ') || (c1 == ' ' && c2 == '/'))
		return c2 - c1;
    return c1 - c2;
}
void close_win()
{
	static_i = 0;
	dircount = 0;
	gtk_widget_destroy(nb);
	gtk_widget_destroy(b_win);
	free(musicdir);
	b_win = NULL;
}
