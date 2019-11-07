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

#define _MAX_DOMAIN_SIZE 100
#define _MAX_INPUT_SIZE 2000
#define _MAX_RETURN 2000

int main(int argc, char const *argv[]) 
{ 
    int sock = 0, valread;
    struct sockaddr_in serv_addr;

    int PORT;
    int port_index = 0;
    char domain[_MAX_DOMAIN_SIZE] = {0};
    char port_str[20] = {0};

    int max_input = _MAX_INPUT_SIZE;
    int max_return = _MAX_RETURN;
    int input_len = 0;
    int retlen = 0;
    char *in_buffer = (char *)calloc(_MAX_INPUT_SIZE, sizeof(char));
    char *ret_buffer = (char *)calloc(_MAX_RETURN, sizeof(char));

    FILE *stdin_fp = fdopen(0, "r");

    if(argv[2][0] == '\0'){
        PORT = 7070;
    }else{
        while((argv[2][port_index] - "0" >= 0) && (argv[2][port_index] - "0" <= 9)){
            PORT = PORT * 10 + (argv[2][port_index] - "0");
            port_index ++;
        };
    };
    port_index = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    };
    while(getline(&in_buffer, &max_input, stdin_fp) != 0){
        //
        input_len = 0
        while(in_buffer[input_len] != '\n'){
            input_len ++;
        };
        input_len ++;
        send(sock, input_buffer, input_len, 0);

    }

    /*
    send(sock , hello , strlen(hello) , 0 ); 
    printf("Hello message sent\n"); 
    valread = read( sock , buffer, 1024); 
    printf("%s\n",buffer ); 
    return 0; 
    */
};