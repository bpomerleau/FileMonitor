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

#define INOTIFY_ITERATOR ((struct inotify_event *)((unsigned char *)inotify_buffer + pointvar))

int main(int argc, char *argv[]){
	
	sigset_t signal_mask;	
	DIR *dirStream;
	struct dirent *next;
	struct inotify_event *inotify_buffer;
	uint32_t mask;
	char *path;
	int period;
	int printflag;	
	int fd;
	int wd;
	char type;

	void sig_handler(int signal){
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
	inotify_buffer = malloc(sizeof(struct inotify_event) * 10 );
	
	mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO;
	fd = inotify_init();
	wd = inotify_add_watch(fd, path, mask);
	
	int bytestoread = 0;
	int pointvar = 0;
	while (1) {	
	sigprocmask(SIG_BLOCK, &signal_mask, NULL);
	if (printflag == 1) {
		printflag = 0;
		system("date");
		ioctl(fd, FIONREAD, &bytestoread);

		if (bytestoread > 0) {
			read(fd, inotify_buffer, sizeof(struct inotify_event)*10); 
			while (pointvar < bytestoread) {
				printf("%i --- %s\n", INOTIFY_ITERATOR->mask, INOTIFY_ITERATOR->name);
				pointvar += sizeof(struct inotify_event) + INOTIFY_ITERATOR->len;
			}
			pointvar = 0;
		}
	}
	sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
 }

	free(inotify_buffer);
	return 0;
}