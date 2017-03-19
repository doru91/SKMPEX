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

/* Server program for MPTPCP API demo
 * 
 * API usage 
 * - specify the path index of the desired subflow to send data
 * using third byte of the 'flags' parameter from the 'send' call
 * - get path index of the desired sublfow using getsockopt
 * with MPTCP_GET_SUB_IDS option
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
 * - server sends 'cnt' buffers on the first subflow containing
 * the string 'Data from subflow id: 1'
 * - second subflow is established
 * - server sends 'cnt' buffers on the second subflow containing
 * the string 'Data from subflow id: 2'
 * - server sends 'cnt' buffers on both subflows (default lowest-RTT
 * scheduler behaviour) containing the string 'Data from random
 * subflow'
 */

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct mptcp_sub_ids *ids;
    int i, optlen, cnt, flags = 0;
    char sendBuff[1025];
    int used_path_index = -1;
    int used_both_subflows = 0;
    char *p;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, 10);
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
 
    optlen = 32;
    ids = malloc(optlen);

send_data:
    /* get subflow path index set */
    optlen = 32;
    getsockopt(connfd, IPPROTO_TCP, MPTCP_GET_SUB_IDS, ids, &optlen);
    printf("\nsubflow_count = %i\n", ids->sub_count);

    for (i = 0; i < ids->sub_count; i++) {
	/* have we already send data on this subflow? */
        if (ids->sub_status[i].id == used_path_index) {
	    used_both_subflows = 1;
	    continue;
	}
	printf("send data on subflow_id = %i\n", ids->sub_status[i].id);
	/* remember that we sent data on this subflow */
	if (used_path_index == -1)
		used_path_index = ids->sub_status[i].id;
	
	/* specify a subflow path index using the third byte of the
	 * 'flags' parameter from the 'send' call. 
	 */
	flags = 0;
	p = &flags;
	p[2] |= (ids->sub_status[i].id);

	cnt = 1000;
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
     * after we send one window of data (see RFC6824 for details
     */
    if (!used_both_subflows)
    	goto send_data;

    printf("send data on random subflow\n");
    flags = 0;
    while (cnt--) {
	snprintf(sendBuff, sizeof(sendBuff),
		"Data from random subflow, iteration: %i\n", cnt);
		send(connfd, sendBuff, strlen(sendBuff), flags);
    }

    close(connfd);
}
