#include "ftp.h"

void dns(char *domain_name, char *ip)
{
    struct hostent *host;
    host = gethostbyname(domain_name);
    inet_ntop(host->h_addrtype, host->h_addr_list[0], ip, IP_LEN);

}

int get_data_port(char *buf)
{
    char temp[CMD_LEN];
    char *pc_1 = NULL;
    char *pc_2 = NULL;
    int port_low = 0;
    int port_high = 0;
    int port = 0;
    int count = 0;

    //ip&port (ip1,ip2,ip3,ip4,port1,port2)
    //ip = "ip1.ip2.ip3.ip4"
    //port = port1 * 256 + port2
    pc_1 = strstr(buf, "(");
    while (1)
    {
        if (*pc_1 == ',')
        {
            count++;
        }
        pc_1++;
        if (count == 4)
        {
            break;
        }
    }
    pc_2 = strstr(pc_1, ",");
    strncpy(temp, pc_1, pc_2 - pc_1);
    port_high = atoi(temp);
    memset(temp, 0, sizeof(temp));
    pc_1 = ++pc_2;
    pc_2 = strstr(pc_1, ")");
    strncpy(temp, pc_1, pc_2 - pc_1);
    port_low = atoi(temp);
    port = port_high * 256 + port_low;
    return port;
}

int get_file_size(const char *path)
{
    int filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0) 
    {
        return filesize;
    }
    else
    {
        filesize = statbuff.st_size;
    }
    return filesize;
}

int login(char *ip, char *username, char *password, int *socket_fd, int *data_port)
{
    int _socket_fd = -1;
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE];
    char *port_buf = malloc(BUF_SIZE);

    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_socket_fd < 0)
    {
        printf("Fail to create a socket to connect with server: %s\n", strerror(errno));
        return -1;
    }

    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    
    if(connect(_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Fail to connect with the server %s : %s\n", ip, strerror(errno));
        return -1;
    }
    else
    {
        recv(_socket_fd, buf, BUF_SIZE, 0);
        printf("%s\n", buf);
        memset(buf, 0, BUF_SIZE);
    }

    // send(_socket_fd, userstr, strlen(userstr), 0);
    // send(_socket_fd, "USER ", 5, 0);
    // send(_socket_fd, username, strlen(username), 0);
    // send(_socket_fd, "\r\n", 2, 0);
    char userstr[CMD_LEN];
    sprintf(userstr, "USER %s\r\n", username);
    send(_socket_fd, userstr, strlen(userstr), 0);
    recv(_socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    // send(_socket_fd, "PASS ", 5, 0);
    // send(_socket_fd, password, strlen(password), 0);
    // send(_socket_fd, "\r\n", 2, 0);
    char passwordstr[CMD_LEN];
    sprintf(passwordstr, "PASS %s\r\n", password);
    send(_socket_fd, passwordstr, strlen(passwordstr), 0);
    recv(_socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

     //type
    send(_socket_fd, "TYPE I\r\n", strlen("TYPE I\r\n"), 0);
    recv(_socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    send(_socket_fd, "PASV\r\n", strlen("PASV\r\n"), 0);
    recv(_socket_fd, buf, BUF_SIZE, 0);
    printf("%s\n", buf);

    strncpy(port_buf, buf, BUF_SIZE);
    *data_port = get_data_port(port_buf);
    free(port_buf);
    memset(buf, 0, BUF_SIZE);

    *socket_fd = _socket_fd;
    return 1;
}

int upload_file(char *ip, int socket_fd, int data_port, char *filename)
{
    int data_fd = -1;
    int remote_fd = -1;
    struct sockaddr_in data_addr;
    char buf[BUF_SIZE];

    data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(data_fd < 0)
    {
        printf("Fail to create a socket to transmit the file data: %s\n", strerror(errno));
        return -1;
    }

    // send(socket_fd, "STOR ", 5, 0);
    // send(socket_fd, filename, strlen(filename), 0);
    // send(socket_fd, "\r\n", 2, 0);
    char uploadstr[CMD_LEN];
    sprintf(uploadstr, "STOR %s\r\n", filename);
    send(socket_fd, uploadstr, strlen(uploadstr), 0);

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    inet_aton(ip, &data_addr.sin_addr);


    if(connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
    {
        printf("Fail to connect with the data port %s : %s\n", ip, strerror(errno));
        return -1;
    }
    //open local file
    remote_fd = open(filename, O_RDONLY);
    if(remote_fd < 0)
    {
        printf("Fail to open the local file : %s\n", strerror(errno));
        return -1;
    }
    //get filesize
    int filesize = get_file_size(filename);
    if(read(remote_fd, buf, filesize) < 0)
    {
        printf("Fail to read the local file :%s\n", strerror(errno));
        return -1;
    }

    send(data_fd, buf, filesize, 0);
    printf("Upload success.\n");
    return 1;

}

int download_file(char *ip, int socket_fd, int data_port, char *filename)
{
    int data_fd = -1;
    int remote_fd = -1;
    struct sockaddr_in data_addr;
    char buf[BUF_SIZE];
    int filesize = 0;
    // char *download_buf;

    //get filesize
    char sizestr[CMD_LEN];
    sprintf(sizestr, "SIZE %s\r\n", filename);
    send(socket_fd, sizestr, strlen(sizestr), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s", buf);
    filesize = get_file_size_ftp(buf);
    memset(buf, 0, BUF_SIZE);
    // download_buf = (char *)malloc(filesize);

    //download file
    data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(data_fd < 0)
    {
        printf("Fail to create a socket to transmit the file data: %s\n", strerror(errno));
        return -1;
    }

    // send(socket_fd, "RETR ", 5, 0);
    // send(socket_fd, filename, strlen(filename), 0);
    // send(socket_fd, "\r\n", 2, 0);
    // send(socket_fd, "REST 50\r\n",strlen("REST 50\r\n"), 0);
    // recv(socket_fd, buf, BUF_SIZE, 0);
    // printf("%s", buf);
    // memset(buf, 0, BUF_SIZE);

    char downloadstr[CMD_LEN];
    sprintf(downloadstr, "RETR %s\r\n", filename);
    send(socket_fd, downloadstr, strlen(downloadstr), 0);
    // recv(socket_fd, buf, BUF_SIZE, 0);
    // printf("%s", buf);
    // memset(buf, 0, BUF_SIZE);

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    inet_aton(ip, &data_addr.sin_addr);

    if(connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
    {
        printf("Fail to connect with the data port %s : %s\n", ip, strerror(errno));
        return -1;
    }
    remote_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664);
    if(remote_fd < 0)
    {
        printf("Fail to open the local file : %s\n", strerror(errno));
        return -1;
    }
    int sum = 0;
    int rec;
    do
    {
        rec = recv(data_fd, buf, BUF_SIZE, 0);
        printf("%d\n", rec);
        sum += rec;
        write(remote_fd, buf, rec);
        memset(buf, 0, BUF_SIZE);
    } while (sum < filesize);
    
    // free(download_buf);
    printf("Download success!\n");
    return 1;

}

int download_file_multiProc(char *ip, char *username, char *password, int socket_fd, int data_port, char *filename, int num)
{
    time_t begin = time((time_t *)NULL);
    int filesize = 0;
    pid_t pid;
    char buf[BUF_SIZE];
    // char aa[DOWNLOAD_BUF_SIZE];
    //get filesize
    char sizestr[CMD_LEN];
    sprintf(sizestr, "SIZE %s\r\n", filename);
    send(socket_fd, sizestr, strlen(sizestr), 0);
    recv(socket_fd, buf, BUF_SIZE, 0);
    printf("%s", buf);
    filesize = get_file_size_ftp(buf);
    memset(buf, 0, BUF_SIZE);

    close(socket_fd);

    // send(socket_fd, "REST 100\r\n", strlen("REST 100\r\n"), 0);
    // recv(socket_fd, buf, BUF_SIZE, 0);
    // printf("%s", buf);
    // memset(buf, 0, BUF_SIZE);

    int bs = filesize / num;
    int mod = filesize % bs;

    int remote_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664);
    ftruncate(remote_fd, filesize);
    char *mp_download = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, remote_fd, 0);
    char *tmp_dlm = mp_download;
    int i;

    for(i = 0; i < num; i++)
    {
        if((pid = fork()) == 0)
        {
            break;
        }
    }
    if(i == num)
    {
        // free(download_buf);
        for(i = 0; i < num; i++)
        {
            wait(NULL);
        }
        printf("bs: %d,bs + mod: %d\n", bs, bs + mod);
        // printf("%s", mp_download);
        printf("Download success!\n");
        time_t end = time((time_t *)NULL);
        printf("Used time: %ld\n", (end - begin));
    }
    else if(i < num - 1)
    {
        int _socket_fd = -1;
        int _data_port = -1;
        // char *download_buf = malloc(bs + 1);
        login(ip, username, password, &_socket_fd, &_data_port);
        // printf("%d, %d, %d",socket_fd, _socket_fd, _data_port);
        int data_fd = -1;
        struct sockaddr_in data_addr;
        // char _buf[BUF_SIZE];

        data_fd = socket(AF_INET, SOCK_STREAM, 0);
        // printf("%d", data_fd);
        if(data_fd < 0)
        {
            printf("Fail to create a socket to transmit the file data: %s\n", strerror(errno));
            return -1;
        }

        char reststr[CMD_LEN];
        sprintf(reststr, "REST %d\r\n", i * bs);
        // send(_socket_fd, "REST 0\r\n", strlen("REST 0\r\n"), 0);
        send(_socket_fd, reststr, strlen(reststr), 0);
        recv(_socket_fd, buf, BUF_SIZE, 0);
        printf("%s", buf);
        memset(buf, 0, BUF_SIZE);

        char downloadstr[CMD_LEN];
        sprintf(downloadstr, "RETR %s\r\n", filename);
        send(_socket_fd, downloadstr, strlen(downloadstr), 0);

        // printf("%d\n", data_fd);

        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(_data_port);
        inet_aton(ip, &data_addr.sin_addr);


        if(connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
        {
            printf("Fail to connect with the data port %s : %s\n", ip, strerror(errno));
            return -1;
        }

        // printf("%d\n", data_fd);

        int sum = 0;
        do
        {
            int rec = recv(data_fd, buf, BUF_SIZE, 0);
            // printf("%d: %d\n", i, rec);
            int lastsum = sum;
            sum += rec;
            printf("%d: %d\n", i, sum);
            if(sum > bs)
            {
                memcpy(tmp_dlm + i * bs + lastsum, buf, rec - (sum - bs));
            }
            else
            {
                memcpy(tmp_dlm + i * bs + lastsum, buf, rec);
            }
            memset(buf, 0, BUF_SIZE); 
        } while (sum < bs);

        // int rec = recv(data_fd, download_buf, bs, 0);
        // printf("%d: %d\n", i, rec);
        // memcpy(tmp_dlm + i * bs, download_buf, bs);
        // printf("%s", tmp_dlm);
        // free(download_buf);
        close(_socket_fd);
        close(data_fd);
    }
    else if(i == num - 1)
    {
        int _socket_fd = -1;
        int _data_port = -1;
        // char *download_buf = malloc(bs + mod + 1);
        login(ip, username, password, &_socket_fd, &_data_port);
        int data_fd = -1;
        struct sockaddr_in data_addr;
        // char _buf[BUF_SIZE];

        data_fd = socket(AF_INET, SOCK_STREAM, 0);
        // printf("%d", data_fd);
        if(data_fd < 0)
        {
            printf("Fail to create a socket to transmit the file data: %s\n", strerror(errno));
            return -1;
        }

        char reststr[CMD_LEN];
        int a = 50;
        sprintf(reststr, "REST %d\r\n", i * bs);
        // send(_socket_fd, "REST 0\r\n", strlen("REST 0\r\n"), 0);
        send(_socket_fd, reststr, strlen(reststr), 0);
        recv(_socket_fd, buf, BUF_SIZE, 0);
        printf("%s", buf);
        memset(buf, 0, BUF_SIZE);

        char downloadstr[CMD_LEN];
        sprintf(downloadstr, "RETR %s\r\n", filename);
        send(_socket_fd, downloadstr, strlen(downloadstr), 0);

        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(_data_port);
        inet_aton(ip, &data_addr.sin_addr);

        // printf("%d\n", data_fd);

        if(connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)
        {
            printf("Fail to connect with the data port %s : %s\n", ip, strerror(errno));
            return -1;
        }

        // printf("%d\n", data_fd);
        
        int sum = 0;
        do
        {
            int rec = recv(data_fd, buf, BUF_SIZE, 0);
            // printf("%d: %d\n", i, rec);
            int lastsum = sum;
            sum += rec;
            printf("%d: %d\n", i, sum);
            memcpy(tmp_dlm + i * bs + lastsum, buf, rec);
            memset(buf, 0, BUF_SIZE);
        } while (sum < (bs + mod));

        // int rec = recv(data_fd, download_buf, bs + mod, 0);
        // printf("%d: %d\n", i, rec);
        // memcpy(tmp_dlm + i * bs, download_buf, bs + mod);
        // printf("%s", tmp_dlm);
        // free(download_buf);
        close(_socket_fd);
        close(data_fd);
    }
    munmap(mp_download, filesize);
}

int get_file_size_ftp(char *recvbuf)
{
    char temp[CMD_LEN];
    char *pc_1 = NULL;
    char *pc_2 = NULL;
    int filesize = 0;

    //213 filesize\r\n
    pc_1 = strstr(recvbuf," ");
    pc_1++;
    pc_2 = strstr(pc_1, "\r");
    strncpy(temp, pc_1, pc_2 - pc_1);
    filesize = atoi(temp);
    return filesize;
}

void test_time(char *ip, char *username, char *password, int *socket_fd, int *data_port, char *filename, int num)
{
    login(ip, username, password, socket_fd, data_port);
    download_file_multiProc(ip, username, password, *socket_fd, *data_port, filename, num);
}