#ifndef _FTP_H
#define _FTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>


#define PORT 21
#define DOMAIN_NAME "XXX.XXX.com"
#define BUF_SIZE 65536
#define DOWNLOAD_BUF_SIZE 268435456
#define CMD_LEN 64
#define IP_LEN 32

// char download_buf[DOWNLOAD_BUF_SIZE];

void dns(char *domain_name, char *ip);
int get_data_port(char *buf);
int get_file_size(const char *path);
int login(char *ip, char *username, char *password, int *socket_fd, int *data_port);
int login_SSL(char *ip, char *username, char *password, int *socket_fd, int *data_port);
int upload_file(char *ip, int socket_fd, int data_port, char *filename);
int download_file(char *ip, int socket_fd, int data_port, char *filename);
int download_file_multiProc(char *ip, char *username, char *password, int socket_fd, int data_port, char *filename, int num);
int download_file_multiProc_SSL(char *ip, char *username, char *password, int socket_fd, int data_port, char *filename, int num);
int get_file_size_ftp(char *recvbuf);
void test_time(char *ip, char *username, char *password, int *socket_fd, int *data_port, char *filename, int num);


#endif