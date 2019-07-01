/*
 * comm.h
 *
 *  Created on: Jun 14, 2011
 *      Author: bob
 */

#ifndef COMM_H_
#define COMM_H_
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glib.h>
#include <gio/gio.h>

#include "playing.h"//for the struct tag_data

#define BUFF 65536
#define BIGBUFF 2000000
#define PORT 23529

//socket/data functions
int g_connect_to_server(void);

#endif /* COMM_H_ */
