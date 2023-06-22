#include "mpc_ofi_dns.h"
#include "mpc_common_spinlock.h"
#include "mpc_ofi_helpers.h"
#include "rdma/fabric.h"
#include "rdma/fi_domain.h"

#include <mpc_common_debug.h>
#include <mpc_common_helper.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


static void __dump_addr(char *p, size_t len)
{
	size_t i = 0;

	for(i = 0; i < len; i++)
	{
		printf("%x", p[i]);
	}
	printf("\n");
}

/************************************
 * HASH TABLE TO UINT64_T TO VOID * *
 ************************************/

int mpc_ofi_dns_ht_init(struct mpc_ofi_dns_ht_t * ht, uint32_t size)
{
   ht->size = size;
   ht->heads = calloc(size, sizeof(struct mpc_ofi_dns_ht_cell_t *));

   if(!ht->heads)
   {
      perror("calloc");
      return -1;
   }

   mpc_common_spinlock_init(&ht->lock, 0);

   return 0;
}

int mpc_ofi_dns_ht_release(struct mpc_ofi_dns_ht_t * ht, void (*release)(void *pentry))
{
   uint32_t i = 0;

   mpc_common_spinlock_lock(&ht->lock);

   for(i = 0 ; i < ht->size; i++)
   {
      struct mpc_ofi_dns_ht_cell_t * cur = ht->heads[i];

      while(cur != NULL)
      {
         struct mpc_ofi_dns_ht_cell_t * tmp = NULL;
         tmp = cur;
         cur = cur->next;
         if(release)
         {
            (release)(tmp->value);
         }
         free(tmp);
      }
   }

   free(ht->heads);

   mpc_common_spinlock_unlock(&ht->lock);

   return 0;
}

static struct mpc_ofi_dns_ht_cell_t * mpc_ofi_dns_ht_get_cell_nolock(struct mpc_ofi_dns_ht_t * ht, uint64_t rank)
{
   uint32_t targ = rank % ht->size;

   struct mpc_ofi_dns_ht_cell_t * cur = ht->heads[targ];

   while(cur != NULL)
   {
      if(cur->rank == rank)
      {
         return cur;
      }
      cur = cur->next;
   }

   return NULL;
}

int mpc_ofi_dns_ht_put(struct mpc_ofi_dns_ht_t * ht, uint64_t rank, void * value)
{

   mpc_common_spinlock_lock(&ht->lock);

   struct mpc_ofi_dns_ht_cell_t *exists = mpc_ofi_dns_ht_get_cell_nolock(ht, rank);

   if(exists)
   {
      exists->value = value;
   }
   else
   {
      uint32_t targ = rank % ht->size;
      struct mpc_ofi_dns_ht_cell_t *new = malloc(sizeof(struct mpc_ofi_dns_ht_cell_t));

      if(!new)
      {
         perror("malloc");
         mpc_common_spinlock_unlock(&ht->lock);
         return -1;
      }

      new->rank = rank;
      new->value = value;
      new->next = ht->heads[targ];

      ht->heads[targ] = new;
   }

   mpc_common_spinlock_unlock(&ht->lock);

   return 0;
}

void * mpc_ofi_dns_ht_get(struct mpc_ofi_dns_ht_t * ht, uint64_t rank, int * found)
{
   void * ret = NULL;

   struct mpc_ofi_dns_ht_cell_t *exists = NULL;
   mpc_common_spinlock_lock(&ht->lock);

   exists = mpc_ofi_dns_ht_get_cell_nolock(ht, rank);

   mpc_common_spinlock_unlock(&ht->lock);

   if(exists)
   {
      *found = 1;
      ret = exists->value;
   }
   else
   {
      *found = 0;
   }

   return ret;
}

/***************************
 * SOCKET-BASED RESOLUTION *
 ***************************/

static inline void * __mpc_ofi_dns_socket_resolution_server_thread(void *prsvl)
{
   struct mpc_ofi_dns_socket_resolution_t * rsvl = (struct mpc_ofi_dns_socket_resolution_t*)prsvl;

   while(rsvl->is_running)
   {
      struct sockaddr client_info;
      socklen_t addr_len = 0;

      int clientsock = accept(rsvl->server_socket,
                              &client_info,
                              &addr_len);

      if(clientsock < 0)
      {
         if(errno != EINVAL)
         {
            /* Ignore socket closure */
            perror("accept");
         }
         continue;
      }

      struct mpc_ofi_dns_socket_resolution_request_t cmd = {0};

      if( mpc_common_io_safe_read(clientsock, &cmd, sizeof(struct mpc_ofi_dns_socket_resolution_request_t)) < 0)
      {
         close(clientsock);
         continue;
      }

      switch (cmd.type)
      {
         case MPC_OFI_DNS_SOCKET_RESOLUTION_GET:
            /* Clear */
            cmd.len = MPC_OFI_ADDRESS_LEN;
            cmd.address[0] = '\0';
            if(mpc_ofi_dns_resolve(rsvl->dns,
                                 cmd.rank,
                                 cmd.address,
                                 &cmd.len))
            {
               /* Set len to 0 to notify error */
               cmd.address[0] = '\0';
               cmd.len = 0;
            }
         break;
         case MPC_OFI_DNS_SOCKET_RESOLUTION_PUT:
            if(mpc_ofi_dns_register(rsvl->dns,
                                 cmd.rank,
                                 cmd.address,
                                 cmd.len))
            {
               /* Set len to 0 to notify error */
               cmd.address[0] = '\0';
               cmd.len = 0;
            }
         break;
         default:
            close(clientsock);
            continue;
      }

      mpc_common_io_safe_write(clientsock, &cmd, sizeof(struct mpc_ofi_dns_socket_resolution_request_t));

      close(clientsock);
   }

   mpc_common_errorpoint("Server thread leaving");

   return NULL;
}

static inline int __mpc_ofi_dns_socket_resolution_server_start(struct mpc_ofi_dns_socket_resolution_t*rsvl)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo * result = NULL;

	int ret = getaddrinfo(NULL, "0", &hints, &result);

	if(ret < 0)
	{
		herror("getaddrinfo");
		return -1;
	}

	int listening = 0;

	struct addrinfo *tmp = NULL;
	for(tmp = result; tmp != NULL; tmp=tmp->ai_next)
	{
		rsvl->server_socket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

		if(rsvl->server_socket < 0)
		{
			perror("socket");
			continue;
		}


		if( bind(rsvl->server_socket, tmp->ai_addr, tmp->ai_addrlen) < 0)
		{
         close(rsvl->server_socket);
			perror("bind");
			continue;
		}


		if( listen(rsvl->server_socket, 10) < 0)
		{
         close(rsvl->server_socket);
			perror("listen");
			continue;
		}

		listening = 1;
		break;
	}

	if(listening == 0)
	{
      mpc_common_errorpoint("Failed to start socket-based resolution server");
		return -1;
	}

   rsvl->is_running = 1;

   /* Extract server address and port */
   struct sockaddr_in sin;
   socklen_t len = sizeof(sin);
   if (getsockname(rsvl->server_socket, (struct sockaddr *)&sin, &len) < 0)
   {
      perror("getsockname");
      return -1;
   }
   
   (void)snprintf(rsvl->port, MPC_OFI_DNS_PORT_LEN, "%d", ntohs(sin.sin_port));
   if( gethostname(rsvl->server_addr, MPC_OFI_ADDRESS_LEN) < 0 )
   {
      perror("gethostname");
      return -1;
   }

   /* Now start the request processing thread */
   if(pthread_create(&rsvl->server_thread, NULL, __mpc_ofi_dns_socket_resolution_server_thread, rsvl))
   {
      close(rsvl->server_socket);
      perror("pthread_create");
      return -1;
   }

   return 0;
}

int mpc_ofi_dns_socket_resolution_init(struct mpc_ofi_dns_socket_resolution_t*rsvl,
                                      struct mpc_ofi_dns_t * dns)
{
   rsvl->dns = dns;

   char * master_addr = getenv("MPC_OFI_SOCKET_MASTER");

   if(!master_addr)
   {
      /* I am server */
      if(__mpc_ofi_dns_socket_resolution_server_start(rsvl))
      {
         mpc_common_errorpoint("Failed to start revolv socket server");
         return -1;
      }
      rsvl->is_master_process = 1;
      (void)fprintf(stderr, "Started main resolution server on %s:%s\n", rsvl->server_addr, rsvl->port);
   }
   else
   {
      /* I am client */
      char * sep = strchr(master_addr, ':');

      if(!sep)
      {
         mpc_common_errorpoint("Bad address format in MPC_OFI_SOCKET_MASTER expected addr:port");
         return -1;
      }

      *sep = '\0';

      int port = 0;
      port = atoi(sep + 1);

      if(port == 0)
      {
         mpc_common_errorpoint("Failed to resolve master port");
         return -1;
      }

      (void)snprintf(rsvl->port, MPC_OFI_DNS_PORT_LEN, "%d", port);
      (void)snprintf(rsvl->server_addr, MPC_OFI_ADDRESS_LEN, "%s", master_addr);
   }

   return 0;
}

int mpc_ofi_dns_socket_resolution_release(struct mpc_ofi_dns_socket_resolution_t*rsvl)
{
   rsvl->is_running = 0;

   shutdown(rsvl->server_socket, SHUT_RDWR);
   close(rsvl->server_socket);

   if(rsvl->is_master_process)
   {
      pthread_join(rsvl->server_thread, NULL);
   }

   return 0;
}

static inline int __mpc_ofi_dns_socket_resolution_exec_command(struct mpc_ofi_dns_socket_resolution_t * rsvl, struct mpc_ofi_dns_socket_resolution_request_t *cmd)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo * result = NULL;

	int ret = getaddrinfo(rsvl->server_addr, rsvl->port, &hints, &result);


	if(ret < 0)
	{
		herror("getaddrinfo");
		return -1;
	}

	int sock = 0;
	int connected = 0;

	struct addrinfo *tmp = NULL;
	for(tmp = result; tmp != NULL; tmp=tmp->ai_next)
	{
		sock = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);

		if(sock < 0)
		{
			perror("socket");
			continue;
		}

		if( connect(sock, tmp->ai_addr, tmp->ai_addrlen) < 0)
		{
			perror("connect");
			continue;
		}

		connected = 1;
		break;
	}

	if(connected == 0)
	{
      mpc_common_errorpoint("Failed to connect to server");
		return -1;
	}

   if( mpc_common_io_safe_write(sock, cmd, sizeof(struct mpc_ofi_dns_socket_resolution_request_t)) )
   {
      close(sock);
      return -1;
   }

   if( mpc_common_io_safe_read(sock, cmd, sizeof(struct mpc_ofi_dns_socket_resolution_request_t)) < 0)
   {
      close(sock);
      return -1;
   }

   close(sock);

   return 0;
}


int mpc_ofi_dns_socket_resolution_register(void *prsvl,
                                          uint64_t rank,
                                          char *addr,
                                          size_t *len)
{
   struct mpc_ofi_dns_socket_resolution_t * rsvl = (struct mpc_ofi_dns_socket_resolution_t*)prsvl;

   if(rsvl->is_master_process)
   {
      /* No OP */
      return 0;
   }

   struct mpc_ofi_dns_socket_resolution_request_t cmd = { 0 };

   if(MPC_OFI_ADDRESS_LEN < *len)
   {
      mpc_common_errorpoint("Address too long");
      return -1;
   }

   memcpy(cmd.address, addr, *len);
   cmd.len = *len;
   cmd.type = MPC_OFI_DNS_SOCKET_RESOLUTION_PUT;
   cmd.rank = rank;

   if(__mpc_ofi_dns_socket_resolution_exec_command(rsvl, &cmd))
   {
      mpc_common_errorpoint("Failed to run command");
      return -1;
   }

   /* Extract return vals */
   if(cmd.len == 0)
   {
      mpc_common_errorpoint("Server side error");
      return -1;
   }

   return 0;
}

int mpc_ofi_dns_socket_resolution_lookup(void* prsvl,
                                        uint64_t rank,
                                        char *addr,
                                        size_t *len)
{
   struct mpc_ofi_dns_socket_resolution_t * rsvl = (struct mpc_ofi_dns_socket_resolution_t*)prsvl;

   if(rsvl->is_master_process)
   {
      /* No OP should be in cache */
      return -1;
   }

   struct mpc_ofi_dns_socket_resolution_request_t cmd = { 0 };

   if(MPC_OFI_ADDRESS_LEN < *len)
   {
      mpc_common_errorpoint("Address too long");
      return -1;
   }

   cmd.address[0] = '\0';
   cmd.len = 0;
   cmd.type = MPC_OFI_DNS_SOCKET_RESOLUTION_GET;
   cmd.rank = rank;

   if(__mpc_ofi_dns_socket_resolution_exec_command(rsvl, &cmd))
   {
      mpc_common_errorpoint("Failed to run command");
      return -1;
   }

   /* Extract return vals */
   if(cmd.len == 0)
   {
      mpc_common_errorpoint("Server side error");
      return -1;
   }

   if(*len < cmd.len)
   {
      mpc_common_errorpoint("Address overflows buffer");
      return -1;
   }

   memcpy(addr, cmd.address, cmd.len);
   *len = cmd.len;

   return 0;
}

/*******************
 * THE CENTRAL DNS *
 *******************/

int mpc_ofi_dns_init(struct mpc_ofi_dns_t * dns, mpc_ofi_dns_resolution_t resolution_type)
{
   dns->resolution_type = resolution_type;

   switch (dns->resolution_type)
   {
      case MPC_OFI_DNS_RESOLUTION_SOCKET:
      {
         if(mpc_ofi_dns_socket_resolution_init(&dns->socket_rsvl, dns))
         {
            mpc_common_errorpoint("Failed to init Socket based resolver");
            return -1;
         }
         dns->operation_first_arg = &dns->socket_rsvl;
         dns->op_lookup = mpc_ofi_dns_socket_resolution_lookup;
         dns->op_register = mpc_ofi_dns_socket_resolution_register;
         break;

      }
      default:
      {
         mpc_common_errorpoint("No such resolution type");
         return -1;
      }
   }

   if(mpc_ofi_dns_ht_init(&dns->cache, MPC_OFI_DNS_DEFAULT_HT_SIZE))
   {
      mpc_common_errorpoint("Failed to initialize cache");
      return -1;
   }

   return 0;
}
int mpc_ofi_dns_release(struct mpc_ofi_dns_t * dns)
{
   switch (dns->resolution_type)
   {
      case MPC_OFI_DNS_RESOLUTION_SOCKET:
      {
         if(mpc_ofi_dns_socket_resolution_release(&dns->socket_rsvl))
         {
            mpc_common_errorpoint("Failed to release based resolver");
            return -1;
         }
         break;
      }
      default:
         mpc_common_errorpoint("No such resolution type");
         return -1;
   }

   dns->op_lookup = NULL;
   dns->op_register = NULL;

   if(mpc_ofi_dns_ht_release(&dns->cache, mpc_ofi_dns_name_entry_release))
   {
      mpc_common_errorpoint("Failed to release cache");
      return -1;
   }

   return 0;
}


struct mpc_ofi_dns_name_entry_t * mpc_ofi_dns_name_entry(char * buff, size_t len)
{
   struct mpc_ofi_dns_name_entry_t *entry = malloc(sizeof(struct mpc_ofi_dns_name_entry_t));

   if(!entry)
   {
      perror("malloc");
      return NULL;
   }

   entry->value = malloc(len * sizeof(char));

   if(!entry->value)
   {
      perror("malloc");
      return NULL;
   }

   memcpy(entry->value, buff, len);
   entry->len = len;

   return entry;
}

void mpc_ofi_dns_name_entry_release(void *pentry)
{
   struct mpc_ofi_dns_name_entry_t * entry = (struct mpc_ofi_dns_name_entry_t *)pentry;

   free(entry->value);
   free(entry);
}



int mpc_ofi_dns_resolve(struct mpc_ofi_dns_t * dns, uint64_t rank, char * outbuff, size_t *outlen)
{
   /* First look in local cache */
   int found = 0;
   struct mpc_ofi_dns_name_entry_t *entry = (struct mpc_ofi_dns_name_entry_t *)mpc_ofi_dns_ht_get(&dns->cache, rank, &found);

   if(entry)
   {
      /* Cache HIT */
      if(*outlen < entry->len)
      {
         mpc_common_errorpoint("Address truncated");
         return -1;
      }

      memcpy(outbuff, entry->value, entry->len);
      *outlen = entry->len;

      return 0;
   }

   if(dns->op_lookup)
   {
      /* Proceed to resolv lookup */
      return (dns->op_lookup)(dns->operation_first_arg, rank, outbuff, outlen);
   }

   return 0;
}

int mpc_ofi_dns_register(struct mpc_ofi_dns_t * dns, uint64_t rank, char * buff, size_t len)
{
   /* Copy address out of the input buff for persistence */
   struct mpc_ofi_dns_name_entry_t *entry = mpc_ofi_dns_name_entry(buff, len);

   (void)fprintf(stderr, "New address for %ld @", rank);
   __dump_addr(buff, len);

   /* Add to cache */
   if(mpc_ofi_dns_ht_put(&dns->cache, rank, (void *)entry))
   {
      free(entry);
      return -1;
   }

   /* Notify to resolver */
   if(dns->op_register)
   {
      if((dns->op_register)(dns->operation_first_arg, rank, buff, &len))
      {
         mpc_common_errorpoint("Failed to register in resolver");
         return -1;
      }
   }

   return 0;
}


/**********************
 * THE PER DOMAIN DNS *
 **********************/



int mpc_ofi_domain_dns_init(struct mpc_ofi_domain_dns_t *ddns,
                           struct mpc_ofi_dns_t * main_dns,
                           struct fid_domain *domain)
{
   ddns->main_dns = main_dns;

   /* Create hashtable */
   if(mpc_ofi_dns_ht_init(&ddns->cache, MPC_OFI_DNS_DEFAULT_HT_SIZE) < 0)
   {
      mpc_common_errorpoint("Failed to create hash-table");
      return -1;
   }

   /* Init the AV */
   struct fi_av_attr av_attr = {0};
   av_attr.type = FI_AV_MAP;
   MPC_OFI_CHECK_RET(fi_av_open(domain, &av_attr, &ddns->av, NULL));

   return 0;
}


struct fid * mpc_ofi_domain_dns_av(struct mpc_ofi_domain_dns_t *ddns)
{
   return &ddns->av->fid;
}

int mpc_ofi_domain_dns_release(struct mpc_ofi_domain_dns_t *ddns)
{
   MPC_OFI_CHECK_RET(fi_close(&ddns->av->fid));
   mpc_ofi_dns_ht_release(&ddns->cache, free);
   return 0;
}

int mpc_ofi_domain_dns_resolve(struct mpc_ofi_domain_dns_t * dns, uint64_t rank, fi_addr_t *addr)
{

   int found = 0;
   void * cachedfid = mpc_ofi_dns_ht_get(&dns->cache, rank, &found);

   if(!found)
   {
      char buff[512];
      size_t len = 512;

      /* We need to insert it */
      if(mpc_ofi_dns_resolve(dns->main_dns, rank, buff, &len))
      {
         mpc_common_errorpoint("Failed to resolve rank adress in domain from main");
         return -1;
      }

      /* Now insert in AV */
      MPC_OFI_CHECK_RET(fi_av_insert(dns->av, buff, 1, addr, 0, NULL));

      /* and now save in HT for next lookup */
      if(mpc_ofi_dns_ht_put(&dns->cache, rank, (void*)*addr))
      {
         mpc_common_errorpoint("Failed to push new address in HT");
         return -1;
      }
   }
   else
   {
      *addr = (fi_addr_t)cachedfid;
   }

   return 0;
}