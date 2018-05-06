#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clipboard.h"
//#include "action.h"


// Clipboard data
clipboard_struct clipboard;
// New data received
char *data = NULL;

// Socket structs
// UNIX
struct sockaddr_un local_addr_un;
struct sockaddr_un client_addr_un;

// INET
struct sockaddr_in local_addr_in;
struct sockaddr_in upperClipboard_addr;
struct sockaddr_in clientClipboard_addr;

// Sockets
int sock_fd_unix;
int sock_fd_inet;
int sock_fd_inetIP;

// Kill singnal
int killSignal = 0;

// Backup Signal
int backupSignal = 1;


// Unlinks the sockets when the program stops
void ctrl_c_callback_handler(int signum){
	struct flock fl;
	printf("aught signal Ctr-C\n");

	killSignal = 1;

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		free(clipboard.clipboard[i]);
	}
	printf("free clipboard\n");
	unlink(SOCKET_ADDR);
	close(sock_fd_inet);
	exit(0);
}





/***********************
 * Socket Functions
 ***********************/

void connect_unix() {
	// Create socket unix
	sock_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock_fd_unix == -1) {
		perror("socket unix");
		exit(-1);
	}

	local_addr_un.sun_family = AF_UNIX;
	strcpy(local_addr_un.sun_path, SOCKET_ADDR);

	// Bind the socket with the address assignet to it
	int err_unix = bind(sock_fd_unix, (struct sockaddr *) &local_addr_un, sizeof(struct sockaddr_un));
	if(err_unix == -1) {
		perror("bind");
		exit(-1);
	}

	// Listen
	err_unix = listen(sock_fd_unix, 5);
	if(err_unix == -1) {
		perror("listen");
		exit(-1);
	}

	printf("Local socket initiated\n");
}

// Connects to socket responsable with down connections on tree
void connect_inet(int portDown) {
	sock_fd_inet = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inet == -1) {
		perror("socket inet");
		exit(-1);
	}

	local_addr_in.sin_family = AF_INET;
	local_addr_in.sin_port= htons(portDown);
	local_addr_in.sin_addr.s_addr= INADDR_ANY;
	int err = bind(sock_fd_inet, (struct sockaddr *) &local_addr_in, sizeof(struct sockaddr_in));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf("Socket created and binded\n");

	if(listen(sock_fd_inet, 2) == -1) {
		perror("listen)");
		exit(-1);
	}

	printf("Ready to accept connections\n");
}

void connect_inetIP(int port, char ip[]) {
	sock_fd_inetIP = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd_inetIP == -1) {
		perror("socket");
		exit(-1);
	}

	upperClipboard_addr.sin_family = AF_INET;
	upperClipboard_addr.sin_port= htons(port);
	inet_aton(ip, &upperClipboard_addr.sin_addr);

	// Bind
	/*int err_inet = bind(sock_fd_inet, (struct sockaddr *) &local_addr, sizeof(local_addr));
	if(err_inet == -1) {
		perror("bind");
		exit(-1);
	}*/

	if( -1 == connect(sock_fd_inetIP, (const struct sockaddr *) &upperClipboard_addr, sizeof(struct sockaddr_in))) {
			printf("Error connecting to backup server\n");
			exit(-1);
	}
	printf("Online socket initiated\n");
}





/***********************
 * Clipboard Functions
 ***********************/

int copy(Message_struct messageReceived, int client) {
	
	int error = 0;
	int success = 1;

	// Allocs memory to store new data
	data = (char *)malloc(sizeof(char)*messageReceived.size[messageReceived.region]);
	if(data == NULL) {
		write(client, &error, sizeof(int));
		return 0;
	}

	// Store the size of the clipboard region
	clipboard.size[messageReceived.region] = messageReceived.size[messageReceived.region];

	// Informs the client that as allocated memory to receive the data
	write(client, &success, sizeof(int));

	// Receives the data from the client
	int numberOfBytesCopied = read(client, data, clipboard.size[messageReceived.region]);

	// Erases old data
	if(clipboard.clipboard[messageReceived.region] != NULL) {
		printf("Region cleared\n");
		free(clipboard.clipboard[messageReceived.region]);
	}

	// Assigns new data to the clipboard
	clipboard.clipboard[messageReceived.region] = data;

	printf("Received %d bytes - data: %s\n", numberOfBytesCopied, clipboard.clipboard[messageReceived.region]);

	return 1;
}

int paste(Message_struct messageReceived, int client) {
	
	int error = 0;
	int success = 1;


	Message_struct messageSend;
	if(messageReceived.size[messageReceived.region] < clipboard.size[messageReceived.region]) {
		write(client, &error, sizeof(int));
	}
	else {
		write(client, &success, sizeof(int));

		// Loads the structure with the information to the client
		messageSend.action = PASTE;
		messageSend.size[messageReceived.region] = clipboard.size[messageReceived.region];
		messageSend.region = messageReceived.region;

		write(client, &messageSend, sizeof(Message_struct));

		// Sends the data to the client
		int numberOfBytesPaste = write(client, clipboard.clipboard[messageSend.region], clipboard.size[messageReceived.region]);
		printf("Sent %d bytes - data: %s\n", numberOfBytesPaste, clipboard.clipboard[messageSend.region]);
	}

	return 1;
}

int backupPaste(Message_struct messageClipboard, int clipboard_client) {
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		messageClipboard.size[i] = clipboard.size[i];
	}

	// Sends the amount of data present in the clipboard
	write(clipboard_client, &messageClipboard, sizeof(Message_struct));

	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageClipboard.size[i] != 0) {
			printf("region %d ", i);
			printf("clipboard content %s size %d\n", clipboard.clipboard[i], (int ) clipboard.size[i]);
			write(clipboard_client, clipboard.clipboard[i], clipboard.size[i]);
		} 
	}
	printf("backupPaste complete\n");

	return 1;
}


int backupCopy() {
	Message_struct messageClipboard;
	messageClipboard.action = BACKUP;

printf("backupCopy sock_fd_inetIP %d\n", sock_fd_inetIP);

	int statusBackup = write(sock_fd_inetIP, &messageClipboard, sizeof(Message_struct));
	if(statusBackup == 0) {
		printf("cannot acess backup\n");
	}

	if(read(sock_fd_inetIP, &messageClipboard, sizeof(Message_struct)) != sizeof(Message_struct)) {
		perror("Commucation");
	}

	printf("Performing backup\n");
	for (int i = 0; i < NUMBEROFPOSITIONS; ++i)
	{
		if(messageClipboard.size[i] != 0) {
			printf("region %d ", i);
			data = (char *)malloc(sizeof(char)*messageClipboard.size[i]);
			if(data == NULL) {
				perror("malloc");
				exit(0);
			}

			// Store the size of the clipboard region
			clipboard.size[i] = messageClipboard.size[i];
			int numberOfBytesBackup = read(sock_fd_inetIP, data, clipboard.size[i]);
			if(numberOfBytesBackup != clipboard.size[i]) {
				printf("Number of bytes received backup is Incorrect. Received %d and it should be %d\n", numberOfBytesBackup, (int ) clipboard.size[i]);
				break;
			}
			clipboard.clipboard[i] = data;
			printf("	received %s, size %d\n", clipboard.clipboard[i], numberOfBytesBackup);
		}
	}

	printf("backupCopy complete\n");

	return 1;
}






/***********************
 * Thread Functions
 ***********************/
// Thread responsable to communicate with AP_TEST
void * clientThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;
	printf("Thread %lu Client %d\n", threadInfo->thread_id, threadInfo->inputArgument);
	int receivedBytes;

	Message_struct messageReceived;
	printf("Client Thread\n");
	while(killSignal == 0){
		// Reads the inbood message from the client
		receivedBytes = read(client, &messageReceived, sizeof(Message_struct));

		// If client sends EOF, terminates connection
		if(receivedBytes == 0) {
			break;
		}

		//printf("Client Thread - message receive\n");
		if(messageReceived.action == COPY) {
			printf("\nCOPY\n");
			if(copy(messageReceived, client) == 0) {
				printf("Error on copy\n");
				break;
			}
		}
		else if(messageReceived.action == PASTE) {
			//printf("Received information - action: PASTE\n");
			printf("\nPASTE\n");

			if(paste(messageReceived, client) == 0) {
				printf("Error on pasting\n");
				break;
			}
		}
	}
	close(client);
	// Free the struct attach to the thread
	free(threadInfo);
	printf("GoodBye - clientThread\n");
}


// Thread responsable to communicate with down Clipboards
void * clipboardThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int clipboard_client = threadInfo->inputArgument;

	Message_struct messageClipboard;

	// Update the info between clipboards
	while(killSignal == 0) {
		printf(".       clipboardThread\n");
		int numberOfBytesReceived = read(clipboard_client, &messageClipboard, sizeof(Message_struct));

printf("message received    %d   clipboardThread - bytes %d\n", messageClipboard.action, numberOfBytesReceived);

		if(numberOfBytesReceived == 0) {
			printf("Clipboard disconected\n");
			break;
		}
		if(messageClipboard.action == BACKUP) {
			printf("BACKUP\n");
			if( backupPaste(messageClipboard, clipboard_client) == 0) {
				printf("Error on backup\n");
			}

			
			
		}
		/*else if(messageClipboard.action == COPY) {
			//printf("Received information - action: COPY\n");
			printf("\nCOPY\n");
			// Allocs memory to store new data
			data = (char *)malloc(sizeof(char)*messageClipboard.size[messageClipboard.region]);
			if(data == NULL) {
				printf("Error alocating memory\n");
				write(clipboard_client, &error, sizeof(int));
				break;
			}

			// Informs the client that as allocated memory to receive the data
			write(clipboard_client, &success, sizeof(int));

			// Store the size of the clipboard region
			clipboard.size[messageClipboard.region] = sizeof(char)*messageClipboard.size[messageClipboard.region];

			// Receives the data from the client
			int numberOfBytesCopied = read(clipboard_client, data, clipboard.size[messageClipboard.region]);

			// Erases old data
			if(clipboard.clipboard[messageClipboard.region] != NULL) {
				printf("Region cleared\n");
				free(clipboard.clipboard[messageClipboard.region]);
			}

			// Assigns new data to the clipboard
			clipboard.clipboard[messageClipboard.region] = data;

			printf("Received %d bytes in region %d - data: %s\n", numberOfBytesCopied, messageClipboard.region, clipboard.clipboard[messageClipboard.region]);
		
		}*/
	}
	close(clipboard_client);
	// Free the struct attach to the thread
	free(threadInfo);
	printf("GoodBye - clipboardThread\n");
}


// Thread responsable to accept clipboards trying to connect to this one
void * downThread(void * arg) {
	thread_info_struct *threadInfo = arg;
	int client = threadInfo->inputArgument;

	pthread_t thread_id;
	socklen_t size_addr;

	size_addr = sizeof(struct sockaddr);
	
	while(killSignal == 0) {
		printf(". downThread\n");
		int clipboard_client = accept(client, (struct sockaddr *) &clientClipboard_addr, &size_addr);
		if(clipboard_client != -1) {
			printf("Accepted connection from other clipboard - down\n");

			thread_info_struct *threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
			if(threadInfo == NULL) {
				perror("malloc");
				exit(0);
			}

			threadInfo->inputArgument = clipboard_client;
			printf("threadInfo->inputArgument %d\n", threadInfo->inputArgument);
			// Creates new thread to handle the comunicatuion with the client
			pthread_create(&threadInfo->thread_id, NULL, &clipboardThread, threadInfo);
			printf("Thread created clipboard - ID %lu\n",  threadInfo->thread_id);

			size_addr = sizeof(struct sockaddr);			
		}

	}
	free(threadInfo);
	printf("GoodBye - downThread\n");
}


// Thread respnsable to communcate with upeer Clipboards
void * upThread(void *arg) {
	thread_info_struct *threadInfo = arg;
	int clipboardClient = threadInfo->inputArgument;

	Message_struct messageClipboard;

	

	while(killSignal == 0) {

		

	}

	close(clipboardClient);
	free(threadInfo);
	printf("GoodBye - upThread\n");	
}







int main(int argc, char const *argv[])
{
	int modeOfFunction;

	// Atach the ctrl_c_callback_handler to the SIGINT signal
	signal(SIGINT, ctrl_c_callback_handler);

	int portUp;
	char ip[14];

	if(argc == 1) {
		printf("Only local host\n");
		modeOfFunction = LOCAL;
	}
	else if(argc == 4) {
		// Server will function only as backup
		if(strcmp(argv[1], ONLINE_FLAG) != 0) {
			printf("Incorrect initialization\n");
			exit(0);
		}
		else {
			printf("Server with backup\n");
			modeOfFunction = 1;
		}

		// Copy IP from the argv
		strcpy(ip, argv[2]);

		// Copy port from argv
		portUp = atoi(argv[3]);
	}
	else {
		printf("Incorrect initialization - number of arguments\n");
		exit(0);
	}

	// Creates port to comunnicate with clipboards down on the tree
	srand(getpid());   // seeds the port number on the pid of the process
	int portDown = rand()%(64738-1024) + 1024; 

	// Informs the user of what is the port to connect
	printf("Port to acess machine: %d\n", portDown);


unlink(SOCKET_ADDR);

	// Init the clipboard struct
	for (int i = 0; i < 10; i++)
	{
		clipboard.clipboard[i] = NULL;
		clipboard.size[i] = 0;
	}

	// Creates the unix socket to communicate with local apps
	connect_unix();

	// Create socket inet
	// To communicate with down stages 
	connect_inet(portDown);

	thread_info_struct *threadInfo = NULL;
	
	// Creates a thread to comunnicate with clipboard up on the tree
	if(modeOfFunction == ONLINE) {
		connect_inetIP(portUp, ip);

		// Receives the backup from the other clipboard
		backupCopy();

		printf("Received backup\n");

		threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
		if(threadInfo == NULL) {
			perror("malloc");
			exit(0);
		}

		threadInfo->inputArgument = sock_fd_inetIP;
		pthread_create(&threadInfo->thread_id, NULL, &upThread, threadInfo);
		printf("Thread created to handle clipboards up on the tree - ID %lu\n",  threadInfo->thread_id);
	}

	printf("signal 1\n");

	// Creates a thread to comunnicate with clipboards down on the tree
	threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
	if(threadInfo == NULL) {
		perror("malloc");
		exit(0);
	}

	threadInfo->inputArgument = sock_fd_inet;
	pthread_create(&threadInfo->thread_id, NULL, &downThread, threadInfo);
	printf("Thread created to handle clipboards down on the tree - ID %lu\n",  threadInfo->thread_id);


	printf("Ready to accept clients\n");

	while(1){
		printf(". main\n");

		// Reset hold variable
		socklen_t size_addr = sizeof(struct sockaddr);

		// Accept client to communicate
		int client =  accept(sock_fd_unix, (struct sockaddr *) &client_addr_un, &size_addr);
		printf("client %d\n", client);
		if(client == -1) {
			perror("accept");
			exit(-1);
		}

		// Creates a structure to hold the information about the thread and the input argument
		threadInfo = (thread_info_struct *)malloc(sizeof(thread_info_struct));
		if(threadInfo == NULL) {
			perror("malloc");
			exit(0);
		}

		threadInfo->inputArgument = client;

		printf("Accepted connection\n");
		// Creates new thread to handle the comunicatuion with the client
		pthread_create(&threadInfo->thread_id, NULL, &clientThread, threadInfo);
		printf("Thread created AT - ID %lu\n",  threadInfo->thread_id);

	}
	return 0;
}
