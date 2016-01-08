#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXARG 128
#define MAXLINE 1024
#define MAXJOBS 16
#define MAXJID 1<<16

/*job status*/
#define FG 1
#define BG 2 
#define ST 3
#define UNDF 0

extern char **environ;

struct job_t{
	pid_t pid;	/*The job PID*/
	int jid;	/*job ID*/
	int state;	/*BG,FG,ST*/
	char cmdline[MAXLINE];
};

struct job_t jobs[MAXJOBS];
int nextjid = 0;

void eval(char *cmdline);
int parseline(const char *cmdline,char **argv);
int builtin_command(char **argv);

void unix_error(char *msg);
void app_error(char *msg);

typedef void handler_t(int);
handler_t *Signal(int signum,handler_t *handler);

void sigchld_handler(int sig);
void sigstp_handler(int sig);
void sigint_handler(int sig);

void do_bgfg(char **argv);
void waitfg(pid_t pid);

void clearjobs(struct job_t *jobs);
void initjobs(struct job_t *jobs);
int addjob(struct job_t *jobs,pid_t pid,int state,char *cmdline);
int deletejob(struct job_t *jobs,pid_t pid);
int maxjid(struct job_t *jobs);
void listjobs(struct job_t *jobs);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs,pid_t pid);

int main(){
	char cmdline[MAXLINE];

	Signal(SIGCHLD,sigchld_handler);
	Signal(SIGTSTP,sigstp_handler);
	Signal(SIGINT,sigint_handler);

	initjobs(jobs);
	while(1){
		printf("tsh> ");
		if((fgets(cmdline,MAXLINE,stdin) == NULL && ferror(stdin)))
			app_error("fgets_error");
		if(feof(stdin)){
			fflush(stdout);
			exit(0);
		}
		eval(cmdline);
		fflush(stdout);
	}
	exit(0);
}

void eval(char *cmdline){
	char *argv[MAXARG];
	int bg;
	pid_t pid;

	sigset_t mask;

	bg = parseline(cmdline,argv);
	if(argv[0] == NULL)
		return;
	if(!builtin_command(argv)){
		sigemptyset(&mask);
		sigaddset(&mask,SIGCHLD);
		sigprocmask(SIG_BLOCK,&mask,NULL);

		if((pid = fork())==0){
			sigprocmask(SIG_UNBLOCK,&mask,NULL);
			setpgid(0,0);
			if(execve(argv[0],argv,environ)<0){
				printf("%s: Command not found.\n",argv[0]);
				exit(0);
			}
		}
		int state =  bg ? BG:FG; 
		addjob(jobs,pid,state,cmdline);
		sigprocmask(SIG_UNBLOCK,&mask,NULL);
		 if(!bg)
		 	waitfg(pid);
		// unix_error("waitfg: waitpid error");
		// }else
		// 	printf("%d %s",pid,cmdline);
		fflush(stdout);
	}

	return;
}

int builtin_command(char** argv){
	if(!strcmp(argv[0],"jobs")){
		listjobs(jobs);
		return 1;
	}
	if(!strcmp(argv[0],"quit"))
		exit(0);
	if(!strcmp(argv[0],"&"))
		return 1;
	if(!strcmp(argv[0],"bg")||!strcmp(argv[0],"fg")){
		do_bgfg(argv);
		return 1;
	}
	return 0;
}

int parseline(const char *cmdline,char **argv){
	char *delim;
	int argc;
	int bg;
	static char buffer[MAXLINE];
	char *buf = buffer;
	strcpy(buf,cmdline);
	buffer[strlen(buf)-1] = ' ';
	while(*buf && (*buf==' '))
		buf++;

	argc = 0;
	while((delim = strchr(buf,' '))){
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim+1;
		while(*buf && (*buf==' '))
			buf++;
	}
	argv[argc] = NULL;
	if(argc==0)
		return 1;

	if((bg = (*argv[argc-1]=='&')) !=0)
		argv[--argc] = NULL;

	return bg;
}

void waitfg(pid_t pid){
	struct job_t *job = getjobpid(jobs,pid);
	while(job->state==FG){
		sleep(0);
	}
	fflush(stdout);
}

void do_bgfg(char **argv){
	if(argv[1] == NULL){
		printf("%s command requries PID \n",argv[0]);
		return;
	}

	pid_t pid;
	if((pid = atoi(argv[1])) <=0){
		printf("%s :argument must be a PID \n",argv[0]);
		return;
	}
	struct job_t *this_job = getjobpid(jobs,pid);
	if(this_job == NULL)
	{
		printf("%s: no such job \n",argv[1]);
		return;
	}
	if(!strcmp(argv[0],"bg")){
		kill(-pid,SIGCONT);
		this_job->state = BG;
		printf("pid: %d ,jid: %d,cmd: %s\n",this_job->pid,this_job->jid,this_job->cmdline);
	}else{
		kill(-pid,SIGCONT);
		this_job->state = FG;
		waitfg(pid);
	}
	return;
}

void sigchld_handler(int sig){
	pid_t pid;
	int status;
	while((pid=waitpid(-1,&status,WNOHANG|WUNTRACED))>0){
		if(WIFSTOPPED(status)){
			printf("pid: %d stopped by signal: %d\n",pid,WSTOPSIG(status));
			struct job_t *job = getjobpid(jobs,pid);
			if(job == NULL){
				app_error("pid do not exist\n");
				return;
			}
			job->state = ST;
		}else if(WIFSIGNALED(status)){
			printf("pid: %d terminated by signal: %d\n",pid,WTERMSIG(status));
			deletejob(jobs,pid);
		}else if(WIFEXITED(status)){
			printf("pid: %d exit normally with exit status = %d\n",pid,WEXITSTATUS(status));
			deletejob(jobs,pid);
		}
	}

}

void sigint_handler(int sig){
	pid_t fpid = fgpid(jobs);
	if(fpid<=0)
		return;
	if(kill(-fpid,SIGINT)<0){
		unix_error("error when kill in sigint_handler");
		return;
	}
	printf("pid: %d terminated by SIGINT!\n",fpid);
	deletejob(jobs,fpid);
}

void sigstp_handler(int sig){
	pid_t fpid = fgpid(jobs);
	if(fpid<=0)
		return;
	if(kill(-fpid,SIGTSTP)<0){
		unix_error("error when kill in sigstp_handler");
		return;
	}
	printf("pid %d stopped by SIGTSTP!\n",fpid);
	struct job_t* fjob = getjobpid(jobs,fpid);
	fjob->state = ST;
	return;
}

int maxjid(struct job_t *jobs){
	int i, max=0;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].jid>max)
			max = jobs[i].jid;
	}
	return max;
}

/*functions for job management*/
void clearjob(struct job_t *job){
	job->pid = 0; 
	job->jid = 0;
	job->state = UNDF;
	job->cmdline[0] = '\0';
}

void initjobs(struct job_t *jobs){
	int i;
	for(i=0;i<MAXJOBS;i++)
		clearjob(&jobs[i]); 
}

int addjob(struct job_t *jobs,pid_t pid,int state,char *cmdline){
	int i;
	if(pid <1)
		return 0;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].pid==0){
			jobs[i].pid = pid;
			jobs[i].jid = ++nextjid;
			if(nextjid>MAXJID)
				nextjid = 1;
			strcpy(jobs[i].cmdline,cmdline);
			jobs[i].state = state;
			return 1;
		}
	}
	printf("create too many jobs\n");
	return 0;
}

int deletejob(struct job_t *jobs,int pid){
	int i;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].pid==pid){
			clearjob(&jobs[i]);
			nextjid = maxjid(jobs);
			return 1;
		}
	}
	return 0;
}

void listjobs(struct job_t *jobs){
	int i;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].pid!=0){
			printf("pid: %d jid: [%d] ",jobs[i].pid,jobs[i].jid);
			switch(jobs[i].state){
				case BG:
					printf("Running ");
					break;
				case FG:
					printf("Foreground ");
					break;
				case ST:
					printf("Stopped ");
					break;
				default:
					printf(" listjobs: Interal error: jobs[%d].state = %d",
							i,jobs[i].state);
			}
			printf(" %s\n",jobs[i].cmdline);
		}
	}
}

pid_t fgpid(struct job_t *jobs){
	int i;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].state==FG)
			return jobs[i].pid;
	}
	return 0;
}

struct job_t *getjobpid(struct job_t *jobs,pid_t pid){
	if(pid<1)
		return NULL;
	int i;
	for(i=0;i<MAXJOBS;i++){
		if(jobs[i].pid==pid)
			return &jobs[i];
	}
	return NULL;
}

/*help function for sigaction and error output*/
handler_t *Signal(int signum,handler_t *handler){
	struct sigaction action,old_action;
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_RESTART;

	if(sigaction(signum,&action,&old_action) < 0)
		unix_error("Signal error");
	return old_action.sa_handler;
}

void unix_error(char *msg){
	fprintf(stdout, "%s: %s\n",msg,strerror(errno));
	exit(1);
}

void app_error(char *msg){
	fprintf(stdout, "%s\n",msg);
	exit(1);
}

