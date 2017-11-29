#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>



int main(int argc,char** argv){
    prepare();

    char* arglist0[] = {"ls", "-la", "|", "grep", "-i", "hw", NULL};
    process_arglist(6, arglist0);

    char* arglist2[] = {"sleep", "10", "&", NULL};
    process_arglist(3, arglist2);

    char* arglist[] = {"sleep", "60", NULL};
    process_arglist(2, arglist);

    finalize();
}

int find_pipe(int count, char ** arglist){
    int i;
    for (i=0 ; i < count ; i++){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
    }
    return -1;
}

int is_background(int count, char ** arglist){
    int i;
    if (strcmp(arglist[count-1], "&") == 0){
        return 1;
    }

    return 0;
}

// run child command in case it's a legal one, if not, returns 0 - this function takes care only in case there is not |
void execute_child(char** arglist){
    if (execvp(arglist[0], arglist) == -1){
        fprintf(stderr,"error in execvp\n",16);
        exit(1);
    }
}

int process_arglist(int count, char** arglist){

    // checks if there is positive amount of commands, if not, exit
    if (count <= 0){
        fprintf(stderr,"number of commands has to be a positive numbers\n",48);
        return 1;
    }

    struct sigaction sig_chld;
    memset(&sig_chld, 0, sizeof(sig_chld));
    sig_chld.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sig_chld, NULL);

    struct sigaction sig_dfl;
    //can fail?
    memset(&sig_dfl, 0, sizeof(sig_dfl));
    sig_dfl.sa_handler = SIG_DFL;

    struct sigaction sig_ign;
    memset(&sig_ign, 0, sizeof(sig_ign));
    sig_ign.sa_handler = SIG_IGN;

    //ignore_signal.sa_sigaction = signal_ignore_handler;
    if (sigaction(SIGINT, &sig_ign, NULL) < 0){
        fprintf(stderr,"error in sigaction\n",19);
        exit(1);
    }

    // checks if there is an &
    int is_background_process = 0;
    is_background_process = is_background(count, arglist);

    // find pipe
    int p_index=-1;
    p_index = find_pipe(count, arglist);
    int pfd[2] = {0};
    if (p_index >= 0){
        if (pipe(pfd)){
            fprintf(stderr,"error in pipe\n",14);
            exit(1);
        }
    }

    pid_t chld1 = fork();
    pid_t chld2;

    // we are in child process
    if (chld1 == 0) {
        if (p_index == -1) { //no pipe
            if (is_background_process) { //background child
                arglist[count - 1] = NULL; //ignore &
                execute_child(arglist);
            }
            else { //foreground child
                sigaction(SIGINT, &sig_dfl, NULL);
                execute_child(arglist);
            }
        }
        else{ //pipe
            sigaction(SIGINT, &sig_dfl, NULL);
            close(pfd[0]);
            if (dup2(pfd[1], 1) == -1){
                fprintf(stderr,"error in dup2\n",15);
                exit(1);
            }
            close(pfd[1]);
            arglist[p_index] = NULL;
            execute_child(arglist);
        }
    }


    if (p_index != -1) { //pipe
        chld2 = fork();
        if (chld2 == 0) {
            sigaction(SIGINT, &sig_dfl, NULL);
            close(pipe_fd[1]);
            if (dup2(pfd[0], 0) == -1){
                fprintf(stderr,"error in dup!\n\0",15);
                exit(1);
            }
            close(fd[0]);
            execute_child(&(arglist[pipe_index+1]));
        }
        close(pfd[0]);
        close(pfd[1]);
    }

    if (is_background_process){ //not waiting for a background child
        return 1;
    }

    int statchld1 = 0;
    int statchld2 = 0;
    if (p_index == -1) {
        if (waitpid(chld1, &stachld1, 0) == chld1) {return 1;}
    }
    else{
        if ((waitpid(chld1, &statchld1, 0) == chld1) && (waitpid(chld2, &statchld2, 0) == chld2)){return 1;}
    }

    return 1;
}

// prepare and finalize calls for initialization and destruction of anything required

int prepare(void){
    return 0;
}


int finalize(void){
    return 0;
}

