#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),kill(0,SIGKILL),  exit(EXIT_FAILURE))

void child_work()
{
    srand(time(NULL)*getpid());
    int x = rand() % 6 + 5;
    sleep(x);
    printf("Im a Child Process! My pid is: [%d]. sleep time was = %d\n", getpid(), x);
    exit(EXIT_SUCCESS);
}

void make_processes(int n)
{
    if(n==0)
        return;
    int pid;
    for(int i=0; i<n; i++)
    {
        pid = fork();
        switch(pid)
        {
            case -1:
            {
                ERR("fork");
            }
            case 0:
            {
                child_work();
            }
            default:
                break;
        }
    }
}

int main(int argc, char** argv)
{
    if(argc == 1)
    {
        fprintf(stderr, "Wrong nr. of args\n");
        exit(EXIT_FAILURE);
    }
    int children = atoi(argv[1]);
    make_processes(children);
    while(children > 0)
    {

        errno = 0;
        sleep(3);
        while(true)
        {
            pid_t pid = waitpid(0,NULL,WNOHANG);
            if(pid ==-1)
            {
                if(errno == EINTR)
                {
                    printf("Process was interrupted\n");
                    exit(EXIT_FAILURE);
                }
                if(errno == ECHILD)
                    break;
                else{
                    ERR("waitpid");
                }
            }
            if(pid>0)
            {
                children--;
            }
            if(pid==0)
                break;
        }
        printf("The process still has %d kids\n", children);
        fflush(stdout);
    }
    printf("The Process has no kids left. Exiting...\n");
    return 0;
}



