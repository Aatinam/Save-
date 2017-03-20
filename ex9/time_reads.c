#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

/*the number of seconds s that the program will run */
static int s;
/*number of reads completed by program */
static int completed_reads = 0;

void handler(int code){
  fprintf(stderr, "The program ran for %d seconds and completed %d reads.\n",s, completed_reads);
  exit(1);
}

int main(int argc, char **argv) {
  struct sigaction newact;
  newact.sa_handler = handler ;
  newact.sa_flags = 0;
  sigemptyset(&newact.sa_mask);
  sigaction( SIGALRM, &newact, NULL);

    if(argc != 3){
      fprintf(stderr, "Usage: time_reads filename\n");
      exit(1);
    }
    s = strtol(argv[1], NULL, 10 );
    struct itimerval new_value;
    new_value.it_value.tv_sec = s;
    new_value.it_value.tv_usec = 0;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_usec = 0;
    if((setitimer(ITIMER_REAL, &new_value, NULL)) == -1){
      perror("setitimer");
      exit(1);
    }


    FILE *f;
    if((f = fopen(argv[2], "rb")) == NULL){
      perror("fopen");
      exit(1);
    }
    int random_num, num_read, error;
    for(;;) {
    random_num = (random() % 100);
    fseek(f, random_num*sizeof(int), SEEK_SET);
    error = fread(&num_read, sizeof(int), 1, f);
    if(error == 1){//successfully read one int
    completed_reads++;
    printf("%d\n", num_read);
    }
    else {
      fprintf(stderr, "%s\n", "Error could not be read.");
    }
  }

return 0;
}
