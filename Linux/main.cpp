#include <unistd.h>
#include <sys/epoll.h> 
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <vector>
#define MAX_CONNECTION_COUNT 1024
#define BUF_SIZE 1024

// 임시 소켓 저장
int socket_table[MAX_CONNECTION_COUNT];
struct sockaddr_in socket_addr_table[MAX_CONNECTION_COUNT];
struct epoll_event epoll_event_table[MAX_CONNECTION_COUNT];
struct epoll_event p_epoll_event[MAX_CONNECTION_COUNT];
char buf[BUF_SIZE];
int epoll_fd;

int	setnonblocking(int sockfd)
{
	int opts;
	opts = fcntl(sockfd, F_GETFL);
	if (opts < 0) return -1;
	opts = (opts | O_NONBLOCK);
	if (fcntl(sockfd, F_SETFL, opts) < 0) return -1;

	return 0;
}


void* work_routine(void* arg)
{
	while (true)
	{
		
		int completion_num = epoll_wait(epoll_fd, p_epoll_event, MAX_CONNECTION_COUNT, -1);
		if (completion_num < 0)
		{
			perror("epoll_wait");
			continue;
		}
		else
		{
			// notified fd
			for (int iter = 0; iter < completion_num; ++iter)
			{

				if (p_epoll_event[iter].events & EPOLLIN)
				{
					int buffered_len = recv(p_epoll_event[iter].data.fd, buf, BUF_SIZE, 0);
					if (buffered_len == 0)
					{
						// 연결 해제
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, p_epoll_event[iter].data.fd, NULL) < 0)
						{
							printf("epoll del error\n");
						}
						close(p_epoll_event[iter].data.fd);
						// 리소스 해제

					}
					else
					{
						printf("%s\n", buf);
					}
				}
			}
		}

	}
}


int main()
{
	struct sockaddr_in mSockAddr;
	memset(&mSockAddr, 0, sizeof(struct sockaddr_in));
	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mSockAddr.sin_port = htons(7777);

	// Specifying a protocol of 0 causes socket() to use an unspecified default protocol appropriate for the requested socket type.
	int enter_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (enter_socket_fd < 0)
	{
		printf("enter socket error\n");
		return 0;
	}
	int result = 0;

	result = bind(enter_socket_fd, (struct sockaddr*)&mSockAddr, sizeof(struct sockaddr_in));
	if (result < 0)
	{
		printf("bind failed\n");
		return 0;
	}

	result = listen(enter_socket_fd, 10);
	if (result < 0)
	{
		printf("listen failed\n");
		return 0;
	}
	// int epoll_create(int size);
	// int epoll_create1(int flags);
	// epoll 생성
	epoll_fd = epoll_create1(0);
	if (epoll_fd < 0)
	{
		printf("epoll initialization failed\n");
		return 0;
	}

	//	epoll_event 구조체
	//		typedef union epoll_data {
	//		void* ptr;
	//		int fd;
	//		__uint32_t u32;
	//		__uint64_t u64;
	//	} epoll_data_t;
	//	struct epoll_event {
	//		__uint32_t events; /* Epoll events */
	//		epoll_data_t data; /* User data variable */
	//	};



	//	int EpollAdd(const int fd)
	//	{
	//		struct epoll_event ev;
	//		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR;
	//		ev.data.fd = fd;
	//		return epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &ev);
	//	}

	// 스레드 생성
	pthread_t thread;
	pthread_create(&thread, NULL, work_routine, NULL);
	
	// 접속 받음
	int acc_connection_cnt = 0;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	while (true)
	{
		int client_fd = accept(enter_socket_fd,(struct sockaddr*)&socket_addr_table[acc_connection_cnt], &addr_len);
		if (client_fd < 0)
		{
			printf("client connetion is failed\n");
			continue;
		}
		else
		{
			printf("client connetion is succeeded\n");
			// 클라이언트 정보 저장
			if (setnonblocking(client_fd) < 0)
			{
				printf("making nonblocking socket is failed\n");
				close(client_fd);
				continue;
			}
			socket_table[acc_connection_cnt] = client_fd;
			// epoll 등록
			epoll_event_table[acc_connection_cnt].events = EPOLLIN | EPOLLOUT | EPOLLET;
			epoll_event_table[acc_connection_cnt].data.fd = client_fd;
			
			if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &epoll_event_table[acc_connection_cnt]) < 0)
			{
				// epoll 등록 실패
				printf("epoll add is faild\n");
				close(client_fd);
				continue;
			}
			// recv(client_fd, buf, BUF_SIZE, 0);
			// 다음
			++acc_connection_cnt;
		}
	}
	
	//ssize_t recv_len = 0;
	//char buf[1024];
	//int cnt = 10;
	//while (cnt--)
	//{
	//	memset(buf, 0, sizeof(buf));
	//	recv_len = recv(client_fd, buf, 1024, 0);
	//	if (recv_len > 0)
	//	{
	//		printf("%s\n", buf);
	//	}
	//}
	close(epoll_fd);
	return 0;
}