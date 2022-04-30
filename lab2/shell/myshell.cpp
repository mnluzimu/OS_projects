#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <vector>

#define MAX_CMDLINE_LENGTH  1024    /* max cmdline length in a line*/
#define MAX_BUF_SIZE        4096    /* max buffer size */
#define MAX_CMD_ARG_NUM     32      /* max number of single command args */
#define WRITE_END 1     // pipe write end
#define READ_END 0      // pipe read end
pid_t PID;  //the first layer of child process
typedef void(* signal_handler)(int );
char *argv[MAX_CMD_ARG_NUM];
int FD[2];
char file_path[MAX_CMDLINE_LENGTH];
char alias_path[MAX_CMDLINE_LENGTH];

void signal_handler_fun(int signal_num){
    // printf("catch signal %d\n", signal_num);
    // std::cout << "\n";
    // printf("wtf");
    if(PID == 0){
      printf("\n");
        exit(0);
    }
}



int split_string(char* string, const char *sep, char** string_clips) {
    
    char string_dup[MAX_BUF_SIZE];
    string_clips[0] = strtok(string, sep);
    int clip_num=0;
    
    do {
        char *head, *tail;
        head = string_clips[clip_num];
        tail = head + strlen(string_clips[clip_num]) - 1;
        while(*head == ' ' && head != tail)
            head ++;
        while(*tail == ' ' && tail != head)
            tail --;
        *(tail + 1) = '\0';
        string_clips[clip_num] = head;
        clip_num ++;
    }while(string_clips[clip_num]=strtok(NULL, sep));
    return clip_num;
}


int exec_builtin(int argc, char**argv, int *fd) {
    // printf("entered exec_builtin\n");
    // printf("argv0=%s\n", argv[0]);
    if(argc == 0) {
        return 0;
    }


    if (strcmp(argv[0], "cd") == 0) {
        // printf("entered cd\n");
        // printf("wtf\n");
        // printf("%s\n", argv[1]);
        close(FD[0]);
        ssize_t s = write(FD[1], argv[1], strlen(argv[1]) + 1);
        // printf("%d\n", s);
        close(FD[1]);
        return 110;
    } else if (strcmp(argv[0], "exit") == 0){
        // printf("entered exit\n");
        return 101;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        // printf("entered pwd\n");
        std::string cwd;
        cwd.resize(MAX_CMDLINE_LENGTH);

        // std::string to char *: &s[0]（C++17 以上可以用 s.data()）
        // std::string 保证其内存是连续的
        const char *ret = getcwd(&cwd[0], MAX_CMDLINE_LENGTH);
        if (ret == nullptr) {
            // std::cout << "cwd failed\n";
        } else {
            std::cout << ret << "\n";
        }
        return 1;
    }
    if(strcmp(argv[0], "history") == 0){
        // printf("entered history1\n");
        std::vector<std::string> historys;
        char str[100];
        // printf("entered history\n");
        FILE * fp1 = fopen(file_path, "r");
        char * s = fgets(str, 100, fp1);
        while(s){
            // puts(str);
            std::string stri;
            stri = str;
            // std::cout << stri;
            historys.push_back(stri);
            s = fgets(str, 100, fp1);
        }
        fclose(fp1);
        int n = atoi(argv[1]);
        int len = historys.size();
        for(int i = len - n; i < len; i++){
            printf("%d %s", i + 1, historys[i].c_str());
        }
        return 1;
    }


    if (strcmp(argv[0], "export") == 0) {
        // printf("entered export\n");
        char** string_clips;
        split_string(argv[1], "=", string_clips);
        setenv(string_clips[0], string_clips[1], 1);
        return 1;
    }

    if(strcmp(argv[0], "alias") == 0){
        // printf("entered alias\n");
        if(argc == 1 || strcmp(argv[1], "-p") == 0){
            FILE * fp1 = fopen(alias_path, "r");
            char str1[MAX_CMDLINE_LENGTH];
            char * s1 = fgets(str1, 100, fp1);
            while(s1){
                // puts(str);
                std::string stri;
                stri = str1;
                // std::cout << stri;
                printf("%s", str1);
                s1 = fgets(str1, 100, fp1);
            }
            fclose(fp1);
        }
        else{
            FILE* fp_a;
            fp_a = fopen(alias_path, "a");
            fprintf(fp_a, "alias");
            for(int i = 1; i < argc; i++){
                fprintf(fp_a, " %s", argv[i]);
            }
            fprintf(fp_a, "\n");
            fclose(fp_a);
        }
        return 1;
    }

     else {
        return -1;
    }
    return 1;
}



int process_redirect(int argc, char** argv, int *fd) {
    fd[READ_END] = STDIN_FILENO;
    fd[WRITE_END] = STDOUT_FILENO;
    int i = 0, j = 0;
    while(i < argc) {
        int tfd;
        if(strcmp(argv[i], ">") == 0) {
            tfd = open(argv[i+1],O_RDWR|O_CREAT|O_TRUNC,0666);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[WRITE_END] = tfd;
            }
            i += 2;
        } else if(strcmp(argv[i], ">>") == 0) {
            tfd = open(argv[i+1],O_RDWR|O_CREAT|O_APPEND,0666);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[WRITE_END] = tfd;
            }
            i += 2;
        } else if(strcmp(argv[i], "<") == 0) {
            tfd = open(argv[i+1],O_RDWR,0666);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[READ_END] = tfd;
            }
            i += 2;
        } else {
            argv[j++] = argv[i++];
        }
    }
    argv[j] = NULL;
    return j;
}




int execute(int argc, char** argv) {
    int fd[2];

    fd[READ_END] = STDIN_FILENO;
    fd[WRITE_END] = STDOUT_FILENO;

    argc = process_redirect(argc, argv, fd);
    // printf("executing exec_builtin in execute\n");
    if(exec_builtin(argc, argv, fd) == 0) {
        exit(0);
    }
    dup2(fd[READ_END], STDIN_FILENO);
    dup2(fd[WRITE_END], STDOUT_FILENO);
    execvp(argv[0],argv);
    exit(255);
}

void process_commandline(char * cmdline){
    if(cmdline[0] == '!'){
        std::vector<std::string> historys;
        char str[100];
        // printf("entered history\n");
        FILE * fp1 = fopen(file_path, "r");
        std::string stri;
        char * s = fgets(str, 100, fp1);
        while(s){
            // puts(str);
            std::string stri;
            stri = str;
            // std::cout << stri;
            historys.push_back(stri);
            s = fgets(str, 100, fp1);
        }
        fclose(fp1);
        int n;
        if(cmdline[1] == '!'){
            // printf("cmdline[1]=%c", cmdline[1]);
            n = historys.size() - 2;
        }
        else{
            // printf("cmdline[1]=%c", cmdline[1]);
            int len = strlen(cmdline);
            char num[100];
            for(int i = 1; i < len; i++){
                num[i - 1] = cmdline[i];
            }
            num[len - 1] = '\0';
            n = atoi(num) - 1;
        }
        // printf("n = %d\n", n);
        char newcmd[100];
        // std::cout << historys[n].c_str() << "\n";
        strcpy(newcmd, historys[n].c_str());
        int l = strlen(newcmd);
        if(newcmd[l - 1] == '\n'){
            newcmd[l - 1] = '\0';
        }
        strcpy(cmdline, newcmd);
        printf("%s\n", cmdline);
        process_commandline(cmdline);
    }
}

void convert_commandline(char * cmdline){
    // printf("entered convert line\n");
    char tem[MAX_CMDLINE_LENGTH];
    strcpy(tem, cmdline);
    // printf("tem:%s\n", tem);
    char *argv0[MAX_CMD_ARG_NUM];
    int n = split_string(cmdline, " ", argv0);
    FILE * fp;
    fp = fopen(alias_path, "a");
    fclose(fp);
    fp = fopen(alias_path, "r");
    if(fp == NULL){
        printf("cannot open file\n");
        return;
    }
    char str[MAX_CMDLINE_LENGTH];
    char * p = fgets(str, MAX_CMDLINE_LENGTH, fp);
    while(p){
        int len = strlen(str);
        if(str[len - 1] == '\n'){
            str[len - 1] = '\0';
        }
        char str1[MAX_CMDLINE_LENGTH];
        strcpy(str1, str);
        // printf("str=%s\n", str);
        // puts(str + 6);
        char *argv1[MAX_CMD_ARG_NUM];
        split_string(str, " ", argv1);
        char *argv2[MAX_CMD_ARG_NUM];
        // puts(str1 + 6);
        split_string(str1 + 6, "=", argv2);
        // printf("%s %s\n", argv2[0], argv2[1]);
        // printf("argv2[1]=%s\n", argv2[1]);
        if(strcmp(argv2[0], argv0[0]) == 0){
            char newcmd[MAX_CMDLINE_LENGTH];
            int i = 0;
            int j = 0;
            while(argv2[1][i] != '\0'){
                if(argv2[1][i] != '\''){
                    newcmd[j] = argv2[1][i];
                    i++;
                    j++;
                }
                else{
                    i++;
                }
            }
            for(int k = 1; k < n; k++){
                strcat(newcmd, " ");
                strcat(newcmd, argv0[k]);
            }
            strcpy(cmdline, newcmd);
            // printf("%s\n", cmdline);
            fclose(fp);
            return;
        }
        p = fgets(str, MAX_CMDLINE_LENGTH, fp);
    }
    strcpy(cmdline, tem);
    // printf("%s\n", cmdline);
    fclose(fp);
}

void convert_echo(char * cmdline){
    char temp[MAX_CMDLINE_LENGTH];
    strcpy(temp, cmdline);
    char *argv0[MAX_CMD_ARG_NUM];
    int n = split_string(temp, " ", argv0);
    if(strcmp(argv0[0], "echo") == 0){
        strcpy(cmdline, argv0[0]);
        for(int i = 1; i < n; i++){
            strcat(cmdline, " ");
            if(strcmp(argv0[i], "~root") == 0){
                strcat(cmdline, "/root");
            }
            else{
                strcat(cmdline, argv0[i]);
            }
        }
    }
}

int main() {
    char cmdline[MAX_CMDLINE_LENGTH];
    char path[MAX_CMDLINE_LENGTH];
    char *commands[128];
    char *commands1[128];
    int cmd_count;
    int cmd_count1;
    signal_handler p_signal = signal_handler_fun;
    signal(SIGINT, p_signal);

    getcwd(file_path,MAX_CMDLINE_LENGTH);
    strcat(file_path, "/mybash_history");
    getcwd(alias_path,MAX_CMDLINE_LENGTH);
    strcat(alias_path, "/myalias");
    // puts(alias_path);
    // puts(file_path);
    while (1) {

        pipe(FD);
        PID = fork();
        if(PID == 0){
            printf("%s@myshell# ",getcwd(path,MAX_CMDLINE_LENGTH));
            fflush(stdout);

            char * p = fgets(cmdline, 256, stdin);
            if(p == NULL){
                printf("\n");
                exit(255);
            }
            // printf("%s\n", cmdline);
            // FILE* fp = fopen("/home/matyr/mybash_history", "a");
            FILE* fp = fopen(file_path, "a");
            if(fp == NULL){
                printf("cannot open file\n");
                exit(255);
            }
            int t = fputs(cmdline, fp);
            // printf("t = %d\n", t);
            fclose(fp);
            // printf("fp closed\n");
            int len = strlen(cmdline);
            if(cmdline[len - 1] == '\n'){
                cmdline[len - 1] = '\0';
            }
            // printf("1:%s\n", cmdline);
            process_commandline(cmdline);
            // printf("2:%s\n", cmdline);
            convert_commandline(cmdline);
            convert_echo(cmdline);
            cmd_count = split_string(cmdline, "|", commands);

            if(cmd_count == 0) {
                continue;
            } else if(cmd_count == 1) {
                // char *argv[MAX_CMD_ARG_NUM];
                int argc;
                int fd[2];
                argc = split_string(commands[0], " ", argv);
                // printf("executing exec_builtin in main\n");
                // printf("PID=%d\n", PID);
                int temp = exec_builtin(argc, argv, fd);
                // printf("temp = %d\n", temp);
                if(temp == 0) {
                    exit(0);
                }
                else if(temp == 101){
                    exit(255);
                }
                else if(temp == 110){
                    exit(25);
                }
                else if(temp == 1){
                    exit(0);
                }
                int pid = fork();
                if(pid == 0){
                    // printf("PID=%d\n", PID);
                    execute(argc,argv);
                    exit(255);
                }
                while(wait(NULL)>0);

            } else if(cmd_count == 2) {
                int pipefd[2];
                int ret = pipe(pipefd);
                if(ret < 0) {
                    printf("pipe error!\n");
                    continue;
                }
                int pid = fork();
                if(pid == 0) {  
                    close(pipefd[READ_END]);
                    dup2(pipefd[WRITE_END], STDOUT_FILENO);
                    close(pipefd[WRITE_END]);
                    char *argv[MAX_CMD_ARG_NUM];
                    int argc = split_string(commands[0], " ", argv);
                    execute(argc, argv);
                    exit(255);
                    
                }
                pid = fork();
                if(pid == 0) {  
                    close(pipefd[WRITE_END]);
                    dup2(pipefd[READ_END], STDIN_FILENO);
                    close(pipefd[READ_END]);
                    int argc = split_string(commands[1], " ", argv);
                    execute(argc, argv);
                    exit(255);
                }
                close(pipefd[WRITE_END]);
                close(pipefd[READ_END]);
                
                while (wait(NULL) > 0);
            } else {
                int read_fd;
                for(int i=0; i<cmd_count; i++) {
                    int pipefd[2];
                    if( i<cmd_count-1 ){
                        int ret = pipe(pipefd);
                        if(ret < 0) {
                            printf("pipe error!\n");
                            continue;
                        }
                    }
                    int pid = fork();
                    if(pid == 0) {
                        if( i<cmd_count-1 ){
                            close(pipefd[READ_END]);
                            dup2(pipefd[WRITE_END], STDOUT_FILENO);
                            close(pipefd[WRITE_END]);
                        }

                        if( i>0 ){
                            close(pipefd[WRITE_END]);
                            dup2(read_fd,STDIN_FILENO);
                            close(read_fd);
                        }

                        int argc = split_string(commands[i], " ", argv);
                        execute(argc, argv);
                        exit(255);
                    }
                    if(i>0) close(read_fd);
                    if(i<cmd_count-1) read_fd=pipefd[READ_END];
                    close(pipefd[WRITE_END]);
                }
                while (wait(NULL) > 0);
            }
            exit(0);
        }
        else{
            int status;
            waitpid(PID, &status, 0);
            int err = WEXITSTATUS(status);
            if (err == 255) { 
                exit(0);
            }
            else if(err == 25){
                // printf("err 25, hopefully cd\n");
                char msg[100];
                // printf("1\n");
                memset(msg, '\0' ,sizeof(msg));  
                // printf("2\n");
                close(FD[1]);
                int s = read(FD[0], msg, sizeof(msg)); 
                // printf("s = %d\n", s);
                close(FD[0]);
                // printf("after read\n");
                // printf("%s\n", msg);
                chdir(msg);
            }
            close(FD[0]);
            close(FD[1]);
        }
    }
}












