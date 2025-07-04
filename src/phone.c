/**
 * phone.c
 * Created June 23, 2025
 * 
 */

#include "phone.h"
#include "models.h"

GtkWidget *phone_win = NULL;

void phone_test_ui()
{
	phone_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (phone_win), "Phone Download");
	gtk_window_set_default_size(GTK_WINDOW (phone_win), 1000, 800);
	g_signal_connect(G_OBJECT (phone_win), "destroy", G_CALLBACK (close_phone_win), NULL);

	GtkWidget *control_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

	GtkWidget *message_entry = gtk_entry_new();
	gtk_widget_set_size_request(message_entry, 800, 14);
	gtk_editable_set_editable(GTK_EDITABLE(message_entry), FALSE);
	//gtk_entry_has_frame(GTK_ENTRY(message_entry), FALSE);

	gtk_box_pack_start(GTK_BOX(control_vbox), message_entry, FALSE, TRUE, 1);

	GtkWidget *songs_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	GtkTreeModel *model;
	GtkWidget *tv = gtk_tree_view_new();
	model = create_standard_model(tv);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tv), model);
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scroll), tv);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_box_pack_start(GTK_BOX(vbox), control_vbox, FALSE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 1);

	gtk_container_add(GTK_CONTAINER(phone_win), vbox);
	gtk_widget_show_all(phone_win);
}
void close_phone_win()
{
	gtk_widget_destroy(phone_win);
	phone_win = NULL;
}
