#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <unordered_set>
#include <vector>

#define NCLIENT 3
#define MAXWORD 32
#define BUFSIZE 1024
#define MAXWORD 32
#define NANO_PER_SEC 1000000000.0

void deleteLineBreak(char *buf);
void printUsage();
int parseArguments(int argc, char *argv[]);
int serverLoop();
int clientLoop(int idNumber, char *inputFile);
void handlePacket(int idNumber, int readFd, int writeFd, struct timespec *start,
                  std::vector<std::unordered_set<std::string>> &tables);
void serverPrintAndWrite(char *result, int fd);
int handleUserInput(std::vector<std::unordered_set<std::string>> &tables);
void handleGTimePacket(struct timespec *start, int writeFd, char *result);
void handlePutPacket(std::vector<std::unordered_set<std::string>> &tables,
                     int writeFd, char *result, std::string &name,
                     int idNumber);
void handleGetPacket(std::vector<std::unordered_set<std::string>> &tables,
                     int writeFd, char *result, std::string &name);
void handleDeletePacket(std::vector<std::unordered_set<std::string>> &tables,
                        int writeFd, char *result, std::string &name,
                        int idNumber);

/**
 * @brief auxiliary functions to delete line break.
 *
 */
void deleteLineBreak(char *buf) {

  int i = 0;
  while (i < strlen(buf) && buf[i] != '\n') {
    i++;
  }
  if (i != strlen(buf)) {
    buf[i] = '\0';
  }
}

void printUsage() { printf("Usage: a2p2 -s || a2p2 -c idNumber inputFile\n"); }

/**
 * @brief auxiliary functions
 *
 */
void serverPrintAndWrite(char *result, int fd) {
  printf("Transmitted (src= server) %s", result);
  write(fd, result, strlen(result));
}

void getTokens(char tokens[][BUFSIZE], char *buf, char *delim,
               int *tokenLength) {
  char *token = strtok(buf, delim);
  while (token != NULL) {
    strcpy(tokens[(*tokenLength)++], token);
    token = strtok(NULL, delim);
  }
}

/**
 * @brief handle the gtime packet
 *
 */
void handleGTimePacket(struct timespec *start, int writeFd, char *result) {
  struct timespec end;
  clock_gettime(CLOCK_REALTIME, &end);
  double start_sec = start->tv_sec + start->tv_nsec / NANO_PER_SEC;
  double end_sec = end.tv_sec + end.tv_nsec / NANO_PER_SEC;
  double elapsed_sec = end_sec - start_sec;
  sprintf(result, "TIME:\t %.3f\n", elapsed_sec);
  serverPrintAndWrite(result, writeFd);
}

/**
 * @brief handle put packet
 *
 */
void handlePutPacket(std::vector<std::unordered_set<std::string>> &tables,
                     int writeFd, char *result, std::string &name,
                     int idNumber) {
  for (int i = 0; i < tables.size(); ++i) {
    if (tables[i].count(name)) {
      sprintf(result, "ERROR: object already exists\n");
      serverPrintAndWrite(result, writeFd);
      return;
    }
  }
  tables[idNumber - 1].insert(name);
  sprintf(result, "OK\n");
  serverPrintAndWrite(result, writeFd);
}

/**
 * @brief Handle the get packet
 *
 */
void handleGetPacket(std::vector<std::unordered_set<std::string>> &tables,
                     int writeFd, char *result, std::string &name) {
  for (int i = 0; i < tables.size(); ++i) {
    if (tables[i].count(name)) {
      sprintf(result, "OK\n");
      serverPrintAndWrite(result, writeFd);
      return;
    }
  }
  sprintf(result, "ERROR: object not found\n");
  serverPrintAndWrite(result, writeFd);
}

/**
 * @brief handle the delete packet
 *
 */
void handleDeletePacket(std::vector<std::unordered_set<std::string>> &tables,
                        int writeFd, char *result, std::string &name,
                        int idNumber) {
  for (int i = 0; i < tables.size(); ++i) {
    if (tables[i].count(name)) {
      if (i + 1 == idNumber) {
        sprintf(result, "OK\n");
        tables[i].erase(name);
        serverPrintAndWrite(result, writeFd);
        return;
      } else {
        sprintf(result, "ERROR: client not owner\n");
        serverPrintAndWrite(result, writeFd);
        return;
      }
    }
  }
  sprintf(result, "ERROR: object not found\n");
  serverPrintAndWrite(result, writeFd);
}

/**
 * @brief handle the packet, put the handled result back to
 *
 */
void handlePacket(int idNumber, int readFd, int writeFd, struct timespec *start,
                  std::vector<std::unordered_set<std::string>> &tables) {

  char buf[BUFSIZE];
  int tokenLength = 0;
  char tokens[MAXWORD][BUFSIZE];
  char result[BUFSIZE] = {};

  read(readFd, buf, BUFSIZE);
  deleteLineBreak(buf);
  printf("Received (src= client:%d) %s\n", idNumber, buf);

  char delim[] = {"\t "};
  getTokens(tokens, buf, delim, &tokenLength);

  if (strcmp(tokens[0], "gtime") == 0) {
    handleGTimePacket(start, writeFd, result);
  } else if (strcmp(tokens[0], "put") == 0) {
    std::string name = std::string(tokens[1]);
    handlePutPacket(tables, writeFd, result, name, idNumber);
  } else if (strcmp(tokens[0], "get") == 0) {
    std::string name = std::string(tokens[1]);
    handleGetPacket(tables, writeFd, result, name);
  } else if (strcmp(tokens[0], "delete") == 0) {
    std::string name = tokens[1];
    handleDeletePacket(tables, writeFd, result, name, idNumber);
  }
}

/**
 * @brief handle the user input
 *
 */
int handleUserInput(std::vector<std::unordered_set<std::string>> &tables) {
  char buf[BUFSIZE];
  read(STDIN_FILENO, buf, BUFSIZE);
  deleteLineBreak(buf);
  if (strcmp(buf, "quit") == 0) {
    printf("quitting\n");
    return 1;
  } else if (strcmp(buf, "list") == 0) {
    printf("Object table:\n");
    for (int i = 0; i < tables.size(); ++i) {
      for (const std::string &str : tables[i]) {
        printf("(owner= %d, name=%s)\n", i + 1, str.data());
      }
    }
  } else {
    printf("Error: unknown user command!\n");
  }
  return 0;
}

int serverLoop() {
  // Here, we must set the flag to the `O_RDWR`, because a pipe
  // for reading that has no writer is always prepared for
  // reading, and the `select()` would fire indefinitely.
  // See
  // https://stackoverflow.com/questions/580013/how-do-i-perform-a-non-blocking-fopen-on-a-named-pipe-mkfifo

  printf("a2p2: do_server\n");

  struct timespec start;

  std::vector<std::unordered_set<std::string>> tables(3);

  // To set up the clock
  clock_gettime(CLOCK_REALTIME, &start);

  int fifo_1_0 = open("fifo-1-0", O_RDWR | O_NONBLOCK);
  int fifo_2_0 = open("fifo-2-0", O_RDWR | O_NONBLOCK);
  int fifo_3_0 = open("fifo-3-0", O_RDWR | O_NONBLOCK);

  // Here, we just need to write.
  int fifo_0_1 = open("fifo-0-1", O_RDWR | O_NONBLOCK);
  int fifo_0_2 = open("fifo-0-2", O_RDWR | O_NONBLOCK);
  int fifo_0_3 = open("fifo-0-3", O_RDWR | O_NONBLOCK);

  while (1) {
    static struct timeval timeout;
    static fd_set set;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    FD_SET(fifo_1_0, &set);
    FD_SET(fifo_2_0, &set);
    FD_SET(fifo_3_0, &set);

    timeout.tv_sec = 5;
    timeout.tv_usec = 100000;

    int maxfd = fifo_1_0;
    if (fifo_2_0 > maxfd) {
      maxfd = fifo_2_0;
    }

    if (fifo_3_0 > maxfd) {
      maxfd = fifo_3_0;
    }

    int rv = select(maxfd + 1, &set, NULL, NULL, &timeout);
    if (rv > 0) {
      // handle the client packet
      if (FD_ISSET(fifo_1_0, &set)) {
        handlePacket(1, fifo_1_0, fifo_0_1, &start, tables);
      } else if (FD_ISSET(fifo_2_0, &set)) {
        handlePacket(2, fifo_2_0, fifo_0_2, &start, tables);
      } else if (FD_ISSET(fifo_3_0, &set)) {
        handlePacket(3, fifo_3_0, fifo_0_3, &start, tables);
      } else {
        // handle the keyboard for a user command.
        if (handleUserInput(tables)) {
          return 0;
        }
      }
    }
  }

  return 0;
}

/**
 * @brief clientLoop will send one line of the inputFile
 * and send it to the server. and receives the packet from
 * the server.
 *
 */
int clientLoop(int idNumber, char *inputFile) {

  printf("main: do_client (idNumer= %d, inputFile= %s)\n", idNumber, inputFile);

  int tokenLength = 0;

  char fifoNameOutput[BUFSIZE];
  char fifoNameInput[BUFSIZE];
  char buf[BUFSIZE];
  char result[BUFSIZE];
  char delim[] = {"\t "};
  char line[BUFSIZE];
  char tokens[MAXWORD][BUFSIZE];

  sprintf(fifoNameOutput, "fifo-%d-0", idNumber);
  sprintf(fifoNameInput, "fifo-0-%d", idNumber);

  FILE *fifoOutput = fopen(fifoNameOutput, "r + w");
  if (fifoOutput == NULL) {
    return -1;
  }
  FILE *fifoInput = fopen(fifoNameInput, "r");
  if (fifoInput == NULL) {
    return -1;
  }

  FILE *input = fopen(inputFile, "r");
  while (fgets(line, BUFSIZE, input) != NULL) {

    tokenLength = 0;

    deleteLineBreak(line);

    // When this line is a comment, do NOTHING
    // When this line is empty, do NOTHING
    if (line[0] == '#' || strlen(line) == 0) {
      continue;
    }

    getTokens(tokens, line, delim, &tokenLength);

    if (atoi(tokens[0]) != idNumber) {
      continue;
    }

    if (strcmp(tokens[1], "delay") == 0) {
      int sleepTime = atoi(tokens[2]);

      printf("*** Entering a delay period of %d msec\n", sleepTime);
      sleep(double(sleepTime) / 1000.0);
      printf("*** Exiting delay period\n");
    } else if (strcmp(tokens[1], "quit") == 0) {
      fclose(input);
      fclose(fifoOutput);
      fclose(fifoInput);
      return 0;
    } else {
      if (tokenLength == 2) {
        printf("Transmitted (src= client: %d) %s\n", idNumber, tokens[1]);
        sprintf(line, "%s\n", tokens[1]);
      } else {
        printf("Transmitted (src= client: %d) (%s:  %s)\n", idNumber, tokens[1],
               tokens[2]);
        sprintf(line, "%s %s\n", tokens[1], tokens[2]);
      }

      // Send the packet to the server
      fputs(line, fifoOutput);
      // Here, we must flush the `fifoOutput`, we should
      // send the packet one by one and thus avoid deadlocks.
      fflush(fifoOutput);
      // Read the response from the server.
      fgets(result, BUFSIZE, fifoInput);
      deleteLineBreak(result);
      printf("Received (src = server) %s\n", result);
    }
  }

  fclose(input);
  fclose(fifoOutput);
  fclose(fifoInput);

  return 0;
}

/**
 * @brief Parse the arguments, one for server, for one client
 *
 */
int parseArguments(int argc, char *argv[]) {
  if (argc != 2 && argc != 4) {
    printUsage();
    return -1;
  }

  // server
  if (argc == 2) {
    if (strcmp(argv[1], "-s") != 0) {
      printUsage();
      return -1;
    }
    return serverLoop();
  }

  int idNumber = 0;

  // client
  if (argc == 4) {
    if (strcmp(argv[1], "-c") != 0) {
      printUsage();
      return -1;
    }
    idNumber = atoi(argv[2]);
    if (idNumber > NCLIENT || idNumber < 1) {
      printf("idNumber %d is not illegal\n", idNumber);
      return -1;
    }
    return clientLoop(idNumber, argv[3]);
  }

  return 0;
}

int main(int argc, char *argv[]) { return parseArguments(argc, argv); }
