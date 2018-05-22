#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

int main(){
	/*int action, region;//,int copyData, pasteData;
	char aux[100], dadosSent[100];
	//char dadosReceived[100];

	// Connects to the cliboard
	int sock_fd = clipboard_connect("./");
	if(sock_fd == -1){
		exit(-1);
	}

	while(1) {
		// Ask the user the action that wants to perform
		printf("\nCOPY [0], PASTE [1] or WAIT [2]: ");
		fgets(aux, 50, stdin);
		sscanf(aux, "%d", &action);

		// Copies the data read to the clipboard server
		if(action == COPY) {
			// Ask the user for some data to be stored
			printf("Write something: ");
			fgets(dadosSent, 100, stdin);
			// Terminates the received string
			dadosSent[strlen(dadosSent) - 1] = '\0';

			// Ask the region to store the data
			printf("Region [0-9]: ");
			fgets(aux, 100, stdin);
			sscanf(aux, "%d", &region);

			// Sends the data to the cliboard server
			copyData = clipboard_copy(sock_fd, region, dadosSent, strlen(dadosSent) + 1);
			if(copyData < 1) {
				printf("Error on copy\n");
			}
			else {
				printf("Sent %d - data: %s\n", copyData, dadosSent);
			}
		}
		else if(action == PASTE) {
			// Ask the region from where will paste data
			printf("Region [0-9]: ");
			fgets(aux, 100, stdin);
			sscanf(aux, "%d", &region);

			// Sends the information to the clipboard
			pasteData = clipboard_paste(sock_fd, region, dadosReceived, 100*sizeof(char));
			if(pasteData < 1) {
				printf("Didn't receive paste information\n");
			}
			else {
				printf("Received %d - data: %s\n", pasteData, dadosReceived);
			}
		}
		else if(action == WAIT) {
			// Ask the region from where will paste data
			printf("Region [0-9]: ");
			fgets(aux, 100, stdin);
			sscanf(aux, "%d", &region);

			// Sends the information to the clipboard
			pasteData = clipboard_wait(sock_fd, region, dadosReceived, 100*sizeof(char));
			if(pasteData < 1) {
				printf("Didn't receive paste information\n");
			}
			else {
				printf("Received %d - data: %s\n", pasteData, dadosReceived);
			}
		}
		else {
			printf("Closing connection\n");
			break;
		}
	}
	
	close(sock_fd);*/
	exit(0);
}
