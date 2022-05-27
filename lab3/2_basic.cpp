#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include<map>
#include<iterator>
using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
int ready = 0;

typedef struct client {
	int id;
	int socket;
}Client;

pair<int, Client> Pair;
map<int,Client> clients;
char Message[] = "Message:";

struct Pipe {
    int fd_send;
    int fd_recv;
};



#define MAX_SIZE 1048576

int conn_num = 0;
#define MAX_CONN 32

void *client_run(void *data) {
    static int flag_overflow = 0;
    Client *client = (Client *)data;
    char buffer[1048580] = "";
    char str[1048580];
    ssize_t len;
    while (1) {
        len = recv(client->socket, buffer, MAX_SIZE, 0);
        if(len <= 0) break;
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
                
                for (auto iter = clients.begin(); iter != clients.end(); ++iter) {//group send
                    if (iter->first == client->id)continue;
                    pthread_mutex_lock(&mutex);
                    send(iter->second.socket, str, k, 0);
                    pthread_mutex_unlock(&mutex);
                }
               
                strcpy(str, Message);
                k = 8;
                continue;
            }
            str[k++] = buffer[i];
        }
        if(k > 8 || (pre_flag_overflow == 1 && flag_out == 0 && k > 0)){
            str[k] = '\0';
            
            for (auto iter = clients.begin(); iter != clients.end(); ++iter) {//group send
                if (iter->first == client->id)continue;
                pthread_mutex_lock(&mutex);
                send(iter->second.socket, str, k, 0);
                pthread_mutex_unlock(&mutex);
            }
           
        }
        // pthread_mutex_unlock(&mutex);
        // send(pipe->fd_recv, str, len, 0);
    }
    clients.erase(client->id);
	conn_num--;
	close(client->socket);
	printf("one client disconnected\n");
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
    if (listen(fd, MAX_CONN)) {
        perror("listen");
        return 1;
    }
    int i = 0;
    Client *client;
	pthread_t *thread;
	struct sockaddr remote_addr;
    socklen_t remote_addr_rlen=sizeof(remote_addr);
    while(conn_num <= MAX_CONN){
        client = (Client *)malloc(sizeof(Client));
		thread= (pthread_t *)malloc(sizeof(pthread_t));
		printf("waiting for connection...\n");
		client->socket = accept(fd, &remote_addr, &remote_addr_rlen);//accept connection
		if (client->socket < 0)
		{
			free(client);
			printf("receive failed\n");
			continue;
		}
		client->id = i;
		clients[i] = *client;//save to map
		printf("one connected, new thread init...\n");
		conn_num++;
		pthread_create(thread, NULL, client_run, &clients[i]);
        pthread_detach(*thread);
		i++;
    }
    return 0;
}