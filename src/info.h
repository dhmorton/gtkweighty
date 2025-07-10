/*
 * info.h
 *
 *  Created on: Jul 10, 2011
 *      Author: bob
 */

#ifndef INFO_H_
#define INFO_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include <errno.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>
//openssl for signing the Amazon request
//#include <openssl/hmac.h>
//#include <openssl/evp.h>
//#include <openssl/bio.h>
//#include <openssl/buffer.h>
//curl for url escaping and image downloading
#include <curl/curl.h>
//Imlib2 for processing images
#include <Imlib2.h>
//libxml2 for parsing Amazon request returns
//#include <libxml/parser.h>
//#include <libxml/tree.h>

struct data {
	char *ptr;
	size_t len;
};

struct curl_data {
  char *data;
  size_t size;
};

extern char* discogs_token;

int launch_info(void);
int populate_stream_tv(time_t, char*, char*, char*);
void change_info_tv_weight(char*, int);
void save_lyrics(char*);
void set_lyrics();
int get_images_by_dir(void);
void update_info_win(void);
void update_streaming_win(void);
int highlight_track(void);
void populate_track_tv(char*, int, int, int i);
void set_streaming(int);
void clear_track_tv(void);
void destroy_track_tv(void);
int clear_info(void);

#endif /* INFO_H_ */
