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
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

#include "ArgParseStandalone.h"
#include "config.h"

bool socket_connect(int* sock, const char* host, const in_port_t port) {
	struct addrinfo hints;
	struct addrinfo* servinfo;
	struct addrinfo* p;
	int rv;

	char port_num[32];
	sprintf(port_num, "%d", port);
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(host, port_num, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return false;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((*sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (connect(*sock, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			close(*sock);
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Failed to connect.\n");
		return false;
	}

	freeaddrinfo(servinfo);
	return true;
}

//This function reboots the machine forcefully.
void reboot() {
	printf("REBOOT Triggered!!\n");
	const char* kernel_sysrq = "/proc/sys/kernel/sysrq";
	const char* kernel_sysrq_trigger = "/proc/sysrq-trigger";

	if(access(kernel_sysrq_trigger, F_OK) == -1) {
		//Activate Magic SysRq Option
		int fd = open(kernel_sysrq, O_WRONLY);
		write(fd, "1", 1);
		close(fd);
		//Sleep for a sec to allow the trigger to appear.
		sleep(1);
	}

	// Wait!!! Give the server a chance to delete the file.
	sleep(10);

	//Reboot!
	int fd = open(kernel_sysrq_trigger, O_WRONLY);
	write(fd, "b", 1);
	close(fd);
	return;
}


void read_configuration_file(std::string& read_host, int& read_port, int& serve_port, std::string& serve_file, const std::string& filepath) {
	const int max_length = 1024;
	char value_str[max_length];
	int value;

	//Don't do anything if can't access
	if(access(filepath.c_str(), F_OK) == -1) {
		return;
	}

	//Open file
	FILE* fconf = fopen(filepath.c_str(), "r");
	if (fconf == 0) {
		return;
	}

	//Can easily fail if not in just the right format.
	fscanf(fconf, "read-host %s\n", value_str);
	read_host = value_str;
	fscanf(fconf, "read-port %d\n", &value);
	read_port = value;
	fscanf(fconf, "serve-port %d\n", &value);
	serve_port = value;
	fscanf(fconf, "serve-file %s\n", value_str);
	serve_file = value_str;
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

	std::string host_input = "localhost";
	std::string host = "localhost";
	bool host_passed = false;
	int port_input = 9090;
	int port = 9090;
	bool port_passed = false;
	bool debug = false;

	std::string config_path = MINI_NETREBOOT_INSTALL_PREFIX "/etc/mini-netreboot/mini-netreboot.conf";

	ArgParse::ArgParser Parser("mini-netreboot-read");
	Parser.AddArgument("-c/--conf", "Config file to use", &config_path, ArgParse::Argument::Optional);
	Parser.AddArgument("--host", "Host to connect to", &host_input, ArgParse::Argument::Optional, &host_passed);
	Parser.AddArgument("-p/--port", "Port to connect to", &port_input,  ArgParse::Argument::Optional,&port_passed);
	Parser.AddArgument("-d/--debug", "Report whats going on", &debug, ArgParse::Argument::Optional);

	if(Parser.ParseArgs(argc, argv) < 0) {
		printf("There was a problem parsing args!");
		return -1;
	}

	if(Parser.HelpPrinted()) {
		return 0;
	}

	// Load config file
	if(debug) {
		printf("Attempting to read config file %s\n", config_path.c_str());
	}

	std::string read_host = "localhost";
	int read_port = 9090;
	int serve_port = 9090;
	std::string serve_file = "/reboot.txt";

	read_configuration_file(read_host, read_port, serve_port, serve_file, config_path);

	if (host_passed) {
		host = host_input;
	} else {
		host = read_host;
	}

	if (port_passed) {
		port = port_input;
	} else {
		port = read_port;
	}

	// Report configuration
	printf("Reading from %s:%i\n", host.c_str(), port);

	while (true) {
		//Connect to specified host/port
		if(!socket_connect(&sockfd, host.c_str(), port)) {
			perror("connect: ");
			return -2;
		}
	
		//Receive message
		const int message_length = 1024;
		char message_buffer[message_length];
		size_t recv_size = recv(sockfd, (void*) message_buffer, message_length, 0);
		if(recv_size < 0) {
			perror("recv: ");
			return -3;
		}
	
		if(recv_size == 0) {
			//Go to next iteration
			sleep(30);
			continue;
		}
	
		if (debug) {
			printf("Received: %s\n", message_buffer);
		}

		std::string message(message_buffer);
		//Trim the message of whitespace
		rtrim(message);
		if(message == "REBOOT") {
			printf("Rebooting!\n");
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
