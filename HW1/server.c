#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define FIRST_ID 902001

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;
typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;                 // server
request* requestP = NULL;   // point to a list of requests
char status[20];            // record 902001~902020 order's in memory
int file_fd;                // fd for file that we open for reading and writing
int maxfd;                  // size of open file descriptor table, size of request list

const char *msg_welcome = "Please enter the id (to check how many masks can be ordered):\n";
const char *msg_fail = "Operation failed.\n";
const char *msg_locked = "Locked.\n";
const char *msg_order = "Please enter the mask type (adult or children) and number of mask you would like to order:\n";

//=============================== Function Declaration ==============================================

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

int isNumber(char *word);
// if word is a number, return by int type. Otherwise, return -1.

int handle_read(request* reqP);
// if not ready to write, examine and set the id, return id for sucessful, -1 for failure.
// if it's ready to write, examine the instruction, return order for sucessful, -1 for failure.

void check_record(request* reqP);
// Check the stock of the request ID.

int modify_record(request* reqP, int order);
// Modify the stock of the request ID.

int set_lock(int ID, char mode);
// set a lock on the part of the ID, return -1 for failure.

void disconn_request(request* reqP, int i, fd_set *fds);
// routine before a soket disconnect

// ========================================== Main function ================================================

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    int conn_fd;                 // fd for a new connection with client
    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;                  // client length


    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    // Get the data from document
    if( (file_fd = open("preorderRecord", O_RDWR) ) == -1) 
        ERR_EXIT(msg_fail);

    // Initialize ID status
    for(int i=0;i<20;i++)
        status[i] = 'n';
    
    // file descripter set for select()
    fd_set real_fds, ready_fds;
    FD_ZERO(&real_fds);
    FD_SET(svr.listen_fd, &real_fds);


    while (1) {
        // TODO: Add IO multiplexing
        // Because select() is destructive, resst ready_fds every loop
        ready_fds = real_fds; 

        if(select(maxfd,&ready_fds,NULL,NULL,NULL) < 0)
            ERR_EXIT("select error\n");

        for(int i=0;i<maxfd;i++){
            if(FD_ISSET(i, &ready_fds)){

                if(i==svr.listen_fd){   // Check new connection
                    clilen = sizeof(cliaddr);
                    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                    if (conn_fd < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;  // try again
                        if (errno == ENFILE) {
                            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                            continue;
                        }
                        ERR_EXIT("accept");
                    }
                    requestP[conn_fd].conn_fd = conn_fd;
                    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                    fprintf(stderr, "getting a new request... fd (%d) from %s\n", conn_fd, requestP[conn_fd].host);
                    write(requestP[conn_fd].conn_fd, msg_welcome, strlen(msg_welcome));  
                    FD_SET(conn_fd, &real_fds);
                }

                else{   // TODO: handle requests from clients
                    int ret = handle_read(&requestP[i]);
                    if (ret < 0) { // handle_read() failed
                        fprintf(stderr, "Client(%d): bad request\n", requestP[i].conn_fd);
                        write(requestP[i].conn_fd, msg_fail, strlen(msg_fail));
                        disconn_request(&requestP[i], i,&real_fds);
                        continue;
                    }

                    int ID = requestP[i].id - FIRST_ID;
                    #ifdef READ_SERVER
                        if(set_lock(ID, 'r') == -1){
                            write(requestP[i].conn_fd, msg_locked, strlen(msg_locked));
                            disconn_request(&requestP[i], i, &real_fds);
                            continue;
                        }
                        check_record(&requestP[i]);
                        set_lock(ID, 'n');
                        disconn_request(&requestP[i], i, &real_fds);
                        
                    #else // WRITE_SERVER
                        if(requestP[i].wait_for_write == 0){    //read
                            if(set_lock(ID, 'w') == -1){
                                write(requestP[i].conn_fd, msg_locked, strlen(msg_locked));
                                disconn_request(&requestP[i], i, &real_fds);
                                continue;
                            }
                            check_record(&requestP[i]);
                            requestP[i].wait_for_write = 1;
                            write(requestP[i].conn_fd, msg_order, strlen(msg_order));
                        }
                        else{                                   //write
                            if(modify_record(&requestP[i], ret) < 0)
                                write(requestP[i].conn_fd, msg_fail, strlen(msg_fail));
                            set_lock(ID, 'n');
                            disconn_request(&requestP[i], i, &real_fds);
                        }
                    #endif
                }
            }     
        }

    }
    free(requestP);
    return 0;
}


// ===================================== self-defined functions ===========================================
int isNumber(char *word){
    if(word == NULL)    
        return -1;

    word = strtok(word, "\r\n");

    for(int i=0 ; i < strlen(word) ; i++)
        if( !isdigit(word[i]) )
            return -1;

    while(word[0]=='0')   
        word++;
    
    return atoi(word);
}

int handle_read(request* reqP) {
    char buf[512];

    int r = read(reqP->conn_fd, buf, sizeof(buf));
    if(r <= 0) 
        return r;

    // p is buf removed \n and \r.
    char *p = strtok(buf,"\n\r");
    if(p != NULL)
        memcpy(reqP->buf, p, strlen(p));
    else 
        return -1;

    
    int ID = isNumber(p);
    
    if(reqP->wait_for_write == 0){  // just reading the record
        reqP->id = ID;
        if( ID >= (FIRST_ID) && ID <= (FIRST_ID + 20) ){
            fprintf(stderr, "Client(%d): %d\n", reqP->conn_fd, ID);
            return ID;
        }
        else
            return -1;
    }
    
    else{ // making order
        strcpy(reqP->buf, strtok(buf, " "));
        int order = isNumber(strtok(NULL, " "));
        if(order < 0)
            return -1;

        fprintf(stderr, "Client(%d): %s %d\n", reqP->conn_fd, reqP->buf, order);
        return order;
    }
}

void check_record(request* reqP){
    char buf[512];
    int aMask, cMask;
    int ID = (reqP->id) - FIRST_ID;
    
    lseek(file_fd, (ID * 3 + 1) * sizeof(int), SEEK_SET);
    read(file_fd, &aMask, sizeof(int));
    read(file_fd, &cMask, sizeof(int));

    sprintf(buf,"You can order %d adult mask(s) and %d children mask(s).\n", aMask, cMask);
    write(reqP->conn_fd, buf, strlen(buf));
    return;
}

int  modify_record(request* reqP, int order){

    char buf[512];
    char type[10];
    int aMask, cMask;
    int ID = (reqP->id) - FIRST_ID;

    if(order <= 0)
        return -1;

    strcpy(type, (reqP->buf) );
    lseek(file_fd, (ID * 3 + 1) * sizeof(int), SEEK_SET);
    read(file_fd, &aMask, sizeof(int));
    read(file_fd, &cMask, sizeof(int));

    if(strcmp(type,"adult") == 0){
        if(aMask < order)
            return -1;
        
        aMask -= order;
        lseek(file_fd, (ID * 3 + 1) * sizeof(int), SEEK_SET);
        write(file_fd, &aMask, sizeof(int));
    }
    else if (strcmp(type,"children") == 0){
        if(cMask < order)
            return -1;
        
        cMask -= order;
        lseek(file_fd, (ID * 3 + 2) * sizeof(int), SEEK_SET);
        write(file_fd, &cMask, sizeof(int));
    }
    else return -1;  // Not above condition

    sprintf(buf,"Pre-order for %d successed, %d %s mask(s) ordered.\n",reqP->id, order, type);
    write(reqP->conn_fd, buf, strlen(buf));
    
    return 0;
}

int set_lock(int ID, char mode){
    int lock_type;
    char backup = status[ID];

    // internal process lock
    if(mode == 'r'){

        if(status[ID] == 'w')
            return -1;
        else
           lock_type = F_RDLCK;
    }

    else if(mode == 'w'){

        if(status[ID] != 'n')
            return -1;
        else
            lock_type = F_WRLCK;
    }

    else
        lock_type = F_UNLCK;
    
    status[ID] = mode;

    // external process lock
    struct flock lock =  {lock_type, SEEK_SET, ID * 3*sizeof(int), 3*sizeof(int), getpid()};

    if(fcntl(file_fd, F_SETLK, &lock) == -1){
        status[ID] = backup;
    	return -1;
    }

    return 0;
}

void disconn_request(request* reqP, int i, fd_set *fds){
    FD_CLR(i, fds);
    close(reqP->conn_fd);
    
    int ID = reqP->id - FIRST_ID;
    if(ID >= 0 && ID < 20)
        set_lock(ID, 'n');
    
    fprintf(stderr, "Client(%d): disconnected\n", reqP->conn_fd);
    free_request(reqP);
}

// ======================================================================================================
// You don't need to know how the following codes are working

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initize request table
    
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}