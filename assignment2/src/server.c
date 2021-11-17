// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>
#include <stdint.h>
#define PORT 8080

char *get_error() {
    if (errno==EAGAIN) {
        return "EAGAIN";
    }
    if (errno==EINVAL) {
        return "EINVAL";
    }
    if (errno==EPERM) {
        return "EPERM";
    }
    return "";
}

int main(int argc, char *const *argv)
{
    char *b_arg_buffer = NULL;
    int s_arg = -1;
    int gopt;
    while ((gopt = getopt(argc, argv, "B:s:")) != -1) {
        switch (gopt) {
            case 'B':
                b_arg_buffer = calloc(strlen(optarg)+1, sizeof(char));
                strcpy(b_arg_buffer, optarg);
                break;
            case 's':
                s_arg = atoi(optarg);
                break;
        }
    }

    int server_fd, new_socket, valread;
    char *hello = "Hello from server";

    if (b_arg_buffer != NULL && s_arg != -1) {
        // Reading user data into the buffer and using it can lead to vuln,
        // so fork+setuid+execv was used prior
        valread = read(s_arg, b_arg_buffer, 1024);
        printf("Read %d bytes: %s\n", valread, b_arg_buffer);
        send(s_arg, hello, strlen(hello), 0);
        printf("Hello message sent\n");

        free(b_arg_buffer);
        return 0;
    }

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
	&opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    if (bind(server_fd, (struct sockaddr *)&address,
	sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Fork and setuid(nobody) on child process
    pid_t pid = fork();
    // The child process
    if (pid==0) {
        struct passwd *passwd_nobody = getpwnam("nobody");
        if (setuid(passwd_nobody->pw_uid) < 0) {
            if (errno==EPERM) {
                puts("Already running as a non-privileged user - setuid() does nothing!");
            }
            else {
                printf("setuid(%d) %s error: ", passwd_nobody->pw_uid, get_error());
                puts(strerror(errno));
                exit(errno);
            }
        }

        char **new_args = calloc(argc+5, sizeof(char *));
        if (new_args==NULL) {
            puts("calloc fail. exiting....");
            exit(EXIT_FAILURE);
        }
        new_args[0] = argv[0];
        new_args[1] = "-B";
        new_args[2] = buffer;
        new_args[3] = "-s";
        char socket_fd_str[12] = {0};
        sprintf(socket_fd_str, "%d", new_socket);
        new_args[4] = socket_fd_str;

        puts("calling execv() after setuid()...");
        execv(argv[0], new_args);
    }
    // Fork error
    else if (pid==-1) {
        printf("fork() %s error: ", get_error());
        puts(strerror(errno));
        exit(errno);
    }
    // Parent process
    else {
        int status;
        // Wait for child process to exit
        do {
            if (waitpid(pid, &status, 0) < 0) {
                printf("waitpid(%d,...) %s error: ", pid, get_error());
                puts(strerror(errno));
                exit(errno);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}
