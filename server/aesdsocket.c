#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <syslog.h>
#include <stdint.h>
#include <time.h>

#include "aesd_thread_list.h"

#define PORT "9000"
#define BACKLOG 10      // how many pending connections queue will hold
#define MAXDATASIZE 100 // max number of bytes we can get at once
#define LOG_FILE "/var/tmp/aesdsocketdata"

#define SERVER_SYSLOG_CONNECT 0
#define SERVER_SYSLOG_DISCONNECT 1
#define SERVER_SYSLOG_EXIT 2

bool should_exit = false;
AESDThreadList *server_thread_list = NULL;
pthread_mutex_t server_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int create_log_file()
{
    FILE *fp;

    fp = fopen(LOG_FILE, "w");
    if (fp == NULL)
    {
        return 1;
    }

    fclose(fp);
    return 0;
}

int log_message(char *message)
{
    FILE *fp;

    fp = fopen(LOG_FILE, "a");
    if (fp == NULL)
    {
        return 1;
    }

    fprintf(fp, "%s", message);
    fclose(fp);
    return 0;
}

int remove_log_file()
{
    int rv = remove(LOG_FILE);
    if (rv == 0)
    {
        // printf("Log file %s deleted successfully.\n", LOG_FILE);
    }
    else
    {
        // printf("Failed to delete log file %s.\n", LOG_FILE);
    }

    return 0;
}

int send_file_content(int fd)
{
    FILE *fp;
    char *buffer;
    int file_size;

    // Open file
    fp = fopen(LOG_FILE, "rb");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate buffer
    buffer = malloc(file_size + 1);
    if (!buffer)
    {
        perror("malloc");
        exit(1);
    }

    // Read file into buffer
    if (fread(buffer, file_size, 1, fp) != 1)
    {
        perror("fread");
        exit(1);
    }

    // Close file
    fclose(fp);

    // Print buffer contents
    int rv = send(fd, buffer, file_size, 0);

    // Free buffer
    free(buffer);

    return rv;
}

void server_syslog(int server_syslog_type, char *ip_str)
{
    openlog("AESD-Socket-Server", LOG_CONS | LOG_PID, LOG_USER);

    char buf[50];
    switch (server_syslog_type)
    {
    case SERVER_SYSLOG_CONNECT:
        sprintf(buf, "Accepted connection from %s\n", ip_str);
        break;
    case SERVER_SYSLOG_DISCONNECT:
        sprintf(buf, "Closed connection from %s\n", ip_str);
        break;
    default:
        sprintf(buf, "Caught signal, exiting\n");
        break;
    }

    // printf("%s", buf);
    syslog(LOG_INFO, "%s", buf);

    closelog();
}

void termination_handler(int sig)
{
    server_syslog(SERVER_SYSLOG_EXIT, NULL);
    should_exit = true;
}

void setup_signal_handlers(struct sigaction *sa)
{
    sa->sa_handler = termination_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    sigaction(SIGINT, sa, NULL);
    sigaction(SIGTERM, sa, NULL);
}

bool is_message_end(char *chunk)
{
    int len = strlen(chunk);
    for (int i = 0; i < len; i++)
    {
        if (chunk[i] == '\n')
        {
            return true;
        }
    }
    return false;
}

int server_bind()
{
    int sockfd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int yes = 1;

    while (1)
    {

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }

        // loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1)
            {
                perror("server: socket");
                continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                           sizeof(int)) == -1)
            {
                perror("setsockopt");
                exit(1);
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(sockfd);
                perror("server: bind");
                continue;
            }

            break;
        }

        freeaddrinfo(servinfo); // all done with this structure

        if (p == NULL)
        {
            fprintf(stderr, "server: failed to bind\n");
            sleep(1);
            continue;
            //exit(1);
        }

        break;
    }

    return sockfd;
}

void server_time_log_thread(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    char buf[60];
    int ctr = 0;

    while (!should_exit)
    {
        sleep(1);
        if (ctr == 9)
        {
            memset(buf, 0, sizeof(buf));
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(buf, sizeof(buf), "timestamp: %Y-%m-%d %H:%M:%S\n", timeinfo);
            pthread_mutex_lock(&server_thread_mutex);
            log_message(buf);
            pthread_mutex_unlock(&server_thread_mutex);
        }
        ctr = ctr == 9 ? 0 : (ctr + 1);
    }
}

void server_handle_connection_thread(void *args)
{
    // Extract args
    uint8_t *args_buf = (uint8_t *)args;
    int new_fd;
    char client_addr[INET_ADDRSTRLEN];
    memcpy(&new_fd, args_buf, sizeof(int));
    memcpy(client_addr, args_buf + sizeof(int), INET_ADDRSTRLEN);
    free(args);

    char buf[MAXDATASIZE];

    int numbytes = 0;
    while (1)
    {
        numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0);
        if (numbytes == -1)
        {
            perror("recv");
            exit(1);
        }
        else if (numbytes > 0)
        {
            buf[numbytes] = '\0';
            pthread_mutex_lock(&server_thread_mutex);
            log_message(buf);
            pthread_mutex_unlock(&server_thread_mutex);
            if (is_message_end(buf))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if (send_file_content(new_fd) == -1)
    {
        perror("send");
    }

    close(new_fd);
    server_syslog(SERVER_SYSLOG_DISCONNECT, client_addr);
}

int server_handle_connections(int sockfd)
{
    int new_fd;
    socklen_t sin_size;
    char s[INET_ADDRSTRLEN];

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    while (1)
    { // main accept() loop
        if (should_exit)
        {
            if (server_thread_list == NULL)
            {
                printf("Thread list is not initialized\n");
                break;
            }

            AESDThread *server_thread = server_thread_list->head;
            while (server_thread != NULL)
            {
                aesd_thread_join(server_thread, NULL);
                server_thread = server_thread->next;
            }

            aesd_thread_list_free(server_thread_list);
            break;
        }

        struct sockaddr_storage their_addr; // connector's address information
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        // printf("server: got connection from %s\n", s);
        server_syslog(SERVER_SYSLOG_CONNECT, s);

        uint8_t *args = malloc(sizeof(int) + INET_ADDRSTRLEN);
        memcpy(args, &new_fd, sizeof(int));
        memcpy(args + sizeof(int), s, INET_ADDRSTRLEN);
        AESDThread *server_thread = aesd_thread_create(NULL, (void *(*)(void *))server_handle_connection_thread, (void *)args);
        aesd_thread_list_push(server_thread_list, server_thread);
    }

    close(sockfd);
    return 0;
}

int start_socket_server(bool daemon)
{

    int sockfd = server_bind();

    if (daemon)
    {
        if (!fork())
        {
            AESDThread *server_timer_thread = aesd_thread_create(NULL, (void *(*)(void *))server_time_log_thread, NULL);
            aesd_thread_list_push(server_thread_list, server_timer_thread);
            server_handle_connections(sockfd);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        AESDThread *server_timer_thread = aesd_thread_create(NULL, (void *(*)(void *))server_time_log_thread, NULL);
        aesd_thread_list_push(server_thread_list, server_timer_thread);
        return server_handle_connections(sockfd);
    }

    return 0;
}

int main(int argc, char **argv)
{
    create_log_file();
    struct sigaction sa;
    setup_signal_handlers(&sa);
    bool daemon = false;

    server_thread_list = aesd_thread_list_init();

    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        if (opt == 'd')
        {
            daemon = true;
        }
    }

    start_socket_server(daemon);

    return 0;
}