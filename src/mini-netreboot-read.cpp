#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "ArgParse/ArgParse.h"

bool socket_connect(int* sock, const char* host, const in_port_t port) {
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

	if(connect((*sock), (struct sockaddr*) &addr, sizeof(struct sockaddr_in)) != 0) {
		perror("connect");
		return false;
	}
	return true;
}

int sockfd = -1;
void terminate(int signum) {
	printf("Gracefully closing..");
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

	std::string address = "localhost";
	int port = 9090;

	ArgParse::ArgParser Parser("mini-netreboot-read");
	Parser.AddArgument("-h/--host", "Host to connect to", &host);
	Parser.AddArgument("-p/--port", "Port to connect to", &port);

	if(Parser.ParseArgs(argc, argv) < 0) {
		printf("There was a problem parsing args!");
		return -1;
	}

	if(Parser.HelpPrinted()) {
		return 0;
	}

	while (true) {
		//Connect to specified host/port
		if(!socket_connect(&sockfd, address.c_str(), port)) {
			return -2;
		}
	
		//Receive message
		const int message_length = 1024;
		char message_buffer[message_length];
		size_t recv_size = recv(sockfd, (void*) message_buffer, message_length, 0);
		if(recv_size < 0) {
			perror("recv");
			return -3;
		}
	
		if(recv_size == 0) {
			//Go to next iteration
			sleep(30);
			continue;
		}
	
		std::string message(message_buffer);
		if(message == "REBOOT") {
			printf("Received the reboot signal!!!");
		}
	
		close(sockfd);
		//Reset socket after closing
		sockfd = -1;
		//Sleep for 30 seconds
		sleep(30);
	}

	return 0;
}