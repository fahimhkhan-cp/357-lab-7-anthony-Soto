#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define PORT 24242

void sigchild_handler(int s) {
	int saved_errno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0){
		errno = saved_errno;
	}	
}


void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r+");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      close(nfd);
      return;
   }

   while ((num = getline(&line, &size, network)) >= 0)
   {
        fprintf(network, "%s", line);
	fflush(network);	
   }

   free(line);
   fclose(network);
}

void run_service(int fd)
{

   struct sigaction sa;
    sa.sa_handler = sigchild_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd == -1){
	continue;	
      }
      
      pid_t pid = fork();
	if (pid == 0){
		close(fd);
		handle_request(nfd);
		exit(0);
	} else if (pid > 0) {
		close(nfd);
	} else {
		perror("fork");
		close(nfd);
	}
	
   }
}

int main(void)
{
   int fd = create_service(PORT);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d (PID: %d)\n", PORT, getpid());
   run_service(fd);
   close(fd);

   return 0;
}
