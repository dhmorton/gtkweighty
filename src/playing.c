/*
 * playing.c
 *
 *  Created on: Dec 31, 2011
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

#include "playing.h"
#include "actions.h"

static struct current playing;

void set_playing_file(char *file)
{
	if (playing.file != NULL)
		free(playing.file);
	playing.file = malloc(strlen(file) + 1);
	memcpy(playing.file, file, strlen(file) + 1);
}
void set_playing_artist(char *artist)
{
	if (playing.artist != NULL)
		free(playing.artist);
	playing.artist = malloc(strlen(artist) + 1);
	memcpy(playing.artist, artist, strlen(artist) + 1);
}
void set_playing_title(char *title)
{
	if (playing.title != NULL)
		free(playing.title);
	playing.title = malloc(strlen(title) + 1);
	memcpy(playing.title, title, strlen(title) + 1);
}
void set_playing_album(char *album)
{
	if (playing.album != NULL)
		free(playing.album);
	playing.album = malloc(strlen(album) + 1);
	memcpy(playing.album, album, strlen(album) + 1);
}
void set_playing_genre(char *genre)
{
	if (playing.genre != NULL)
		free(playing.genre);
	playing.genre = malloc(strlen(genre) + 1);
	memcpy(playing.genre, genre, strlen(genre) + 1);
}
void set_playing_track(char *track)
{
	if (playing.track != NULL)
		free(playing.track);
	playing.track = malloc(strlen(track) + 1);
	memcpy(playing.track, track, strlen(track) + 1);
}
void set_playing_year(char *year)
{
	if (playing.year != NULL)
		free(playing.year);
	playing.year = malloc(strlen(year) + 1);
	memcpy(playing.year, year, strlen(year) + 1);
}
void set_playing_length(char *length)
{
	if (playing.length != NULL)
		free(playing.length);
	playing.length = malloc(strlen(length) + 1);
	memcpy(playing.length, length, strlen(length) + 1);
}
void set_playing_tag(char *field, char *tag, int i, int len)
{
	//printf("set tag %s\n", tag);
	//printf("length %d\n", len);
	playing.tags[i].tag = malloc(len);
	if (len == 1)
		playing.tags[i].tag = NULL;
	else
	{
		memset(playing.tags[i].tag, 0, len);
		memcpy(playing.tags[i].tag, tag, len);
	}
	int flen = strlen(field) + 1;
	playing.tags[i].field = malloc(flen);
	memset(playing.tags[i].field, 0, flen);
	memcpy(playing.tags[i].field, field, flen);
	playing.num_tags = i + 1;
}
void set_playing_time_data(int t)
{
	playing.time = t;
}
void set_playing_weight(int w)
{
	playing.weight = w;
}
void set_playing_sticky(int s)
{
	playing.sticky = s;
}
char* get_playing_file()
{
	if (playing.file != NULL)
		return playing.file;
	else
		return NULL;
}
char* get_playing_artist()
{
	return playing.artist;
}
char* get_playing_title()
{
	return playing.title;
}
char* get_playing_album()
{
	return playing.album;
}
char* get_playing_genre()
{
	return playing.genre;
}
char* get_playing_track()
{
	return playing.track;
}
char* get_playing_length()
{
	return playing.length;
}
char* get_playing_year()
{
	return playing.year;
}
char *get_playing_field(int i)
{
	return playing.tags[i].field;
}
char *get_playing_tag(int i)
{
	return playing.tags[i].tag;
}
int get_num_tags()
{
	return playing.num_tags;
}
void clear_playing()
{
	int i;
	for (i = 0; i < playing.num_tags; i++)
	{
		if (playing.tags[i].field != NULL)
		{
			free(playing.tags[i].field);
			playing.tags[i].field = NULL;
		}
		if (playing.tags[i].tag != NULL)
		{
			free(playing.tags[i].tag);
			playing.tags[i].tag = NULL;
		}
	}

//	if (playing.tags != NULL)
//		free(playing.tags);
	playing.num_tags = 0;

	if (playing.artist != NULL)
		free(playing.artist);
	playing.artist = NULL;
	if (playing.title != NULL)
		free(playing.title);
	playing.title = NULL;
	if (playing.album != NULL)
		free(playing.album);
	playing.album = NULL;
	if (playing.genre != NULL)
		free(playing.genre);
	playing.genre = NULL;
	if (playing.track != NULL)
		free(playing.track);
	playing.track = NULL;
	if (playing.length != NULL)
		free(playing.length);
	playing.length = NULL;
	if (playing.year != NULL)
		free(playing.year);
	playing.year = NULL;
	if (playing.file != NULL)
		free(playing.file);
	playing.file = NULL;
}
int clear_tag_data(tag_data **data, int num_fields)
{
	if (data == NULL)
		return 1;
	int field;
	for (field = 0; field < num_fields; field++)
	{
		if ((*data)[field].field != NULL)
		{
//			printf("field = %s\n", (*data)[field].field);
			free((*data)[field].field);
			(*data)[field].field = NULL;
		}
		if ((*data)[field].tag != NULL)
		{
//			printf("tag = %s\n", (*data)[field].tag);
			free((*data)[field].tag);
			(*data)[field].tag = NULL;
		}
	}
	if (*data != NULL)
		free(*data);
	return 0;
}
