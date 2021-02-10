#include <stdio.h>
#include <stdlib.h>

int main(int argc,char** argv){

    if (argc != 2){
        fprintf(stderr, "argc error\n");
        exit(1);
    }
    int bid_list[21] = {20, 18, 5, 21, 8, 7, 2, 19, 14, 13,
    9, 1, 6, 10, 16, 11, 4, 12, 15, 17, 3};

    int id = atoi(argv[1]);
    char* buf;

    for (int round=1;round<=10;round++){
        int money = bid_list[id+round-2] * 100;
        fprintf(stdout,"%d %d\n", id, money);
    }

    return 0;
}