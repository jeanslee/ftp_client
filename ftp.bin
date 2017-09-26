//
//  main.c
//  ftpClient
//
//  Created by 俞则明 on 14/11/13.
//  Copyright (c) 2014年 ZemingYU. All rights reserved.
//

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/ftp.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define CHECK(b,s) CHECK_LINE_FILE_(b,s,__LINE__,__FILE__)
#define CHECKRET(b,s)CHECK_LINE_FILE_( (b) == 0,s,__LINE__,__FILE__)

#define CHECK_LINE_FILE_(b,s,l,f)  \
do{ \
if (!(b)) \
{ \
fprintf(stderr,"%s:%d %s\n err:%d:%s\n",f,l,s,errno,strerror(errno)); \
exit(EXIT_FAILURE); \
} \
} while(0)

#define SendMsg(fd,str) send(fd,str,strlen(str),0)

int usePASV = 1;

struct sockaddr_in addr;
int port=21;
int sockfd;

char user[BUFSIZ],pass[BUFSIZ];

struct session_t {
    int sockfd;
    int portfd;
    int datafd;
    int state;
    int type;
    struct sockaddr_in dataaddr;
    socklen_t addr_size;
};

int ReadRespose(int sock,char *ret,size_t ret_sz)
{
    int code = -1;
    char buff[BUFSIZ];
    while (1)
    {
        size_t len = 0;
        while (len < BUFSIZ && (buff[len-2]!='\r' || buff[len-1] != '\n'))
        {
            size_t sz;
            while ( (sz = recv(sock,buff+len,1,0)) !=1 )
                if (sz == -1)
                    return -1;
            
            if (buff[len]=='\\')
            {
                recv(sock,buff+len,3,0);
                buff[len+3]='\0';
                sscanf(buff+len,"%d",&code);
                if (code!=377)
                {
                    buff[len] = (unsigned char)code;
                    ++len;
                }
            }
            else
                ++len;
        }
        len -= 2;
        buff[len]='\0';
        printf("%s\n",buff);
        //if (isnumber(buff[0])&&buff[3]==' ')
        if (buff[3]==' ')
        {
            buff[3]='\0';
            sscanf(buff,"%d", &code);
            if (ret!=NULL)
            {
                strncpy(ret, buff+4, ret_sz);
                ret[MIN(len-4,ret_sz)]='\0';
            }
            return code;
        }
    }
    return -1;
}

void send_command_TYPE(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    sprintf(buff,"TYPE %c\r\n",param[0]);
    SendMsg(session->sockfd, buff);
    int code = ReadRespose(session->sockfd, NULL, 0);
    if (code!=200)
        session->type = -1;
    else
        session->type = param[0];
}

void send_command_PASV(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    SendMsg(session->sockfd, "PASV\r\n");
    int code = ReadRespose(session->sockfd, buff, sizeof(buff));
    if (code==227)
    {
        char * tmp = buff;
        int h1,h2,h3,h4,p1,p2;
        //while (!isnumber(*tmp)&&(*tmp)!='\0')
        while ((*tmp)!='\0')
            ++tmp;
        sscanf(tmp,"%d,%d,%d,%d,%d,%d",&h1,&h2,&h3,&h4,&p1,&p2);
        sprintf(buff,"%d.%d.%d.%d",h1,h2,h3,h4);
        inet_pton(AF_INET,buff,&(session->dataaddr.sin_addr));
        session->dataaddr.sin_family = AF_INET;
        session->dataaddr.sin_port = htons(p1*256+p2);
        session->addr_size = sizeof(session->dataaddr);
        session->state = 2;
    
    }
}

void send_command_PORT(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    unsigned short portport = rand() % 40000 + 20000;
    struct sockaddr_in portaddr;
    socklen_t addr_len = sizeof(portaddr);
    unsigned char* s =(void *) &portaddr.sin_addr;
    
    getsockname(session->sockfd,(struct sockaddr *)&portaddr,&(addr_len));
    portaddr.sin_port = htons(portport);
    
    session->portfd = socket(PF_INET,SOCK_STREAM,0);
    
    while (bind(session->portfd,(struct sockaddr *) (&portaddr),sizeof(portaddr))==-1
           && errno == EADDRINUSE)
    {
        portport = rand() % 40000 + 20000;
        portaddr.sin_port = htons(portport);
    }
    
    listen(session->portfd, 10);
    
    sprintf(buff,"PORT %hhu,%hhu,%hhu,%hhu,%d,%d\r\n",s[0],s[1],s[2],s[3],portport/256,portport%256);
    send(session->sockfd, buff,strlen(buff),0);
    
    int code = ReadRespose(session->sockfd, NULL, sizeof(NULL));
    if (code==200)
        session->state = 1;
}

void set_connect_mode(struct session_t *session)
{
    send_command_TYPE(session,"I");
    if (usePASV)
        send_command_PASV(session,NULL);
    if (session->state==0)
        send_command_PORT(session,NULL);

    CHECK(session->state!=0,"PASV/PORT");
}

int get_data_connect(struct session_t *session)
{
    CHECK(session->state!=0,"STATE == 0");
    int code;
    if (session->state==1)
    {   //PORT
        session->datafd= accept(session->portfd,(struct sockaddr*)&(session->dataaddr),&session->addr_size);
    }
    else
    {   //PASV
        session->datafd = socket(PF_INET,SOCK_STREAM,0);
        connect(session->datafd, (struct sockaddr*)&(session->dataaddr), session->addr_size);
    }
    if (session->datafd == -1)
    {
        session->datafd = 0;
        return -1;
    }
    code = ReadRespose(session->sockfd, NULL, 0);
    if (code == 150)
        return session->datafd;
    session->datafd = 0;
    return -1;
}

void close_data_connect(struct session_t *session)
{
    CHECK(session->state!=0,"STATE == 0");
    if (session->state==1)
    {   //PORT
        shutdown(session->portfd, SHUT_RDWR);
        session->portfd = 0;
    }
    shutdown(session->datafd, SHUT_RDWR);
    session->datafd = 0;
    session->state = 0;
}

void send_command_QUIT(struct session_t *session,char *param)
{
    SendMsg(session->sockfd, "QUIT\r\n");
    ReadRespose(session->sockfd, NULL, 0);
    shutdown(session->sockfd, SHUT_RDWR);
    session->sockfd = 0;
    if (session->portfd)
    {
        shutdown(session->portfd, SHUT_RDWR);
    }
    session->state = -1;
}

void send_command_PWD(struct session_t *session,char *param)
{
    SendMsg(session->sockfd, "PWD\r\n");
    ReadRespose(session->sockfd, NULL, 0);
}

void send_command_CD(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    sprintf(buff, "CWD %s\r\n",param);
    SendMsg(session->sockfd, buff);
    ReadRespose(session->sockfd, NULL, 0);
}

void send_command_CDUP(struct session_t *session,char *param)
{
    SendMsg(session->sockfd, "CDUP\r\n");
    ReadRespose(session->sockfd, NULL, 0);
}

void send_command_SYST(struct session_t *session,char *param)
{
    SendMsg(session->sockfd, "SYST\r\n");
    ReadRespose(session->sockfd, NULL, 0);
}

size_t CopyTo(int src_fd,int dst_fd)
{
    char buff[BUFSIZ];
    size_t szrd,szwt,tot=0;
    while ( (szrd = read(src_fd, buff, sizeof(buff)) )>0)
    {
        char * tmp = buff;
        szwt=0;
        while(szrd >0 && (szwt = write(dst_fd, buff, szrd)) !=-1)
        {
            tmp += szwt;
            szrd -= szwt;
            tot += szwt;
        }
        if (szwt == -1) break;
    }
    return tot;
}

void send_command_LIST(struct session_t *session,char *param)
{
    set_connect_mode(session);
    SendMsg(session->sockfd, "LIST\r\n");
    get_data_connect(session);
    CopyTo(session->datafd,STDOUT_FILENO);
    ReadRespose(session->sockfd, NULL, 0);
    close_data_connect(session);
}

void SendFile(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    //int fd = open(param,O_RDONLY|O_SHLOCK);
    int fd = open(param,O_RDONLY);
    if (fd==-1)
    {
        printf("Can't Open local file:%s\n",param);
        return ;
    }
    
    set_connect_mode(session);
    
    sprintf(buff, "STOR %s\r\n",param);
    SendMsg(session->sockfd, buff);
    
    get_data_connect(session);
    CopyTo(fd,session->datafd);
    close_data_connect(session);
    ReadRespose(session->sockfd, NULL, 0);
    close(fd);
}

void RecvFile(struct session_t *session,char *param)
{
    char buff[BUFSIZ];
    //int fd = open(param,O_WRONLY|O_EXLOCK|O_CREAT);
    int fd = open(param,O_WRONLY|O_CREAT);
    if (fd==-1)
    {
        printf("Can't Open local file:%s\n",param);
        return ;
    }
    
    set_connect_mode(session);
    
    sprintf(buff, "RETR %s\r\n",param);
    SendMsg(session->sockfd, buff);
    
    get_data_connect(session);
    CopyTo(session->datafd,fd);
    close_data_connect(session);
    ReadRespose(session->sockfd, NULL, 0);
    close(fd);
}

int main(int argc,char * argv[]) {
    
    if (argc==1)
    {
        printf("usage:%s [-a] ip [port]\n",argv[0]);
        return 0;
    }
    if (strcmp(argv[1],"-a")==0) {
        printf("Use Active Mode.\n");
        usePASV = 0;
        ++argv;
        --argc;
    }
    if (argc == 3)
        sscanf(argv[2], "%d",&port);
    
    inet_pton(AF_INET,argv[1],&(addr.sin_addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sockfd = socket(PF_INET,SOCK_STREAM,0);
    char buff[BUFSIZ];
    
    CHECK(connect(sockfd,(struct sockaddr*)&addr ,sizeof(addr))== 0,"can't connect server.");
    int code;
    
    // Welcome Msg
    code = ReadRespose(sockfd,buff,sizeof(buff));
    CHECK(code == 220,"err:Welcome Msg");
    
    while (1)
    {
    
        printf("USER (anonymous):");
        fgets(user,sizeof(user),stdin);
        size_t userlen = strlen(user);
        if (userlen > 0 && user[userlen-1]=='\n')
            user[--userlen]='\0';
        
        if (strlen(user)==0)
            strcpy(user, "anonymous");
    
        sprintf(buff,"USER %s\r\n",user);
        SendMsg(sockfd,buff);
    
        code = ReadRespose(sockfd,buff,sizeof(buff));
        CHECK(code ==331,"USER");
        
        char *tmppass=getpass("PASS:");
        strcpy(pass, tmppass);
        free(tmppass);
        
        sprintf(buff, "PASS %s\r\n",pass);
        SendMsg(sockfd,buff);

        code = ReadRespose(sockfd,buff,sizeof(buff));
        CHECK(code != -1,"PASS");
        
        if (code==230)
            break;
        if (code==530)
            printf("Try Again\n");
    }
    
    struct session_t session;
    bzero(&session, sizeof(session));
    session.sockfd = sockfd;
    
    send_command_SYST(&session,NULL);
    
    char verb[BUFSIZ],param[BUFSIZ];
    
    while (session.state>=0)
    {
        printf("ftp>");
        scanf("%s",verb);
        if (strcmp(verb, "bye")==0)
            send_command_QUIT(&session,NULL);
        else if (strcmp(verb, "pwd")==0)
            send_command_PWD(&session,NULL);
        else if (strcmp(verb, "cdup")==0)
            send_command_CDUP(&session,NULL);
        else if (strcmp(verb, "ls")==0)
            send_command_LIST(&session,NULL);
        else if (strcmp(verb, "put")==0)
        {
            scanf("%s",param);
            SendFile(&session,param);
        }
        else if (strcmp(verb, "get")==0)
        {
            scanf("%s",param);
            RecvFile(&session,param);
        }
        else if (strcmp(verb, "cd")==0)
        {
            scanf("%s",param);
            send_command_CD(&session,param);
        }
        else
            printf("unknow command:%s\n",verb);
    }
    
    return 0;
}
