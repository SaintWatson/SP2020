#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>

#define ERR_EXIT(a) do {perror(a); exit(-1);}while(1)

//============== These functions are for ranking ====================//

typedef struct{
        int id;
        int win;
        int rank;
    }Player;
int compareByWin(const void *m, const void *n){
    Player *a = (Player*) m;
    Player *b = (Player*) n;
    if(a->win > b->win) return -1;
    if(a->win < b->win) return 1;
    return 0;
}
int compareById(const void *m, const void *n){
    Player *a = (Player*) m;
    Player *b = (Player*) n;
    if(a->id > b->id) return 1;
    if(a->id < b->id) return -1;
    return 0;
}
char *rank(int score[10], int ply_id[8], char *key){
    int acc[13]={0};
    for(int i=0;i<10;i++) acc[score[i]]++;

    
    Player p[8];
    for(int i=0;i<8;i++){
        p[i].id = ply_id[i];
        p[i].win = acc[p[i].id];
    }

    qsort(p, 8, sizeof(Player), compareByWin);
    int prev = 0;
    for(int i=0;i<8;i++){
        p[i].rank = (p[i].win==prev) ? p[i-1].rank : i+1;
        prev =  p[i].win;
    }
    qsort(p, 8, sizeof(Player), compareById);

    char *ret = (char *)malloc(sizeof(char)*128);
    memset(ret, 0, sizeof(char));
    strcat(ret, key);
    for(int i=0;i<8;i++){
        char temp[16];
        sprintf(temp,"\n%d %d", p[i].id, p[i].rank);
        strcat(ret, temp);
    }
    strcat(ret, "\n");

    return ret;
}

// split a string into int array
void split(char *buf, int p[8]){
    
    char *sp = strtok(buf," ");
    for(int i=0;i<8;i++){
        p[i] = atoi(sp);
        sp = strtok(NULL, " ");
    }
    return;
}

int main(int argc, char *argv[]){
    if(argc != 4)   ERR_EXIT("Wrong number of argument");

    int depth = atoi(argv[3]);
    char buf[512];
    int status; 

    if(depth == 0){ // root
        int ply_id[8];

        sprintf(buf, "fifo_%s.tmp", argv[1]);
        int fromFIFO = open(buf, O_RDWR);
        int toFIFO = open("fifo_0.tmp", O_RDWR);

        while(1){
            if(read(fromFIFO, buf, sizeof(buf)) <= 0) // nothing in the FIFO
                continue;
            
            split(buf, ply_id);
            int child1_id, child2_id;
            int pipeFrom1[2], pipeFrom2[2];
            int pipeTo1[2], pipeTo2[2];
            if(pipe(pipeTo1) < 0) ERR_EXIT("Pipe error: root to child1");
            if(pipe(pipeTo2) < 0) ERR_EXIT("Pipe error: root to child2");
            if(pipe(pipeFrom1) < 0) ERR_EXIT("Pipe error: root to child1");
            if(pipe(pipeFrom2) < 0) ERR_EXIT("Pipe error: root to child2");


            if( (child1_id=fork()) && (child2_id=fork()) < 0) ERR_EXIT("Fork error: c1 or c2");
            if(child1_id==0){ // child 1
                dup2(pipeTo1[0], STDIN_FILENO);
                close(pipeTo1[1]);
                dup2(pipeFrom1[1], STDOUT_FILENO);
                close(pipeFrom1[0]);
                
                close(pipeTo2[0]); // Below are child2's fd
                close(pipeTo2[1]);
                close(pipeFrom2[0]);
                close(pipeFrom2[1]);

                execlp("./host","./host", argv[1], argv[2], "1", NULL);
            }
            else if(child2_id==0){ // child 2
                dup2(pipeTo2[0], STDIN_FILENO);
                close(pipeTo2[1]);
                dup2(pipeFrom2[1], STDOUT_FILENO);
                close(pipeFrom2[0]);

                close(pipeTo1[0]); // Below are child1's fd
                close(pipeTo1[1]);
                close(pipeFrom1[0]);
                close(pipeFrom1[1]);

                execlp("./host","./host", argv[1], argv[2], "1", NULL);
            }
            else{ // root
                FILE *fpTo1 = fdopen(pipeTo1[1], "w");
                FILE *fpTo2 = fdopen(pipeTo2[1], "w");
                FILE *fpFrom1 = fdopen(pipeFrom1[0], "r");
                FILE *fpFrom2 = fdopen(pipeFrom2[0], "r");
                close(pipeTo1[0]);
                close(pipeTo2[0]);
                close(pipeFrom1[1]);
                close(pipeFrom2[1]);

                fprintf(fpTo1, "%d %d %d %d\n", ply_id[0], ply_id[1], ply_id[2], ply_id[3]);
                fflush(fpTo1);
                fprintf(fpTo2, "%d %d %d %d\n", ply_id[4], ply_id[5], ply_id[6], ply_id[7]);
                fflush(fpTo2);

                waitpid(child1_id, &status, 0); // end or error input
                if(status != 0) break;
                waitpid(child2_id, &status, 0);
                if(status != 0) break;

                int winner[10] = {0};
                for(int i=0;i<10;i++){
                    int id1, id2, bid1, bid2;
                    fscanf(fpFrom1, "%d %d", &id1, &bid1);
                    fscanf(fpFrom2, "%d %d", &id2, &bid2);
                    if(bid1 > bid2) 
                        winner[i] = id1;
                    else 
                        winner[i] = id2;
                }
                fclose(fpTo1);
                fclose(fpTo2);
                fclose(fpFrom1);
                fclose(fpFrom2);

                strcpy(buf, rank(winner, ply_id, argv[2]));
                write(toFIFO, buf, strlen(buf));
            }
            
        }
        exit(0);
    }
    else if(depth == 1){ // child
        int ply_id[4];

        for(int i=0;i<4;i++) 
            scanf("%d", &ply_id[i]);
        
        int leaf1_id, leaf2_id;
        int pipeFrom1[2], pipeFrom2[2];
        int pipeTo1[2], pipeTo2[2];

        if(pipe(pipeTo1) < 0) ERR_EXIT("Pipe error: child to leaf1");
        if(pipe(pipeTo2) < 0) ERR_EXIT("Pipe error: child to leaf2");
        if(pipe(pipeFrom1) < 0) ERR_EXIT("Pipe error: child to leaf1");
        if(pipe(pipeFrom2) < 0) ERR_EXIT("Pipe error: child to leaf2");

        if( (leaf1_id=fork()) && (leaf2_id=fork()) < 0 ) ERR_EXIT("Fork error: l1 or l2");
        if(leaf1_id==0){ // leaf1
            dup2(pipeTo1[0], STDIN_FILENO);
            close(pipeTo1[1]);
            dup2(pipeFrom1[1], STDOUT_FILENO);
            close(pipeFrom1[0]);

            close(pipeTo2[0]); // Below are leaf2's fd
            close(pipeTo2[1]);
            close(pipeFrom2[0]);
            close(pipeFrom2[1]);

            execlp("./host", "./host", argv[1], argv[2], "2", NULL);
        }
        else if(leaf2_id==0){ // leaf2
            dup2(pipeTo2[0], STDIN_FILENO);
            close(pipeTo2[1]);
            dup2(pipeFrom2[1], STDOUT_FILENO);
            close(pipeFrom2[0]);

            close(pipeTo1[0]); // Below are leaf1's fd
            close(pipeTo1[1]);
            close(pipeFrom1[0]);
            close(pipeFrom1[1]);

            execlp("./host", "./host", argv[1], argv[2], "2", NULL);
        }
        else{ // child

            FILE *fpTo1 = fdopen(pipeTo1[1], "w");
            FILE *fpTo2 = fdopen(pipeTo2[1], "w");
            FILE *fpFrom1 = fdopen(pipeFrom1[0], "r");
            FILE *fpFrom2 = fdopen(pipeFrom2[0], "r");
            close(pipeTo1[0]);
            close(pipeTo2[0]);
            close(pipeFrom1[1]);
            close(pipeFrom2[1]);

            fprintf(fpTo1, "%d %d\n", ply_id[0], ply_id[1]);
            fflush(fpTo1);

            fprintf(fpTo2, "%d %d\n", ply_id[2], ply_id[3]);
            fflush(fpTo2);
            
            waitpid(leaf1_id, &status, 0);
            if(status != 0) exit(-1);
            waitpid(leaf2_id, &status, 0);
            if(status != 0) exit(-1);
            

            for(int i=0;i<10;i++){
                int id1, id2, bid1, bid2;
                fscanf(fpFrom1, "%d %d", &id1, &bid1);
                fscanf(fpFrom2, "%d %d", &id2, &bid2);
                if(bid1 > bid2) {
                    printf("%d %d\n", id1, bid1);
                    fflush(stdout);
                }
                else {
                    printf("%d %d\n", id2, bid2);
                    fflush(stdout);
                }
            }
            fclose(fpTo1);
            fclose(fpTo2);
            fclose(fpFrom1);
            fclose(fpFrom2);
            
            exit(0);
        }
    }
    else if(depth == 2){ // leaf
        int ply_id[2];

        for(int i=0;i<2;i++) 
            scanf("%d", &ply_id[i]);
        
        if(ply_id[0]==ply_id[1] && ply_id[0]==-1)
            exit(-1);
        
        
        int p1_id, p2_id;
        int pipeFrom1[2], pipeFrom2[2];
        if(pipe(pipeFrom1) < 0) ERR_EXIT("Pipe error: leaf to player1");
        if(pipe(pipeFrom2) < 0) ERR_EXIT("Pipe error: leaf to player2");

        if( (p1_id=fork()) && (p2_id=fork()) < 0 ) ERR_EXIT("Fork error: p1 or p2");
        if(p1_id==0){ // player1
            dup2(pipeFrom1[1], STDOUT_FILENO);
            close(pipeFrom1[0]);

            close(pipeFrom2[0]);
            close(pipeFrom2[1]);

            sprintf(buf, "%d", ply_id[0]);
            execlp("./player", "./player", buf, NULL);
        }
        else if(p2_id==0){ // player2
            dup2(pipeFrom2[1], STDOUT_FILENO);
            close(pipeFrom2[0]);

            close(pipeFrom1[0]);
            close(pipeFrom1[1]);

            sprintf(buf, "%d", ply_id[1]);
            execlp("./player", "./player", buf, NULL);
        }
        else{ // leaf
            FILE *fpFrom1 = fdopen(pipeFrom1[0], "r");
            FILE *fpFrom2 = fdopen(pipeFrom2[0], "r");
            close(pipeFrom1[1]);
            close(pipeFrom2[1]);

            for(int i=0;i<10;i++){
                int id1, id2, bid1, bid2;
                fscanf(fpFrom1, "%d %d", &id1, &bid1);
                fscanf(fpFrom2, "%d %d", &id2, &bid2);
                if(bid1 > bid2) {
                    printf("%d %d\n", id1, bid1);
                    fflush(stdout);
                }
                else {
                    printf("%d %d\n", id2, bid2);
                    fflush(stdout);
                }
            }
            fclose(fpFrom1);
            fclose(fpFrom2);

            waitpid(p1_id, &status, 0);
            waitpid(p2_id, &status, 0);
            exit(0);
        }
    }
    return 0;
}
