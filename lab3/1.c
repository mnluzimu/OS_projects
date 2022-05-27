#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
char Message[] = "Message:";

struct Pipe {
    int fd_send;
    int fd_recv;
};

#define MAX_SIZE 1048576


void *handle_chat(void *data) {
    static int flag_overflow = 0;
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[1048580] = "";
    char str[1048580];
    ssize_t len;
    while ((len = recv(pipe->fd_send, buffer, MAX_SIZE, 0)) > 0) {
        //printf("len=%d\n%s\n", len, buffer);
        int k;
        int pre_flag_overflow = 0;
        if(flag_overflow == 0){
            strcpy(str, Message);
            k = 8;
        }
        else{ //if there was an overflow, then this recv is part of the last message
            pre_flag_overflow = 1;
            k = 0;
        }
        if(len == MAX_SIZE && buffer[len - 1] != '\n'){ //see if there is an overflow in recv
            // printf("overflow to 1\n");
            flag_overflow = 1;
        }
        else{
            // printf("overflow to 0\n");
            flag_overflow = 0;
        }
        int flag_out = 0;
        for(int i = 0; i < len; i++){//devide the recv into messages by '\n' and send
            if(buffer[i] == '\n'){
                str[k++] = '\n';
                str[k] = '\0'; 
                flag_out = 1;
                send(pipe->fd_recv, str, k, 0);
                strcpy(str, Message);
                k = 8;
                continue;
            }
            str[k++] = buffer[i];
        }
        if(k > 8 || (pre_flag_overflow == 1 && flag_out == 0 && k > 0)){
            str[k] = '\0';
            send(pipe->fd_recv, str, k, 0);
        }
        // send(pipe->fd_recv, str, len, 0);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2)) {
        perror("listen");
        return 1;
    }
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1) {
        perror("accept");
        return 1;
    }
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}