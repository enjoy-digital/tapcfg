
#include <stdlib.h>
#include <stdio.h>

#include "tapserver.h"

int main() {
	tapcfg_t *tapcfg;
	tapserver_t *server;
	char buffer[256];
	int id;

#ifdef _WIN32
#define sleep(x) Sleep((x)*1000)

	WORD wVersionRequested;
	WSADATA wsaData;
	int ret;

	wVersionRequested = MAKEWORD(2, 2);

	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret) {
		/* Couldn't find WinSock DLL */
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
	    HIBYTE(wsaData.wVersion) != 2) {
		/* Version mismatch, requested version not found */
		return -1;
	}
#endif

	tapcfg = tapcfg_init();
	if (!tapcfg)
		return -1;
	if (tapcfg_start(tapcfg, NULL)) {
		tapcfg_destroy(tapcfg);
		return -1;
	}

	srand(time(NULL));
	id = rand()%0x1000;

	sprintf(buffer, "10.10.%d.%d", (id>>8)&0xff, id&0xff);
	printf("Selected IPv4 address: %s\n", buffer);
	tapcfg_iface_set_ipv4(tapcfg, buffer, 16);

	sprintf(buffer, "fc00::%x", id&0xffff);
	printf("Selected IPv6 address: %s\n", buffer);
	tapcfg_iface_add_ipv6(tapcfg, buffer, 64);

	server = tapserver_init(tapcfg, 1000);

	tapcfg_iface_change_status(tapcfg, 1);
	tapserver_start(server);
	while (1) {
		sleep(10);
	}

	tapserver_stop(server);
	tapserver_destroy(server);
	tapcfg_destroy(tapcfg);

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}

