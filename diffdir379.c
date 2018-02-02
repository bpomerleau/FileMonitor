#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/inotify.h>
#include<sys/time.h>
#include<signal.h>
#include<string.h>


int main(int argc, char *argv[]){

	int period;
	char *path;
	DIR *dirStream;
	struct dirent *next;

	void timer_handler(int signal){
		printf("Timer expired. Resetting...\n");
	}

	void sigusr_handler(int signal){
		printf("User interrupt!!\n");
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
	

	
	//itimer sigaction init	
	sigset_t timer_mask;
	if (sigemptyset(&timer_mask) == -1) printf("Error on sigemptyset, errno: %s\n", strerror(errno));
	if (sigaddset(&timer_mask, SIGUSR1) == -1) printf("Error on sigaddset(SIGUSR1), errno: %s\n", strerror(errno));
	if (sigaddset(&timer_mask, SIGINT) == -1) printf("Error on sigaddset(SIGINT), errno: %s\n", strerror(errno));
		
	struct sigaction reg_report = { .sa_handler = timer_handler, .sa_mask = timer_mask }; 
	
	if (sigaction(SIGALRM, &reg_report, NULL) == -1) printf("Error on sigaction, errno: %s\n", strerror(errno));

	//SIGUSR1 sigaction init	
	sigset_t SIGUSR_mask;
	if (sigemptyset(&SIGUSR_mask) == -1) printf("Error on sigemptyset, errno: %s\n", strerror(errno));
	if (sigaddset(&SIGUSR_mask, SIGALRM) == -1) printf("Error on sigaddset(SIGALRM), errno: %s\n", strerror(errno));
	if (sigaddset(&SIGUSR_mask, SIGINT) == -1) printf("Error on sigaddset(SIGINT), errno: %s\n", strerror(errno));	
	
	struct sigaction sigusr = { .sa_handler = sigusr_handler, .sa_mask = SIGUSR_mask }; 
	
	if (sigaction(SIGUSR1, &sigusr, NULL) == -1) printf("Error on sigaction, errno: %s\n", strerror(errno));
	
		
	//itimter init
	struct timeval interval = { .tv_sec = period };
	const struct itimerval timer = { .it_interval = interval, .it_value = interval};

	if (setitimer(ITIMER_REAL, &timer, NULL) == -1) printf("Error on setitimer. errno: %s\n", strerror(errno));
	
		
	
	printf("period = %i\npath = %s\n",period, path);
	dirStream = opendir(path);
	if (dirStream < 0){
		printf("%s not a valid directory\n", path);
		return 0;
	}

	next = readdir(dirStream);
	
	char type;
	//TODO --- the printing format of this while loop needs to be 
	//  		brought into assignment spec
	while (next != NULL) {		
		if ((*next).d_type == DT_DIR) type = 'd';
		else type = 'f';
		printf("%c - %s\n", type, (*next).d_name);
		next = readdir(dirStream);
	}
	
	closedir(dirStream);

	struct inotify_event *inotify_buffer = malloc(sizeof(struct inotify_event) * 10 );
	
	uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY;
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, path, mask);
	
	//TODO -- instead of this while loop, we need to set up sigaction for 
	//		  handling signals, and get inotify to send a signal when a
	//		  change has occurred. As it is, read blocks, and we have to wait
	//		  for a change for the process to continue
	while (1) {	
		printf("loop\n"); // for some reason if this line gets deleted something breaks
		sleep(period);
		
		//TODO configure read to not block
		//read(fd, inotify_buffer, sizeof(struct inotify_event)*10); //FIONREAD ioctl() for # available bytes
		//printf("%i --- %s", (*inotify_buffer).wd, (*inotify_buffer).name);			
	}

	free(inotify_buffer);
	return 0;
}