#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc, char *argv[]) {
    char buffer[512];
    char* execArgs[MAXARG];
    int pos;

    if(argc < 2) {
        exit();         // default function = echo?
    }

    for(pos = 0; pos + 1 < argc; pos++) {
        execArgs[pos] = argv[pos + 1];
    }

    char *wp;
    wp = buffer;
    execArgs[pos++] = wp;

    while(read(0, wp, sizeof(char)) > 0) {
        if(*wp == ' ' || *wp == '\n' || *wp == '\0') {
            *wp++ = '\0';
            execArgs[pos++] = wp;
        }
        else {
            wp++;
        }
    }
    *wp = '\0';
    execArgs[pos] = 0;

    if(!fork()) {
        exec(execArgs[0], execArgs);
    }
    else {
        wait();
    }

    exit();
}
