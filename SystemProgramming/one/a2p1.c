#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_FILENAME_LENGTH 120

// Indicate whether the program has been done.
int done = 0;
// Indicate whether the duration has been done, it should be set 1 at start
int durationDone = 1;

void printUsage();
int parseArguments(int argc, char *argv[], int *nLine, char *inputFile,
                   int *delay);
void mainLoop(int nLine, int delay, FILE *fp);
void onAlarm(int signum);
void printUsage() { fprintf(stdout, "Usage: a2p1 nLine inputFile delay\n"); }
void deleteLineBreak(char *buf);

void deleteLineBreak(char *buf) {

  int i = 0;
  while (i < strlen(buf) && buf[i] != '\n') {
    i++;
  }
  if (i != strlen(buf)) {
    buf[i] = '\0';
  }
}

/**
 * @brief Parse the arguments and validate the arguments
 *
 * @return 0 means successful, otherwise failed
 */
int parseArguments(int argc, char *argv[], int *nLine, char *inputFile,
                   int *delay) {
  if (argc != 4) {
    printUsage();
    return -1;
  }

  *nLine = atoi(argv[1]);

  if (*nLine <= 0) {
    fprintf(stderr, "nLine should be positive\n");
    return -1;
  }

  if (strlen(argv[2]) > MAX_FILENAME_LENGTH) {
    fprintf(stderr, "Filename should not exceed %d", MAX_FILENAME_LENGTH);
    return -1;
  }

  strcpy(inputFile, argv[2]);

  *delay = atoi(argv[3]);

  if (*delay <= 0) {
    fprintf(stderr, "delay should be positive\n");
    return -1;
  }

  return 0;
}

/**
 * @brief The timer expired, it will execute this function, it should set
 * the durationDone to 1, which will make the `threadLoop` to spin. And
 * make the `mainLoop` to start again.
 *
 */
void onAlarm(int signum) {
  durationDone = 1;
}

/**
 * @brief It will accept the user input in the duration.
 *
 */
void *threadLoop(void *arg) {
  while (!done) {
    static struct timeval timeout;
    static char buf[1024];
    static fd_set set;

    static int commandApplied = 0;

    while (!durationDone) {

      // Here, we should set the timeout to be 0.1s, it is important,
      // we should use non-blocking IO here.
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;

      if (commandApplied == 0) {
        printf("User command: ");
        fflush(stdout);
        commandApplied = 1;
      }


      FD_ZERO(&set);
      FD_SET(STDIN_FILENO, &set);

      int rv = select(1, &set, NULL, NULL, &timeout);
      if (rv > 0) {
        read(STDIN_FILENO, buf, 1024);
        deleteLineBreak(buf);

        if (strcmp(buf, "quit") == 0) {
          durationDone = 1;
          done = 1;
          return NULL;
        }

        // To execute the shell command;
        FILE *fp = popen(buf, "r");

        char *line;
        size_t size = 0;
        while (getline(&line, &size, fp) != EOF) {
          printf("%s", line);
        }

        pclose(fp);
        free(line);
        fflush(stdout);

        // Reset the `commandApplied`
        commandApplied = 0;
      }

    }
    commandApplied = 0;
  }
  return NULL;
}

/**
 * @brief start the loop, it should use `popen` to execute `cat <filename>`.
 * and read N lines. And then, the program should delay `delay` ms. The it
 * will set the `durationDone = 0` to indicate the `threadLoop` start to
 * handle user input.
 *
 */
void mainLoop(int nLine, int delay, FILE *fp) {
  char *line;
  size_t size = 0;

  struct sigaction act;
  act.sa_handler = &onAlarm;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  sigaction(SIGALRM, &act, NULL);

  pthread_t thread;
  pthread_create(&thread, NULL, threadLoop, NULL);

  double time = (double) delay / 1000.0;

  while (!done) {
    for (int i = 0; i < nLine; ++i) {
      if (getline(&line, &size, fp) != EOF) {
        printf("%s", line);
      } else {
        done = 1;
        return;
      }
    }
    // start the timer.
    alarm(time);
    printf("*** Entering a delay period of %d msec\n\n", delay);
    durationDone = 0;

    // spin the mainLoop.
    while (!durationDone) {;}

    if (!done) {
      printf("\n*** Delay period ended\n\n");
    }

  }

  pclose(fp);
  free(line);
}

int main(int argc, char *argv[]) {

  int nLine = 0;
  char *inputFile = malloc(sizeof(char) * MAX_FILENAME_LENGTH);
  if (inputFile == NULL) {
    fprintf(stderr, "memory allocation failed\n");
    return 0;
  }
  int delay = 0;

  // Parse the arguments
  if (parseArguments(argc, argv, &nLine, inputFile, &delay) != 0) {
    return 0;
  }

  // Here, I use cat shell command to produce the contents, which
  // is cat <filename> so we should extend the length for command
  char *command = malloc(sizeof(char) * (MAX_FILENAME_LENGTH + 4));

  sprintf(command, "cat %s", inputFile);

  FILE *fp = popen(command, "r");
  if (fp == NULL) {
    return 0;
  }

  printf("a2p1 starts: (nLine= %d, inFile='%s', delay= %d)\n", nLine, inputFile, delay);
  // start the main loop
  mainLoop(nLine, delay, fp);

  free(inputFile);
  free(command);

  return 0;
}
