#include "ftp.h"

int main(int argc, char *argv[])
{
    int socket_fd;
    int data_port;
    char ip[CMD_LEN];
    char username[CMD_LEN];
    char password[CMD_LEN];
    char filename[CMD_LEN];
    int p;
    if(argc > 1 && !strcmp(argv[1], "-testtime"))
    {
        test_time("127.0.0.1", "kerry", "789789", &socket_fd, &data_port, "ftptest.zip", 5);
    }
    else
    {
        printf("Please input your ip:\n");
        scanf("%s", ip);
        printf("Please input your username:\n");
        scanf("%s", username);
        printf("Please input your password:\n");
        scanf("%s", password);
        login(ip, username, password, &socket_fd, &data_port);
        printf("Please input file name:\n");
        scanf("%s", filename);
        printf("Please input how many processes you need:\n");
        scanf("%d", &p);
        // clock_t begin, end;
        // begin = clock();
        // upload_file(ip, socket_fd, data_port, filename);
        // download_file(ip, socket_fd, data_port, filename);
        download_file_multiProc(ip, username, password, socket_fd, data_port, filename, p);
        // end = clock();
        // double used_time = (double)(end - begin) / CLOCKS_PER_SEC;
        // printf("Download time: %f\n", used_time);
    }
    return 0;
}