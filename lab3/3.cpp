#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <vector>
using namespace std;
vector<int> array;

#define MAX_CONN 32
#define MAX_SIZE 512
char Message[] = "Message:";

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    int Maxfd;
    int ret;
    char buffer[1048580] = "";
    char str[1048580];
    int flag_overflow = 0;
    socklen_t len;//地址大小
    struct sockaddr_in acpaddr;//accpet用到的地址
    int acpsock;//accpet函数返回值
    int resock;//recv中用到的文件么描述符号
    char buf[MAX_SIZE];//接受到的消息内容
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    fd_set refd,allfd;
    
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
    FD_ZERO(&refd);
    FD_ZERO(&allfd);
    FD_SET(fd, &allfd);
    Maxfd=fd;

    while(1){
        refd = allfd;
        ret=select(Maxfd+1, &refd, 0, 0, 0);
        if(ret==-1)return -1;
        if(FD_ISSET(fd,&refd))
		{
			len=sizeof(acpaddr);
			acpsock=accept(fd,(struct sockaddr*) &acpaddr, &len);
			array.push_back(acpsock);
			Maxfd=Maxfd>acpsock?Maxfd:acpsock;
			FD_SET(acpsock,&allfd);
		}
        vector<int>::iterator it = array.begin();
        while(it != array.end()){
            resock = *it;
			if(FD_ISSET(resock,&refd))
			{
				len = 0;
				memset(buffer, 0, sizeof(buffer));
				// 客户端直接退出时为-1
				if((len =recv(resock, buffer, MAX_SIZE, 0))<=0)
				{
                    printf("one exit!\n");
					it = array.erase(it);
					close(resock);
					FD_CLR(resock,&allfd);
				}
				else
				{
					for(vector<int>::iterator it2 = array.begin(); it2 != array.end(); it2++)
					{
						if(*it2 != resock){
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
                            printf("len = %d\n", len);
                            for(int i = 0; i < len; i++){//devide the recv into messages by '\n' and send
                                if(buffer[i] == '\n'){
                                    str[k++] = '\n';
                                    str[k] = '\0'; 
                                    flag_out = 1;
                                    
                                    for (auto iter = array.begin(); iter != array.end(); ++iter) {//group send
                                        if (*iter == resock)continue;
                                        printf("send1:%s\n", str);
                                        send(*iter, str, k, 0);
                                    }
                                
                                    strcpy(str, Message);
                                    k = 8;
                                    continue;
                                }
                                str[k++] = buffer[i];
                            }
                            printf("k = %d, pre_flag_overflow = %d, flag_out = %d\n", k, pre_flag_overflow, flag_out);
                            if(k > 8 || (pre_flag_overflow == 1 && flag_out == 0 && k > 0)){
                                str[k] = '\0';
                                
                                for (auto iter = array.begin(); iter != array.end(); ++iter) {//group send
                                    if (*iter == resock)continue;
                                    printf("send2:%s\n", str);
                                    send(*iter, str, k, 0);
                                }
                            
                            }
                        }
							//send(*it2, szMsg, strlen(szMsg), 0);
					}
					it++;
				}
			}
			else
				it++;
        }
    }
    return 0;
}