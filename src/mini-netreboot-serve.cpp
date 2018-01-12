#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include "ArgParse/ArgParse.h"

bool socket_bind(int* sock, const char* host, const in_port_t port) {
	struct hostent *hp;
	struct sockaddr_in addr;

	if((hp = gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		return false;
	}

	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	if(((*sock) = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket");
		return false;
	}

	if(bind((*sock), (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) != 0) {
		perror("bind");
		return false;
	}
	return true;
}

void fetch_message(char* message, const int message_length, const std::string& filepath) {
	//Check if file exists
	if(access(filepath.c_str(), F_OK) == -1) {
		//Return Default message if it doesn't
		sprintf(message, "NONE");
		return;
	}
	//Open file
	int fd = open(filepath.c_str(), O_RDONLY);
	if (fd < 0) {
		//Return default message
		sprintf(message, "NONE");
		return;
	}
	//Read message from file
	if(read(fd, message, message_length)< 0) {
		//Return default message
		sprintf(message, "NONE");
		close(fd);
		return;
	}
	//Successfully got message, close
	close(fd);
	return;
}

void reset_message(const std::string& filepath) {
	//Check if file exists
	if(access(filepath.c_str(), F_OK) == -1) {
		//Don't do anything
		return;
	}
	//Open file
	int fd = open(filepath.c_str(), O_WRONLY);
	if (fd < 0) {
		//Don't do anything
		return;
	}

	std::string message = "NORMAL";
	//Doesn't matter what was written really.
	write(fd, message.c_str(), message.size());

	//Close file and exit
	close(fd);
	return;
}

int sockfd = -1;
void terminate(int signum) {
	printf("Gracefully closing..\n");
	fflush(stdout);
	if (sockfd != -1) {
		close(sockfd);
	}
	exit(0);
}

int main(int argc, char** argv) {
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = terminate;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	std::string host = "localhost";
	int port = 9090;
	std::string filepath = "/reboot.txt";
	bool debug = false;

	ArgParse::ArgParser Parser("mini-netreboot-serve");
	Parser.AddArgument("-h/--host", "Host to listen on", &host);
	Parser.AddArgument("-p/--port", "Port to listen on", &port);
	Parser.AddArgument("-f/--file", "File containing message to send", &filepath);
	Parser.AddArgument("-d/--debug", "Report whats going on", &debug);

	if(Parser.ParseArgs(argc, argv) < 0) {
		printf("There was a problem parsing args!");
		return -1;
	}

	if(Parser.HelpPrinted()) {
		return 0;
	}

	if(!socket_bind(&sockfd, host.c_str(), port)) {
		printf("1 1\n");
		return -2;
	}

	// Listen to the port
	if (listen(sockfd, 20) != 0) {
		perror("listen");
		return -3;
	}

	const int message_length = 1024;
	char message[message_length]; 

	while (true) {
		int clientfd;
		struct sockaddr_in client_addr;
		unsigned int addrlen=sizeof(client_addr);

		//Accept a client connection
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		if (debug) {
			printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		}

		//Fetch message from disk if it exists.
		fetch_message(message, message_length, filepath);

		if (debug) {
			printf("Sending: %s\n", message);
		}

		//Send the client the message
		send(clientfd, message, message_length, 0);

		reset_message(filepath);

		close(clientfd);
	}

	close(sockfd);
	return 0;
}
