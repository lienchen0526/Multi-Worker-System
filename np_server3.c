#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> //for open() function on redirection
#include <sys/socket.h>
#include <netinet/in.h> 
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>

#define PORT 7070
#define MAXPROCESS 1000     /*allowed forked process number*/
#define MAXPIPE 1000       /*number of all pipes will be shown in |n */
#define MAXSTDLENGTH 15000  /*length of whole command length*/
#define MAXCMDLENGTH 256    /*lenght of single command*/
#define MAXBUF 100
#define DBGLVL 0 /*the bigger the number is, the more detail can be seen, 0 is production mode*/
//--------------------------for project 2---------------------------------------------------------------
#define MAXCLIENT 30
#define MAX_NLEN 100
//------------------------------------------------------------------------------------------------------

#define ORDPIPE 0
#define NUMPIPE 1
#define ERRPIPE 2
#define REDIRECT 3
#define NPIPE_IN 4
#define NPIPE_OUT 5
#define EOFL -1

typedef struct _command
{
    bool builtin;

    int cmd_argc;
    int pipemechanism; 
    /* 0: ordinary pipe, 1: number pipe, 2: error pipe, 3: redirection, 
    4: named pipe out, 5: named pipe in -1: EOF */
    
    int delayval; /*the number of delayed value*/
    int trgt_client;
    int pipefrom_client;
    int client_id;

    char *origin_cmd;
    char **cmd_argv;
    char *filename; /*for redirection*/
    struct _command *next;

} NPcommandPack;

typedef struct _usr_pipe
{
    bool is_activate;
    int readside;
    int writeside;
} UserPipe;

typedef struct _PCB
{
    int readPipe;
    int writePipe;
    int errorPipe;
    int childPID;
    int delay_val; /*for |n implementation*/
    bool _activate;

} PCB;

typedef struct _PipeControllor
{
    PCB **PipeTable;
    int Maxavil;
    int OpenedPipe;

} PipeControllor;

typedef struct _ControllorPool
{
    bool *active_flag;
    int activated_clients_num;
    int *occupied_sfd;

    char **user_name;
    char **port_name;
    char **ip_addr;
    char **PATH_cont;
    UserPipe **userpipe;
    PipeControllor **MainPool;

} ControllorPool;

void childHandler(int signo){
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0);
    return;
};

int dump_argv(char *buffer, char **argv, char *tmp_delimiter = 0){
    char delimiter[1] = {' '};

    if(tmp_delimiter == 0){
        delimiter[1] = {' '};
    }else{
        delimiter[0] = tmp_delimiter[0];
    };
    int row_ind = 0;
    int col_ind = 0;
    int buf_ind = 0;
    while(argv[row_ind] != 0){
        while(argv[row_ind][col_ind] != '\0'){
            
            buffer[buf_ind] = argv[row_ind][col_ind];
            col_ind ++;
            buf_ind ++;
        };
        buffer[buf_ind] = delimiter[0];
        buf_ind ++;
        row_ind ++;
        col_ind = 0;
    };
    buffer[buf_ind] = '\0';
    buf_ind ++;
    return buf_ind;
};

