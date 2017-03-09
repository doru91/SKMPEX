#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <linux/tcp.h>

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    int cnt = 10000;
    struct mptcp_sub_ids *ids;
    int i, optlen, flags = 0;
    char *p;
    char sendBuff[1025];

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
 
	optlen = 32;
	ids = malloc(optlen);

	getsockopt(connfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
	printf("\nsubflow_count = %i\n", ids->sub_count);	
	for (i = 0; i < ids->sub_count; i++) {
		printf("send data on subflow_id = %i\n", ids->sub_status[i].id);
		flags = 0;
		p = &flags;
		p[2] |= (ids->sub_status[i].id);
		while (cnt--) {
			snprintf(sendBuff, sizeof(sendBuff),
			"Data from subflow id: %i, iteration: %i\n",
			ids->sub_status[i].id, cnt);
			send(connfd, sendBuff, strlen(sendBuff), flags);
		}
		cnt = 10000;
	}
	
	/* additional MPTCP subflows will be added after the first one
	 * is fully established. First subflow is fully-established
	 * after we send one window at data (see RFC6824 for details)
	 */
	optlen = 32;
	getsockopt(connfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
	printf("\nsubflow_count = %i\n", ids->sub_count);
	//sleep(5);
	for (i = 0; i < ids->sub_count; i++) {
		printf("send data on subflow_id = %i\n", ids->sub_status[i].id);
		flags = 0;
		p = &flags;
		p[2] |= (ids->sub_status[i].id);
		while (cnt--) {
 			snprintf(sendBuff, sizeof(sendBuff),
			"Data from subflow id: %i, iteration: %i\n",
			ids->sub_status[i].id, cnt);
			send(connfd, sendBuff, strlen(sendBuff), flags);
		}
		cnt = 10000;
	}
	
        close(connfd);
        sleep(1);
    }
}

