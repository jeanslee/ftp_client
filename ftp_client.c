#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <curses.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "dir_list.h"
#include "dir_trans.h"
#include "file_trans.h"

#define DEFAULT_FTP_PORT 21

extern int h_errno;

char user[64]; //ftp usr
char passwd[64]; //ftp passwd

//ftp server address
struct sockaddr_in ftp_server, local_host;
struct hostent * server_hostent;

int sock_control;
int mode = 1; //ftp mode, 0 is PORT, 1 is PASV;

//echo_off and echo_on for get usr password from stdin
static struct termios stored_settings;
void echo_off(void)
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= (~ECHO);
    tcsetattr(0,TCSANOW,&new_settings);
    return;
}
void echo_on(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}

void cmd_err_exit(char * err_msg, int err_code)
{
	printf("%s\n", err_msg);
	exit(err_code);
}

int fill_host_addr(char * host_ip_addr, struct sockaddr_in * host, int port)
{
	if(port <= 0 || port > 65535) 
		return 254;
	bzero(host, sizeof(struct sockaddr_in));
	host->sin_family = AF_INET;
        if(inet_addr(host_ip_addr) != -1)
	{
                host->sin_addr.s_addr = inet_addr(host_ip_addr);
	}
        else 
	{
		if((server_hostent = gethostbyname(host_ip_addr)) != 0)
		{
			memcpy(&host->sin_addr, server_hostent->h_addr,\
			        sizeof(host->sin_addr));
		}
	        else return 253;
	}
        host->sin_port = htons(port);
	return 1;
}

int xconnect(struct sockaddr_in *s_addr, int type)
{
	struct timeval outtime;
	int set;
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
		cmd_err_exit("creat socket error!", 249);

	//set outtime for the control socket
	if(type == 1)
	{
		outtime.tv_sec = 0;
		outtime.tv_usec = 300000;
	}
	else
	{
		outtime.tv_sec = 5;
		outtime.tv_usec = 0;
	}
	set = setsockopt(s, SOL_SOCKET,SO_RCVTIMEO, &outtime,sizeof(outtime));
	if(set !=0)
	{
		printf("set socket %s errno:%d\n",strerror(errno),errno);
		cmd_err_exit("set socket", 1);
	}

	//connect to the server
	if (connect(s,(struct sockaddr *)s_addr,sizeof(struct sockaddr_in)) < 0)
	{
		printf("Can't connect to server %s, port %d\n",\
			inet_ntoa(s_addr->sin_addr),ntohs(ftp_server.sin_port));
		exit(252);
	}
	return s;
}

//send command to server with sock_fd
int ftp_send_cmd(const char *s1, const char *s2, int sock_fd)
{
	char send_buf[256];
	int send_err, len;
	if(s1) 
	{
		strcpy(send_buf,s1);
		if(s2)
		{	
			strcat(send_buf, s2);
			strcat(send_buf,"\r\n");
			len = strlen(send_buf);
			send_err = send(sock_fd, send_buf, len, 0);
		}
		else 
		{
			strcat(send_buf,"\r\n");
			len = strlen(send_buf);
			send_err = send(sock_fd, send_buf, len, 0);
		}
    	}
	if(send_err < 0)
		printf("send() error!\n");
	return send_err;
}

//get the server's reply message from sock_fd
int ftp_get_reply(int sock_fd)
{
	static int reply_code = 0,count=0;
	char rcv_buf[512];
	count=read(sock_fd, rcv_buf, 510);
	if(count > 0)
		reply_code = atoi(rcv_buf);
	else
		return 0;
	while(1)
	{
		if(count <= 0)
			break;
		rcv_buf[count]='\0';
		printf("%s",rcv_buf);
		count=read(sock_fd, rcv_buf, 510);
	}
	return reply_code;
}

int get_port()
{
	char port_respond[512];
	char *buf_ptr;
	int count,port_num;
	ftp_send_cmd("PASV",NULL,sock_control);
	count = read(sock_control, port_respond, 510);
	if(count <= 0)
		return 0;
	port_respond[count]='\0';
	if(atoi(port_respond) == 227)
	{
		//get low byte of the port
		buf_ptr = strrchr(port_respond, ',');
		port_num = atoi(buf_ptr + 1);
		*buf_ptr = '\0';
		//get high byte of the port
		buf_ptr = strrchr(port_respond, ',');
		port_num += atoi(buf_ptr + 1) * 256;
		return port_num;
	}
	return 0;
}

int rand_local_port()
{
	int local_port;
	srand((unsigned)time(NULL));
	local_port = rand() % 40000 + 1025;
	return local_port;
}

//connect data stream
int xconnect_ftpdata()
{
	if(mode)
	{
		int data_port = get_port();
		if(data_port != 0)
			ftp_server.sin_port=htons(data_port);
		return(xconnect(&ftp_server, 0));
	}
	else
	{
		int client_port, get_sock, opt, set;
		char cmd_buf[32];
		struct timeval outtime;
		struct sockaddr_in local;
		char local_ip[24];
		char *ip_1, *ip_2, *ip_3, *ip_4;
		int addr_len =  sizeof(struct sockaddr);
		client_port = rand_local_port();
		get_sock = socket(AF_INET, SOCK_STREAM, 0);
		if(get_sock < 0)
		{
			cmd_err_exit("socket()", 1);
		}

		//set outtime for the data socket
		outtime.tv_sec = 7;
		outtime.tv_usec = 0;
		opt = SO_REUSEADDR;
		set = setsockopt(get_sock, SOL_SOCKET,SO_RCVTIMEO, \
				&outtime,sizeof(outtime));
		if(set !=0)
		{
			printf("set socket %s errno:%d\n",strerror(errno),errno);
			cmd_err_exit("set socket", 1);
		}
		set = setsockopt(get_sock, SOL_SOCKET,SO_REUSEADDR, \
				&opt,sizeof(opt));
		if(set !=0)
		{
			printf("set socket %s errno:%d\n",strerror(errno),errno);
			cmd_err_exit("set socket", 1);
		}

		bzero(&local_host,sizeof(local_host));
		local_host.sin_family = AF_INET;
		local_host.sin_port = htons(client_port);
		local_host.sin_addr.s_addr = htonl(INADDR_ANY);
		bzero(&local, sizeof(struct sockaddr));
		while(1)
		{
			set = bind(get_sock, (struct sockaddr *)&local_host, \
					sizeof(local_host));
			if(set != 0 && errno == 11)
			{
				client_port = rand_local_port();
				continue;
			}
			set = listen(get_sock, 1);
			if(set != 0 && errno == 11)
			{
				cmd_err_exit("listen()", 1);
			}
			//get local host's ip
			if(getsockname(sock_control,(struct sockaddr*)&local,\
                               (socklen_t *)&addr_len) < 0)
				return -1;
			snprintf(local_ip, sizeof(local_ip), inet_ntoa(local.sin_addr));
			//change the format to the PORT command needs.
			local_ip[strlen(local_ip)]='\0';
			ip_1 = local_ip;
			ip_2 = strchr(local_ip, '.');
			*ip_2 = '\0';
			ip_2++;
			ip_3 = strchr(ip_2, '.');
			*ip_3 = '\0';
			ip_3++;
			ip_4 = strchr(ip_3, '.');
			*ip_4 = '\0';
			ip_4++;
			snprintf(cmd_buf, sizeof(cmd_buf), "PORT %s,%s,%s,%s,%d,%d", \
			ip_1, ip_2, ip_3, ip_4,	client_port >> 8, client_port&0xff);
			ftp_send_cmd(cmd_buf, NULL, sock_control);
			if(ftp_get_reply(sock_control) != 200)
			{
				printf("Can not use PORT mode!Please use \"mode\" change to PASV mode.\n");
				return -1;
			}
			else
				return get_sock;
		}
	}
}


//deal with the "list" command
void ftp_list()
{	
	int i = 0,new_sock;
	int set = sizeof(local_host);
	int list_sock_data = xconnect_ftpdata();
	if(list_sock_data < 0)
	{
		ftp_get_reply(sock_control);
		printf("creat data sock error!\n");
		return;
	}
	ftp_get_reply(sock_control);
	ftp_send_cmd("LIST", NULL, sock_control);
	ftp_get_reply(sock_control);
	if(mode)
		ftp_get_reply(list_sock_data);
	else
	{
		while(i < 3)
		{
			new_sock = accept(list_sock_data, (struct sockaddr *)&local_host, \
				(socklen_t *)&set);
			if(new_sock == -1)
			{
				printf("accept return:%s errno: %d\n", strerror(errno),errno);
				i++;
				continue;
			}
			else break;
		}
		if(new_sock == -1)
		{
			printf("Sorry, you can't use PORT mode. There is something wrong when the server connect to you.\n");
			return;
		}
		ftp_get_reply(new_sock);
		close(new_sock);
	}

	close(list_sock_data);
	ftp_get_reply(sock_control);
}

//get filename(s) from user's command
void ftp_cmd_filename(char * usr_cmd, char * src_file, char * dst_file)
{	
	int length,  flag = 0;
	int i = 0, j = 0;
	char * cmd_src;
	cmd_src = strchr(usr_cmd, ' ');
	if(cmd_src == NULL)
	{
		printf("command error!\n");
		return;
	}
	else
	{
		while(*cmd_src == ' ')
			cmd_src ++;
	}
	if(cmd_src == NULL || cmd_src == '\0')
	{
		printf("command error!\n");
	}
	else
	{	
		length = strlen(cmd_src);
		while(i <= length)//be careful with space in the filename
		{	
			if((*(cmd_src+i)) !=' ' && (*(cmd_src+i)) != '\\')
			{
				if(flag == 0)
					src_file[j] = *(cmd_src +i);
				else
					dst_file[j] = *(cmd_src +i);
				j++;
			}
			if((*(cmd_src+i)) == '\\' && (*(cmd_src+i+1)) !=' ')
			{
				if(flag == 0)
					src_file[j] = *(cmd_src +i);
				else
					dst_file[j] = *(cmd_src +i);
				j++;
			}
			if((*(cmd_src+i)) == ' ' && (*(cmd_src+i-1)) != '\\')
			{
				src_file[j] = '\0';
				flag = 1;
				j = 0;
			}
			if((*(cmd_src+i)) == '\\' && (*(cmd_src+i+1)) == ' ')
			{
				if(flag == 0)
					src_file[j] = ' ';
				else
					dst_file[j] = ' ';
				j++;
			}
			i++;
		};
	}
	if(flag == 0)
		strcpy(dst_file, src_file);
	else
		dst_file[j] = '\0';
}

//deal with the "get" command
void ftp_get(char * usr_cmd)
{
	char src_file[512];
	char dst_file[512];
	char* pos = NULL;
	char* real_dst = dst_file;
	ftp_cmd_filename(usr_cmd, src_file, dst_file);
	if (NULL != (pos = strrchr(dst_file, '/')))
		real_dst = ++pos;
	download(src_file, real_dst);
}

//deal with "put" command
void ftp_put(char * usr_cmd)
{	
	char src_file[512];
	char dst_file[512];
	char* pos = NULL;
	char* real_dst = dst_file;
	ftp_cmd_filename(usr_cmd, src_file, dst_file);
	if (local_is_dir(src_file))
	{
		upload_dir(src_file);
	}
	else
	{
		if (NULL != (pos = strrchr(dst_file, '/')))
			real_dst = ++pos;
		upload(src_file, real_dst);
	}
}

//call this function to quit
void ftp_quit()
{
	ftp_send_cmd("QUIT",NULL,sock_control);
	ftp_get_reply(sock_control);
	close(sock_control);
}

//tell the user what current directory is in the server
void ftp_pwd()
{
	ftp_send_cmd("PWD", NULL, sock_control);
	ftp_get_reply(sock_control);
}

//change the directory in the server
void ftp_cd(char * usr_cmd)
{
	char *cmd = strchr(usr_cmd, ' ');
	char path[1024];
	if(cmd == NULL)
	{
		printf("command error!\n");
		return;
	}
	else
	{
		while(*cmd == ' ')
			cmd ++;
	}
	if(cmd == NULL || cmd == '\0')
	{
		printf("command error!\n");
		return;
	}
	else
	{
		strncpy(path, cmd, strlen(cmd));
		path[strlen(cmd)]='\0';
		ftp_send_cmd("CWD ", path, sock_control);
		ftp_get_reply(sock_control);
	}
}

//list files and directories in local host
void local_list()
{
	DIR * dp;
	struct dirent *dirp;
	if((dp = opendir("./")) == NULL)
	{
		printf("opendir() error!\n");
		return;
	}
	printf("Local file list:\n");
	while((dirp = readdir(dp)) != NULL)
	{
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;
		printf("%s\n", dirp->d_name);
	}
}

//print local current directory
void local_pwd()
{
	char curr_dir[512];
	int size = sizeof(curr_dir);
	if(getcwd(curr_dir, size) == NULL)
		printf("getcwd failed\n");
	else
		printf("Current local directory: %s\n", curr_dir);
}

//change local directory
void local_cd(char * usr_cmd)
{
	char *cmd = strchr(usr_cmd, ' ');
	char path[1024];
	if(cmd == NULL)
	{
		printf("command error!\n");
		return;
	}
	else
	{
		while(*cmd == ' ')
			cmd ++;
	}
	if(cmd == NULL || cmd == '\0')
	{
		printf("command error!\n");
		return;
	}
	else
	{
		strncpy(path, cmd, strlen(cmd));
		path[strlen(cmd)]='\0';
		if(chdir(path) < 0)
			printf("Local: chdir to %s error!\n", path);
		else
			printf("Local: chdir to %s\n", path);
	}
}

void show_help()
{	
	printf("\033[32mhelp\033[0m\t--print this command list\n");
	printf("\033[32mpwd\033[0m\t--print the current directory of server\n");
	printf("\033[32mlist\033[0m\t--list the files and directoris in current directory of server\n");
	printf("\033[32mcd [directory]\033[0m\n\t--enter <directory> of server\n");
	printf("\033[32mmode\033[0m\n\t--change current mode, PORT or PASV\n");
	printf("\033[32mput [local_file] <file_name>\033[0m\n\t--send [local_file] to server as <file_name>\n");
	printf("\tif <file_name> isn't given, it will be the same with [local_file] \n");
	printf("\tif there is any \' \' in <file_name>, write like this \'\\ \'\n");
	printf("\033[32mget [remote file] <file_name>\033[0m\n\t--get [remote file] to local host as<file_name>\n");
	printf("\tif <file_name> isn't given, it will be the same with [remote_file] \n");
	printf("\tif there is any \' \' in <file_name>, write like this \'\\ \'\n");
	printf("\033[32mlpwd\033[0m\t--print the current directory of local host\n");
	printf("\033[32mllist\033[0m\t--list the files and directoris in current directory of local host\n");
	printf("\033[32mlcd [directory]\033[0m\n\t--enter <directory> of localhost\n");
	printf("\033[32mquit\033[0m\t--quit this ftp client program\n");
}

//get user and password for login
void get_user()
{
	char read_buf[64];
	printf("User(Press <Enter> for anonymous): ");
	fgets(read_buf, sizeof(read_buf), stdin);
	if(read_buf[0]=='\n')
		strncpy(user, "anonymous", 9);
	else
		strncpy(user, read_buf, strlen(read_buf)-1);
}
void get_pass()
{
	char read_buf[64];
	printf("Password(Press <Enter> for anonymous): ");
	echo_off();
	fgets(read_buf, sizeof(read_buf), stdin);
	if(read_buf[0]=='\n')
		strncpy(passwd, "anonymous", 9);
	else
		strncpy(passwd, read_buf, strlen(read_buf)-1);
	echo_on();
	printf("\n");
}

//login to the server
int ftp_login()
{
	int err;
	get_user();
	if(ftp_send_cmd("USER ", user, sock_control) < 0)
		cmd_err_exit("Can not send message",1);;
	err = ftp_get_reply(sock_control);
	if(err == 331)
	{
		get_pass();
		if(ftp_send_cmd("PASS ", passwd, sock_control) <= 0)
			cmd_err_exit("Can not send message",1);
		else
			err = ftp_get_reply(sock_control);
		if(err != 230)
		{
			printf("Password error!\n");
			return 0;
		}
		return 1;
	}
	else
	{
		printf("User error!\n");
		return 0;
	}
}

//deal with user's command
int ftp_usr_cmd(char * usr_cmd)
{
	if(!strncmp(usr_cmd,"list",4))
		return 1;
	if(!strncmp(usr_cmd,"pwd",3))
		return 2;
	if(!strncmp(usr_cmd,"cd ",3))
		return 3;
	if(!strncmp(usr_cmd,"put ",4))
		return 4;
	if(!strncmp(usr_cmd,"get ",4))
		return 5;
	if(!strncmp(usr_cmd,"quit",4))
		return 6;
	if(!strncmp(usr_cmd,"mode",4))
		return 7;
	if(!strncmp(usr_cmd,"llist",5))
		return 11;
	if(!strncmp(usr_cmd,"lpwd",4))
		return 12;
	if(!strncmp(usr_cmd,"lcd ",4))
		return 13;
	return -1;
}

int start_ftp_cmd(char * host_ip_addr, int port) 
{
	int err;
	int cmd_flag;
	char usr_cmd[1024];
	err = fill_host_addr(host_ip_addr, &ftp_server, port);
	if(err == 254)
		cmd_err_exit("Invalid port!",254);
	if(err == 253)
		cmd_err_exit("Invalid server address!",253);

	sock_control = xconnect(&ftp_server,1);
	if((err =  ftp_get_reply(sock_control)) != 220)
		cmd_err_exit("Connect error!",220);
	do
	{
		err = ftp_login();
	}while(err != 1);

	while(1)
	{
		printf("ftp_client>");
		fgets(usr_cmd, 510, stdin);
		fflush(stdin);
		if(usr_cmd[0] == '\n')
			continue;
		usr_cmd[strlen(usr_cmd)-1] = '\0';
		cmd_flag = ftp_usr_cmd(usr_cmd);
		switch(cmd_flag)
		{
			case 1:
				ftp_list();
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 2:
				ftp_pwd();
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 3:
				ftp_cd(usr_cmd);
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 4:
				ftp_put(usr_cmd);
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 5:
				ftp_get(usr_cmd);
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 6:
				ftp_quit();
				exit(0);
			case 7:
				mode = (mode + 1)%2;
				if(mode)
					printf("change mode to PASV\n");
				else
					printf("change mode to PORT\n");
				break;
			case 11:
				local_list();
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 12:
				local_pwd();
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			case 13:
				local_cd(usr_cmd);
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
			default:
				show_help();
				memset(usr_cmd, '\0',sizeof(usr_cmd));
				break;
		}
	}
	return 1;
}

//main process
int main(int argc, char * argv[])
{
	if(argc != 2 && argc != 3)
	{
		printf("Usage: %s <ftp server ip> [port]\n",argv[0]);
		exit(1);
	}
	else
	{
		if(argv[2]==NULL)
			start_ftp_cmd(argv[1], DEFAULT_FTP_PORT);
		else
			start_ftp_cmd(argv[1], atol(argv[2]));
	}
	return 1;
}
