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

void strSplit(char *str, char* head[], int len);

int openBuf(char *bp, int mode) {
    int fd;
    char* paths[2];

    strSplit(bp, paths, 2);

    if((fd = open(paths[0], mode)) < 0) {
        fprintf(2, "open %s failed\n", paths[0]);
        fprintf(2, "mode is %d \n", mode);
        exit(-1);
    };

    return fd;
}


char blanks[] = " \t\r\n\v";


void strSplit(char *str, char* head[], int headLen) {
    char *bp;
    int pos;
    
    for(bp = str; strchr(blanks, *bp); bp++);
    
    pos = 0;
    
    while(*bp && pos < headLen - 1) {
        head[pos++] = bp;
        while(*bp && !strchr(blanks, *bp)) bp++;
        while(strchr(blanks, *bp)) {
            *bp++ = '\0';
        }
    }
    
    head[pos] = 0;
    
    return;
}


void execBuf(char *bp) {
    char* execArgs[MAXARG+1];
    int result;
    
    if(*bp == 0)
        exit(0);         // empty

    strSplit(bp, execArgs, MAXARG+1);

    if(fork() == 0) {
        exec(execArgs[0], execArgs);
        fprintf(2, "exec %s failed", bp);
        exit(-1);
    }
    else {
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
