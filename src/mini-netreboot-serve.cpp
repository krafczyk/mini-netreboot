#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

#include "ArgParseStandalone.h"
#include "config.h"

bool socket_bind(int* sock, const in_port_t port) {
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

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
	write(fd, message.c_str(), 6);

	//Close file and exit
	close(fd);
	return;
}

void read_configuration_file(std::string& read_host, int& port, std::string& serve_file, const std::string& filepath) {
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
	fscanf(fconf, "port %d\n", &value);
	port = value;
	fscanf(fconf, "serve-file %s\n", value_str);
	serve_file = value_str;
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

	std::string config_path = MINI_NETREBOOT_INSTALL_PREFIX "/etc/mini-netreboot/mini-netreboot.conf";
	bool debug = false;

	int port = 9090;
	int port_input = 9090;
	bool port_passed = false;
	std::string serve_file_input = "/reboot.txt";
	bool serve_file_passed = false;

	ArgParse::ArgParser Parser("mini-netreboot-serve");
	Parser.AddArgument("-c/--conf", "Filepath of config file", &config_path, ArgParse::Argument::Optional);
	Parser.AddArgument("-p/--port", "Port to use", &port_input, ArgParse::Argument::Optional, &port_passed);
	Parser.AddArgument("-s/--serve-file", "Serve file path", &serve_file_input, ArgParse::Argument::Optional, &serve_file_passed);
	Parser.AddArgument("-d/--debug", "Report whats going on", &debug);

	if(Parser.ParseArgs(argc, argv) < 0) {
		printf("There was a problem parsing args!");
		return -1;
	}

	if(Parser.HelpPrinted()) {
		return 0;
	}

	std::string read_host = "localhost";
	std::string serve_file = "/reboot.txt";

	read_configuration_file(read_host, port, serve_file, config_path);

	if (port_passed) {
		port = port_input;
	}

	if (serve_file_passed) {
		serve_file = serve_file_input;
	}

	// Report configuration
	printf("Serving %s on port %i\n", serve_file.c_str(), port);

	if(!socket_bind(&sockfd, port)) {
		perror("bind: ");
		return -2;
	}

	const int message_length = 1024;
	char message[message_length]; 

	while (true) {
		// Listen to the port
		if (listen(sockfd, 20) != 0) {
			perror("listen: ");
			return -3;
		}

		int clientfd;
		struct sockaddr_in client_addr;
		unsigned int addrlen=sizeof(client_addr);

		//Accept a client connection
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		if (debug) {
			printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		}

		//Fetch message from disk if it exists.
		fetch_message(message, message_length, serve_file);

		if (debug) {
			printf("Sending: %s\n", message);
		}

		//Reset the message before sending!
		reset_message(serve_file);

		//Send the client the message
		send(clientfd, message, message_length, 0);

		close(clientfd);
	}

	close(sockfd);
	return 0;
}
