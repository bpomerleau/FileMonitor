#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/inotify.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<signal.h>
#include<string.h>
#include<time.h>

int main(int argc, char *argv[]){
    
    sigset_t signal_mask;    
    DIR *dirStream;
    struct dirent *next;
    char inotify_longbuffer[(sizeof(struct inotify_event) + 256) * 1000];
    uint32_t mask;
    char *path;
    int period;
    int printflag;    
    int fd;
    int wd;
    char type;

    void sig_handler(int signal){
   	 system("date");
   	 printflag = 1;
    }

    void init() {
   	 //signal mask    
   	 if (sigemptyset(&signal_mask) == -1)
   		 printf("Error on sigemptyset, errno: %s\n", strerror(errno));
   	 if (sigaddset(&signal_mask, SIGUSR1) == -1)
   		 printf("Error on sigaddset(SIGUSR1), errno: %s\n", strerror(errno));
   	 if (sigaddset(&signal_mask, SIGINT) == -1)
   		 printf("Error on sigaddset(SIGINT), errno: %s\n", strerror(errno));
   	 if (sigaddset(&signal_mask, SIGALRM) == -1)
   		 printf("Error on sigaddset(SIGALRM), errno: %s\n", strerror(errno));   	 
   	 
   	 struct sigaction sig_struct = { .sa_handler = sig_handler, .sa_mask = signal_mask };
   	 
   	 //timer sigaction
   	 if (sigaction(SIGALRM, &sig_struct, NULL) == -1)
   		 printf("Error on sigaction, errno: %s\n", strerror(errno));
   	 //SIGUSR1 sigaction
   	 if (sigaction(SIGUSR1, &sig_struct, NULL) == -1)
   		 printf("Error on sigaction, errno: %s\n", strerror(errno));
    
   	 
   	 //itimter init
   	 struct timeval interval = { .tv_sec = period };
   	 const struct itimerval timer = { .it_interval = interval, .it_value = interval};

   	 if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
   		 printf("Error on setitimer. errno: %s\n", strerror(errno));
    
    }

    int first_print() {
   	 //first time print of everything in directory

   	 dirStream = opendir(path);

   	 if (dirStream < 0){
   		 printf("%s not a valid directory\n", path);
   		 return -1;
   	 }

   	 system("date"); //prints the date
   	 next = readdir(dirStream);
   	 while (next != NULL) {   	 
   		 if (next->d_type == DT_DIR)
   			 type = 'd';
   		 else if (next->d_type == DT_REG)
   			 type = '+';
   		 else
   			 type = 'o';

   		 if ((strcmp(next->d_name,".") != 0) && (strcmp(next->d_name,"..") != 0)) {
   			 printf("%c %s\n", type, (*next).d_name);    
   		 }
   		 next = readdir(dirStream);
   	 }
    
   	 closedir(dirStream);
   	 return 0;
    }
    
    if (argc == 3){
   	 period = atoi(argv[1]);
   	 path = argv[2];
    } else if (argc > 3) {
   	 printf("Too many arguments\n");
   	 return 0;
    } else {
   	 printf("Too few arguements\n");
   	 return 0;
    }

    init();    
    if (first_print()==-1) return 0;    
    
    //init inotify
    
    mask = IN_CREATE | IN_DELETE | IN_MODIFY;
    fd = inotify_init1(IN_NONBLOCK);
    wd = inotify_add_watch(fd, path, mask);
   	int bytechec = 0;
    while (1) {    
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    if (printflag == 1) {
   	printflag = 0;
   	ioctl(fd, FIONREAD, &bytechec);
   	 //TODO configure read to not block
    	while (bytechec > 0){
   	  	//FIONREAD ioctl() for # available bytes
        	int r = read(fd, inotify_longbuffer, (sizeof(struct inotify_event) + 256) * 1000);
        	int pointvar = 0;
        	while (pointvar < r){
   	        struct inotify_event *inotify_buffer = ( struct inotify_event * ) &inotify_longbuffer[ pointvar ];
       		//printf("%i --- %s", (inotify_buffer)->mask, (inotify_buffer)->name);
            pointvar += (sizeof(struct inotify_event) + (inotify_buffer)->len);
            if ((*inotify_buffer).mask & IN_CREATE){
                if ((*inotify_buffer).mask & IN_ISDIR){
                    printf("d %s", (inotify_buffer)->name);
                }
                else if (strchr((inotify_buffer)->name, '.')){
                    printf("+ %s", (inotify_buffer)->name);
                }
                else{
                    printf("o %s", (inotify_buffer)->name);                    
                }
            }

            if ((*inotify_buffer).mask & IN_DELETE){
                printf("- %s", (inotify_buffer)->name);
            }
            if ((*inotify_buffer).mask & IN_MODIFY){
                printf("* %s", (inotify_buffer)->name);
            }

   	        //r += (sizeof(struct inotify_event)+(inotify_buffer)=>len);
       		printf("\n");    
        	}  	 
        	//fflush(stdout);
        	ioctl(fd, FIONREAD, &bytechec);
    	}
    }
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    }

    //free(inotify_buffer);
    return 0;
}

//stackoverflow.com/questions/5211993/using-read-with-inotify

