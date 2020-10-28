#include "kernel/types.h"
#include "user/user.h"


void screen(int readEnd){
    int base, tmp;
    int rightPipe[2];

    close(0);           // redirect
    dup(readEnd);       // use 0
    close(readEnd);
    
    if(read(0, &base, sizeof(base)) != 0) {
        printf("prime %d\n", base);
        pipe(rightPipe);

        if(fork() != 0) {    
            close(1);               // redirect
            dup(rightPipe[1]);      // use 1
            close(rightPipe[0]);
            close(rightPipe[1]);
            while(read(0, &tmp, sizeof(tmp)) != 0) {
                if(tmp % base != 0) {
                    write(1, &tmp, sizeof(tmp));
                }
            }
            close(0);
            close(1);
            exit();
        }
        else {
            close(rightPipe[1]);
            screen(rightPipe[0]);
        }
    }
}


int main(int argc, char* argv[]) {
    int i;
    int p[2];
    
    pipe(p);

    if(!fork()) {
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        for(i = 2; i < 35; i++) {
            write(1, &i, sizeof(i));
        }
        close(1);
        exit();
    }
    else {
        close(p[1]);
        screen(p[0]);
    }
    
    exit();
}


