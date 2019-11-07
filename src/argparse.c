#include <stdio.h>
#include <stdlib.h>

int load_argv(int buflen = -1, char *buffer, char *delimiter, char **argv){
    int argc = 1;
    int buf_index = 0;
    int row_index = 0;
    int column_index = 0;

    if(delimiter == 0){
        char delimiter[1] = {' '};
    }else{}

    if(buflen < 0){
        while(buffer[buf_index] != '\0'){
            if(buffer[buf_index] == delimiter[0]){
                argc ++;
                argv[row_index][column_index] = '\0';
                row_index ++;
                column_index = 0;
            }else{
                argv[row_index][column_index] = buffer[buf_index];
                column_index ++;
            };
        };
        argv[row_index][column_index] = '\0';
        argv[row_index + 1][0] = '\0';
        return argc;
    }else{
        for(int i = 0; i < buflen; i++){
            if(buffer[i] == delimiter[0]){
                argc ++;
                argvp[row_index][column_index] = '\0';
                row_dex ++;
                column_index = 0;
            }else{
                argv[row_index][column_index] = buffer[i];
                column_index ++;
            };
        };
        argv[row_index][column_index] = '\0';
        argv[row_index + 1][0] = '\0';
        return argc;
    };
};

int dump_argv(char *buffer, char **argv, char *tmp_delimiter = 0){
    if(delimiter == 0){
        char delimiter[1] = {' '};
    }else{
        char *delimiter = tmp_delimiter;
    };
    int row_ind = 0;
    int col_ind = 0;
    int buf_ind = 0;
    while(argv[row_ind][0] != '\0'){
        while(argv[row_ind][col_ind] != '\0'){
            //
            buffer[buf_ind] = argv[row_ind][col_ind];
            col_ind ++;
            buf_ind ++;
        };
        buffer[buf_ind] = delimiter[0];
        row_ind ++;
        col_ind = 0;
    };
    buffer[buf_ind] = '\0';
    buf_ind ++;
    return buf_ind;
};

int get_line(char *ret_buf, FILE *fdp){
    int index = 0;
    if(getline())
};