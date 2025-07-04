/*
 * comm.c
 *
 *  Created on: Jun 14, 2011
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
#include "comm.h"
#include "config.h"
#include "myutils.h"
#include "actions.h"
#include "stats_actions.h"
#include "browser.h"
#include "info.h"
#include "stats.h"
#include "playing.h"

const char *ip = "127.0.0.1";
struct sockaddr_in addr;
static GSocket * sock = NULL;
static GSource * source = NULL;
static char data_flag = '\0';
char data_buf[BIGBUFF];
static char *pbuf;//pointer to the read position in data_buf
static int bytes_parsed = 0;
static int data_buf_len = 0;
static int song_count_holder = -1;
static int song_total_holder = -1;
static int num_fields_holder = -1;

static void clear_data(void);
static gboolean handle_data(GSocket*, GIOCondition, gpointer);
static gssize check_for_data(void);
static gssize recv_data(char*);
static void flush_buffer(void);
static void buf_shift(void);
static int get_num_from_buf(void);
static int get_string_from_buf(char*);
static int count_nts(int);
static void parse_data(void);
static void step(void);
//parse functions
static void parse_song_data(void);
static int parse_tag_data(void);
static void parse_list_of_fields(void);
static int parse_tag_tv_data(tag_data**);
static void parse_update(void);
static void parse_highlight_playlist(void);
static void parse_volume(void);
static void parse_time(void);
static void parse_remaining(void);
static void parse_weight_and_sticky(void);
static void parse_progressbar_data(void);
static void parse_stats(void);
static void parse_config(void);
static void parse_playing(void);
static void parse_lyrics(void);
static void parse_stream_data(void);
static void parse_add_stream_data(void);
static void parse_stream_history(void);
static void parse_discogs_token(void);

int g_connect_to_server()
{
	GError *error = NULL;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(ip);

	//g_type_init();
	sock = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &error);
	GIOChannel *io = g_io_channel_unix_new(g_socket_get_fd(sock));
	g_io_channel_set_encoding(io, NULL, &error);
	if (error)
		g_error("%s", error->message);
	printf("new socket \n");
	GSocketAddress * address = g_socket_address_new_from_native(&addr, sizeof(addr));
	g_socket_connect(sock, address, NULL, &error);
	g_socket_set_blocking(sock, FALSE);
	if (error)
		g_error("%s", error->message);
	source = g_socket_create_source(sock, G_IO_IN, NULL);
	g_source_set_callback(source,(GSourceFunc) handle_data, NULL, NULL);
	g_source_attach(source, NULL);
	set_sock(sock);
	//set up a couple of things
	clear_data();

	return 0;
}
//clears the playing structure so nothing stale sticks around
void clear_data()
{
	clear_info();
	clear_playing();
	clear_track_tv();
}
gboolean handle_data(GSocket* sock, GIOCondition condition, gpointer data)
{
	//there is data available so get it
	//printf("buf len = %d\n", data_buf_len);
	check_for_data();
	while ((data_buf_len - bytes_parsed) > 0)
	{
		parse_data();
		usleep(50);
	}

	return TRUE;
}
gssize check_for_data()
{
	char string[BIGBUFF];
	char buf[BUFF];
	gssize recv = 0;
	gssize ret = 0;
	while (ret != -1)
	{
		ret = recv_data(buf);
		if (ret != -1)
		{
			int i;
			for (i = recv; i < recv + ret; i++) {
				string[i] = buf[i - recv];
			}
			recv += ret;
		}
	}
	if (recv != 0)
	{
		if (data_buf[0] != '\0')
		{
			int i;
			for (i = 0; i < recv; i++)
				data_buf[data_buf_len+i] = string[i];
			data_buf_len += recv;
		}
		else//otherwise put it in the holding buffer because we don't know if it's complete or not
		{
			memcpy(data_buf, string, recv);
			data_buf_len = recv;
			pbuf = &data_buf[0];
		}
	}
	return recv;
}
gssize recv_data(char *buf)
{
	GError *error = NULL;
	gssize recv = g_socket_receive(sock, (gchar *) buf, BUFF, NULL, &error);
	if(error) {
	   g_error_free(error);
	}
	if (recv == 0)//connection is dead
		quit();

	return recv;
}
void flush_buffer()
{
	memset(&data_buf[0], 0, BIGBUFF);
	pbuf = &data_buf[0];
	data_buf_len = bytes_parsed = 0;
}
//used letters: ABCDEFGHIJKLMNOPQRSTUVWXY
void parse_data()
{/*
	if (*pbuf != 'B')
	{
		int print_len = ((data_buf_len - bytes_parsed) < 100) ? (data_buf_len - bytes_parsed) : 100;
		print_data(pbuf, print_len);
	}*/

	//print_data(pbuf, data_buf_len - bytes_parsed);
	if (data_flag == '\0')//a new command
	{
		//parse the first byte and figure out what to do with it
		if (*pbuf == 'B')//update progressbar
		{
			data_flag = 'B';
			step();
			parse_progressbar_data();
		}
		else if (*pbuf == 'T')//tag data
		{
			data_flag = 'T';//flag the fact that we got here and drop the initial byte
			step();
			parse_tag_data();
		}
		else if (*pbuf == 'W')//weight and sticky data
		{
			data_flag = 'W';
			step();
			parse_weight_and_sticky();
		}
		else if (*pbuf == 'F')//data for file_tv
		{
			data_flag = 'F';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'P')//data for pl_tv
		{
			data_flag = 'P';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'E')//list of fields
		{
			data_flag = 'E';
			step();
			parse_list_of_fields();
		}
		else if (*pbuf == 'G')//tag_tv data
		{
			data_flag = 'G';
			step();
			tag_data *my_data = NULL;
			my_data = malloc(2000*sizeof(tag_data));
			parse_tag_tv_data(&my_data);
		}
		else if (*pbuf == 'C')//get config
		{
			data_flag = 'C';
			step();
			parse_config();
		}
		else if (*pbuf == 'H')//get history
		{
			data_flag = 'H';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'Q')//get search results
		{
			data_flag = 'Q';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'S')//get stats
		{
			data_flag = 'S';
			step();
			parse_stats();
		}
		else if (*pbuf == 'M')//parse time data
		{
			data_flag = 'M';
			step();
			parse_time();
		}
		else if (*pbuf == 'R')//parse songs remaining data
		{
			data_flag = 'R';
			step();
			parse_remaining();
		}
		else if (*pbuf == 'K')//parse update history data
		{
			data_flag = 'K';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'L')//highlight playlist
		{
			data_flag = 'L';
			step();
			parse_highlight_playlist();
		}
		else if (*pbuf == 'U')//update
		{
			data_flag = 'U';
			step();
			parse_update();
		}
		else if (*pbuf == 'V')
		{
			data_flag = 'V';

			step();
			parse_volume();
		}
		else if (*pbuf == 'A')//field data
		{
			data_flag = 'A';

			step();
			parse_song_data();
		}
		else if (*pbuf == 'D')//field list data
		{
			data_flag = 'D';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'I')//track list for info win
		{
			data_flag = 'I';
			step();
			parse_song_data();
		}
		else if (*pbuf == 'X')//lyrics
		{
			data_flag = 'X';
			step();
			parse_lyrics();
		}
		else if (*pbuf == 'Y')//currently playing
		{
			data_flag = 'Y';
			step();
			parse_playing();
		}
		else if (*pbuf == 'N')//set stream metadata
		{
			data_flag = 'N';
			step();
			parse_stream_data();
		}
		else if (*pbuf == 'O')//set stream metadata
		{
			data_flag = 'O';
			step();
			parse_add_stream_data();
		}
		else if (*pbuf == 'J')//parse stream history
		{
			data_flag = 'J';
			step();
			parse_stream_history();
		}
		else if (*pbuf == 'Z')//parse discogs token
		{
			data_flag = 'Z';
			step();
			parse_discogs_token();
		}
		else
		{
			printf("%c: %c is not a command\n", data_flag, *pbuf);
			print_data(pbuf, data_buf_len - bytes_parsed);
			flush_buffer();
		}
	}
	else if (data_flag == 'T')//deal with tag data
		parse_tag_data();
	else if (data_flag == 'F')
		parse_song_data();
	else if (data_flag == 'P')
		parse_song_data();
	else if (data_flag == 'E')//list of fields
		parse_list_of_fields();
	else if (data_flag == 'G')//deal with tag data
	{
		tag_data* my_data = NULL;
		my_data = malloc(2000*sizeof(tag_data));
		parse_tag_tv_data(&my_data);
	}
	else if (data_flag == 'H')//get history
		parse_song_data();
	else if (data_flag == 'Q')//get search results
		parse_song_data();
	else if (data_flag == 'C')//get config
		parse_config();
	else if (data_flag == 'B')//update progressbar
		parse_progressbar_data();
	else if (data_flag == 'W')
		parse_weight_and_sticky();
	else if (data_flag == 'M')//parse time data
		parse_time();
	else if (data_flag == 'R')//parse remaining
		parse_remaining();
	else if (data_flag == 'K')
		parse_song_data();
	else if (data_flag == 'L')//highlight playlist
		parse_highlight_playlist();
	else if (data_flag == 'U')
		parse_update();
	else if (data_flag == 'V')//highlight playlist
		parse_volume();
	else if (data_flag == 'A')//parse field
		parse_song_data();
	else if (data_flag == 'D')//parse field entry data
		parse_song_data();
	else if (data_flag == 'I')
		parse_song_data();
	else if (data_flag == 'X')//lyrics
		parse_lyrics();
	else if (data_flag == 'Y')//currently playing
		parse_playing();
	else if (data_flag == 'N')//stream data
		parse_stream_data();
	else if (data_flag == 'O')//new stream data
		parse_add_stream_data();
	else if (data_flag == 'J')
		parse_stream_history();
	else if (data_flag == 'Z')//parse discogs token
		parse_discogs_token();
}
//takes one step along the command buffer
//pbuf is the read pointer along the buffer
//bytes_parsed keeps track of how many bytes we've read for this command
void step()
{
	pbuf++;
	bytes_parsed++;
}
/*
//used letters: ABCDEFGHIJKLMNOPQRSTUVWXYZ
//FIXME: Rewrite this
void parse_data()
{
	if(*pbuf != 'B')
		print_data(pbuf, data_buf_len - bytes_parsed);
	char c = *pbuf;
	pbuf++; bytes_parsed++;
	data_flag = '\0';
	if (c == 'A')//field data
	{
		data_flag = 'A';
		parse_song_data();
	}
	else if (c == 'B')//update progressbar
		parse_progressbar_data();
	else if (c == 'C')//get config
		parse_config();
	else if (c == 'D')//field list data
	{
		data_flag = 'D';
		parse_song_data();
	}
	else if (c == 'E')//list of fields
	{
		data_flag = 'E';
		parse_list_of_fields();
	}
	else if (c == 'F')//data for file_tv
	{
		data_flag = 'F';
		parse_song_data();
	}
	else if (c == 'G')//tag_tv data
	{
		data_flag = 'G';
		tag_data *my_data = NULL;
		my_data = malloc(2000*sizeof(tag_data));
		parse_tag_tv_data(&my_data);
	}
	else if (c == 'H')//get history
	{
		data_flag = 'H';
		parse_song_data();
	}
	else if (c == 'I')//track list for info win
	{
		data_flag = 'I';
		parse_song_data();
	}
	else if (c == 'J')//parse stream history
	{
		data_flag = 'J';
		parse_stream_history();
	}
	else if (c == 'K')//parse update history data
	{
		data_flag = 'K';
		parse_song_data();
	}
	else if (c == 'L')//highlight playlist
		parse_highlight_playlist();
	else if (c == 'M')//parse time data
		parse_time();
	else if (c == 'N')//set stream metadata
		parse_stream_data();
	else if (c == 'O')//set stream metadata
		parse_add_stream_data();
	else if (c == 'P')//data for pl_tv
	{
		data_flag = 'P';
		parse_song_data();
	}
	else if (c == 'Q')//get search results
	{
		data_flag = 'Q';
		parse_song_data();
	}
	else if (c == 'R')//parse songs remaining data
		parse_remaining();
	else if (c == 'S')//get stats
		parse_stats();
	else if (c == 'T')//tag data
		parse_tag_data();
	else if (c == 'U')//update
		parse_update();
	else if (c == 'V')
		parse_volume();
	else if (c == 'W')//weight and sticky data
		parse_weight_and_sticky();
	else if (c == 'X')//lyrics
		parse_lyrics();
	else if (c == 'Y')//currently playing
		parse_playing();
	else if (c == 'Z')//parse discogs token
		parse_discogs_token();
	else
	{
		printf("%c: %c is not a command\n", data_flag, *pbuf);
		//print_data(pbuf, data_buf_len - bytes_parsed);
		flush_buffer();
	}
}
*/
void parse_add_stream_data()
{
	char name[256];
	get_string_from_buf(name);
	char genre[256];
	get_string_from_buf(genre);
	char desc[1024];
	get_string_from_buf(desc);
	char bitrate[4];
	get_string_from_buf(bitrate);
	add_stream_data(name, genre, desc, bitrate);
	data_flag = '\0';
	buf_shift();
}
void parse_stream_data()
{
	set_streaming(1);
	if (*pbuf == 'G')
	{
		pbuf++;
		char genre[256];
		int ret = get_string_from_buf(genre);
		if (ret > 0)
			set_left(genre);
		else {
			pbuf++;
			bytes_parsed++;
		}
	}
	else if (*pbuf == 'B')
	{
		pbuf++;
		char bitrate[256];
		int ret = get_string_from_buf(bitrate);
		if (ret > 0)
			set_playby(bitrate);
		else
		{
			pbuf++;
			bytes_parsed++;
		}
	}
	else if (*pbuf == 'N')
	{
		pbuf++;
		char name[256];
		int ret = get_string_from_buf(name);
		if (ret > 0)
			set_time(name);
		else
		{
			pbuf++;
			bytes_parsed++;
		}
	}
	data_flag = '\0';
	buf_shift();
}
void parse_song_data()
{
	//printf("parse song data\n");
	int songs_left, weight, sticky;
	char file[4096];
	if (count_nts(4))
	{
		songs_left = get_num_from_buf();
		if (song_total_holder < 0)
			song_total_holder = songs_left + 1;
		//printf("songs left = %d\n", songs_left);
		int i = song_total_holder - songs_left - 1;
		sticky = get_num_from_buf();
		weight = get_num_from_buf();
		get_string_from_buf(file);

		if (data_flag == 'F')
			populate_file_tv(file, weight, sticky, i);
		else if (data_flag == 'P')
			populate_pl_tv(file, weight, sticky, i);
		else if (data_flag == 'H')
			populate_hist_tv(file, weight, sticky, i);
		else if (data_flag == 'K')
			update_hist_tv(file, weight, sticky);
		else if (data_flag == 'Q')
			populate_search_tv(file, weight, sticky, i);
		else if (data_flag == 'A')
			populate_list_tv(file, weight, sticky, i);
		else if (data_flag == 'D')
			populate_list_file_tv(file, weight, sticky, i);
		else if (data_flag == 'I')
			populate_track_tv(file, weight, sticky, i);
		if (songs_left == 0)
		{
			song_total_holder = -1;
			buf_shift();
			if (data_flag == 'F')
			{
				set_cursor_on_playing_file(0);
			}
			else if (data_flag == 'Q')
			{
				clear_search_fields();
			}
			else if (data_flag == 'A')
				set_cursor_on_playing();
			else if (data_flag == 'D')
			{
				set_cursor_on_playing_file(2);
			}
			else if (data_flag == 'I')
				highlight_track();
		}
		data_flag = '\0';
	}
	else
	{
		check_for_data();
	}
}
void parse_stream_history()
{
	int songs_left;
	int time;
	char artist[256], title[256], stream[256];
	if (count_nts(5))
	{
		songs_left = get_num_from_buf();
		if (song_total_holder < 0)
			song_total_holder = songs_left + 1;
		time = get_num_from_buf();
		int ret = get_string_from_buf(artist);
		if (ret < 0)
			artist[0] = '\0';
		ret = get_string_from_buf(title);
		if (ret < 0)
			title[0] = '\0';
		get_string_from_buf(stream);
		populate_stream_tv(time, artist, title, stream);
		if (songs_left == 0)
		{
			song_total_holder = -1;
			buf_shift();
		}
		data_flag = '\0';
	}
}
void parse_playing()
{
	set_streaming(0);
	char song[1024];
	get_string_from_buf(song);
	set_playing_file(song);
	data_flag = '\0';
	set_playby(val.playby);
	update_info_win();
}
void parse_list_of_fields()
{
	char flag = pbuf[0];
	pbuf++; bytes_parsed++;
	int num_fields = get_num_from_buf();
	char *fields[num_fields];
	int i;
	for (i = 0; i < num_fields; i++)
	{
		fields[i] = malloc(64);//field names aren't long
		memset(fields[i], 0, 64);
		get_string_from_buf(fields[i]);
	}
	create_tag_model_new(flag, num_fields, fields);
	data_flag = '\0';
	buf_shift();
}
int parse_tag_tv_data(tag_data** data)
{
	//parse the first chunk to see how many tags to parse
	//need at least two data chunks
	int songs_left = -1;
	int num_fields = -1;

	if (count_nts(2))
	{
		if (song_count_holder != -1)
		{
			songs_left = song_count_holder;
			song_count_holder = -1;
			num_fields = num_fields_holder;
			num_fields_holder = -1;
		}
		else
		{
			songs_left = get_num_from_buf();
			song_count_holder = songs_left;

			if (song_total_holder < 0)
				song_total_holder = songs_left + 1;
			num_fields = get_num_from_buf();
			num_fields_holder = num_fields;
		}
//		printf("songs left = %d\n", songs_left);
//		printf("fields = %d\n", num_fields);
	}
	else if(*pbuf == 0)
	{
		get_num_from_buf();
		return 0;
	}
	else
	{
		check_for_data();
		return -1;
	}
	if (count_nts(2 * num_fields))
	{
		int j;
		for (j = 0; j < num_fields; j++)
		{
			char fld[1024];
			get_string_from_buf(fld);
			char *field = malloc(1024);
			translate_field(fld, field);
			(*data)[j].field = malloc(strlen(field) + 1);
			memcpy((*data)[j].field, field, strlen(field) + 1);
			char tag[4096];
			int len = get_string_from_buf(tag);
			(*data)[j].tag = malloc(len);
			memcpy((*data)[j].tag, tag, len);
			//printf("%s\t%s\n", (*data)[j].field, (*data)[j].tag);
		}
		int i = song_total_holder - songs_left - 1;
		populate_tag_tv(data, i, num_fields);
		song_count_holder = -1;
		num_fields_holder = -1;
		data_flag = '\0';
		if (songs_left == 0)
		{
			song_total_holder = -1;
			buf_shift();
		}
	}
	else if (num_fields == 0)
	{
		if (songs_left == 0)
		{
			data_flag = '\0';
			song_total_holder = -1;
			song_count_holder = -1;
			num_fields_holder = -1;
			buf_shift();
		}
	}
	else
		check_for_data();

	return 0;
}
int parse_tag_data()
{
	//print_data(pbuf, data_buf_len - bytes_parsed);
	int num_fields;
	if (count_nts(1))
	{
		if (num_fields_holder != -1)
		{
			num_fields = num_fields_holder;
			num_fields_holder = -1;
		}
		else
		{
			num_fields = get_num_from_buf();
			if (num_fields < 0)
				num_fields = -1;
			num_fields_holder = num_fields;
		}
	}
	else
	{
		check_for_data();
		return -1;
	}
//	int w;
//	for (w = 0; w < playing.num_tags; w++)
//		printf("\t%s\t%s\n", playing.tags[w].field, playing.tags[w].tag);
	clear_data();
	clear_entry_fields();
	if (num_fields == 0)
	{
		num_fields_holder = -1;
		data_flag = '\0';
		buf_shift();
	}
	else if (count_nts(num_fields))
	{
		//now we can finally parse this shit
		int j;
		int num_tags = 0;
		for (j = 0; j < num_fields; j++)
		{
			char fld[1024];
			memset(fld, 0, 1024);
			get_string_from_buf(fld);
			char *field = malloc(64);
			translate_field(fld, field);
			char tag[65535];
			memset(tag, 0, 65535);
			int len = get_string_from_buf(tag);
			if (len < 0)
			{
				pbuf++; bytes_parsed++;
				len = 1;
			}
			if (! strcasecmp(field, "artist"))
			{
//				printf("%s = %s\n", field, tag);
				set_artist(tag);
				set_playing_artist(tag);
			}
			else if (! strcasecmp(field, "title"))
			{
//				printf("%s = %s\n", field, tag);
				set_title(tag);
				set_playing_title(tag);
			}
			else if (! strcasecmp(field, "album"))
			{
//				printf("%s = %s\n", field, tag);
				set_playing_album(tag);
			}
			else if (! strcasecmp(field, "genre"))
			{
//				printf("%s = %s\n", field, tag);
				set_playing_genre(tag);
			}
			else if (! strcasecmp(field, "track #"))
			{
//				printf("%s = %s\n", field, tag);
				set_playing_track(tag);
			}
			else if (! strcasecmp(field, "length"))
			{
//				printf("%s = %s\n", field, tag);
				set_playing_length(tag);
			}
			else if ((strcasecmp(field, "year") == 0) || (strcasecmp(field, "date") == 0))
			{
//				printf("%s = %s\n", field, tag);
				set_playing_year(tag);
			}
			else
			{
				set_playing_tag(field, tag, num_tags, len);
				num_tags++;
			}
		}
		data_flag = '\0';
		num_fields_holder = -1;
	}
	else//not enough data, we're out
		check_for_data();
	update_streaming_win();
	return 0;
}
void parse_update()
{
	int flag = -1;
	if (*pbuf == 'A')//added
		flag = 0;
	else if (*pbuf == 'D')//deleted
		flag = 1;
	else if (*pbuf == 'M')//moved
		flag = 2;
	else if (*pbuf == 'N')//no change
		flag = 3;
	else
		printf("Bad update command %c\n", *pbuf);
	pbuf++; bytes_parsed++;
	if (flag < 3)
	{
		char song[BUFF];
		get_string_from_buf(song);
		add_update_data(flag, song);
	}
	else
		add_update_data(flag, NULL);
	data_flag = '\0';
}
void parse_highlight_playlist()
{
	print_data(pbuf, data_buf_len);
	int index = get_num_from_buf();
	printf("highlight playlist index = %d\n", index);
	highlight_playlist(index);
	data_flag = '\0';
	buf_shift();
}
void parse_volume()
{
	int vol = get_num_from_buf();
	set_volume(vol);
	data_flag = '\0';
	buf_shift();
}
void parse_time()
{
	int msec = get_num_from_buf();
	char time_s[6];
	if (msec == 0)
	{
		memcpy(time_s, "--:--", 5);
		time_s[5] = '\0';
	}
	else
	{
		set_playing_time_data(msec);
		int sec = msec / 1000;
		int min = sec / 60;
		int rem = sec % 60;
		char mm[3];
		char ss[3];
		snprintf(&mm[0], 3, "%d", min);
		snprintf(&ss[0], 3, "%d", rem);
		mm[2] = 0;
		ss[2] = 0;
		int i = 0;
		time_s[i] = mm[i];
		i++;
		if (min < 10)
			time_s[i] = ':';
		else
		{
			time_s[i] = mm[1];
			i++;
			time_s[i] = ':';
		}
		i++;
		if (rem < 10)
		{
			time_s[i] = '0';
			i++;
		}
		time_s[i] = ss[0];
		i++;
		time_s[i] = ss[1];
		i++;
		time_s[i] = '\0';
	}
	set_time(time_s);
	data_flag = '\0';
	buf_shift();
}
void parse_remaining()
{
	char remaining[16];
	memset(remaining, 0, 16);
	get_string_from_buf(remaining);
	set_left(remaining);
	data_flag = '\0';
	buf_shift();
}
void parse_weight_and_sticky()
{
	if (count_nts(2))
	{
		char weight[4];
		int sticky;
		get_string_from_buf(weight);
		//printf("weight = %s\n", weight);
		sticky = get_num_from_buf();
		//printf("sticky = %d\n", sticky);
		set_playing_sticky(sticky);
		set_playing_weight(atoi(weight));
		set_weight_and_sticky(weight, sticky);
		data_flag = '\0';
	}
	buf_shift();
}
void parse_config()
{
	get_string_from_buf(val.musicdir);
	val.threshhold = get_num_from_buf();
	get_string_from_buf(val.type);
	val.var = get_num_from_buf();
	get_string_from_buf(val.playby);
	val.song_rand = get_num_from_buf();
	val.song_skip = get_num_from_buf();
	val.album_rand = get_num_from_buf();
	val.album_skip = get_num_from_buf();
	val.artist_rand = get_num_from_buf();
	val.artist_skip = get_num_from_buf();
	val.genre_rand = get_num_from_buf();
	val.genre_skip = get_num_from_buf();
	set_config_data();
	data_flag = '\0';
}
void parse_progressbar_data()
{
	int permil = get_num_from_buf();
	update_progressbar(permil);
	buf_shift();
	data_flag = '\0';
}
void parse_stats()
{
	stats.mp3 = get_num_from_buf();
	stats.ogg = get_num_from_buf();
	stats.mpc = get_num_from_buf();
	stats.wav = get_num_from_buf();
	stats.flac = get_num_from_buf();
	stats.m4a = get_num_from_buf();
	stats.wma = get_num_from_buf();
	stats.other = get_num_from_buf();
	stats.zero = get_num_from_buf();
	stats.hund = get_num_from_buf();
	stats.sticky = get_num_from_buf();
	stats.total = get_num_from_buf();
	stats.albums = get_num_from_buf();
	stats.artists = get_num_from_buf();
	stats.genres = get_num_from_buf();
	int i;
	int max = 0;
	for(i = 0; i <= 100; i++)
	{
		stats.count[i] = get_num_from_buf();
		if (stats.count[i] > max)
			max = stats.count[i];
	}
	stats.max = max;
	buf_shift();
	data_flag = '\0';
	//redraw_canvas();
	update_stats_label();
}
void parse_lyrics()
{
	char lyrics[65535];
	get_string_from_buf(lyrics);
	set_lyrics(lyrics);
	buf_shift();
	data_flag = '\0';
}
void parse_discogs_token()
{
	char token[42];
	//token[40] = 0;
	get_string_from_buf(token);
	discogs_token = malloc(42 * sizeof(char));
	memcpy(discogs_token, token, 42);
	buf_shift();
	data_flag = '\0';
}

//returns true if the number of null terminators in the string in data_buf
//is greater than or equal to min
int count_nts(int min)
{
	int i = 0;
	int count = 0;//count the NTs
	char *p = pbuf;
	while (i++ < (data_buf_len - bytes_parsed))
	{
		if (*(p++) == '\0')
			count++;
		if (count >= min)
			return 1;
	}
	return 0;
}
int get_string_from_buf(char* s)
{
	int j = 0;
	while (*pbuf != '\0')
	{
		s[j++] = *(pbuf++);
		bytes_parsed++;
		if (bytes_parsed == data_buf_len)
			return -999;
	}
	s[j++] = *(pbuf++);
	bytes_parsed++;
	return j;
}
int get_num_from_buf()
{
	char s[10];
	memset(s, 0, 10);
	int j = 0;
	do
	{
		s[j] = *pbuf;
		pbuf++; j++;
		bytes_parsed++;
		if (bytes_parsed == data_buf_len)
			return -1;
	} while (*pbuf != '\0' && j < 10);
	pbuf++; j++; bytes_parsed++;
	if (j == 0)
		return -999;
	return atoi(s);
}
void buf_shift()
{
	if (data_buf_len == bytes_parsed)
	{
		memset(&data_buf[0], 0, BIGBUFF);
		pbuf = &data_buf[0];
		data_buf_len = bytes_parsed = 0;
	}
	else
	{
		char *p = pbuf;
		int i;
		for (i = 0; i < (data_buf_len - bytes_parsed); i++)
			data_buf[i] = *(p++);
		data_buf_len -= bytes_parsed;
		bytes_parsed = 0;
		pbuf = &data_buf[0];
	}
}
