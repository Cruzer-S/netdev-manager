#ifndef NETDEV_TOOL_H__
#define NETDEV_TOOL_H__

typedef struct netdev_manager *NetdevManager;

NetdevManager ndevmgr_create(void);

int ndevmgr_bind_rx_queue(NetdevManager ndevmgr,
			  int ifindex, int queue_idx, int num_queue,
			  int dmabuf_fd);
int ndevmgr_bind_tx_queue(NetdevManager ,
			  int ifindex, int queue_idx, int num_queue,
			  int dmabuf_fd);

int ndevmgr_get_dmabuf_id(NetdevManager );

void ndevmgr_release_rx_queue(NetdevManager );
void ndevmgr_release_tx_queue(NetdevManager );

void ndevmgr_destroy(NetdevManager );

const char *ndevmgr_get_error(void);

#endif
