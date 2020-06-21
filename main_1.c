#include "ftp.h"

int main(int argc, char **argv)
{
    int socket_fd = -1;
    int data_fd = -1;
    int remote_fd = -1;
    // char *ip = malloc(IP_LEN);
    // char *ip = "106.15.207.47";
    char *ip = "127.0.0.1";
    struct sockaddr_in server_addr;
    struct sockaddr_in data_addr;
    char buf[BUF_SIZE];
    char *port_buf = malloc(BUF_SIZE);
    char cmd[CMD_LEN];
    int data_port = 0;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0)
    {
        printf("Fail to create a socket to connect with server: %s\n", strerror(errno));
        return -1;
    }
    data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(data_fd < 0)
    {
        printf("Fail to create a socket to transmit the file data: %s\n", strerror(errno));
        return -1;
    }

    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    
    if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Fail to connect with the server %s : %s\n", ip, strerror(errno));
        return -1;
    }
    else
    {
        recv(socket_fd, buf, BUF_SIZE, 0);
        printf("%s\n", buf);
        memset(buf, 0, BUF_SIZE);
    }
    
    //user name
    send(socket_fd, "USER kerry\r\n", strlen("USER kerry\r\n"), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    //password
    send(socket_fd, "PASS 789789\r\n", strlen("PASS 789789\r\n"), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    //type
    send(socket_fd, "TYPE A\r\n", strlen("TYPE A\r\n"), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    //pasv
    send(socket_fd, "PASV\r\n", strlen("PASV\r\n"), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);

    //get port that the server open
    strncpy(port_buf, buf, BUF_SIZE);
    data_port = get_data_port(port_buf);
    free(port_buf);
    memset(buf, 0, BUF_SIZE);

    //upload file
    send(socket_fd, "STOR test.txt\r\n", strlen("STOR test.txt\r\n"), 0);

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    inet_aton(ip, &data_addr.sin_addr);

    if(connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
    {
        printf("Fail to connect with the data port %s : %s\n", ip, strerror(errno));
        return -1;
    }
    remote_fd = open("./test.txt", O_RDONLY);
    if(remote_fd < 0)
    {
        printf("Fail to open the local file : %s\n", strerror(errno));
        return -1;
    }
    int filesize = get_file_size("./test.txt");
    if(read(remote_fd, buf, filesize) < 0)
    {
        printf("Fail to read the local file :%s\n", strerror(errno));
        return -1;
    }

    send(data_fd, buf, filesize, 0);
    printf("Upload success.\n");

    return 0;
}