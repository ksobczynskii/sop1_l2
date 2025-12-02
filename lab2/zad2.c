#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),kill(0,SIGKILL),  exit(EXIT_FAILURE))

volatile sig_atomic_t sig = 0;

void usr1_action(int s)
{
    sig=1;
}
void usr2_action(int s)
{
    sig=2;
}
void change_action()
{
    struct sigaction sa1;
    struct sigaction sa2;

    sa1.sa_handler = usr1_action;
    sa2.sa_handler = usr2_action;


    sigaction(SIGUSR1,&sa1,NULL);
    sigaction(SIGUSR2, &sa2,NULL);
}
void children_work(int r)
{
    srand(getpid());
    int sleep_time = rand() % 6 + 5;
    for(int i=0;i<r;i++)
    {
        sleep(sleep_time);
        if(sig==1)
            printf("SUCCESS from process: [%d]\n", getpid());
        else if(sig==2)
            printf("FAILURE from process: [%d]\n", getpid());
        fflush(stdout);
    }
    printf("Child with pid = [%d] Exiting...\n", getpid());
    fflush(stdout);
    exit(EXIT_SUCCESS);
}
void make_kids(int n,int r)
{
    pid_t pid;
    for (int i=0; i<n; i++)
    {
        pid = fork();
        if(pid==0)
        {
            children_work(r);
        }
        if(pid==-1)
            ERR("fork");
    }
}

int main(int argc, char** argv)
{
    if(argc != 9)
    {
        fprintf(stderr,"Wrong nr. of arguments!\n");
        exit(EXIT_FAILURE);
    }


    int n = -1;
    int k = -1;
    int p = -1;
    int r = -1;
    int c;
    while((c=getopt(argc,argv,"n:k:p:r:")) != -1)
    {
        switch(c)
        {
            case 'n':
                n = strtol(optarg,NULL,10);
                break;
            case 'k':
                k = strtol(optarg,NULL,10);
                break;
            case 'p':
                p = strtol(optarg,NULL,10);
                break;
            case 'r':
                r = strtol(optarg,NULL,10);
                break;
            case ':':
                fprintf(stderr, "No argument on flag");
                exit(EXIT_FAILURE);
            case '?':
                fprintf(stderr, "Wrong Flag");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
    if(n>0 && p>0 && k>0 && r>0)
    {
        change_action();
        make_kids(n,r);
    }
    else{
        ERR("getopt");
    }
    struct sigaction sa1;
    struct sigaction sa2;

    sa1.sa_handler = SIG_IGN;
    sa2.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &sa1, NULL);
    sigaction(SIGUSR2, &sa2, NULL);

    while(1)
    {
        if(kill(0,SIGUSR1)<0)
            ERR("kill");
        sleep(k);
       if(kill(0,SIGUSR2) <0)
           ERR("kill");
        sleep(p);

        errno=0;
        while(1)
        {
            pid_t pid = waitpid(0,NULL,WNOHANG);
            if(pid == -1)
            {
                if(errno == ECHILD)
                {
                    exit(EXIT_SUCCESS);
                }
                else{
                    ERR("waitpid");
                }
            }
            if(pid ==0)
                break;
        }
    }
    return 0;
}