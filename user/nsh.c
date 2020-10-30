#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"

#define EXEC    1
#define REDIR   2
#define PIPE    3

#define BUFSIZE 128

#define TOLEFT  1
#define TORIGHT 2

/*
   execBuf, redirBuf, pipeBuf never return.
*/
void execBuf(char *bp);
void redirBuf(char *leftBp, char *fileBp, char tokken);
void pipeBuf(char *leftBp, char *rightBp);
void parseBuf(char *buf);


int openBuf(char *bp, int mode) {
    int fd;
    while(*bp == ' ') {
        bp++;
    }

    if((fd = open(bp, mode)) < 0) {
        fprintf(2, "open %s failed\n", bp);
        exit(-1);
    };

    return fd;
}


void execBuf(char *bp) {
    char* execArgs[MAXARG];
    int pos = 0;
    int result;
    
    if(*bp == 0)
        exit(0);         // empty

    while(*bp == ' ') {
        bp++;           // redundant blank at the beginning
    }

    execArgs[pos++] = bp++;

    while(*bp) {
        if(*bp == ' ' || *bp == '\n') {
            *bp++ = '\0';
            if(*bp)                     // redundant blank at the end
                execArgs[pos++] = bp;
        }
        else {
            bp++;
        }
    }
    execArgs[pos] = 0;
    
    if(fork() == 0) {
        exec(execArgs[0], execArgs);
    }
    else {
        fprintf(2, "checkout exec args\n");
        for(pos = 0; execArgs[pos]; pos++) {
            fprintf(2, "debug: %s\n", execArgs[pos]);
        }
        close(0);
        close(1);
        wait(&result);
    }
    
    exit(0);    
}


void redirBuf(char *leftBp, char *rightBp, char tokken) {
    int fd;
    switch(tokken) {
        case '>':
            fd = openBuf(rightBp, O_WRONLY|O_CREATE);
            close(1);
            dup(fd);
            close(fd);
            break;
        case '<':
            fd = openBuf(rightBp, O_RDONLY);
            close(0);
            dup(fd);
            close(fd);
            break;
    }
    parseBuf(leftBp);    
    fprintf(2, "parse %s failed\n", leftBp);
    exit(-1);
}

void pipeBuf(char *leftBp, char *rightBp) {
    int p[2];
    int rLeft;
    int rRight;

    pipe(p);

    if(fork() == 0){
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        parseBuf(leftBp);
    }
    if(fork() == 0){
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        parseBuf(rightBp);
    }
    close(p[0]);
    close(p[1]);
    wait(&rLeft);
    wait(&rRight);
    exit(0);
}


void parseBuf(char *buf) {
    char *p;
    
    fprintf(2, "parse:%s\n", buf);

    for(p = buf; *p; p++);
    for(p--; p >= buf; p--) {
        switch(*p){
            case '|':
                *p++ = 0;
                pipeBuf(buf, p);
                fprintf(2, "pipe %s failed", buf);
                exit(-1);
                break;
            case '<':
                *p++ = 0;
                redirBuf(buf, p, '<');
                fprintf(2, "redir %s failed", buf);
                exit(-1);
                break;
            case '>':
                *p++ = 0;
                redirBuf(buf, p, '>');
                fprintf(2, "redir %s failed", buf);
                exit(-1);
                break;
        }
    }
    execBuf(buf);       // case exec
    fprintf(2, "exec %s failed", buf);
    exit(-1);
}


int getcmd(char *buf, int nbuf) {
    fprintf(1, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if(buf[0] == 0)
        return -1;
    return 0;
}


int main(int argc, char *argv[]) {
    static char buf[128];
    int result;

    while(getcmd(buf, sizeof(buf)) >= 0){
        buf[strlen(buf)-1] = 0;     // change '\n' to '\0'
        if(fork() == 0)
            parseBuf(buf);
        wait(&result);
    }
    exit(0);
}
