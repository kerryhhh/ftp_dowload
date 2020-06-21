#include "ftp.h"
#include <resolv.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

void ShowCerts(SSL * ssl);

int main()
{
    int socket_fd;
    int data_port;
    char ip[CMD_LEN];
    char username[CMD_LEN];
    char password[CMD_LEN];
    char filename[CMD_LEN];
    int p;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    printf("Please input your ip:\n");
    scanf("%s", ip);
    printf("Please input your username:\n");
    scanf("%s", username);
    printf("Please input your password:\n");
    scanf("%s", password);
    login_SSL(ip, username, password, &socket_fd, &data_port);
    printf("Please input file name:\n");
    scanf("%s", filename);
    printf("Please input how many processes you need:\n");
    scanf("%d", &p);
    // clock_t begin, end;
    // begin = clock();
    // upload_file(ip, socket_fd, data_port, filename);
    // download_file(ip, socket_fd, data_port, filename);
    // download_file_multiProc_SSL(ip, username, password, socket_fd, data_port, filename, p);
    // end = clock();
    // double used_time = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Download time: %f\n", used_time);

    return 0;
}

int login_SSL(char *ip, char *username, char *password, int *socket_fd, int *data_port)
{
    int _socket_fd = -1;
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE];
    char *port_buf = malloc(BUF_SIZE);
    SSL_CTX *ctx;
    SSL *ssl;

     
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

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
    // else
    // {
    //     recv(_socket_fd, buf, BUF_SIZE, 0);
    //     printf("%s\n", buf);
    //     memset(buf, 0, BUF_SIZE);
    // }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, _socket_fd);
    if(SSL_connect(ssl) == -1)
    {
        ERR_print_errors_fp(stderr);
    }
    else
    {
        printf("connected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);
    }
    

    // send(_socket_fd, userstr, strlen(userstr), 0);
    // send(_socket_fd, "USER ", 5, 0);
    // send(_socket_fd, username, strlen(username), 0);
    // send(_socket_fd, "\r\n", 2, 0);
    char userstr[CMD_LEN];
    sprintf(userstr, "USER %s\r\n", username);
    SSL_write(ssl, userstr, strlen(userstr));
    SSL_read(ssl, buf, BUF_SIZE);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    // send(_socket_fd, "PASS ", 5, 0);
    // send(_socket_fd, password, strlen(password), 0);
    // send(_socket_fd, "\r\n", 2, 0);
    char passwordstr[CMD_LEN];
    sprintf(passwordstr, "PASS %s\r\n", password);
    SSL_write(ssl, passwordstr, strlen(passwordstr));
    SSL_read(ssl, buf, BUF_SIZE);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

     //type
    SSL_read(ssl, "TYPE I\r\n", strlen("TYPE I\r\n"));
    SSL_write(ssl, buf, BUF_SIZE);
    printf("%s\n", buf);
    memset(buf, 0, BUF_SIZE);

    SSL_write(ssl, "PASV\r\n", strlen("PASV\r\n"));
    SSL_read(ssl, buf, BUF_SIZE);
    printf("%s\n", buf);

    strncpy(port_buf, buf, BUF_SIZE);
    *data_port = get_data_port(port_buf);
    free(port_buf);
    memset(buf, 0, BUF_SIZE);

    *socket_fd = _socket_fd;
    return 1;
}

int download_file_multiProc_SSL(char *ip, char *username, char *password, int socket_fd, int data_port, char *filename, int num)
{
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

void ShowCerts(SSL * ssl) 
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("Digital certificate information:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Certificate: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
    printf("No certificate information！\n");
}