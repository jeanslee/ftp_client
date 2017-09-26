/*****************************************************************************/
/*  file name: dir_trans.c													 */
/*  author: Tao Tong														 */
/*  function: upload directories with ftp server						 */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "dir_list.h"
#include "dir_trans.h"
#include "file_trans.h"

#define BUFSIZE 1024
extern int sock_control;

int make_remote_dir(char* remote_dir, int sockfd)
{
	int ret_value = 0;
	char cmd[40];
	sprintf(cmd, "MKD %s\r\n", remote_dir);
	ret_value = send_command(cmd, NULL, sockfd);
	if (ret_value == 257)	return (0);
	else					return (-1);
}

int upload_dir(char* local_dir)
{
	int i = 0;
	dir_list* p_dir_list = create_dir_list(1024, 1);
	add_dir_list(p_dir_list, "", local_dir);
	get_dir_list(p_dir_list, local_dir);
	for (i = 0; i < p_dir_list->count; i++)
	{
		if (p_dir_list->list[i]->is_dir)
			make_remote_dir(p_dir_list->list[i]->dir_name, sock_control);
//			printf("%s\n", p_dir_list->list[i]->dir_name);
	}
	for (i = 0; i < p_dir_list->count; i++)
	{
		if (!p_dir_list->list[i]->is_dir)
			upload(p_dir_list->list[i]->dir_name, p_dir_list->list[i]->dir_name);
	}
	clean_dir_list(p_dir_list);
	return (0);
}

