/*
 * history.c
 *
 *  Created on: Jun 30, 2011
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

#include "stats.h"
#include "stats_actions.h"
#include "models.h"
#include "myutils.h"
#include "config.h"

static GtkWidget *nb, *da;
static GtkWidget *hist_tv, *update_tv;

static void update(void);
static void stats_expose_event(GtkWidget*);
static GtkTreeModel* create_update_model(GtkWidget*);

int launch_stats(void)
{
	printf("Stats opened\n");

	GtkWidget *stats_win = create_stats_win();
	if (stats_win == NULL)
		return 1;
	send_command("DS", 2);
	//create the notebook and the various tabs
	nb = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_TOP);
	gtk_container_add(GTK_CONTAINER(stats_win), nb);

	//set up the first tab, Stats
	GtkWidget *stats_tab_image = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_MENU);
	GtkWidget *stats_tab_label = gtk_label_new("Stats");
	GtkWidget *stats_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(stats_vbox_tab), stats_tab_image);
	gtk_container_add(GTK_CONTAINER(stats_vbox_tab), stats_tab_label);
	gtk_widget_show_all(stats_vbox_tab);

	GtkWidget *stats_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(GTK_WIDGET(da), 707, 700);
	g_signal_connect(da, "draw", G_CALLBACK(stats_expose_event), NULL);

	GtkWidget *stats_label_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	GtkWidget *stats_label = create_stats_label();
	gtk_box_pack_start(GTK_BOX(stats_label_vbox), stats_label, FALSE, FALSE, 1);

	GtkWidget *stats_close_but = gtk_button_new();
	GtkWidget *stats_close_img = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(stats_close_but), stats_close_img);
	g_signal_connect(stats_close_but, "clicked", G_CALLBACK(close_stats_win), NULL);
	gtk_box_pack_end(GTK_BOX(stats_label_vbox), stats_close_but, FALSE, FALSE, 1);

	gtk_box_pack_start(GTK_BOX(stats_hbox), da, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(stats_hbox), stats_label_vbox, FALSE, FALSE, 5);

	//set up the second tab, history
	GtkWidget *hist_tab_image = gtk_image_new_from_icon_name("edit-undo", GTK_ICON_SIZE_MENU);
	GtkWidget *hist_tab_label = gtk_label_new("History");
	GtkWidget *hist_vbox_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(hist_vbox_tab), hist_tab_image);
	gtk_container_add(GTK_CONTAINER(hist_vbox_tab), hist_tab_label);
	gtk_widget_show_all(hist_vbox_tab);

	GtkTreeModel *hist_model;
	hist_tv = gtk_tree_view_new();
	hist_model = create_standard_model(hist_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(hist_tv), hist_model);
	GtkWidget *hist_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(hist_scroll), hist_tv);


	//set up the third tab, update
	GtkWidget *update_tab_image = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_MENU);
	GtkWidget *update_tab_label = gtk_label_new("Update");
	GtkWidget *update_tab_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(update_tab_vbox), update_tab_image);
	gtk_container_add(GTK_CONTAINER(update_tab_vbox), update_tab_label);
	gtk_widget_show_all(update_tab_vbox);

	GtkWidget *update_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);//the main box for the tab

	GtkWidget *update_scroll = gtk_scrolled_window_new(NULL, NULL);
	GtkWidget *update_label = gtk_label_new("");
	gtk_widget_set_size_request(update_label, 200, 600);
	gtk_label_set_line_wrap(GTK_LABEL(update_label), TRUE);

	GtkWidget *update_results_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(update_results_hbox), update_scroll, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(update_results_hbox), update_label, FALSE, TRUE, 0);

	GtkWidget *update_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *update_button = gtk_button_new();
	GtkWidget *update_image = gtk_image_new_from_icon_name("view-refresh", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(update_button), update_image);
	g_signal_connect(update_button, "clicked", G_CALLBACK(update), NULL);
	gtk_box_pack_start(GTK_BOX(update_button_hbox), update_button, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(update_vbox), update_button_hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(update_vbox), update_results_hbox, TRUE, TRUE, 0);

	GtkTreeModel *update_model;
	update_tv = gtk_tree_view_new();
	update_model = create_update_model(update_tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(update_tv), update_model);
	gtk_container_add(GTK_CONTAINER(update_scroll), update_tv);

	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), stats_hbox, stats_vbox_tab, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), hist_scroll, hist_vbox_tab, NULL);
	gtk_notebook_append_page_menu(GTK_NOTEBOOK(nb), update_vbox, update_tab_vbox, NULL);

	update_stats_label();
	send_command("DH", 2);
	gtk_widget_show_all(stats_win);
	return 0;
}
//send command to update database
void update()
{
	GtkTreeModel *model;
	if(update_tv == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(update_tv));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	send_command("DU", 2);
}
void stats_expose_event (GtkWidget *da)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(da));
	//GdkDrawingContext context = gdk_window_begin_draw_frame(gtk_widget_get_window(da), gdk_get_visible_region(gtk_widget_get_window(da)));
	//cairo_t *cr = gdk_drawing_context_get_cairo_context(context);
	GtkAllocation al;
	gtk_widget_get_allocation(da, &al);
	int w = al.width;
	int h = al.height;
	cairo_set_line_width(cr, 0.5);
	cairo_rectangle(cr, 0, 0, w, h);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_fill(cr);
	if (stats.max != 0)
	{
		int i;
		float scale = ((float)h) / stats.max;
		int pixw = (int) (w / 101);
		for (i = 0; i <= 100; i++)
		{
			if (i%2)
				cairo_set_source_rgba(cr, 1, 0, 0, 0.9);
			else if (i % 10)
				cairo_set_source_rgba(cr, 1, 0, 0, 0.8);
			else
				cairo_set_source_rgba(cr, 0, 1, 0, 0.8);
			cairo_rectangle(cr, i * pixw, h - scale * stats.count[i], pixw, scale * stats.count[i]);
			cairo_fill(cr);
		}
	}
	cairo_destroy(cr);
}
void populate_hist_tv(char *song, int weight, int sticky, int i)
{
	GtkTreeModel *model;
	if(hist_tv == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(hist_tv));
	gtk_tree_view_column_set_sort_order(gtk_tree_view_get_column(GTK_TREE_VIEW(hist_tv), 0), GTK_SORT_DESCENDING);
	GtkTreeIter iter;
	gtk_list_store_insert(GTK_LIST_STORE(model), &iter, (gint) i + 1);
	char* file = malloc(strlen(song) + 1);
	strncpy(file, song, strlen(song) + 1);
	song += strlen(val.musicdir);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, i+1, 1, sticky, 2, weight, 3, song, 5, file, -1);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(GTK_TREE_VIEW(hist_tv), 0), 0);
}
void update_hist_tv(char *song, int weight, int sticky)
{
	GtkTreeModel *model;
	if(hist_tv == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(hist_tv));
	gint i = gtk_tree_model_iter_n_children(model, NULL);
	//printf("have %d rows\n", (int) i);
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	char* file = malloc(strlen(song) + 1);
	strncpy(file, song, strlen(song) + 1);
	song += strlen(val.musicdir);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, i+1, 1, sticky, 2, weight, 3, song, 5, file, -1);
	gtk_tree_view_column_set_sort_column_id(gtk_tree_view_get_column(GTK_TREE_VIEW(hist_tv), 0), 0);
}
GtkTreeModel* create_update_model(GtkWidget* tv)
{
	GtkListStore *model;
	model = gtk_list_store_new
	(		3,
			G_TYPE_STRING, 	//file name
			G_TYPE_STRING,	//full path of song, as a reference
			GDK_TYPE_RGBA //color
	);
	GtkTreeViewColumn* action_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(action_col, "Action");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), action_col, 0);
	GtkCellRenderer* action_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(action_col, action_rend, TRUE);
	gtk_tree_view_column_add_attribute(action_col, action_rend, "text", 0);
	gtk_tree_view_column_add_attribute(action_col, action_rend, "foreground-rgba", 2);
	gtk_tree_view_column_set_sort_column_id(action_col, 0);
	gtk_tree_view_column_set_sort_indicator(action_col, TRUE);

	GtkTreeViewColumn* file_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(file_col, "Song");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), file_col, 1);
	GtkCellRenderer* file_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(file_col, file_rend, TRUE);
	gtk_tree_view_column_add_attribute(file_col, file_rend, "text", 1);
	gtk_tree_view_column_add_attribute(file_col, file_rend, "foreground-rgba", 2);
	gtk_tree_view_column_set_sort_column_id(file_col, 1);
	gtk_tree_view_column_set_sort_indicator(file_col, TRUE);

	//gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), TRUE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(model));

	return GTK_TREE_MODEL(model);
}
void add_update_data(int flag, char *song)
{
	GtkTreeModel *model;
	if(update_tv == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(update_tv));
	GtkTreeIter iter;
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);

	GdkColor color;
	char action[16];
	if (flag == 0)
	{
		memcpy(action, "Added\0", 6);
		color = color_green;
	}
	else if (flag == 1)
	{
		memcpy(action, "Deleted\0", 8);
		color = color_red;
	}
	else if (flag == 2)
	{
		memcpy(action, "Moved\0", 6);
		color = color_blue;
	}
	else if (flag == 3)
	{
		memcpy(action, "No updates\0", 11);
		color = color_black;
	}
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, action, 1, song, 2, &color, -1);
	gtk_widget_show_all(update_tv);
}
