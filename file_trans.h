#ifndef _FILE_TRANS_H
#define _FILE_TRANS_H

#define BUFSIZE 1024
extern int sock_control;

// send ftp command to the server, buf argument is to store reply
int send_command(char* cmd, char* buf, int sockfd);

// create data socket to translate files
int create_data_sock();

// TYPE I or TYPE A, for image mode or ascii mode
// argument mode: 1 for TYPE I, 0 for TYPE A
int set_bin_mode(int mode, int sockfd);

// copy file from srcfd file describe to destfd file describe
int file_copy(int srcfd, int destfd, int* psize);

// get remote file size
long get_remote_file_size(char* remote_file, int sockfd);

// down load remote file
int download(char* remote_file, char* local_file);

//upload local file to remote
int upload(char* local_file, char* remote_file);


#endif
