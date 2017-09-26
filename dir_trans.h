#ifndef _DIR_TRANS_H
#define _DIR_TRANS_H

#define BUFSIZE 1024
extern int sock_control;

// make dir in remote server
int make_remote_dir(char* remote_dir, int sockfd);

// upload a local dir and sub dir and sub files to ftp server
int upload_dir(char* local_dir);

#endif
