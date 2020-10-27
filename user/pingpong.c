#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
	int parent_fd[2];
    int child_fd[2];
    char buff[32];
	
    pipe(parent_fd);
    pipe(child_fd);

    if(fork() == 0) {
        close(parent_fd[1]);
        close(child_fd[0]);
        
        read(parent_fd[0], buff, sizeof(buff));
        printf("%d: received %s\n", getpid(), buff);
        
        write(child_fd[1], "pong", strlen("pong"));
        
        close(parent_fd[0]);
        close(child_fd[1]);
	}
    else {
        close(parent_fd[0]);
        close(child_fd[1]);

        write(parent_fd[1], "ping", strlen("ping"));

        read(child_fd[0], buff, sizeof(buff));
        printf("%d: received %s\n", getpid(), buff);

        close(parent_fd[1]);
        close(child_fd[0]);
    }

	exit();
}
