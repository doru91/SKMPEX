#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

/* Client program for MPTPCP API demo
 *
 * API usage
 * - specify the path index of the desired subflow to receive data
 * using third byte of the 'flags' parameter from the 'recv' call
 * - if third byte of the 'flags' parameter is 0 then the default
 * behaviour of the lowest-RTT scheduler is kept and no subflow
 * preferance is used
 *
 * Testbed: http://multipath-tcp.org/pmwiki.php/Users/UML
 * This program should be run on the UML Server using the
 * full-mesh path manager and the lowest-RTT packet
 * scheduler
 *
 * Two subflows are established between the server and client:
 * 1) Server (eth0: 10.2.1.2) <---> Client (eth0: 10.1.1.2)
 * 2) Server (eth0: 10.2.1.2) <---> Client (eth1: 10.1.2.2)
 *
 * Flow of data between the server and the client:
 * - first subflow is established (Server,eth0 -> Client,eth0)
 * - client received 1000 buffers on the first subflow containing
 * the string 'Data from subflow id: 1'
 * - second subflow is established
 * - client receives 1000 buffers on the second subflow containing
 * the string 'Data from subflow id: 2'
 * - client receives 'cnt' buffers on both subflows (default lowest-RTT
 * scheduler behaviour) containing the string 'Data from random
 * subflow'
 */

#define RANDOM_PATH_INDEX 0
#define SUBFLOW1_PATH_INDEX 0x00010000
#define SUBFLOW2_PATH_INDEX 0x00020000

int main(int argc, char *argv[])
{
	int sockfd = 0, n = 0;
	char recvBuff[1024];
	struct sockaddr_in serv_addr;
	int i, ids, optlen, flags, iteration = 0;

	if (argc != 2) {
		printf("usage: %s <ip of server>\n",argv[0]);
		return 1;
	}

	memset(recvBuff, '0',sizeof(recvBuff));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Error : Could not create socket \n");
		return 1;
	}
	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5000);
	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <=0) {
		printf("\n inet_pton error occured\n");
		return 1;
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\n error: connect failed \n");
		return 1;
	}

	flags = SUBFLOW1_PATH_INDEX;

receive_data:
	while ((n = recv(sockfd, recvBuff, sizeof(recvBuff)-1, flags)) > 0) {
		recvBuff[n] = 0;
		if(fputs(recvBuff, stdout) == EOF) {
			printf("fputs error\n");
		}
	}

	if (n < 0) {
		printf("reading data error\n");
	}

	if (errno) {
		printf("errno is set: %i\n", errno);
		/* 3 receive iterations (data from subflow1,
		 * then subflow2, then random subflow)
		 */
		if (++iteration == 3) {
			close(sockfd);
			return 0;
		}

		if (flags == SUBFLOW1_PATH_INDEX) {
			flags = SUBFLOW2_PATH_INDEX;
			goto receive_data;
		}
		else if (flags == SUBFLOW2_PATH_INDEX) {
			flags = RANDOM_PATH_INDEX;
			goto receive_data;
		}
	}

	close(sockfd);
	return 0;
}
