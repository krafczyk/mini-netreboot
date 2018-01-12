#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

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

	std::string host = "localhost";
	int port = 9090;
	std::string filepath = "/reboot.txt"

	ArgParse::ArgParser Parser("mini-netreboot-serve");
	Parser.AddArgument("-h/--host", "Host to listen on", &host);
	Parser.AddArgument("-p/--port", "Port to listen on", &port);
	Parser.AddArgument("-f/--file", "File containing message to send", &filepath)

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

	std::string message = "Hello!";

	while (true) {
		int clientfd;
		struct sockaddr_in client_addr;
		unsigned int addrlen=sizeof(client_addr);

		//Accept a client connection
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		//printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		//Send the client the message
		send(clientfd, message.c_str(), message.size(), 0);

		close(clientfd);
	}

	printf("4\n");

	close(sockfd);

	return 0;
}
