#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"


int main(int argc, char *argv[]) {
    char buffer[256];
    char* execArgs[MAXARG];
    int pos;

    char tmp;
    char *wp, *argp;

    if(argc < 2) {
        exit();         // default function = echo?
    }

    for(pos = 0; pos + 1 < argc; pos++) {
        execArgs[pos] = argv[pos + 1];
    }

    wp = buffer;         // always point at previous character
    argp = wp + 1;       // point at next argument
    *wp = '\0';          // helps to determine redundant blanks at the beginning      

    while(read(0, &tmp, sizeof(tmp)) > 0) {
        switch(tmp) {
            case ' ':                   // split arguments
                if(*wp != '\0') {       // skip redundant blanks
                    *(++wp) = '\0';
                    execArgs[pos++] = argp;
                    argp = wp + 1;
                }
                break;
            case '\n':
            case '\0':
                if(*wp != '\0') {       // receive last argument
                    *(++wp) = '\0';
                    execArgs[pos++] = argp;
                }
                execArgs[pos] = 0;
                if(fork() == 0) {
                    exec(execArgs[0], execArgs);
                    fprintf(2, "exec %s failed", execArgs[0]);
                    exit();
                }
                pos = argc - 1;
                wp = buffer;
                argp = wp + 1;
                wait();
                *wp = '\0';
                break;
            default:
                *(++wp) = tmp;
                break;
        }
    }

    // EOF
    if(*wp != '\0') {       // receive last argument
        *(++wp) = '\0';
        execArgs[pos++] = argp;
    }
    execArgs[pos] = 0;
    if(fork() == 0) {
        exec(execArgs[0], execArgs);
        fprintf(2, "exec %s failed", execArgs[0]);
        exit();
    }
    wait();
    exit();
}
