#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fileName(char *path) {
    char *p;
    for(p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    return p;
}

void find(char *path, char *name) {
    char buff[512];
    char *p;
    int fd;
    struct stat st;
    struct dirent de;

    if((fd = open(path, 0)) < 0) {
        return;
    }

    if(fstat(fd, &st) < 0) {
        close(fd);
        return;
    }

    switch(st.type) {
        case T_FILE:
            if(!strcmp(name, fileName(path))) {
                printf("%s\n", path);
            }
            break;
        
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buff)) {
                printf("path too long\n");
                break;
            }
            strcpy(buff, path);
            p = buff + strlen(buff);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                if(!strcmp(de.name, ".") || !strcmp(de.name, "..")) {
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buff, name);
                }
            }
            
            break;
    }
    close(fd);
    return;
}


int main(int argc, char *argv[]) {
    if(argc != 3) {
        exit();
    }
    find(argv[1], argv[2]);
    exit();
}
