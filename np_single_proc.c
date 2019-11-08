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

int max(int a, int b){
    if(a > b){
        return a;
    }else{
        return b;
    };
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
        /*
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
        }else if((trgt -> pipemechanism == NPIPE_IN)){
            printf("pipemechanism is Name Pipe in\n");
            printf("target command set the pipe in from client: %d\n", trgt -> pipefrom_client);
        }else if(trgt -> pipemechanism == NPIPE_OUT){
            printf("pipemechanism is Name Pipe out\n");
            printf("target command set the pipe to client: %d\n", trgt -> trgt_client);
        };
        if((trgt -> pipemechanism == NPIPE_IN) || (trgt -> pipemechanism == NPIPE_OUT)){
            
        }else{
            printf("delay value is %d\n", trgt -> delayval);
        };
        */
        printf("pipe from client number %d\n", trgt -> pipefrom_client);
        if(trgt -> pipefrom_client >= 0){
            printf("pipe from client number %d\n", trgt -> pipefrom_client);
        }else{};
        if(trgt -> trgt_client == 0){
            printf("pipe to client itself\n");
        }else if(trgt -> trgt_client > 0){
            printf("pipe to client number %d\n", trgt -> trgt_client);
        };
        if(trgt -> delayval >= 0){
            printf("pipe to next %d command for client itself [client id is %d]\n", trgt -> delayval, trgt -> client_id);
        }else{};
        if((trgt -> filename)[0] != '\0'){
            printf("pipe to file name with %s\n", (trgt -> filename));
        }else{};
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
    NPprintDBG("Get into initCMDpkg function", DBGLVL);
    (trgt -> trgt_client) = -1;
    trgt -> pipefrom_client = -1;
    trgt -> delayval = -1;
    trgt -> client_id = -1;
    (trgt -> origin_cmd) = (char *)calloc(2 * MAXCMDLENGTH, sizeof(char));
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
    free(trgt -> origin_cmd);
    free(trgt -> cmd_argv);
    NPprintDBG("in finalizeCMDpgk: sussessfully finalize command pack", 3);
    
};

int initusrpipe(UserPipe *trgt){
    (trgt -> is_activate) = false;
    trgt -> readside = -1;
    trgt -> writeside = -1;
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

int NPprintControllorPool(ControllorPool *trgt){
    printf("----------\nprinting ControllorPool maintain by server\n--------\n");
    printf("activated clients number is %d\n", trgt -> activated_clients_num);
    for(int i = 0; i < MAXCLIENT; i++){
        if((trgt -> active_flag)[i] == true){
            printf("In client id %d, printing the detail...\n", i);
            printf("    The occupied data transfer socket is %d \n", (trgt -> occupied_sfd)[i]);
            printf("    The occupied socket file descriptor is %d\n", (trgt -> occupied_sfd)[i]);
            printf("    The name of client id %d is %s \n", i, (trgt -> user_name)[i]);
            printf("    The port of the client id %d is %s\n", i, (trgt -> port_name)[i]);
            printf("    The ip address of the client id %d is %s\n", i, (trgt -> ip_addr)[i]);
            printf("    The PATH content of the client id %d is %s\n", i , (trgt -> PATH_cont)[i]);
            printf("--------------------------------------------------\n");
        };
    };
    fflush(stdout);
};


/*
This function initialize the Controllor pool
according to the pointer passed by caller. obje-
ct point by trgt at best lie in heap.
*/
int initControllorPool(ControllorPool *trgt){
    //
    char dflt_path_cont[] = "bin:.";
    char dflt_usr_name[] = "(no name)";
    trgt -> activated_clients_num = 0;
    trgt -> MainPool = (PipeControllor **)calloc(MAXCLIENT, sizeof(PipeControllor *));
    trgt -> active_flag = (bool *)calloc(MAXCLIENT, sizeof(bool));
    trgt -> occupied_sfd = (int *)calloc(MAXCLIENT, sizeof(int));
    trgt -> user_name = (char **)calloc(MAXCLIENT, sizeof(char *));
    trgt -> userpipe = (UserPipe **)calloc(MAXCLIENT, sizeof(UserPipe*));
    trgt -> port_name = (char **)calloc(MAXCLIENT, sizeof(char *));
    trgt -> ip_addr = (char **)calloc(MAXCLIENT, sizeof(char *));
    trgt -> PATH_cont = (char **)calloc(MAXCLIENT, sizeof(char *));
    printf("initControllor Pool first check point \n");
    fflush(stdout);
    for(int i = 0; i < MAXCLIENT; i ++){
        (trgt -> user_name)[i] = (char *)calloc(MAX_NLEN, sizeof(char));
        (trgt -> port_name)[i] = (char *)calloc(20, sizeof(char));
        (trgt -> ip_addr)[i] = (char *)calloc(200, sizeof(char));
        (trgt -> userpipe)[i] = (UserPipe *)calloc(MAXCLIENT, sizeof(UserPipe));
        (trgt -> PATH_cont)[i] = (char *)calloc(200, sizeof(char));
        (trgt -> MainPool)[i] = (PipeControllor *)calloc(1, sizeof(PipeControllor));
        initControllor((trgt -> MainPool)[i]);
        strcpy((trgt -> PATH_cont)[i], dflt_path_cont);
        strcpy((trgt -> user_name)[i], dflt_usr_name);
        for(int j = 0; j < MAXCLIENT; j++){
            initusrpipe((trgt -> userpipe)[i] + j);
        };
    };
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
int DelZDELAY(PipeControllor *dst, int offset = 0){
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
            if(((dst -> PipeTable)[i] -> delay_val) <= offset){
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
            while(buff[i+1] == ' '){
                i++;
            };
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
NPcommandPack *ParseCMD(char **argv, char *origin, int client_id = -1){
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
    int tmp_named_pipe = 0;
    int tmp_iterator = 1;
    char *noline = 0;
    char **exec_argv = argv;
    char yell_dflt[] = "yell";
    char tell_dflt[] = "tell";

    NPcommandPack *Head, *tmp;
    Head = 0;
    tmp = 0;

    exec_argc = 0;

    if((strcmp(argv[0], yell_dflt) == 0) || 
        (strcmp(argv[0], tell_dflt) == 0)){

        tmp = (NPcommandPack *)malloc(sizeof(NPcommandPack));
        initCMDpkg(tmp);
        strcpy(tmp -> origin_cmd, origin);
        for(int i = 0; i < argc; i++){
            strcpy((tmp -> cmd_argv)[i], argv[i]);
        };
        (tmp -> cmd_argv)[argc] = 0;
        tmp -> cmd_argc = argc;
        return tmp;
    }else{};

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
            strcpy(tmp -> origin_cmd, origin);
            tmp -> client_id = client_id;
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
            if(argv[i + 1][0] == '<'){
                //oh~, some client have to pipe in to this command
                tmp_iterator = 1;
                tmp -> pipefrom_client = 0;
                while(argv[i + 1][tmp_iterator] != '\0'){
                    tmp -> pipefrom_client = (tmp -> pipefrom_client) * 10 + (argv[i + 1][tmp_iterator] - '0');
                    tmp_iterator ++;
                };
                tmp -> pipefrom_client --;
                i ++;
                tmp_iterator = 1;
            }else{};
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
            strcpy(tmp -> origin_cmd, origin);
            tmp -> client_id = client_id;
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
            if(argv[i + 1][0] == '<'){
                //oh~, some client have to pipe in to this command
                tmp_iterator = 1;
                tmp -> pipefrom_client = 0;
                while(argv[i + 1][tmp_iterator] != '\0'){
                    tmp -> pipefrom_client = (tmp -> pipefrom_client) * 10 + (argv[i + 1][tmp_iterator] - '0');
                    tmp_iterator ++;
                };
                tmp -> pipefrom_client --;
                i ++;
                tmp_iterator = 1;
            }else{};

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
            strcpy(tmp -> origin_cmd, origin);
            tmp -> client_id = client_id;
            for(int j = 0; j < exec_argc; j++){
                strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
            };
            (tmp -> cmd_argv)[exec_argc] = 0;
            tmp -> cmd_argc = exec_argc;
            if(argv[i][1] != '\0'){
                tmp -> trgt_client = 0;
                // this is named pipe out
                tmp -> pipemechanism = NPIPE_OUT;
                tmp_iterator = 1;
                while(argv[i][tmp_iterator] != '\0'){
                    tmp -> trgt_client = (tmp -> trgt_client) * 10 + (argv[i][tmp_iterator] - '0');
                    tmp_iterator ++;
                };
                tmp -> trgt_client --;
                tmp_iterator = 1;
                if(argv[i+1][0] == '<'){
                    //pipein from certain client id
                    tmp -> pipefrom_client = 0;
                    tmp_iterator = 1;
                    while(argv[i+1][tmp_iterator] != '\0'){
                        tmp -> pipefrom_client = (tmp -> pipefrom_client) *10 + (argv[i+1][tmp_iterator] - '0');
                        tmp_iterator ++;
                    };
                    tmp -> pipefrom_client --;
                    tmp_iterator = 1;
                    i++;
                }else{};
            }else{
                //redirect pipe out
                tmp -> pipemechanism = REDIRECT;
                strcpy(tmp -> filename, argv[i + 1]);
                tmp -> delayval = -1;
                i++;
                if(argv[i+1][0] == '<'){
                    tmp -> pipefrom_client = 0;
                    tmp_iterator = 1;
                    while(argv[i+1][tmp_iterator] != '\0'){
                        tmp -> pipefrom_client = (tmp -> pipefrom_client) *10 + (argv[i+1][tmp_iterator] - '0');
                        tmp_iterator ++;
                    };
                    tmp -> pipefrom_client --;
                    tmp_iterator = 1;
                    i++;
                };
            };
            exec_argv = argv + i + 1;
            exec_argc = 0;

        }else if(argv[i][0] == '<'){
            if(Head == 0){
                Head = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = Head;
            }else{
                tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
                tmp = tmp -> next;
            };
            initCMDpkg(tmp);
            strcpy(tmp -> origin_cmd, origin);
            tmp -> client_id = client_id;
            for(int j = 0; j < exec_argc; j++){
                strcpy((tmp -> cmd_argv)[j], exec_argv[j]);
            };
            (tmp -> cmd_argv)[exec_argc] = 0;
            tmp -> cmd_argc = exec_argc;
            tmp -> pipefrom_client = 0;
            tmp_iterator = 1;
            tmp -> pipemechanism = NPIPE_IN;
            while(argv[i][tmp_iterator] != '\0'){
                tmp -> pipefrom_client = (tmp -> pipefrom_client) * 10 + (argv[i][tmp_iterator] - '0');
                tmp_iterator ++;
            };
            tmp -> pipefrom_client --;
            tmp_iterator = 1;
            if(argv[i+1][0] == '|'){
                i++;
                if(argv[i][1] == '\0'){
                    tmp -> pipemechanism = ORDPIPE;
                    tmp -> delayval = 1;
                }else{
                    tmp -> pipemechanism = NUMPIPE;
                    tmp -> delayval = 0;
                    tmp_iterator = 1;
                    while(argv[i][tmp_iterator] != '\0'){
                        tmp -> delayval = (tmp -> delayval) * 10 + (argv[i][tmp_iterator] - '0');
                        tmp_iterator ++;
                    };
                    tmp_iterator = 1;
                };
            }else if(argv[i+1][0] == '!'){
                i++;
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
            }else if(argv[i+1][0] == '>'){
                i++;
                if(argv[i][1] != '\0'){
                    tmp -> trgt_client = 0;
                    // this is named pipe out
                    tmp -> pipemechanism = NPIPE_OUT;
                    tmp_iterator = 1;
                    while(argv[i][tmp_iterator] != '\0'){
                        tmp -> trgt_client = (tmp -> trgt_client) * 10 + (argv[i][tmp_iterator] - '0');
                        tmp_iterator ++;
                    };
                    tmp -> trgt_client --;
                    tmp_iterator = 1;
                }else{
                    tmp -> pipemechanism = REDIRECT;
                    strcpy(tmp -> filename, argv[i + 1]);
                    tmp -> delayval = -1;
                    i++;
                };
            };
            exec_argv = argv + i + 1;
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
            tmp -> client_id = client_id;
            NPprintDBG("In ParseCMD: Success initialize Head", DBGLVL);
        }else{
            tmp -> next = (NPcommandPack *)malloc(sizeof(NPcommandPack));
            tmp = tmp -> next;
            tmp -> next = 0;
            NPprintDBG("In ParseCMD: Success get next block", DBGLVL);
        };
        NPprintDBG("In ParseCMD: Prepare to init command pack", DBGLVL);
        initCMDpkg(tmp);
        strcpy(tmp -> origin_cmd, origin);
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
int NPwho(ControllorPool *ref, NPcommandPack *cmd, int src_id){
    char basemsg[] = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    char true_msg[500] = {0};
    write((ref -> occupied_sfd)[src_id], basemsg, strlen(basemsg));
    for(int i = 0; i < MAXCLIENT; i ++){
        if((ref -> active_flag)[i]){
            //
            if(src_id == i){
                //indicate me
                sprintf(true_msg, "%d\t%s\t%s:%s\t <- me\n", i + 1, (ref -> user_name)[i],
                (ref -> ip_addr)[i], (ref -> port_name)[i]);
            }else{
                sprintf(true_msg, "%d\t%s\t%s:%s\n", i + 1, (ref -> user_name)[i],
                (ref -> ip_addr)[i], (ref -> port_name)[i]);
                //do not indicat me
            };
            write((ref -> occupied_sfd)[src_id], true_msg, strlen(true_msg));
        }else{};
    };
};

int NPtell(ControllorPool *ref, NPcommandPack *cmd, int src_id){
    // the src_id should be pre-processed
    int trgt_csfd, trgt_id;
    int src_csdf;
    int argv_index = 0;
    char tellmsg[1024] = {0};
    char fullmsg[1024] = {0};
    int msg_ind = 0;
    char *msgstrt;
    trgt_id = 0;
    while((cmd -> cmd_argv)[1][argv_index] != '\0'){
            trgt_id = trgt_id * 10 + (cmd -> cmd_argv)[1][argv_index] - '0';
            argv_index ++;
        };
    trgt_id -= 1;
    msgstrt = cmd -> origin_cmd;
    msgstrt += 5;
    while((msgstrt[msg_ind] - '0' >= 0) && (msgstrt[msg_ind] - '0' <= 10)){
        msgstrt ++;
    };
    msgstrt ++;
    if(dump_argv(tellmsg, (cmd -> cmd_argv) + 2, 0) < 0){
        sprintf(fullmsg, "You must tell something to your target\n");
        write((ref -> occupied_sfd)[src_id], fullmsg, strlen(fullmsg));
        return -1;
    };
    (ref -> active_flag)[trgt_id];
    if(!(ref -> active_flag)[trgt_id]){
        sprintf(fullmsg, "Error: user #%d down not exist yet.\n", trgt_id + 1);
        write((ref -> occupied_sfd)[src_id], fullmsg, strlen(fullmsg));
        return -1;
    }else{
        trgt_csfd = (ref -> occupied_sfd)[trgt_id];
        
        sprintf(fullmsg, "*** %s told you ***: %s\n", 
            (ref -> user_name)[src_id], msgstrt);

        write(trgt_csfd, fullmsg, strlen(fullmsg));
        return 1;
    };
    return -1;

};

int NPyell(ControllorPool *ref, char *msg, int src_sfd = -1){
    if(src_sfd >= 0){
        char strtmsg[1024] = {0};
        for(int i = 0; i < MAXCLIENT; i ++){
            if((ref -> occupied_sfd)[i] == src_sfd && (ref -> active_flag)[i]){
                //
                sprintf(strtmsg, "*** %s yelled ***: %s\n", (ref -> user_name)[i], msg);
                msg = strtmsg;
                break;
            }else{
                printf("no such client in pool\n");
                fflush(stdout);
            }
        };
        
    }else{};
    for(int i = 0; i < MAXCLIENT; i++){
        if((ref -> active_flag)[i]){
            // you have to be told
            write((ref -> occupied_sfd)[i], msg, strlen(msg));
        }else{};
    };
    return 1;
};

int NPlogin(int client_sfd, struct sockaddr_in addr, ControllorPool *trgt_pool){
    //yell to every one that one client login
    //prepare pool for the client_fd
    /*
    bool *active_flag;
    int activated_clients_num;
    int *occupied_sfd;

    char **user_name;
    char **port_name;
    char **ip_addr;
    char **PATH_cont;
    UserPipe **userpipe;
    PipeControllor *MainPool;
    */
    if(client_sfd < 0){
        return -1;
    };

    char welcommsg_head[80] = {0};
    char welcommsg_body[80]= {0};
    char welcommsg[200] = {0};
    sprintf(welcommsg_head, "***************************************\n");
    sprintf(welcommsg_body, "** Welcome to the information server **\n");
    sprintf(welcommsg, "%s%s%s", welcommsg_head, welcommsg_body, welcommsg_head);

    printf("login detected\n");
    if((trgt_pool -> activated_clients_num) >= MAXCLIENT){
        printf("login failed\n");
        return -1;
    }else{
        for(int i = 0; i < MAXCLIENT; i++){
            if((trgt_pool -> active_flag)[i]){
                //
                continue;
            }else{
                //
                char login_msg[200] = {0};
                (trgt_pool -> active_flag)[i] = true;
                (trgt_pool -> occupied_sfd)[i] = client_sfd;
                (trgt_pool -> activated_clients_num)++;
                sprintf((trgt_pool -> port_name)[i], "%d", ntohs(addr.sin_port));
                sprintf((trgt_pool -> ip_addr)[i], "%s", inet_ntoa(addr.sin_addr));
                printf("after login, the client ip is %s\n", inet_ntoa(addr.sin_addr));
                printf("after login, the client port is from %d\n", ntohs(addr.sin_port));
                fflush(stdout);
                sprintf(login_msg, "*** User '%s' entered from %s:%s.***\n",(trgt_pool -> user_name)[i] , (trgt_pool -> ip_addr)[i],
                        (trgt_pool -> port_name)[i]);
                write(client_sfd, welcommsg, strlen(welcommsg));
                NPyell(trgt_pool, login_msg);
                return 1;
            };
        };
        return -1;
    };
};

int NPlogout(int client_sfd, ControllorPool *trgt_pool){
    /*
    bool *active_flag;
    int activated_clients_num;
    int *occupied_sfd;

    char **user_name;
    char **port_name;
    char **ip_addr;
    char **PATH_cont;
    UserPipe **userpipe;
    PipeControllor *MainPool;
    */
    char dflt_path[] = "bin:.";
    char dflt_name[] = "(no name)";
    char logout_msg[200] = {0};
    for(int i = 0; i < MAXCLIENT; i ++){
        if((trgt_pool -> occupied_sfd)[i] == client_sfd){
            //
            (trgt_pool -> active_flag)[i] = false;
            (trgt_pool -> occupied_sfd)[i] = -1;
            sprintf(logout_msg, "*** User '%s' left. ***\n", (trgt_pool -> user_name)[i]);
            write(client_sfd, logout_msg, strlen(logout_msg));
            close(client_sfd);
            memset((trgt_pool -> user_name)[i], '\0', strlen((trgt_pool -> user_name)[i]));
            memset((trgt_pool -> port_name)[i], '\0', strlen((trgt_pool -> port_name)[i]));
            strcpy((trgt_pool -> user_name)[i], dflt_name);
            memset((trgt_pool -> ip_addr)[i], '\0', strlen((trgt_pool -> ip_addr)[i]));
            memset((trgt_pool -> PATH_cont)[i], '\0', strlen((trgt_pool -> PATH_cont)[i]));
            strcpy((trgt_pool -> PATH_cont)[i], dflt_path);
            trgt_pool -> activated_clients_num --;
    
            for(int j = 0; j < MAXCLIENT; j++){
                if(((trgt_pool -> userpipe)[i][j]).is_activate){
                    close(((trgt_pool -> userpipe)[i][j]).writeside);
                    close(((trgt_pool -> userpipe)[i][j]).readside);
                    ((trgt_pool -> userpipe)[i][j]).writeside = -1;
                    ((trgt_pool -> userpipe)[i][j]).readside = -1;
                    ((trgt_pool -> userpipe)[i][j]).is_activate = false;
                }else{};
                if(((trgt_pool -> userpipe)[j][i]).is_activate){
                    close(((trgt_pool -> userpipe)[j][i]).readside);
                    close(((trgt_pool -> userpipe)[i][j]).writeside);
                    ((trgt_pool -> userpipe)[j][i]).is_activate = false;
                    ((trgt_pool -> userpipe)[j][i]).readside = -1;
                    ((trgt_pool -> userpipe)[i][j]).writeside = -1;
                }else{};
            };
            DelZDELAY((trgt_pool -> MainPool)[i], 65536);
            finalizeControllor((trgt_pool -> MainPool)[i]);
            initControllor((trgt_pool -> MainPool)[i]);
            NPyell(trgt_pool, logout_msg);
            return 1;
            break;
  
        }else{};
    };
    return -1;
};

/*
In the builtin function, it will not do close or open pipe
operation. Every detail of pipe operation should  be  done
at caller side.

Every function will not free NPcommandPack either. The re-
sponsibility is on caller.
*/
int NPexit(int client_sfd, ControllorPool *trgt_pool){
    NPlogout(client_sfd, trgt_pool);
    return 1;
};

int NPsetenv(NPcommandPack *dscrpt, int client_id, ControllorPool *ClientPool){
    if(!(ClientPool -> active_flag)[client_id]){
        printf("un active client trying to setenv\n");
        fflush(stdout);
        return -1;
    };
    if ((dscrpt -> cmd_argc) != 3){
        return -1;
    }else{
        memset((ClientPool -> PATH_cont)[client_id], '\0', 200);
        strcpy((ClientPool -> PATH_cont)[client_id], (dscrpt -> cmd_argv[2]));
        return setenv((dscrpt -> cmd_argv)[1], (dscrpt -> cmd_argv)[2], 1);
    };
};

int NPprintenv(NPcommandPack *dscrpt, int client_id, ControllorPool *ClientPool){
    /**/
    if(client_id < 0){
        printf("invalid for negative client_id number %d\n", client_id);
        fflush(stdout);
    }else if(!(ClientPool -> active_flag)[client_id]){
        printf("unactive client movement\n");
        fflush(stdout);
    };
    char gtchar[200] = {0};
    char newline[] = "\n";
    int gtrslt;
    strcpy(gtchar, (ClientPool -> PATH_cont)[client_id]);
    if(gtchar == NULL){
        return 0;
    }else{
        gtrslt = write((ClientPool -> occupied_sfd)[client_id], gtchar, strlen(gtchar));
        write((ClientPool -> occupied_sfd)[client_id], newline, 1);
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

int NPname(ControllorPool *ref, NPcommandPack *cmd, int src_id){
    //The src_id should be wellset and preproces
    bool same_flag = false;
    char yellmsg[1024] = {0};
    if(src_id < 0){
        printf("invalid named operation by negative src_id %d\n");
        fflush(stdout);
        return -1;
    }
    char name[20] = {0};
    strcpy(name, (cmd -> cmd_argv)[1]);
    for(int i = 0; i < MAXCLIENT; i ++){
        if(strcmp(name, (ref -> user_name)[i]) == 0){
            same_flag = true;
            break;
        }else{};
    };
    if(!(same_flag)){
        strcpy((ref -> user_name)[src_id], name);
        sprintf(yellmsg, "*** User from %s is named '%s'. ***\n", (ref -> ip_addr)[src_id], name);
        NPyell(ref, yellmsg);
        return 1;
    }else{
        sprintf(yellmsg, "User '%s' already exists.\n", name);
        write((ref -> occupied_sfd)[src_id], yellmsg, strlen(yellmsg));
        return -1;
    };
};
// ------------------- end of builtin function --

/*
This function execute single command pack
*/
int NPexeSingPack(NPcommandPack *tmp, ControllorPool *ClientPool, int exe_csfd, int client_id){
    //printPKG(tmp);
    if(tmp == 0){
        return -1;
    };

    char exitcmd[] = "exit";
    char setenvcmd[] = "setenv";
    char penvcmd[] = "printenv";
    char printtable[] = "table";
    char tell[] = "tell";
    char name[] = "name";
    char yll[] = "yell";
    char who[] = "who";
    char tstmsg[] = "enter child\n";
    int execrslt;
    char fllcmd[200] = {0};
    char pipemsg[2000] = {0};
    char errmsg[MAXCMDLENGTH] = {0};
    int errlen = 0;
    int pid, status;
    int pipes[2] = {0,1};

    PCB *ref;
    PCB *new_blk;

    if(strcmp((tmp -> cmd_argv)[0], exitcmd) == 0){
        NPexit(exe_csfd, ClientPool);
    }else if(strcmp((tmp -> cmd_argv)[0], setenvcmd) == 0){
        NPsetenv(tmp, client_id, ClientPool);
    }else if(strcmp((tmp -> cmd_argv)[0], penvcmd) == 0){
        NPprintenv(tmp, client_id, ClientPool);
    }else if(strcmp((tmp -> cmd_argv)[0], printtable) == 0){
        NPprintPTable(tmp, (ClientPool -> MainPool)[client_id]);
    }else if(strcmp((tmp -> cmd_argv)[0], tell) == 0){
        NPprintSinglePack(tmp);
        NPtell(ClientPool, tmp, client_id);
    }else if(strcmp((tmp -> cmd_argv)[0], name) == 0){
        NPname(ClientPool, tmp, client_id);
    }else if(strcmp((tmp -> cmd_argv)[0], yll) == 0){
        NPprintSinglePack(tmp);
        dump_argv(pipemsg, tmp -> cmd_argv + 1);
        NPyell(ClientPool, pipemsg, exe_csfd);
    }else if(strcmp((tmp -> cmd_argv)[0], who) == 0){
        //
        NPwho(ClientPool, tmp, client_id);
    }else{
        NPprintSinglePack(tmp);
        /*before fork operations on pipes*/
        if((tmp -> pipefrom_client) >= 0){
            if(!(ClientPool -> active_flag)[tmp -> pipefrom_client]){
                sprintf(errmsg, "*** Error: user #%d does not exist yet. ***\n", tmp -> pipefrom_client +1);
                write(exe_csfd, errmsg, strlen(errmsg));
                DelZDELAY((ClientPool -> MainPool)[client_id]);
                DECDVAL((ClientPool -> MainPool)[client_id]);
                return -1;
            }else{};
            if(!(ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].is_activate){
                sprintf(errmsg, "The client you specified does not pipe you any thing\n");
                write(exe_csfd, errmsg, strlen(errmsg));
                DelZDELAY((ClientPool -> MainPool)[client_id]);
                DECDVAL((ClientPool -> MainPool)[client_id]);
                return -1;
            }else{};
        };

        if(tmp -> trgt_client >= 0){
            // user pipe
            if(!(ClientPool -> active_flag)[tmp -> trgt_client]){
                //
                sprintf(errmsg, "*** Error: user #%d does not exist yet. ***\n", tmp -> trgt_client +1);
                write(exe_csfd, errmsg, strlen(errmsg));
                DelZDELAY((ClientPool -> MainPool)[client_id]);
                DECDVAL((ClientPool -> MainPool)[client_id]);
                return -1;
            }else{};

            if(!(ClientPool -> userpipe)[client_id][tmp -> trgt_client].is_activate){
                pipe(pipes);
                (ClientPool -> userpipe)[client_id][tmp -> trgt_client].is_activate = true;
                (ClientPool -> userpipe)[client_id][tmp -> trgt_client].readside = pipes[0];
                (ClientPool -> userpipe)[client_id][tmp -> trgt_client].writeside = pipes[1];
                
            }else{
                //print error message
                sprintf(errmsg, "*** Error: the pipe already exists. ***\n");
                write(exe_csfd, errmsg, strlen(errmsg));
                DelZDELAY((ClientPool -> MainPool)[client_id]);
                DECDVAL((ClientPool -> MainPool)[client_id]);
                return -1;
            };
        }else if((tmp -> filename)[0] != '\0'){
            pipes[1] = open(tmp -> filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        }else if(tmp -> delayval >= 0){
            ref = SearchPCB((ClientPool -> MainPool)[client_id], tmp -> delayval);
            if(ref == 0){
                //NPprintSinglePack(tmp);
                pipe(pipes);
                //printf("read side of pipe: %d, write side of pipe: %d\n", pipes[0], pipes[1]);

                /*insert pipe result into pipe controllor*/
                new_blk = (PCB *)malloc(sizeof(PCB));
                new_blk -> readPipe = pipes[0];
                new_blk -> writePipe = pipes[1];
                new_blk -> errorPipe = 2;
                new_blk -> delay_val = tmp -> delayval;
                InsertPCB((ClientPool -> MainPool)[client_id], new_blk);
            }else{
                pipes[1] = ref -> writePipe;
            };
        }else{
            pipes[1] = exe_csfd;
        };
        /*
        if(ref == 0){
            
            NPprintDBG("in NPexeSingPack: Do not get any write destination block", 3);
            if(tmp -> pipemechanism == EOFL){
                pipes[1] = exe_csfd;
            }else if((tmp -> filename)[0] != '\0'){
                pipes[1] = open(tmp -> filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
            }else if(tmp -> trgt_client > 0){
                // user pipe
                if(!(ClientPool -> userpipe)[client_id][tmp -> trgt_client].is_activate){
                    pipe(pipes);
                    (ClientPool -> userpipe)[client_id][tmp -> trgt_client].is_activate = true;
                    (ClientPool -> userpipe)[client_id][tmp -> trgt_client].readside = pipes[0];
                    (ClientPool -> userpipe)[client_id][tmp -> trgt_client].writeside = pipes[1];
                }else{
                    pipes[1] = (ClientPool -> userpipe)[client_id][tmp -> trgt_client].writeside;
                };
            }else{
                //NPprintSinglePack(tmp);
                pipe(pipes);
                //printf("read side of pipe: %d, write side of pipe: %d\n", pipes[0], pipes[1]);

                //insert pipe result into pipe controllor
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
        */

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
            if(tmp -> pipefrom_client >= 0){
                pipes[0] = (ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].readside;
                dup2(pipes[0], 0);
            }else{
                ref = SearchPCB((ClientPool -> MainPool)[client_id], 0);
                if(ref != 0){
                    dup2(ref -> readPipe, 0);
                }else{
                    NPprintDBG("In child process: no pipe in message side found", 3);
                };
                /*check if the command is error pipe mechanism*/
            };
            if(tmp -> pipemechanism == ERRPIPE){
                //printf("error pipe mechanism detected\n");
                dup2(pipes[1],2);
            }else{
                dup2(exe_csfd, 2);
            };

            /*child suppose to close all pipe except stdin and stdout and stderr*/
            for(int i = 3; i < 1024; i++){
                close(i);
            };
            
            execrslt = execvp((tmp -> cmd_argv)[0], (tmp -> cmd_argv));
            if(execrslt < 0){
                char strtmsg[2] = {'%', ' '};
                errlen = sprintf(errmsg, "Unknown command: [%s].\n\0", tmp -> cmd_argv[0]);
                write(1, errmsg, errlen);
                exit(0);
            };
            exit(0);
        }else{
            DelZDELAY((ClientPool -> MainPool)[client_id]);
            if(tmp -> pipefrom_client >= 0){
                close((ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].readside);
                //close((ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].writeside);
                (ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].readside = -1;
                (ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].writeside = -1;
                (ClientPool -> userpipe)[tmp -> pipefrom_client][client_id].is_activate = false;
                sprintf(pipemsg, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", 
                    (ClientPool -> user_name)[client_id], client_id + 1, (ClientPool -> user_name)[tmp -> pipefrom_client],
                    tmp -> pipefrom_client + 1, tmp -> origin_cmd);
                NPyell(ClientPool, pipemsg);
            };
            if((tmp -> trgt_client) >= 0){
                sprintf(pipemsg, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", 
                    (ClientPool -> user_name)[client_id], client_id + 1, tmp -> origin_cmd, 
                    (ClientPool -> user_name)[tmp -> trgt_client], (tmp -> trgt_client) + 1);
                NPyell(ClientPool, pipemsg);
                close((ClientPool -> userpipe)[client_id][tmp -> trgt_client].writeside);
            };
            if(((tmp -> trgt_client < 0) && (tmp -> delayval < 0)) || ((tmp -> filename)[0] != '\0')){
                waitpid(pid, &status, 0);
                if(((tmp -> filename)[0] != '\0')){
                    close(pipes[1]);
                };
            }else{};
        };
    };
    // parent must set userpipe table write side to -1
    DECDVAL((ClientPool -> MainPool)[client_id]);
    return 1;

};

/*
exec whole list of Command pack
*/
int NPexeCMDPack(NPcommandPack *trgt, ControllorPool *ClientPool, int exe_csfd){
    NPprintDBG("In NPexeCMDPack: get into NPexeCMDPack", 3);
    int client_id = -1;
    for(int i = 0; i < MAXCLIENT; i++){
        if((ClientPool -> occupied_sfd)[i] == exe_csfd){
            client_id = i;
            setenv("PATH", (ClientPool -> PATH_cont)[i], 1);
            break;
        };
    };
    NPcommandPack *tmp;
    /*printPKG(trgt);*/
    
    while(trgt != 0){
        tmp = trgt;
        trgt = trgt -> next;
        NPprintDBG("In NPexeCMDPack: Trying to execute single pack because target is not 0", 3);
        //printf("---------------------------------------------------------------\n");
        //printf("in piping number %d\n", numb);
        NPexeSingPack(tmp, ClientPool, exe_csfd, client_id);
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
    
    
    if(main_argc > 1){

    }
    FILE *stdin_fp = fdopen(0, "r");
    
    // -----------------------starting create socket---------------------------
    int server_fd, new_client_socket, valread, pid, port;
    int port_index = 0;
    struct sockaddr_in address;
    int opt = 1;
    int max_skfd;
    int addrlen = sizeof(address);
    fd_set rset;
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
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){ 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    };
    printf("successfully get server_fd\n");
    fflush(stdout);

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))){ 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port );

    printf("attach socket to %d\n", port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    };
    printf("bind socket to %d\n", port);
    if (listen(server_fd, 3) < 0){
        printf("listen fail\n");
        perror("listen"); 
        exit(EXIT_FAILURE); 
    };
    printf("successfully listening\n");
    fflush(stdout);
    max_skfd = max(server_fd, 0);
    printf("the max_skfd is %d\n", max_skfd);
    FD_ZERO(&rset);
    FD_SET(server_fd, &rset);
    // -----------------------end of creating socket---------------------------

    NPcommandPack *ExecCMD, *Head;
    PipeControllor *Controllor;
    ControllorPool *ClientPool;

    Controllor = (PipeControllor *)malloc(sizeof(PipeControllor));
    initControllor(Controllor);

    ClientPool = (ControllorPool *)malloc(sizeof(ControllorPool));
    printf("trying to initialize client pool\n");
    fflush(stdout);
    initControllorPool(ClientPool);
    printf("sussfully initialize client pool\n");
    fflush(stdout);

    clearenv();
    setenv("PATH", "bin:.", 1);
    write(1, strtmsg, 2);
    
    while(1){
        FD_ZERO(&rset);
        FD_SET(server_fd, &rset);
        for(int i = 0; i < MAXCLIENT; i ++){
            if((ClientPool -> active_flag)[i]){
                FD_SET((ClientPool -> occupied_sfd)[i], &rset);
            }else{};
        };

        printf("preapre for select with max_skfd %d\n", max_skfd);
        while(select(max_skfd + 1, &rset, NULL, NULL, NULL) < 0){
            usleep(1000);
        };
        printf("select detect activity\n");
        fflush(stdout);
        for(int i = 0; i < max_skfd + 1; i++){
            if(FD_ISSET(i, &rset)){
                if(i == server_fd){
                    //do server stuff
                    new_client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                    if(NPlogin(new_client_socket, *(struct sockaddr_in *)&address, ClientPool) > 0){
                        //set fd_set
                        //print % to client
                        max_skfd = max(max_skfd, new_client_socket);
                        FD_SET(new_client_socket, &rset);
                        write(new_client_socket, strtmsg, 2);
                        break;
                    }else{
                        // login fail
                        close(new_client_socket);
                    };
                }else{
                    //do client stuff
                    printf("client movement detected with fd %d\n", i);
                    fflush(stdout);
                    stdin_fp = fdopen(i, "r");
                    if(getline(&readbuf, &max_len, stdin_fp) != -1){
                        if(readbuf[0] == '\0'){
                            NPlogout(i, ClientPool);
                        };
                        while((readbuf[bufsize] != '\n') && (readbuf[bufsize] != '\r')){
                            bufsize++;
                        };
                        readbuf[bufsize] = '\0';
                        bufsize ++;
                        argv = ParseBuffer(argv, readbuf, bufsize);
                        bufsize = 0;
                        argc = CountArgc(argv);
                        Head = ParseCMD(argv, readbuf);
                        NPexeCMDPack(Head, ClientPool, i);
                        write(1, strtmsg, 2);
                        write(i, strtmsg, 2);
                    };

                }
            }else{
                continue;
            };
        };
    };
    /*
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
    };

    finalizeControllor(Controllor);
    free(Controllor);
    */
    
    
};



/*
parent will crash when decdval;
unhandle unknown command yet;
mishandle ordinary pipe;
ls |2 ls |1 number is ok but ls |1 ls |1 number will cause segmentation fault
ls | ls | cat, cat will not get propriate stdin
reidrection mode have some issue when open file
have to wait error pipe to finish writing then print %

nhtos
*/