#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <openssl/md5.h>
#define ASCII_S 32
#define ASCII_E 126
FILE *fp;
char rand_char(){
    int r = rand();
    r = r % (ASCII_E - ASCII_S + 1) + ASCII_S;
    if(r == ASCII_E + 1)
        return '\0';
    else
        return (char)r;
}
char *rand_str(){
    char *str = (char*) malloc(4);
    int len = rand() % 4 + 1;
    for(int i=0;i<len;i++)
        str[i] = rand_char();
    return str;
}
char *md5(char *plainText){

    static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    unsigned char digest[16];
    MD5_CTX *context = (MD5_CTX*) malloc(sizeof(MD5_CTX));

    MD5_Init(context);
    MD5_Update(context, plainText, strlen(plainText));
    MD5_Final(digest, context);

    char *cypherText = malloc(sizeof(char) * 32);
    char buf[2];

    for(int i = 0; i< 16; i++){
        cypherText[2*i] = hex[digest[i] / 16];
        cypherText[2*i+1] = hex[digest[i] % 16];
    }

    return cypherText;
}
void digTreature(char *prefix, char *goal, int N, int *M, int length){
    char word[256];
    if(length > N)
        return;

    while(*M != 0){
        sprintf(word, "%s%s", prefix, rand_str());
        char *cypher = md5(word);
        if(strncmp(cypher, goal, length) == 0){ // find the target substring

            fprintf(fp, "%s\n", word);

            if(length == N) {
                (*M)--;
                return;
            }
            else{
                digTreature(word, goal, N, M, length+1);
                if(length != 1)
                    return;
            }
            if(length == 1)
                fprintf(fp, "===\n");
        }
    }
    return;
}
int main(int argc, char *argv[]){
    char *prefix = argv[1];
    char *goal = argv[2];
    int N = atoi(argv[3]);  //  N <= 5
    int M = atoi(argv[4]);  //  M <= 10
    char *outfile = argv[5];
    fp = fopen(outfile, "w");

    srand(time(NULL));
    digTreature(prefix, goal, N, &M, 1);

    fclose(fp);
    return 0;
}