/*
    Marcel Morin - 260605670
    ECSE 427 Operating Systems
    Assignment 1 - TinyShell Implementation
    February 4th, 2017
*/

#include <stdio.h>
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h> 

#define JOBCAP  12 //max # of jobs
#define MAXINPUT 100

//Global Vars
struct job {
    pid_t pid;
    int jobid;
    char command[100];//longest single input string
};
struct job;
struct job jobList[JOBCAP]; //Max # of jobs
int jobnumber = 1;
char *pipeargs[MAXINPUT];


//Function Declarations
char getcmd(char *prompt, char *args[], int *background, int *outpipe);
int processCommand (char *args[], int *bg, int *outpipe, int * count);
int addJobList(pid_t pid, char *args[]);
int builtInCommand (char *args[], int * outpipe, char * outputLoc);
void backgrounJobs(void);
void checkJobList(void);


//Signal Handler
void sigHandler(int signal){
    
    if (signal == SIGINT){//kill program
        //printf(" Caught signal for Ctrl+C\n");
        printf("\n");
        //exit(0);
    }
    if (signal == SIGTSTP){//doing nothing when Ctrl+Z is detected
        printf("caught Z");
        printf("\n");
    }
    
    setvbuf (stdout, NULL, _IONBF, 0);
    printf("tS~> ");
}


//Gets user input and tokenizes it. Also flags if piping or redirection is found in input.
//If piping is flagged it will tokenized two different commands.
char getcmd(char *prompt, char *args[], int *background, int *outpipe) {
    
    int length, i = 0;
    char *token,*token2, *loc;
    char *line = NULL;
    char *linecopy=NULL;
    char *pipeline = NULL; //to be used if piping is needed
    char *cmd = NULL;
    size_t size = MAXINPUT;
    
    int spaces = 0;
    int characters = 0;

    while(characters == 0 ){ //checking if user input a string that isnt just spaces and a null character
        spaces = 0;
        characters = 0;
        
        printf("%s", prompt);
        length = getline(&line, &size, stdin);
        
        if (length <= 0) {
            exit(-1);
        }
        for(int k = 0; k < length; k++){
            if(line[k] == ' '){
                spaces++;
            }else if(line[k] == '\n'){
                
            }else {
                characters++;
            }
            
        }
    }

    
    if ((loc = index(line, '>')) != NULL) { //output redirecting flag
        *outpipe = 1;
        *loc = ' ';
    } else {
        
        if ((loc = index(line, '|')) != NULL){ //piping flag
            pipeline = strsep(&line, "|");
            linecopy = pipeline;
            pipeline = line;
            line = linecopy;

            *outpipe = 2;
            *loc = ' ';
            
            //tokenize pipeline
            while ((token2 = strsep(&pipeline, " \t\n")) != NULL) {
                for (int j = 0; j < strlen(token2); j++)
                    if (token2[j] <= 32)
                        token2[j] = '\0';
                if (strlen(token2) > 0)
                    pipeargs[i++] = token2;
            }
            
            pipeargs[i++] = NULL;
            
            
        } else {
        *outpipe = 0;
        }
    }
    
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;
    
    i = 0; //reset i if it was used in tokenizing the pipe command
    while ((token = strsep(&line, " \t\n")) != NULL) {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    
    args[i++] = NULL;
    
    
    return i;
}


//function that depending on if redirection, piping, or builtin commands was flagged will proceed appropriately
int processCommand (char *args[], int *bg, int *outpipe, int *count){
    pid_t pid, waitpid;
    int status;
    char *outputLoc = NULL; //location to redirect to
    
    if (builtInCommand(args, outpipe, args[*count]) != 1){
        pid = fork();
        
        if (pid == 0) {   // Child process
            if(*outpipe == 1){//output redirection
                
                outputLoc = args[*count-2];//copies outputloc
                args[*count-2] = NULL; //deletes last input of array
                close(fileno(stdout));
                int file;
                file = open(outputLoc, O_RDWR|O_CREAT, 0777);
                
                //close(file);
                
            }else if(*outpipe ==2){   //Piping
                pid_t cpid;
                int pip[2];
                int status2;
                if (pipe(pip) == -1) //creates pipe
                    exit(EXIT_FAILURE);
                
                
                cpid = fork();
                
                if(cpid == 0){ //child
                    close(pip[0]);
                    close(1);
                    dup(pip[1]);
                    close(pip[1]);
                    execvp(args[0], args);
                }else if(cpid < 0){
                    exit(EXIT_FAILURE);
                }else {//parent
                    close(pip[1]);
                    close(0);
                    dup(pip[0]);
                    close(pip[0]);
                    if (execvp(pipeargs[0], pipeargs) == -1) {
                        perror("**No command found** \n");
                    }

                    close(STDIN_FILENO);
                    dup(pip[0]);
                    close(pip[1]);
                    close(pip[0]);
                }
                if (execvp(args[0], args) == -1) {
                    perror("**No command found** \n");
                }
                
                exit(EXIT_FAILURE);

            }
            
            if (execvp(args[0], args) == -1) {
                perror("**No command found** \n");
            }
            exit(EXIT_FAILURE);
            

            
        } else if (pid < 0) { //error
            exit(EXIT_FAILURE);
        } else {
            
            if(*bg == 1){
                addJobList(pid, args);
            }else{
                
                while(wait(&status) != pid){
                    //waits till child is done
                }
            }
        }
    }

    return 1;
}


//Includes comands to be checked before deciding to fork
int builtInCommand (char *args[], int * outpipe, char * outputLoc){
    char directory[MAXINPUT]; //for directory to get copied into

        if(!strcmp(args[0], "exit")){ //exit program
            exit(0);
        }
    
        if(!strcmp(args[0], "cd")){ //change directories
            printf("args[1]: %s\n",args[1]);
            chdir(args[1]);
            getcwd(directory, MAXINPUT);
            printf("%s\n",directory);
            return 1;
        }
    
        if(!strcmp(args[0], "pwd")){ //present working directory
            getcwd(directory, MAXINPUT);
            printf("%s\n",directory);
            return 1;
        }
    
        if(!strcmp(args[0], "fg")){
            int status;
            char * j = args[1];
            int value = atoi(j);
            for (int i = 0; i < JOBCAP; i++){ //go through job array to find jobid
                if(value == jobList[i].jobid){
                    while(wait(&status) != jobList[i].pid){
                        //waits till child is done
                    }
                    return 1;
                }
            }
            return 1;
        }
    
        if(!strcmp(args[0], "jobs")){
            backgrounJobs();
            return 1;
        }

        return 0;
}

//Adds job to the list and sends error if list is full
int addJobList(pid_t pid, char *args[]){
    int i;
    for (i = 0; i < JOBCAP; i++) {
        if (jobList[i].pid == 0) {
            jobList[i].pid = pid;
            jobList[i].jobid = jobnumber++;
            sprintf( jobList[i].command, "%s %s", args[0], args[1]);
            return 1;
        }
    }
    printf("Too many jobs in jobList\n");
    return 0;
}

//Prints the jobs in the jobs list, ignores empty slots
void backgrounJobs(){
    int i = 0;
    while(i<JOBCAP){
        if (jobList[i].pid != 0){
            printf("<%d>  (%d) %s\n", jobList[i].jobid, jobList[i].pid, jobList[i].command);
        }
        i++;
    }
}

//Checks if any jobs in the joblist are done, if they are remove them and make the slot available
void checkJobList(){
    int m = 0;
    while(m < JOBCAP){
        int status;
        if(jobList[m].pid == 0){
        }else {
            if (waitpid(jobList[m].pid, &status, WNOHANG)){
                jobList[m].pid = 0;
                jobList[m].jobid = 0;
                strcpy(jobList[m].command,"\0");
            }
        }
        m++;
    }

}


int main(void) {
    
    if(signal(SIGINT,sigHandler) == SIG_ERR){
        printf("ERROR in binding singal handler\n");
        exit(1);
    }
    
    if(signal(SIGTSTP,sigHandler) == SIG_ERR){
        printf("ERROR in binding singal handler\n");
        exit(1);
    }
    signal(SIGTSTP,SIG_IGN);
    
    int i;
    for (i = 0; i < JOBCAP; i++){ //initialized all jobs with pid 0
        jobList[i].pid = 0;
         jobList[i].jobid = 0;
        strcpy(jobList[i].command, "\0");
    }
    
    int bg;
    int outpipe;
    
        while(1) {
            char *args[MAXINPUT];
            fflush( NULL );
            
            bg = 0;
            outpipe = 0;
            checkJobList();
            int cnt = getcmd("\ntS~> ", args, &bg, &outpipe); //cnt will help me find the last input of the array

            checkJobList(); //checkjoblist after user enters command as he may have taken some time and jobs may have ended between the last checkjoblist

            int proc = processCommand(args, &bg, &outpipe, &cnt); //execute command

            }
}



