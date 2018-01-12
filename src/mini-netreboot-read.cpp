#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <algorithm>

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

//This function reboots the machine forcefully.
void reboot() {
	printf("REBOOT Triggered!!\n");
	const char* kernel_sysrq = "/proc/sys/kernel/sysrq";
	const char* kernel_sysrq_trigger = "/proc/sysrq-trigger";
	if(access(kernel_sysrq_trigger, F_OK) == -1) {
		int fd = open(kernel_sysrq, O_WRONLY);
		//Activate Magic SysRq Option
		write(fd, "1\n", 2);
		close(fd);
		//Sleep for a sec to allow the trigger to appear.
		sleep(1);
	}

	//Reboot!
	int fd = open(kernel_sysrq_trigger, O_WRONLY);
	write(fd, "b\n", 2);
	close(fd);
	return;
}

static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

int sockfd = -1;
void terminate(int signum) {
	printf("Gracefully closing..\n");
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
	bool debug = false;

	ArgParse::ArgParser Parser("mini-netreboot-read");
	Parser.AddArgument("-h/--host", "Host to connect to", &host);
	Parser.AddArgument("-p/--port", "Port to connect to", &port);
	Parser.AddArgument("-d/--debug", "Report whats going on", &debug);

	if(Parser.ParseArgs(argc, argv) < 0) {
		printf("There was a problem parsing args!");
		return -1;
	}

	if(Parser.HelpPrinted()) {
		return 0;
	}

	while (true) {
		//Connect to specified host/port
		if(!socket_connect(&sockfd, host.c_str(), port)) {
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
	
		if (debug) {
			printf("%s\n", message_buffer);
		}

		std::string message(message_buffer);
		//Trim the message of whitespace
		rtrim(message);
		if(message == "REBOOT") {
			reboot();
		}

		close(sockfd);
		//Reset socket after closing
		sockfd = -1;
		//Sleep for 30 seconds
		sleep(30);
	}

	return 0;
}
