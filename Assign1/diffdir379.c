#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
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

		if (dirStream == NULL){
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
	inotify_buffer = malloc(sizeof(struct inotify_event) * 100 );
	
	mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF;
	fd = inotify_init();
	wd = inotify_add_watch(fd, path, mask);
	
	//main eternal loop
	int bytestoread = 0;
	int pointvar = 0;
	// name_array is used to check whether file was created in this report period
	// if it was, then don't print other types of reports
	char name_array[100][16];
	int array_len = 0;
	while (1) {	
		sigprocmask(SIG_BLOCK, &signal_mask, NULL);
		if (printflag == 1) {
			printflag = 0;
			system("date");
			ioctl(fd, FIONREAD, &bytestoread);

			if (bytestoread > 0) {
				read(fd, inotify_buffer, sizeof(struct inotify_event)*100); 

				while (pointvar < bytestoread) {
					if (INOTIFY_ITERATOR->mask & (IN_CREATE | IN_MOVED_TO)) {
						strcpy(name_array[array_len], INOTIFY_ITERATOR->name);
						array_len++;	
				
						// use readdir to get file type and check whether file is still in directory
						dirStream = opendir(path);

						if (dirStream == NULL){
							printf("%s not a valid directory\n", path);
							return -1;
						}
						next = readdir(dirStream);
						while (next != NULL) {
							if (strcmp(next->d_name, INOTIFY_ITERATOR->name) == 0) { 	
								if (next->d_type == DT_DIR) 
									printf("d %s\n", INOTIFY_ITERATOR->name);
								else if (next->d_type == DT_REG)
									printf("+ %s\n", INOTIFY_ITERATOR->name);
								else
									printf("o %s\n", INOTIFY_ITERATOR->name);
								break;
								// if file not in directory, then it was deleted since its creation
								// and should not be reported.
							}
							next = readdir(dirStream);
						}
	
						closedir(dirStream);
					
					} else if (INOTIFY_ITERATOR->mask & IN_MODIFY) {
						bool print = true;
						for (int i = 0; i < array_len; i++) {
							if (strcmp(INOTIFY_ITERATOR->name, name_array[i]) == 0) {
								print = false;
							}
						}
						if (print) {
							printf("* %s\n", INOTIFY_ITERATOR->name);
						}
					
					} else if (INOTIFY_ITERATOR->mask & (IN_DELETE | IN_MOVED_FROM)) {
						bool print = true;					
						for (int i = 0; i < array_len; i++) {
							if (strcmp(INOTIFY_ITERATOR->name, name_array[i]) == 0) {
								print = false;
							}
						}						
						if (print) {
							printf("- %s\n", INOTIFY_ITERATOR->name);
						}
					} else if (INOTIFY_ITERATOR->mask & IN_DELETE_SELF){
						return 0;
					}
					pointvar += sizeof(struct inotify_event) + INOTIFY_ITERATOR->len;
				}
				array_len = 0;
				pointvar = 0;
			}
		}
		sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
	}

	free(inotify_buffer);
	free(name_array);
	return 0;
}