/*
 * models.c
 *
 *  Created on: Jul 20, 2011
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

#include "models.h"
#include "myutils.h"

static GtkWidget *list_tree_tv = NULL;

static void sticky_toggled(GtkCellRendererToggle*, gchar*, GtkTreeModel*);

GtkWidget* create_list_tree_tv()
{
	list_tree_tv = gtk_tree_view_new();
	return list_tree_tv;
}
GtkTreeModel* create_standard_model(GtkWidget* tv)
{
	GtkListStore *model;
	model = gtk_list_store_new
	(		6,
			G_TYPE_UINT, 	//index number of the track
			G_TYPE_BOOLEAN, //sticky flag
			G_TYPE_UINT, 	//weight
			G_TYPE_STRING, 	//file name
			GDK_TYPE_RGBA, //color
			G_TYPE_STRING	//full path of song, as a reference, or the field reference for tags
	);

	GtkTreeViewColumn* index_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(index_col, "#");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), index_col, 0);
	GtkCellRenderer* index_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(index_col, index_rend, TRUE);
	gtk_tree_view_column_add_attribute(index_col, index_rend, "text", 0);
	gtk_tree_view_column_add_attribute(index_col, index_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(index_col, 0);
	gtk_tree_view_column_set_sort_indicator(index_col, TRUE);

	GtkTreeViewColumn* sticky_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(sticky_col, "S");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), sticky_col, 1);
	gtk_tree_view_column_set_max_width(sticky_col, 35);
	GtkCellRenderer* sticky_rend = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(sticky_rend), "toggled", G_CALLBACK(sticky_toggled), GTK_TREE_MODEL(model));
	gtk_tree_view_column_pack_start(sticky_col, sticky_rend, FALSE);
	gtk_tree_view_column_add_attribute(sticky_col, sticky_rend, "active", 1);
	gtk_tree_view_column_set_sort_column_id(sticky_col, 1);
	gtk_tree_view_column_set_sort_indicator(sticky_col, TRUE);

	GtkTreeViewColumn* weight_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(weight_col, "Weight");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), weight_col, 2);
	GtkCellRenderer* weight_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(weight_col, weight_rend, TRUE);
	gtk_tree_view_column_add_attribute(weight_col, weight_rend, "text", 2);
	gtk_tree_view_column_add_attribute(weight_col, weight_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(weight_col, 2);
	gtk_tree_view_column_set_sort_indicator(weight_col, TRUE);

	GtkTreeViewColumn* file_col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(file_col, "Song");
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tv), file_col, 3);
	GtkCellRenderer* file_rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(file_col, file_rend, TRUE);
	gtk_tree_view_column_add_attribute(file_col, file_rend, "text", 3);
	gtk_tree_view_column_add_attribute(file_col, file_rend, "foreground-rgba", 4);
	gtk_tree_view_column_set_sort_column_id(file_col, 3);
	gtk_tree_view_column_set_sort_indicator(file_col, TRUE);

	//gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tv), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tv)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(model));
	return GTK_TREE_MODEL(model);
}
//this is the callback for every CellRenererToggle in the standard model
void sticky_toggled(GtkCellRendererToggle *rend, gchar *path, GtkTreeModel *model)
{
	GtkTreeIter iter;
	gboolean sticky;
	printf("toggled\n");
	g_object_get(G_OBJECT(rend), "active", &sticky, NULL);
	printf("sticky = %d\n", sticky);
	gtk_tree_model_get_iter_from_string(model, &iter, path);
	gchar *file;
	gtk_tree_model_get(model, &iter, FULLPATH, &file, -1);
	printf("sticky %s\n", file);
	if (file[0] == '/')
	{
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, STICKY, ! sticky, -1);

		char com[32767];
		memset(com, 0, 32767);
		if (sticky)
			memcpy(com, "SPS0\0", 5);
		else
			memcpy(com, "SPS1\0", 5);
		memcpy(&com[5], file, strlen(file) + 1);
		send_command(com, strlen(file) + 6);
//		if (strcmp(file, playing.file) == 0)
//			set_sticky(! sticky);
	}
	else
	{
		GtkTreeModel *list_model;
		GtkTreeIter list_iter;
		gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list_tree_tv)), &list_model, &list_iter);
		gchar *list;
		gtk_tree_model_get(list_model, &list_iter, 0, &list, -1);
		printf("list = %s\n", list);
		if (! strncasecmp(list, "last", 4)) {
			gtk_tree_model_get_iter_first(list_model, &list_iter);
			gtk_tree_model_get(list_model, &list_iter, 0, &list, -1);
		}
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, STICKY, ! sticky, -1);
		printf("list = %s\n", list);

		char com[32767];
		memset(com, 0, 32767);
		//remember to flip the flag
		if (! strcasecmp(list, "album"))
		{
			if (sticky)
				memcpy(com, "SPA0\0", 5);
			else
				memcpy(com, "SPA1\0", 5);
		}
		else if (! strcasecmp(list, "artist"))
		{
			if (sticky)
				memcpy(com, "SPT0\0", 5);
			else
				memcpy(com, "SPT1\0", 5);
		}
		else if (! strcasecmp(list, "genre"))
		{
			if (sticky)
				memcpy(com, "SPG0\0", 5);
			else
				memcpy(com, "SPG1\0", 5);
		}
		char *pcom = &com[5];
		memcpy(pcom, file, strlen(file) + 1);
		send_command(com, strlen(file) + 6);
	}
	g_free(file);
}
