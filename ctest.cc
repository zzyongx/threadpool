#include <iostream>
#include <memory>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "threadpool.h"

class EchoTask : public Task {
public:
	EchoTask(const char *ip, uint16_t port);
	~EchoTask();
	bool doit();

private:
	int fd_;
};

EchoTask::EchoTask(const char *ip, uint16_t port) {

	fd_ = socket(AF_INET, SOCK_STREAM, 0); 

	if (fd_ < 0) {
		std::cerr << "socket create error:" << errno << "\t" << strerror(errno) << std::endl;
		throw int(errno);
	}

	struct sockaddr_in  servaddr;
	memset(&servaddr, 0x00, sizeof(servaddr));
	servaddr.sin_family         = AF_INET;
	servaddr.sin_port           = htons(port);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

	if (connect(fd_, (struct sockaddr*)(&servaddr), sizeof(servaddr)) != 0) {
		close(fd_);
		throw int(errno);
	}
}

EchoTask::~EchoTask() {
	if (fd_ > 0) {
		close(fd_);
	}
}

bool EchoTask::doit() {

	size_t hn = 0;
	ssize_t n = 0;

	char buf[128];
	// std::string s('a', 20);
	std::string s(20, 'a');
	s.append("\r\n");

	while (hn != s.size() && (n = send(fd_, s.c_str() + hn, s.size() - hn, 0)) > 0) {
		hn += n;
	}
	if (n < 0) {
		std::cerr << "error in send\n";
		return false;
	}

	s.clear();
	while ((n = recv(fd_, buf, 127, 0)) > 0) {
		s.append(buf, n);
	}

	if (n < 0) {
		std::cerr << "error in recv\n";
	}

	std::cout << s << std::endl;

	return true;
}

void onfinished(Task *task) {
	delete task;
}

int main(int argc, char *argv[]) {

	if (argc != 3) {
		std::cerr << "echo_client ip port\n";
		return EXIT_FAILURE;
	}

	int maxfd = 10240;
	setMaxfd(&maxfd);
	
	ThreadPool tpool(50);

	while (true) {

		try {

			std::auto_ptr<EchoTask> echo(new EchoTask(argv[1], atoi(argv[2])));
			// echo->doit();
			
			if (tpool.submit(echo.get(), onfinished)) {
				echo.release();
			} else {
				std::cerr << "can't submit" << std::endl;
			}
			
		} catch (int e) {
			std::cerr << e << "\t" << strerror(e) << std::endl;
		}

	}

	// wait for task to be finished
	do {
		sleep(1);
		std::cout << "s\n";
	} while (tpool.taskNum() != 0);

	return EXIT_SUCCESS;
}
