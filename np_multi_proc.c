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

#include "./src/argparse.h"

#define PORT 7070
#define MAXPROCESS 1000     /*allowed forked process number*/
#define MAXPIPE 1000       /*number of all pipes will be shown in |n */
#define MAXSTDLENGTH 15000  /*length of whole command length*/
#define MAXCMDLENGTH 256    /*lenght of single command*/
#define MAXBUF 100
#define DBGLVL 0 /*the bigger the number is, the more detail can be seen, 0 is production mode*/

#define ORDPIPE 0
#define NUMPIPE 1
#define ERRPIPE 2
#define REDIRECT 3
#define EOFL -1

typedef struct _command
{
    
    int cmd_argc, pipefrom_client, trgt_client;
    int pipemechanism; /* 0: ordinary pipe, 1: number pipe, 2: error pipe, 3: redirection, -1: EOF */
    int delayval; /*the number of delayed value*/
    bool builtin;
    char **cmd_argv;
    char *filename; /*for redirection*/
    struct _command *next;

} NPcommandPack;

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

void childHandler(int signo){
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0);
    return;
};

int NPprintDBG(const char *msg, int flag){
    if(flag < DBGLVL){
        printf("%s\n", msg);
    };
};

int NPprintSinglePack(NPcommandPack *trgt){
    if(trgt == 0){
        printf("NULL command\n");
    }else{
        printf("try to print \n");
        int ref = 0;
        printf("--------------------\n");
        printf("cmd_argc is %d\n", trgt -> cmd_argc);
        if((trgt -> pipemechanism) == 0){
            printf("pipemechanism is ordinary pipe\n");
        }else if((trgt -> pipemechanism) == 1){
            printf("pipemechanism is number pipe\n");
        }else if((trgt -> pipemechanism) == 2){
            printf("pipemechanism is error pipe\n");
        }else if((trgt -> pipemechanism) == 3){
            printf("pipemechanism is redirection pipe\n");
        }else if((trgt -> pipemechanism == -1)){
            printf("pipemechanism is EOF\n");
        };
        printf("delay value is %d\n", trgt -> delayval);
        printf("the argv array is:\n---\n");
        for(int i = 0; i < trgt -> cmd_argc; i++){
            printf("%s \n", (trgt -> cmd_argv)[i]);
        };

        printf("--------------------\n");
    };
};

int printPKG(NPcommandPack *trgt){
    if(trgt == 0){
        printf("NULL command\n");
    }else{
        printf("try to print \n");
        int ref = 0;
        while(trgt != 0){
            NPprintSinglePack(trgt);
            trgt = trgt -> next;
        };
    };
};

int initCMDpkg(NPcommandPack *trgt){
    /*
    the function initialize a pre-initialized
    NPcommandPack.
    */
    NPprintDBG("Get into initCMDpgk function", DBGLVL);
    trgt -> trgt_client = -1;
    trgt -> pipefrom_client = -1;
    trgt -> delayval = -1;

    (trgt -> filename) = (char *)calloc(2 * MAXCMDLENGTH, sizeof(char));
    (trgt -> cmd_argv) = (char **)calloc(2 * MAXCMDLENGTH, sizeof(char *));
    for(int i = 0; i < 2 * MAXCMDLENGTH; i++){
        (trgt -> cmd_argv)[i] = (char *)calloc(2 * MAXCMDLENGTH, sizeof(char));
    };
    trgt -> next = 0;
    return 1;
};

int finalizeCMDpkg(NPcommandPack *trgt){
    NPprintDBG("in finalizeCMDpkg: Get into finalizeCMDpgk function", 3);
    if(trgt -> filename != 0){
        NPprintDBG("in finalizeCMDpkg: try to free filename field",3);
        free(trgt -> filename);
        NPprintDBG("in finalizeCMDpkg: successfully free filename field",3);
    };
    NPprintDBG("in finalizeCMDpkg: try to get into loop to free argv field",3);
    for(int i = 0; i < 2 * MAXCMDLENGTH; i++){
        NPprintDBG("in finalizeCMDpkg: try to free single row of argv",4);
        free((trgt -> cmd_argv)[i]);
        NPprintDBG("in finalizeCMDpkg: successfully free single row of argv",4);
    };
    free(trgt -> cmd_argv);
    NPprintDBG("in finalizeCMDpgk: sussessfully finalize command pack", 3);
};

/*
This function initialize the PCB element acc-
ording to the pointer passed by caller. obje-
ct point by trgt at best be lie in heap
*/
int initPCB(PCB *trgt){

    trgt -> readPipe = -1;
    trgt -> writePipe = -1;
    trgt -> errorPipe = -1;
    trgt -> childPID = -1;
    trgt -> delay_val = -1;
    trgt -> _activate = false;
    return 1;
};

/*
This function initialize the PipeControllor a-
ccording to the pointer passed by caller. obje-
ct point by trgt at best be lie in heap.
*/
int initControllor(PipeControllor *trgt){
    trgt -> PipeTable = (PCB **)calloc(1000, sizeof(PCB));
    trgt -> OpenedPipe = 0;
    trgt -> Maxavil = 1000;
    return 1;
};

/*
This function decrease the delay  value  inside
every PCB in in the table trgt
*/
int DECDVAL(PipeControllor *trgt){
    NPprintDBG("In DECDVAL: Get into DECDVAL function", 3);
    for(int i = 0; i < trgt -> Maxavil; i++){
        NPprintDBG("In DECDVAL: prepare to decrease value", 5);
        /*the code below will cause the crash, need to debug*/
        if(trgt -> PipeTable[i] != 0){
            NPprintDBG("In DECDVAL: detected nonzero pointer in ith position, try to decrease the value", 3);
            (((trgt -> PipeTable)[i]) -> delay_val) --;
            //printf("In DECDVAL: after deletion of %d th position of pipetable, the delay value becom %d\n", i, (((trgt -> PipeTable)[i]) -> delay_val));
            NPprintDBG("In DECDVAL: successfully free the nonzero pointer in ith position, try to decrease the value", 3);
        }else{
            NPprintDBG("In DECDVAL: Detect zero pointer", 5);
        };
        NPprintDBG("In DECDVAL: Finish one iteration", 5);
    };
    NPprintDBG("In DECDVAL: Successfully execute DECDVAL", 3);
    return 1;
};

int PrintPCB(PCB *trgt){
    printf("-------------\n");
    printf("read pipe: %d\n", trgt -> readPipe);
    printf("write pipe: %d\n", trgt -> writePipe);
    printf("delay value: %d\n", trgt -> delay_val);
    printf("-------------\n");
};
/*
trgt has been initialized
*/
int InsertPCB(PipeControllor *dst, PCB *trgt){
    NPprintDBG("in InsertPCB: Get into InsertPCB function",3);
    if(dst -> OpenedPipe == dst -> Maxavil){
        //printf("Pipe number overflow");
        return -1;
    };
    int i = 0;
    NPprintDBG("in InsertPCB: try to get into while loop", 3);
    while((dst -> PipeTable)[i] != 0){
        NPprintDBG("in InsertPCB: trying to get the next block in PipeTable", 3);
        i++;
    };
    //printf("in InsertPCB: insert block into %d th position with delay value %d \n", i, trgt -> delay_val);
    (dst -> PipeTable)[i] = trgt;
    (dst -> OpenedPipe) ++;
    NPprintDBG("in InsertPCB: finish executing InsertPCB", 3);
    return 1;
};

PCB *SearchPCB(PipeControllor *trgt, int delayval){
    PCB *rslt = 0;
    for(int i = 0; i < trgt -> Maxavil; i++){
        if((trgt -> PipeTable)[i] != 0){
            if ((((trgt -> PipeTable)[i]) -> delay_val) == delayval){
                rslt = trgt -> PipeTable[i];
                break;
            };
        };
    };
    return rslt;
};
/*
This function delete zero delay value pipe an-
d close the pipe

issue:
1) have to shift the array
2) if not shift, it will cause segm fault
3) have to decrease opened pipe
*/
int DelZDELAY(PipeControllor *dst){
    NPprintDBG("in DelZDELAY: entering DelXEDELAY", 3);
    NPprintDBG("in DelZDELAY: trying to access openPipe value", 3);
    int tmp = dst -> OpenedPipe;
    dst -> OpenedPipe;
    NPprintDBG("in DelZDELAY: successfully retrive OpenedPipe value", 3);
    for(int i = 0; i < dst -> Maxavil; i++){
        NPprintDBG("in DelZDELAY: trying to access PipeTable", 5);
        dst -> PipeTable;
        NPprintDBG("in DelZDELAY: successfully access PipeTable", 5);
        NPprintDBG("in DelZDELAY: trying to access ith element in PipeTable", 5);
        (dst -> PipeTable)[i];
        NPprintDBG("in DelZDELAY: successfully access ith element in PipeTable", 5);
        if((dst -> PipeTable[i]) != 0){
            if(((dst -> PipeTable)[i] -> delay_val) <= 0){
                //printf("in DelZDELAY: try to eleminate the %d th position in Pipetable with delay_val %d\n", i, ((dst -> PipeTable)[i] -> delay_val));
                close(((dst -> PipeTable)[i]) -> readPipe);
                //printf("in DelZDELAY: close the pipe discripter for read side: %d\n", ((dst -> PipeTable)[i] -> readPipe));
                close(((dst -> PipeTable)[i]) -> writePipe);
                //printf("in DelZDELAY: close the pipe discripter for write: %d\n", ((dst -> PipeTable)[i] -> writePipe));
                //close(((dst -> PipeTable)[i]) -> errorPipe);
                //printf("close the delay value %d\n", (dst -> PipeTable)[i] -> delay_val);
                NPprintDBG("in DelZDELAY: prepare to free the PCB", 3);
                free((dst -> PipeTable)[i]);
                (dst -> PipeTable)[i] = 0;
                dst -> OpenedPipe --;
            };
        };
    };
    NPprintDBG("in DelZDELAY: Successfully finish DelZDELAY", 3);
};

/*
This function finalize the PipeControllor. Be-
fore free the object pointed by trgt, it must
be finalized first.
*/
int finalizeControllor(PipeControllor *trgt){
    free(trgt -> PipeTable);
    return 1;
};

/*
This function return a argv array but no count argc
This function is highly probability return a NULL matrix
*/
char **ParseBuffer(char ** argv, char *buff, int bufsize){
    /*
    This function process the original, unmodified got from stdin
    */
    NPprintDBG("inside ParseBuffer function", DBGLVL);

    //char **argv;
    char newline[] = "\n";
    char delimitor[] = " ";

    if(buff[bufsize -1] == '\n'){
        NPprintDBG("newline from command input detected", DBGLVL);
        buff[bufsize -1] = '\0';
    };
    /*
    argv = (char **)calloc((MAXSTDLENGTH + 500), sizeof(char*));
    for(int i = 0; i < MAXSTDLENGTH + 500; i++){
        argv[i] = (char *)calloc((MAXSTDLENGTH + 500), sizeof(char));   
    };
    */

    int indention = 0;
    int shift = 0;

    for(int i = 0; i < bufsize; i++){
        /*parse each symbol*/
        if(buff[i] == ' '){
            NPprintDBG(argv[indention], DBGLVL);
            NPprintDBG("New argv detected", DBGLVL);
            argv[indention][shift] = 0;
            indention ++;
            shift = 0;
        }else{
            argv[indention][shift] = buff[i];
            shift ++;
        };
    };
    //printf("indention: %d, shift %d \n", indention, shift);
    argv[indention][shift] = 0;
    //printf("trying to set zero");
    argv[indention + 1][0] = 0;
    //printf("successfully set zero");
    NPprintDBG("prepare to get out ParseBuffer", DBGLVL);
    return argv;
};

/*
This function count argc according to get in argv,
not counting \0
*/
int CountArgc(char **argv){
    /*
    count argc for the argv
    */
    int rslt = 0;
    
    while(argv[rslt][0] != '\0'){
        rslt ++;
    };
    return rslt;
};

/*
This function take a processed argv array and
return a pointer for the first command pack
note: caller take account for free the returned
pointer
*/
NPcommandPack *ParseCMD(char **argv){
    /*
    the buffer is original, unmodified got from stdin
    */
    NPprintDBG("Just come inside ParseCMD", DBGLVL);

    int argc = CountArgc(argv);
    //printf("In ParseCMD: total argc is: %d \n", argc);
    if(argc == 0){
        return 0;
    };

    NPprintDBG("In ParseCMD: argv is not NULL", DBGLVL);
    int argv_indexer = 0;
    int exec_argc;
    int tmp_dlay = 0;
    int tmp_iterator = 1;
    char *noline = 0;
    char **exec_argv = argv;

    NPcommandPack *Head, *tmp;
    Head = 0;
    tmp = 0;

    exec_argc = 0;

    for(int i = 0; i < argc; i++){
        if(argv[i][0] == '|'){
            /**/
            if (Head == 0){
                /*Initiate Block*/
                Head = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = Head;
            }else{
                tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = tmp -> next;
            };
            initCMDpkg(tmp);
            for(int j = 0; j < exec_argc; j++){
                strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
            };
            (tmp -> cmd_argv)[exec_argc] = 0;
            tmp -> cmd_argc = exec_argc;
            if(argv[i][1] == '\0'){
                tmp -> pipemechanism = ORDPIPE;
                tmp -> delayval = 1;
            }else{
                tmp -> pipemechanism = NUMPIPE;
                tmp -> delayval = 0;
                while(argv[i][tmp_iterator] != '\0'){
                    tmp -> delayval = (tmp -> delayval) * 10 + (argv[i][tmp_iterator] - '0');
                    tmp_iterator ++;
                };
                tmp_iterator = 1;
            };

            exec_argv = argv + i + 1;
            exec_argc = 0;

        }else if(argv[i][0] == '!'){
            if(Head == 0){
                Head = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = Head;
            }else{
                tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = tmp -> next;
            };
            initCMDpkg(tmp);
            for(int j = 0; j < exec_argc; j++){
                strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
            };
            (tmp -> cmd_argv)[exec_argc] = 0;
            tmp -> cmd_argc = exec_argc;
            tmp -> pipemechanism = ERRPIPE;
            if(argv[i][0] == '\0'){
                tmp -> delayval = 1;
            }else{
                tmp -> delayval = 0;
                while(argv[i][tmp_iterator] != '\0'){
                    tmp -> delayval = (tmp -> delayval) * 10 + (argv[i][tmp_iterator] - '0');
                    tmp_iterator ++;
                };
                tmp_iterator = 1;
            };

            exec_argv = argv + i + 1;
            exec_argc = 0;

        }else if(argv[i][0] == '>'){
            /*unhandled yet*/
            if(Head == 0){
                Head = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = Head;
            }else{
                tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = tmp -> next;
            };
            initCMDpkg(tmp);
            for(int j = 0; j < exec_argc; j++){
                strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
            };
            (tmp -> cmd_argv)[exec_argc] = 0;
            tmp -> cmd_argc = exec_argc;
            tmp -> pipemechanism = REDIRECT;
            strcpy(tmp -> filename, argv[i + 1]);
            tmp -> delayval = -1;
            i++;
            exec_argc = 0;

        }else{
            exec_argc ++;
        };
    };
    if(exec_argc != 0){
        /*EOF mechanism*/
        NPprintDBG("In ParseCMD: EOFL detected", DBGLVL);
        if(Head == 0){
            NPprintDBG("In ParseCMD: Try to initialize Head", DBGLVL);
            Head = (NPcommandPack *)malloc(sizeof(NPcommandPack));
            tmp = Head;
            NPprintDBG("In ParseCMD: Success initialize Head", DBGLVL);
        }else{
            tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
            tmp = tmp -> next;
            tmp -> next = 0;
            NPprintDBG("In ParseCMD: Success get next block", DBGLVL);
        };
        NPprintDBG("In ParseCMD: Prepare to init command pack", DBGLVL);
        initCMDpkg(tmp);
        NPprintDBG("In ParseCMD: Success to init command pack", DBGLVL);
        for(int j = 0; j < exec_argc; j++){
            strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
        };
        (tmp -> cmd_argv)[exec_argc] = 0;
        tmp -> cmd_argc = exec_argc;
        tmp -> pipemechanism = EOFL;
        tmp -> delayval = -1;
    };
    return Head;
};

NPcommandPack *NPgetail(NPcommandPack *Head){
    if(Head == 0){
        return 0;
    };

    NPcommandPack *tail;
    tail = 0;
    while(Head -> next != 0){
        Head = Head -> next;
    };
    tail = Head;
    return tail;
};
/*
NPcommandPack *NPreadline(){

    char strtmsg[2] = {'%',' '};
    char *readbuf = NULL;
    char *buf_tmp;

    ssize_t readlen;
    size_t max_len = MAXSTDLENGTH;
    int tmp_readlen;
    int argc;
    char **argv;

    FILE *stdin_fp = fdopen(0, "r");

    NPcommandPack *Head, *tmp_Head, *tail;
    Head = tmp_Head = tail = 0;

    write(1, strtmsg, 2);
    readlen = getline(&readbuf, &max_len, stdin_fp);
    if(readlen <= 0){
        printf("No content read\n");
        exit(0);
    };
    //printf("readed len: %d\n", readlen);
    if(readbuf[0] == '\0'){
        printf("\n");
        exit(0);
    };
    
    argv = ParseBuffer(readbuf, readlen);
    argc = CountArgc(argv);
    //printf("argc = %d\n", argc);
    Head = ParseCMD(argv);

    for(int i = 0; i < MAXSTDLENGTH + 500; i++){
        free(argv[i]);
    };

    free(argv);
    
    return Head;
};
*/

// ------------------- builtin function ----------
/*
In the builtin function, it will not do close or open pipe
operation. Every detail of pipe operation should  be  done
at caller side.

Every function will not free NPcommandPack either. The re-
sponsibility is on caller.
*/
int NPexit(){
    exit(0);
};

int NPsetenv(NPcommandPack *dscrpt){
    if ((dscrpt -> cmd_argc) != 3){
        return -1;
    }else{
        return setenv((dscrpt -> cmd_argv)[1], (dscrpt -> cmd_argv)[2], 1);
    };
};

int NPprintenv(NPcommandPack *dscrpt){
    /**/
    char *gtchar;
    char newline[] = "\n";
    int gtrslt;
    gtchar = getenv((dscrpt -> cmd_argv)[1]);
    if(gtchar == NULL){
        return 0;
    }else{
        gtrslt = write(1, gtchar, strlen(gtchar));
        write(1, newline, 1);
        return gtrslt;
    };
};

int NPredirect(NPcommandPack *dscrpt, int readside){
    if(dscrpt == NULL){
        return -1;
    };
    
    int writeside;
    char readbuf[MAXSTDLENGTH] = {0};
    int readlen;
    writeside = open(dscrpt -> filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
    readlen = read(readside, readbuf, MAXSTDLENGTH);
    while(readlen >= 0){
        write(writeside, readbuf, readlen);
        readlen = read(readside, readbuf, MAXSTDLENGTH);
    };
    close(writeside);
};

int NPprintPTable(NPcommandPack *dscrpt, PipeControllor *trgt){
    for(int i = 0; i < trgt -> Maxavil; i++){
        if((trgt -> PipeTable)[i] != 0){
            printf("in %d th position:\n", i);
            PrintPCB((trgt -> PipeTable)[i]);
        }
    }
};

// ------------------- end of builtin function --

/*
This function execute single command pack
*/
int NPexeSingPack(NPcommandPack *tmp, PipeControllor *PTable){
    //printPKG(tmp);
    if(tmp == 0){
        return -1;
    };

    char exitcmd[] = "exit";
    char setenvcmd[] = "setenv";
    char penvcmd[] = "printenv";
    char printtable[] = "table";
    char tstmsg[] = "enter child\n";
    int execrslt;
    char errmsg[MAXCMDLENGTH] = {0};
    int errlen = 0;
    int pid, status;
    int pipes[2] = {0,1};

    PCB *ref;
    PCB *new_blk;

    if(strcmp((tmp -> cmd_argv)[0], exitcmd) == 0){
        NPexit();
    }else if(strcmp((tmp -> cmd_argv)[0], setenvcmd) == 0){
        NPsetenv(tmp);
    }else if(strcmp((tmp -> cmd_argv)[0], penvcmd) == 0){
        NPprintenv(tmp);
    }else if(strcmp((tmp -> cmd_argv)[0], printtable) == 0){
        NPprintPTable(tmp, PTable);
    }else{
        /*before fork operations on pipes*/
        ref = SearchPCB(PTable, tmp -> delayval);
        if(ref == 0){
            /*have to exam if the pipemechanism is ordinary one or number pipe or EOF or redirection*/
            NPprintDBG("in NPexeSingPack: Do not get any write destination block", 3);
            if(tmp -> pipemechanism == EOFL){
                pipes[1] = 1;
            }else if(tmp -> pipemechanism == REDIRECT){
                pipes[1] = open(tmp -> filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
            }else{
                //NPprintSinglePack(tmp);
                pipe(pipes);
                //printf("read side of pipe: %d, write side of pipe: %d\n", pipes[0], pipes[1]);

                /*insert pipe result into pipe controllor*/
                new_blk = (PCB *)malloc(sizeof(PCB));
                new_blk -> readPipe = pipes[0];
                new_blk -> writePipe = pipes[1];
                new_blk -> errorPipe = 2;
                new_blk -> delay_val = tmp -> delayval;
                InsertPCB(PTable, new_blk);
            };
        }else{
            pipes[1] = ref -> writePipe;
        };
        // printf("trying to fork\n");
        pid = fork();
        // printf("forked pid: %d\n", pid);
        while(pid < 0){
            //printf("fork fail \n");
            usleep(1000);
            pid = fork();
        };
        if(pid == 0){
            
            dup2(pipes[1], 1);
            ref = SearchPCB(PTable, 0);
            if(ref != 0){
                dup2(ref -> readPipe, 0);
            }else{
                NPprintDBG("In child process: no pipe in message side found", 3);
            };
            /*check if the command is error pipe mechanism*/
            if(tmp -> pipemechanism == ERRPIPE){
                //printf("error pipe mechanism detected\n");
                dup2(pipes[1],2);
            };
            /*child suppose to close all pipe except stdin and stdout and stderr*/
            for(int i = 3; i < 1024; i++){
                close(i);
            };
            
            execrslt = execvp((tmp -> cmd_argv)[0], (tmp -> cmd_argv));
            if(execrslt < 0){
                errlen = sprintf(errmsg, "Unknown command: [%s].\n", tmp -> cmd_argv[0]);
                write(2, errmsg, strlen(errmsg));
            };
            exit(0);
        }else{
            DelZDELAY(PTable);
            if((tmp -> pipemechanism == EOFL) || (tmp -> pipemechanism == REDIRECT)){
                waitpid(pid, &status, 0);
                if(tmp -> pipemechanism == REDIRECT){
                    close(pipes[1]);
                };
            };
        };
    };
    DECDVAL(PTable);

};

/*
exec whole list of Command pack
*/
int NPexeCMDPack(NPcommandPack *trgt, PipeControllor *PTable){
    NPprintDBG("In NPexeCMDPack: get into NPexeCMDPack", 3);

    NPcommandPack *tmp;
    /*printPKG(trgt);*/
    
    while(trgt != 0){
        tmp = trgt;
        trgt = trgt -> next;
        NPprintDBG("In NPexeCMDPack: Trying to execute single pack because target is not 0", 3);
        //printf("---------------------------------------------------------------\n");
        //printf("in piping number %d\n", numb);
        NPexeSingPack(tmp, PTable);
        //printf("---------------------------------------------------------------\n");
        NPprintDBG("In NPexeCMDPack: Finish executing single pack", 3);
        NPprintDBG("In NPexeCMDPack: trying to finalize the target pack", 3);
        finalizeCMDpkg(tmp);
        NPprintDBG("In NPexeCMDPack: trying to free the target pack", 3);
        free(tmp);
    };
    
    NPprintDBG("In NPexeCMDPack: Finish the NPexeCMDPack function", 3);
};

int main(int main_argc, char **main_argv){
    signal(SIGCHLD, childHandler);

    // ------------------- for socket creation ---------------------
    int server_fd, new_socket, valread, pid, port, status;
    int port_index = 0;
    struct sockaddr_in address; 
    int opt = 1;
    int addrlen = sizeof(address); 
    char buffer[1024] = {0};
    char tmp_port[20] = {0};

    if(main_argc > 1){
        port = 0;
        while(main_argv[1][port_index] != '\0'){
            port = port * 10 + (main_argv[1][port_index] - '0');
            port_index ++;
        };
        port_index = 0;
    }else{
        port = 7070;
    };
    clearenv();
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("successfully get server_fe\n");
       
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    }
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port );
    printf("attach socket to %d\n", port);
       
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    printf("bind socket to %d\n", port);
    if (listen(server_fd, 3) < 0) {
        printf("listen fail\n");
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }
    printf("successfully listening\n");
    fflush(stdout);
    // ------------------- end socket creation ---------------------

    char strtmsg[2] = {'%',' '};
    char *readbuf = NULL;
    char *buf_tmp;

    ssize_t readlen;
    size_t max_len = MAXSTDLENGTH;
    int tmp_readlen;
    int argc;
    int bufsize = 0;
    char **argv;
    argv = (char **)calloc((MAXSTDLENGTH + 500), sizeof(char*));
    for(int i = 0; i < MAXSTDLENGTH + 500; i++){
        argv[i] = (char *)calloc((MAXSTDLENGTH + 500), sizeof(char));   
    };

    FILE *stdin_fp = fdopen(0, "r");
    
    // -----------------------starting create socket---------------------------
    // -----------------------end of creating socket---------------------------

    NPcommandPack *ExecCMD, *Head;
    PipeControllor *Controllor;

    Controllor = (PipeControllor *)malloc(sizeof(PipeControllor));
    initControllor(Controllor);

    setenv("PATH", "bin:.", 1);
    write(1, strtmsg, 2);

    while((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                (socklen_t*)&addrlen))>=0){
        printf("accept new connection\n");
        fflush(stdout);
        pid = fork();
        if(pid > 0){
            close(new_socket);
            waitpid(pid, &status, 0);
        }else{
            dup2(new_socket, 0);
            dup2(new_socket, 1);
            dup2(new_socket, 2);
            write(1, strtmsg, 2);
            while(readlen = getline(&readbuf, &max_len, stdin_fp) != -1){
                if(readbuf[0] == '\0'){
                    printf("\n");
                    exit(0);
                };
                while((readbuf[bufsize] != '\n') && (readbuf[bufsize] != '\r')){
                    bufsize++;
                };
                readbuf[bufsize] = '\0';
                bufsize ++;
                argv = ParseBuffer(argv, readbuf, bufsize);
                bufsize = 0;
                argc = CountArgc(argv);
                Head = ParseCMD(argv);
                //write(1, "H", 1);
                NPexeCMDPack(Head, Controllor);
                write(1, strtmsg, 2);
                /*
                while(readlen = getline(&readbuf, &max_len, stdin_fp) != -1){
                    printf("Retrieved line of length %zu :\n", readlen);
                    printf("%s", readbuf);
                };
                for(int i = 0; i < MAXSTDLENGTH + 500; i++){
                    free(argv[i]);
                };

                free(argv);
                */
                
            };

            finalizeControllor(Controllor);
            free(Controllor);
            exit(0);
        };
    };
};



/*
parent will crash when decdval;
unhandle unknown command yet;
mishandle ordinary pipe;
ls |2 ls |1 number is ok but ls |1 ls |1 number will cause segmentation fault
ls | ls | cat, cat will not get propriate stdin
reidrection mode have some issue when open file
have to wait error pipe to finish writing then print %
*/

//---------------------------------------------- new