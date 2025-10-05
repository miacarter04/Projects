/**
 * filecopy.c
 * 
 * This program copies files using a pipe.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define READ_END 0
#define WRITE_END 1
#define BUFSIZE 8192

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Error: 3 arguments needed: %s <source> <destination>\n", argv[0]);
		return 1;
	}

	const char *source_name = argv[1]; //parsing the arguments to retrieve source and destination files!
	const char *destination_name = argv[2];

	char buffer[BUFSIZE];
	
	int pipefd[2]; //file descriptors; pipefd[0] is read end, pipefd[1] is write end of pipe
	if (pipe(pipefd) < 0) { //creates pipe and handles potential error
		fprintf(stderr, "Error creating pipe.");
		return 1;
	}

	pid_t pid = fork(); //creating child process

	if (pid < 0) {
		fprintf(stderr, "Error: Child was not created");
		return 1;
	}
	if (pid == 0) { //Child Process pipe -> destination
		if (close(pipefd[WRITE_END]) == -1) { //if close returns -1, it was unsuccessful; error handling
			fprintf(stderr, "Error: Child can not close write end\n");
			_exit(1);
		}

		FILE *pDestination = fopen(destination_name, "wb");
		if (pDestination == NULL) {  //NULL if invalid file or can not open; error handling
			fprintf(stderr, "Error: Invalid Destination File. Could not open file for writing\n");
			_exit(1); //exit for child process
		}
		
		ssize_t bytes_read;
		while ((bytes_read = read(pipefd[READ_END], buffer, BUFSIZE)) > 0) { //reads from pipe read end to buffer
			size_t written = fwrite(buffer, 1, (size_t)bytes_read, pDestination); //writes from buffer to destination file

			if (written != (size_t)bytes_read) { //what is written should equal what was read, if not error
				fprintf(stderr, "Error: Child wrote short to destination file\n");
				close(pipefd[READ_END]);
				fclose(pDestination);
				_exit(1);
			}		
		}
		close(pipefd[READ_END]);
		fclose(pDestination);
		_exit(0);
		}
	

	if (pid > 0) { //Parent Process source -> pipe
		if (close(pipefd[READ_END]) == -1) {  //if close returns -1, it was unssuccessful; error handling
			fprintf(stderr, "Error: Parent can not close read end");
			return 1;
		}

		FILE *pSource = fopen(source_name, "rb");
		if (pSource == NULL) {  //invalid file or unsuccessful opening if fopen returns NULL; error handling
			fprintf(stderr, "Error: Invalid Source File. Could not open file for reading\n");
			return 1;
		}

		size_t num_read;
		while((num_read = fread(buffer, 1, BUFSIZE, pSource)) > 0) { //reads from source file to buffer
			ssize_t num_wrote = write(pipefd[WRITE_END], buffer, num_read); //writes to pipe write end

			if (num_wrote != num_read || num_wrote < 0) { //what ir read and written should be equal, and it should write more than 0 bytes, if not; error
				fprintf(stderr, "Parent wrote short to pipe\n");
				fclose(pSource);
				close(pipefd[WRITE_END]);
				wait(NULL); //waits for child process to finish
				return 1;
			}

			if (num_read < BUFSIZE) {
				if (ferror(pSource)){ //display an error if one is caught
					fprintf(stderr, "Parent failed to read from source\n");
					fclose(pSource);
					close(pipefd[WRITE_END]);
					wait(NULL); //wait for child process to finish to exit cleanly
					return 1;
				}
				break; //terminates loop once all contents are read
			}
		}

		fclose(pSource);
		close(pipefd[WRITE_END]);
		wait(NULL); //waits for child process to finish
		printf("File copied successfully!");
	}

	return 0;
}
