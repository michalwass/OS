#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


//looks for '|' and returns its index. If not found, return 0
int find_pipe(int count, char ** arglist){
    int i;
    for (i=0 ; i < count ; i++){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
    }
    return -1;
}
//checks if the last value in arglist is &- returns 1 if it is, else 0
int is_background(int count, char ** arglist){
    int i;
    if (strcmp(arglist[count-1], "&") == 0){
        return 1;
    }

    return 0;
}

// execute command if legal, else exits with exit value 1.
void execute_child(char** arglist){
    if (execvp(arglist[0], arglist) == -1){
        fprintf(stderr,"error in execvp: %s\n",strerror(errno));
        exit(1);
    }
}

//print error if sigaction fails
void error_in_sigaction(){
    fprintf(stderr,"error in sigaction: %s\n", strerror(errno));
    exit(1);
}

//print error if dup2 fails
void error_in_dup2(){
    fprintf(stderr,"error in dup2: %s\n",strerror(errno));
    exit(1);
}

int process_arglist(int count, char** arglist) {
    // checks if number of commads is valid, i.e. positive
    if (count <= 0) {
        fprintf(stderr, "Invalid number of commands- Must be positive number\n");
        return 1;
    }
    //initializing structs for sigaction
    struct sigaction sig_chld;
    struct sigaction sig_dfl;
    struct sigaction sig_ign;
    memset(&sig_chld, 0, sizeof(sig_chld));
    memset(&sig_dfl, 0, sizeof(sig_dfl));
    memset(&sig_ign, 0, sizeof(sig_ign));
    //set handlers
    sig_chld.sa_handler = SIG_IGN;
    sig_dfl.sa_handler = SIG_DFL;
    sig_ign.sa_handler = SIG_IGN;
    // for parent to catch SIGCHLD signal
    if (sigaction(SIGCHLD, &sig_chld, NULL) < 0) {
        error_in_sigaction();
    }
    //for parent to ignore SIGINT
    if (sigaction(SIGINT, &sig_ign, NULL) < 0) {
        error_in_sigaction();
    }

    int is_background_process = 0; //0 means foreground, 1 means background
    is_background_process = is_background(count, arglist);

    // check if there is a pipe
    int p_index = -1;
    p_index = find_pipe(count, arglist);

    int pfd[2] = {0};
    if (p_index > -1) {
        if (pipe(pfd)) {
            fprintf(stderr, "error in pipe: %s\n",strerror(errno));
            exit(1);
        }
    }

    pid_t chld1_pid = fork();
    pid_t chld2_pid;

    // we are in child process
    if (chld1_pid == 0) {
        if (p_index == -1) { //no pipe
            if (is_background_process) { //background child
                arglist[count - 1] = NULL; //ignore &
                execute_child(arglist);
            } else { //foreground child does not ignore SIGINT
                if (sigaction(SIGINT, &sig_dfl, NULL) < 0) {
                    error_in_sigaction();
                }
                execute_child(arglist);
            }
        } else { //pipe- alwayes foreground- so should not ignore SIGINT
            if (sigaction(SIGINT, &sig_dfl, NULL) < 0) {
                error_in_sigaction();
            }
            close(pfd[0]);
            if (dup2(pfd[1], 1) == -1) {
                error_in_dup2();
            }
            close(pfd[1]);
            arglist[p_index] = NULL;
            execute_child(arglist); //execute first child
        }
    }


    if (p_index != -1) { //pipe -execute second child
        chld2_pid = fork();
        if (chld2_pid == 0) {
            if (sigaction(SIGINT, &sig_dfl, NULL) < 0) {
                error_in_sigaction();
            }
            close(pfd[1]);
            if (dup2(pfd[0], 0) == -1) {
                error_in_dup2();
            }
            close(pfd[0]);
            execute_child(&(arglist[p_index + 1])); //execute second child, i.e. the process after '|'
        }
        close(pfd[0]);
        close(pfd[1]);
    }
    if (is_background_process) { //not waiting for a background child
        return 1;
    }

    int statchld1 = 0;
    int statchld2 = 0;
    if (p_index == -1) {
        if (waitpid(chld1_pid, &statchld1, 0) ==
            chld1_pid) { return 1; } //waiting for foreground process that is not a pipe
    } else {
        if ((waitpid(chld1_pid, &statchld1, 0) == chld1_pid) &&
            (waitpid(chld2_pid, &statchld2, 0) == chld2_pid)) { return 1; } //waiting for both foregrounds in pipe
    }

    return 1;
}

int prepare(void){
    return 0;
}

int finalize(void){
    return 0;
}