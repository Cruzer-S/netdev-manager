#include "netdev-manager.h"

#include <stdio.h>	// BUFSIZ
#include <stdbool.h>	// false
#include <stdlib.h>	// malloc(), free()
#include <string.h>	// strerror()
#include <errno.h>	// errno

#include <ynl/netdev-user.h>	// struct netdev_bind_rx_req, ...
#include <ynl/ynl.h>		// struct ynl_error

#define ERROR(...)	do {				\
	snprintf(error, BUFSIZ, __VA_ARGS__);		\
} while(false)

#define YNL_ERROR_STR_LEN	512

static char error[BUFSIZ + YNL_ERROR_STR_LEN];

struct netdev_manager {
	struct ynl_sock *ys;
	struct ynl_error yerr;

	int dmabuf_id;
};

const char *ndevmgr_get_error(void)
{
	return error;
}

NetdevManager ndevmgr_create(void)
{
	NetdevManager ndevmgr;

	ndevmgr = malloc(sizeof(struct netdev_manager));
	if (ndevmgr == NULL) {
		ERROR("failed to malloc(): %s", strerror(errno));
		return NULL;
	}

	ndevmgr->yerr.code = YNL_ERROR_NONE;

	return ndevmgr;
}

int ndevmgr_bind_rx_queue(NetdevManager ndevmgr,
			  int ifindex, int queue_idx, int num_queue,
			  int dmabuf_fd)
{
	struct netdev_bind_rx_req *req = NULL;
	struct netdev_bind_rx_rsp *rsp = NULL;
	struct netdev_queue_id *queues;

	ndevmgr->ys = ynl_sock_create(&ynl_netdev_family, &ndevmgr->yerr);
	if (ndevmgr->ys == NULL) {
		ERROR("failed to ynl_sock_create(): %s", ndevmgr->yerr.msg);
		goto RETURN_ERROR;
	}

	queues = malloc(sizeof(struct netdev_queue_id) * num_queue);
	if (queues == NULL) {
		ERROR("failed to malloc(): %s", strerror(errno));
		goto YNL_SOCK_DESTROY;
	}

	for (int i = 0; i < num_queue; i++) {
		queues[i]._present.type = 1;
		queues[i]._present.id = 1;
		queues[i].type = NETDEV_QUEUE_TYPE_RX;
		queues[i].id = queue_idx + i;
	}

	req = netdev_bind_rx_req_alloc();
	if (req == NULL) {
		ERROR("failed to netdev_bind_rx_req_alloc(): %s",
		      strerror(errno));
		goto FREE_QUEUES;
	}

	netdev_bind_rx_req_set_ifindex(req, ifindex);
	netdev_bind_rx_req_set_fd(req, dmabuf_fd);
	__netdev_bind_rx_req_set_queues(req, queues, num_queue);

	// __netdev_bind_rx_req_set_queues() will free `queues`
	queues = NULL;

	rsp = netdev_bind_rx(ndevmgr->ys, req);
	if (rsp == NULL) {
		ERROR("failed to netdev_bind_rx(): %s", ndevmgr->yerr.msg);
		goto RX_REQ_FREE;
	}

	if (rsp->_present.id == 0) {
		ERROR("failed to netdev_bind_rx(): %s", ndevmgr->yerr.msg);
		goto RX_RSP_FREE;
	}

	ndevmgr->dmabuf_id = rsp->id;

	netdev_bind_rx_rsp_free(rsp);
	netdev_bind_rx_req_free(req);

	return 0;

RX_RSP_FREE:		netdev_bind_rx_rsp_free(rsp);
RX_REQ_FREE:		netdev_bind_rx_req_free(req);
FREE_QUEUES:		free(queues); // free(NULL); okay.
YNL_SOCK_DESTROY:	ynl_sock_destroy(ndevmgr->ys);
RETURN_ERROR:		return -1;
}

int ndevmgr_get_dmabuf_id(NetdevManager ndevmgr)
{
	return ndevmgr->dmabuf_id;
}

void ndevmgr_release_rx_queue(NetdevManager ndevmgr)
{
	ynl_sock_destroy(ndevmgr->ys);
}

void ndevmgr_destroy(NetdevManager ndevmgr)
{
	free(ndevmgr);
}
