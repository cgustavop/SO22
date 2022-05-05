#ifndef __PROCESS_H__
#define __PROCESS_H__


#define PATH_SIZE 1024

#define CLIENT_TO_SERVER_FIFO "/tmp/client_to_server_fifo" //path para o fifo que permite a comunicação entre o cliente e o servidor


typedef enum RequestType{
    STATUS_REQUEST,
    PROCESS_REQUEST
}RequestType;

typedef struct processRequestData {
    int transf_num;
    int input_len;
    int output_len;
    char transf_names[16][16];
    char input[PATH_SIZE];
    char output[PATH_SIZE];
} ProcessRequestData;

typedef struct request {
    RequestType type;
    int client_pid;
    int priority;
    ProcessRequestData data[];
}Request;




#endif