/*
 * models.h
 *
 *  Created on: Jul 20, 2011
 *      Author: bob
 */

#ifndef MODELS_H_
#define MODELS_H_

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#define INDEX 0
#define STICKY 1
#define WEIGHT 2
#define FILENAME 3
#define COLOR 4
#define FULLPATH 5

//model creation functions
GtkTreeModel* create_standard_model(GtkWidget*);
GtkWidget* create_list_tree_tv(void);

#endif /* MODELS_H_ */
