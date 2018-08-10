/*
 * utils.c
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
#include "myutils.h"

static GSocket *sock = NULL;

GdkColor color_black  = {0, 0x0000, 0x0000, 0x0000};
GdkColor color_white  = {0, 0xffff, 0xffff, 0xffff};
GdkColor color_red    = {0, 0xffff, 0x0000, 0x0000};
GdkColor color_blue   = {0, 0x0000, 0x0000, 0xffff};
GdkColor color_yellow = {0, 0xffff, 0xffff, 0x0000};
GdkColor color_green  = {0, 0x0000, 0xa000, 0x0000};
GdkColor color_cyan   = {0, 0x66ff, 0xffff, 0xffff};
GdkColor color_gray   = {0, 0x82ff, 0x82ff, 0x82ff};

static void reverse(char*);
struct counts stats;
struct config val;

int get_playby()
{
	if(strcmp(val.playby, "song") == 0)
		return SONG;
	else if (strcmp(val.playby, "album") == 0)
		return ALBUM;
	else if (strcmp(val.playby, "artist") == 0)
		return ARTIST;
	else if (strcmp(val.playby, "genre") == 0)
			return GENRE;
	else
		return -1;
}
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}
void reverse(char *p)
{
  char *q = p;
  while(q && *q) ++q;
  for(--q; p < q; ++p, --q)
    *p = *p ^ *q,
    *q = *p ^ *q,
    *p = *p ^ *q;
}
void print_data(char *s, int len)
{
	printf("PRINT DATA\n");
	if(s == NULL)
		return;
	int i;
	for (i = 0; i < len; i++)
	{
		if (s[i] == '\0')
			printf("|");
		else
			printf("%c", s[i]);
	}
	printf("\n");
}
void set_sock(GSocket *s)
{
	sock = s;
}
void send_command(char *s, int len)
{
	GError *error = NULL;
	gssize sent = 0;
	if (len > 49152)//3*2^14 is the max sent in one chunk apparantly
	{
		while (len > 49152)
		{
			if (g_socket_condition_wait(sock, G_IO_OUT, NULL, NULL))
				sent += g_socket_send(sock, s, 49152, NULL, &error);
			if (error)
			{
				perror(error->message);
				error = NULL;
			}
			else
			{
				s += 49152;
				len -= 49152;
			}
		}
		if (len > 0)
		{
			if (g_socket_condition_wait(sock, G_IO_OUT, NULL, NULL))
				sent = g_socket_send(sock, s, len, NULL, &error);
			if (error)
			{
				perror(error->message);
				error = NULL;
			}
		}
	}
	else
	{
		if (g_socket_condition_wait(sock, G_IO_OUT, NULL, NULL))
			sent = g_socket_send(sock, s, len, NULL, &error);
		if (error)
		{
			perror(error->message);
			error = NULL;
		}
	}
	//print_data(s, len);
	if (error)
		g_error("%s", error->message);
}
//replace field with a translated version
void translate_field(char* field, char *trfield)
{
	if (strcmp(field, "TPE1") == 0)
		memcpy(trfield, "Artist\0", 7);
	else if (strcmp(field, "TPE2") == 0)
		memcpy(trfield, "Band/orchestra/accompaniment\0", 29);
	else if (strcmp(field, "TALB") == 0)
		memcpy(trfield, "Album\0", 6);
	else if (strcmp(field, "TCON") == 0)
		memcpy(trfield, "Genre\0", 6);
	else if (strcmp(field, "TIT2") == 0)
		memcpy(trfield, "Title\0", 6);
	else if (strcmp(field, "COMM") == 0)
		memcpy(trfield, "Comments\0", 9);
	else if (strcmp(field, "TCOM") == 0)
		memcpy(trfield, "Composer\0", 9);
	else if (strcmp(field, "TLEN") == 0)
		memcpy(trfield, "Length\0", 7);
	else if (strcmp(field, "TRCK") == 0)
		memcpy(trfield, "Track #\0", 8);
	else if (strcmp(field, "TYER") == 0)
		memcpy(trfield, "Year\0", 5);
	else if (strcmp(field, "TCOP") == 0)
		memcpy(trfield, "Copyright\0", 10);
	else if (strcmp(field, "TDRC") == 0)
		memcpy(trfield, "Recording time\0", 15);
	else if (strcmp(field, "TPUB") == 0)
		memcpy(trfield, "Publisher\0", 10);
	else if (strcmp(field, "TPOS") == 0)
		memcpy(trfield, "Part of a set\0", 14);
	else if (strcmp(field, "TFLT") == 0)
		memcpy(trfield, "File type\0", 10);
	else if (strcmp(field, "TENC") == 0)
		memcpy(trfield, "Encoded by\0", 11);
	else if (strcmp(field, "TPE3") == 0)
		memcpy(trfield, "Conductor/performer refinement\0", 31);
	else
	{
//		printf("MISSING FIELD %s\n", field);
		memcpy(trfield, field, strlen(field) + 1);
	}
}
//reverse of the above function
void back_translate(char *field, char **trfield)
{
	if (strcasecmp(field, "Artist") == 0)
		memcpy(*trfield, "TPE1\0", 5);
	else if (strcasecmp(field, "Band/orchestra/accompaniment") == 0)
		memcpy(*trfield, "TPE2\0", 5);
	else if (strcasecmp(field, "Album") == 0)
		memcpy(*trfield, "TALB\0", 5);
	else if (strcasecmp(field, "Genre") == 0)
		memcpy(*trfield, "TCON\0", 5);
	else if (strcasecmp(field, "Title") == 0)
		memcpy(*trfield, "TIT2\0", 5);
	else if (strcasecmp(field, "Comments") == 0)
		memcpy(*trfield, "COMM\0", 5);
	else if (strcasecmp(field, "Composer") == 0)
		memcpy(*trfield, "TCOM\0", 5);
	else if (strcasecmp(field, "Length") == 0)
		memcpy(*trfield, "TLEN\0", 5);
	else if ((strcasecmp(field, "Track #") == 0) || (strcmp(field, "Track Number") == 0))
		memcpy(*trfield, "TRCK\0", 5);
	else if ((strcasecmp(field, "Year") == 0)|| (strcmp(field, "DATE") == 0) || (strcmp(field, "Date") == 0))
		memcpy(*trfield, "TYER\0", 5);
	else if (strcasecmp(field, "Copyright") == 0)
		memcpy(*trfield, "TCOP\0", 5);
	else if ((strcasecmp(field, "Recording time") == 0))
		memcpy(*trfield, "TDRC\0", 5);
	else if (strcasecmp(field, "Publisher") == 0)
		memcpy(*trfield, "TPUB\0", 5);
	else if (strcasecmp(field, "Part of a set") == 0)
		memcpy(*trfield, "TPOS\0", 5);
	else if (strcasecmp(field, "File type") == 0)
		memcpy(*trfield, "TFLT\0", 5);
	else if (strcasecmp(field, "Encoded by") == 0)
		memcpy(*trfield, "TENC\0", 5);
	else if (strcasecmp(field, "Conductor/performer refinement") == 0)
		memcpy(*trfield, "TPE3\0", 5);
	else
		memset(*trfield, 0, 5);
}
void error(char *s) {
	perror(s);
	printf("\n");
	exit(EXIT_FAILURE);
}
void quit() { exit(0); }
