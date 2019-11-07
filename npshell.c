#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXPROCESS 1000     /*allowed forked process number*/
#define MAXPIPE 1000       /*number of all pipes will be shown in |n */
#define MAXSTDLENGTH 15000  /*length of whole command length*/
#define MAXCMDLENGTH 256    /*lenght of single command*/
#define MAXBUF 100
#define DBGLVL 1

typedef struct _command
{
    char *inputbuf;
    int cmd_argc;
    char *cmd_argv[];
    char *cmd_envp[];
} NPcommand;

typedef struct _PCB
{
    int readPipe;
    int writePipe;
    int errorPipe;
    int childPID;
    char *childcmd;
    int delay_val; /*for |n implementation*/
    bool _activate;
} PCB;

typedef struct _PipeControllor
{
    PCB *PipeTable;
    int OpenedPipe;

} PipeControllor;

PCB initPCB(){
    PCB trgt;
    trgt.childcmd = (char *)malloc(MAXCMDLENGTH*sizeof(char));
    trgt._activate = false;
    trgt.readPipe = -1;
    trgt.writePipe = -1;
    trgt.errorPipe = -1;
    trgt.childPID = -1;
    trgt.delay_val = -1;
    return trgt;
};

int purgePCB(PCB *trgt){
    trgt -> _activate = false;
    trgt -> readPipe = -1;
    trgt -> writePipe = -1;
    trgt -> errorPipe = -1;
    trgt -> childPID = -1;
    trgt -> delay_val = -1;
    trgt -> _activate = false;
    return 1;
};

int finalizePCB(PCB *trgt){
    free(trgt -> childcmd);
    free(trgt);
    return 1;
};

PipeControllor *initPipeControllor(){

    PipeControllor *trgt;
    trgt = (PipeControllor *)malloc(sizeof(PipeControllor));

    trgt -> PipeTable = (PCB *)malloc(MAXPIPE * sizeof(PCB));
    trgt -> OpenedPipe = MAXPIPE;
    for(int i = 0; i < MAXPIPE; i++){
        (trgt -> PipeTable)[i] = initPCB();
    };
    return trgt;
};

int finalizePipeControllor(PipeControllor *trgt){
    for(int i = 0; i < MAXPIPE; i++){
        finalizePCB((trgt -> PipeTable) + i);
    };
    free(trgt -> PipeTable);
    free(trgt);
    return 1;
};

int initargvArr(char **argv){
    argv = (char **)malloc(MAXSTDLENGTH*sizeof(char*));
    for(int malloc_iterator = 0; malloc_iterator < MAXSTDLENGTH; malloc_iterator ++){
        argv[malloc_iterator] = (char *)malloc(MAXCMDLENGTH*sizeof(char));
    };
    return 1;
};

int finalizedargvArr(char **argv){
    for(int malloc_iterator = 0; malloc_iterator < MAXSTDLENGTH; malloc_iterator ++){
        free(argv[malloc_iterator]);
    };
    free(argv);
    return 1;
};
/*Add pipe to PipeControllor*/
int AddPipe(PipeControllor *controller, PCB *trgt){
    if ((*controller).OpenedPipe == MAXPIPE){
        return -1;
    }else{
        ((*controller).PipeTable)[(*controller).OpenedPipe] = *trgt;
        (*controller).OpenedPipe ++;
        return 1;
    };
};

int NPprintdebug(char *msg, int level){
    
    char newline[] = "\n\0";
    if(level % 2){
        write(1, msg, strlen(msg));
        write(1, newline, 1);
    };
    return 1;
};

/*parseline is to build argv array and return argc*/
int parseline(char *buff, char **argv, int bufsize){

    if(buff[bufsize - 1] == '\n'){
        /*printf("in parseline function detected newline at the end of buffer\n");*/
        buff[bufsize - 1] = '\0';
    };
    bufsize --;

    if(bufsize   == 0){
        argv = NULL;
        return 0;
    };

    char *tmp;
    int argc = 1;
    int shift = 0;
    int indent = 0;

    for(int i = 0; i < bufsize; i++){
        if(buff[i] == ' '){
            argv[shift][indent] = '\0';
            /*printf("change\n");*/
            shift += 1;
            indent = 0;
            argc += 1;
        }else{
            /*printf("add %s into argv with shift %d \n", buff + i, shift);*/
            argv[shift][indent] = buff[i];
            indent += 1;
            
        };
    };
    argv[shift][indent] = '\0';
    return argc;
};

int NPprintargv(int argc, char **argv, char **envp){
    for(int i = 0; i < argc; i++){
        printf("%s \n", argv[i]);
    };
    return 0;
};
int NPsetenv(int argc, char **argv, char **envp){
    int envplocation = 0;
    int shift = 0;
    char **envprmt;
    
    if(argc != 3){
        return -1;
    }else{
        return setenv(argv[1], argv[2], 1);
    };
};

void NPexit(int argc, char **argv, char **envp){
    exit(0);
};

int NPprintenv(int argc, char **argv, char **envp){
    char *gtchar;
    char newline[] = "\n";
    int gtrslt;
    gtchar = getenv(argv[1]);
    if(gtchar == NULL){
        return 0;
    }else{
        gtrslt = write(1, gtchar, strlen(gtchar));
        write(1, newline, 1);
        return gtrslt;
    };
    /*NPprintdebug(gtchar, DBGLVL);*/
};

int _NPswitchfunc(int argc, char **argv, char **envp, int (*_function)(int, char**, char**)){
    return _function(argc, argv, envp);
};

void childHandler(int signo){
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0);
    return;
};

int NPexec(int argc, char **argv, char **envp, PipeControllor *controllor){
    /*for(int i = 0; i < argc; i ++){
        NPprintdebug(argv[i], DBGLVL);
    };*/
    signal(SIGCHLD, childHandler);
    char exitmsg[] = "exit";
    char stenvmsg[] = "setenv";
    char penvmsg[] = "printenv";
    char noline[] = "\0";

    int pid, status;
    int count = 0;

    if(strcmp(argv[0], exitmsg) == 0){
        /*NPprintdebug(exitmsg, DBGLVL);*/
        /*finalizePipeControllor(pipetable);*/
        NPexit(argc, argv, envp);
    }else if(strcmp(argv[0], stenvmsg) == 0){
        /*read end pipe management*/
        for(int i = 0; i < MAXPIPE; i++){
            if ((controllor -> PipeTable)[i].delay_val == 1){
                close((controllor -> PipeTable)[i].writePipe);
                close((controllor -> PipeTable)[i].readPipe);
                close((controllor -> PipeTable)[i].errorPipe);
                purgePCB(&((controllor -> PipeTable)[i]));
            }else if((controllor -> PipeTable)[i].delay_val > 1){
                (controllor -> PipeTable)[i].delay_val --;
            };
        };
        /*end of read pipe management*/

        /*write pipe management*/
        /*end of write pipe management*/

        /*NPprintdebug(stenvmsg, DBGLVL);*/
        return(NPsetenv(argc, argv, envp));
    }else if(strcmp(argv[0], penvmsg) == 0){
        for(int i = 0; i < MAXPIPE; i++){
            if ((controllor -> PipeTable)[i].delay_val == 1){
                close((controllor -> PipeTable)[i].writePipe);
                close((controllor -> PipeTable)[i].readPipe);
                close((controllor -> PipeTable)[i].errorPipe);
                purgePCB(&((controllor -> PipeTable)[i]));
            }else if((controllor -> PipeTable)[i].delay_val > 1){
                (controllor -> PipeTable)[i].delay_val --;
            };
        };
        /*NPprintdebug(penvmsg, DBGLVL);*/
        return(NPprintenv(argc, argv, envp));
    }else if(strcmp(argv[0], noline) == 0){
        printf("go do nothing \n");
        return 1;
    }else{
        /*printf("go exec with %s \n", argv[0]);*/
        /*setup pipe for fork child*/
        /**/
        /*printf("try to fork\n");*/
        pid = fork();
        while(pid < 0){
            /*printf("fork fail");*/
            usleep(1000);
            pid = fork();
        };
        /*printf("pid is %d \n", pid);*/
        if(pid > 0){
            /*parent stuff*/
            /*printf("parent work\n");*/
            if(strcmp(argv[argc], noline) == 0){
                printf("EOF execution \n");
                waitpid(pid, &status, 0);
            };
            return 1;
        }else{
            /*child stuff*/
            /*printf("count number: %d\n", count);*/
            /*count ++;*/
            execvp(argv[0], argv);
        };
    };

};

int NPreadline(char **argvptr){
    
    int readsize;
    int argc;

    char *inputbuf;
    char strtmsg[2] = {'%', ' '};

    write(1, strtmsg, 2);

    inputbuf = (char *)malloc(MAXSTDLENGTH*sizeof(char)); /*initialized a buffer for read input*/
    readsize = read(0, inputbuf, MAXSTDLENGTH);
    if(readsize == -1){
        free(inputbuf);
        return -1;
    }else{
        argc = parseline(inputbuf, argvptr, readsize);
        free(inputbuf);
        return argc;
    };

};


int main(int argc, char *argv[], char *envp[]){

    signal(SIGCHLD, childHandler);
    /*Initialized input buffer*/
    char **cmd_argv;
    char **exe_argv;
    int cmd_argc;
    int exe_argc;
    
    /*initialize the iterators*/
    int malloc_iterator;
    int tst_iterator;
    int exec_iterator;
    /*finished the iterators*/

    /*Initialize Pipe controllor*/
    PipeControllor *ScheduleTable;
    ScheduleTable = initPipeControllor();
    /*Finished the initialization*/

    cmd_argv = (char **)malloc(MAXCMDLENGTH*sizeof(char*));
    for(malloc_iterator = 0; malloc_iterator < MAXCMDLENGTH; malloc_iterator ++){
        cmd_argv[malloc_iterator] = (char *)malloc(MAXSTDLENGTH*sizeof(char));
    };

    while(true){
        /*initialize buffer for argv*/
        /*finished the initialization of argv*/

        printf("prepare to readline \n");
        cmd_argc = NPreadline(cmd_argv);
        /*printf("the got argv is %s and argc is %d \n", cmd_argv[0], cmd_argc);*/

        exe_argv = cmd_argv;
        exe_argc = 0;

        for(int i = 0; i <= cmd_argc; i++){
            if((cmd_argv[i][0] == '|') || (cmd_argv[i][0] == '!') || (cmd_argv[i][0] == '>')){
                NPexec(exe_argc, exe_argv, envp, ScheduleTable);
                printf("successful get our from NPexec 1\n");
                exe_argv = cmd_argv + i + 1;
                exe_argc = 0;
            }else if (cmd_argv[i][0] == '\0'){
                NPexec(exe_argc, exe_argv, envp, ScheduleTable);
                printf("successful get our from NPexec 2\n");
                exe_argc = 0;
            }else{
                exe_argc ++;
            };
        };
        

        /*NPprintargv(cmd_argc, cmd_argv, envp);*/
        for(malloc_iterator = 0; malloc_iterator < MAXCMDLENGTH; malloc_iterator ++){
            for(int i = 0; i < MAXCMDLENGTH; i++){
                cmd_argv[malloc_iterator][i] = '\0';
            };
        };
        
    };
};