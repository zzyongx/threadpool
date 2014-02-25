#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <memory>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "threadpool.h"

class EchoTask : public Task {
public:
	EchoTask(int fd) : fd_(fd) { }
	~EchoTask();
	int getFd() const { return fd_; };
	bool doit();
private:
	int fd_;
};

EchoTask::~EchoTask() {
	close(fd_);
}

bool EchoTask::doit() {

	int n;
	char buf[128] = {0};
	std::string s;

	while ((n = recv(fd_, buf, 127, 0)) > 0) {
		s.append(buf, n);
		if (s.size() >= 2 && s.compare(s.size()-2, 2, "\r\n") == 0) {
			break;
		}
	}
	if (n < 0 || s.size() < 2) {
		std::cerr << "error in recv\t" << errno << "\t" << strerror(errno) << "\n";
		return false;
	}

	std::cout << s << std::endl;

	std::transform(s.begin(), s.end() - 2, s.begin(), toupper);

	size_t hn = 0;
	while (hn != s.size() && (n = send(fd_, s.c_str() + hn, s.size() - hn, 0)) > 0) {
		hn += n;
	}
	if (n < 0) {
		std::cerr << "error in send\n";
	}

	return true;
}

void onfinished(Task *task) {
	delete task;
}

static int init_tcp_socket(uint16_t port);

int main(int argc, char *argv[]) {

	if (argc != 2) {
		std::cerr << "echo_server port\n";
		return EXIT_FAILURE;
	}
	
	int sfd = init_tcp_socket(atoi(argv[1]));
	if (sfd < 0) {
		return EXIT_FAILURE;
	}

	ThreadPool tpool(100);

	while (true) {
		struct sockaddr_in  cliaddr;
		socklen_t slen = sizeof(cliaddr);

		int cfd = accept(sfd, (struct sockaddr *)(&cliaddr), &slen);
		if (cfd > 0) {
			std::auto_ptr<EchoTask> echo(new EchoTask(cfd));
			if (tpool.submit(echo.get(), onfinished)) {
				echo.release();
			} else {
				std::cerr << "can't submit" << std::endl;
			}
		}
	}

	return EXIT_SUCCESS;
}

static int init_tcp_socket(uint16_t port) {

	int     tcpfd;
	struct sockaddr_in  servaddr;
	int32_t         flag;
	int             retval;

	tcpfd = socket(AF_INET, SOCK_STREAM, 0); 

	if (tcpfd < 0) {
		std::cerr << "socket create error:" << errno << "\t" << strerror(errno) << std::endl;
		return tcpfd;
	} else {
		std::cerr << "listening socket is created" << std::endl;
	}   

	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family         = AF_INET;
	servaddr.sin_addr.s_addr    = htonl(INADDR_ANY);
	servaddr.sin_port           = htons(port);

	flag = 1;
	retval = setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if (retval < 0) {
		std::cerr << "setsockopt error:" << errno << "\t" << strerror(errno) << std::endl;
		close(tcpfd);
		return (-1);
	}   

	retval = bind(tcpfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (retval < 0) {
		std::cerr << "bind error:" << errno << "\t" << strerror(errno) << std::endl;
		close(tcpfd);
		return (-1);
	}   

	retval = listen(tcpfd, 1024);
	if (retval < 0) {
		std::cerr << "listen error:" << errno << "\t" << strerror(errno) << std::endl;
		close(tcpfd);
		return (-1);
	} else {
		std::cerr << "start listening" << std::endl;
	}   

	return tcpfd;
}
