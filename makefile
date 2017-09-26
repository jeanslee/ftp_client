CC = gcc
CFLAGS = -g -Wall
all: ftp_client
ftp_client: ftp_client.o file_trans.o dir_trans.o dir_list.o
	$(CC) $(CFLAGS) -o ftp_client ftp_client.o file_trans.o dir_trans.o dir_list.o
ftp_client.o: ftp_client.c dir_list.c dir_list.h dir_trans.c dir_trans.h file_trans.c file_trans.h
	$(CC) $(CFLAGS) -c ftp_client.c
file_trans.o: file_trans.c ftp_client.c file_trans.h
	$(CC) $(CFLAGS) -c file_trans.c
dir_trans.o: dir_trans.c dir_list.c dir_trans.h dir_list.h
	$(CC) $(CFLAGS) -c dir_trans.c
dir_list.o: dir_list.c dir_list.h
	$(CC) $(CFLAGS) -c dir_list.c

clean:
	rm -f *.o
	rm -f ftp_client

