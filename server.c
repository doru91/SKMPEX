#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <linux/tcp.h>

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    int cnt = 10000;
    struct mptcp_sub_ids *ids;
    int i, optlen;

    char sendBuff[1025];
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        ticks = time(NULL);
	
        while (cnt--) {
            snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
            send(connfd, sendBuff, strlen(sendBuff), 0x00010000);
        }

	cnt = 10000;

	while (cnt--) {
            snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
            send(connfd, sendBuff, strlen(sendBuff), 0x00020000);
        }

	optlen = 32;
	ids = malloc(optlen);

	getsockopt(connfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
	printf("\nsubflow_count = %i\n\n", ids->sub_count);
	
	for (i = 0; i < ids->sub_count; i++) {
		printf("subflow_id = %i\n", ids->sub_status[i].id);
		printf("fully_established: %i\n\n", ids->sub_status[i].fully_established);
	}

	
        close(connfd);
        sleep(1);
    }
}

