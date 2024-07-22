Module MPC_Lowcomm
======================

SUMMARY:
--------

Here are described the internal of the Lowcomm module.

CONTENTS:
---------

* **lcp/**        : LowComm Protocol network layer.
* **lcr/**        : LowComm Rail network layer.
* **cplane/**     : TCP-based network backbone for inter-job communication.
* **tbsm/**       : Driver sources to handle inter-task network layer (Thread-Based Shared Memory).
* **shm/**        : driver sources to handle intra-node communications (SHared Memory).
* **tcp/**        : Driver sources to handle TCP network layer.
* **ptl/**        : Driver sources to handle Portals network layer.
* **ofi/**        : Driver sources to handle OpenFabric network layer

COMPONENTS:
-----------

### LowComm Protocol Layer

LCP is responsible for:

**Resource Initialization**

The Resource Initialization process in the LowComm Protocol Layer automates the
discovery of network resources. It scans the network environment to identify
hardware and software resources that meet application requirements, driven by
user input. This process selects appropriate resources based on user-defined
criteria, simplifying setup and optimizing resource use.  

**Connection Management**

Connection Management facilitates robust communication across networked
environments. It handles the creation of endpoints necessary for establishing
communication links between nodes. This component also manages multirail
configurations where multiple network interfaces are used to increase bandwidth
(and improve fault tolerance, TOBEIMPLEMENTED). It includes the negotiation and
adjustment of connection parameters to maintain stable communication.  

**Communication Protocols**

This function optimizes data paths for transferring information across various
configurations, such as intra-process, intra-node, and inter-node
communications. It adjusts to the network topology and communication hardware
characteristics to ensure efficient data transmission.  

**Communication Progression**

Communication Progression manages the stages and transitions of communication
activities, ensuring the progression of messages within the infrastructure.

**Communication Models**

The layer supports various communication models, including Message Passing and
Remote Memory Access (RMA). Message Passing is suited for general
communications, while RMA allows direct memory access on remote nodes,
beneficial for high-performance computing.

**Memory Management**

Memory Management involves the registration and deregistration of memory for
Remote Direct Memory Access (RDMA) communications. It allocates and manages
memory across communication processes.

#### Definitions

The following term are used throughout the documentation:
- **transport**: physical network (TCP/IP, BXI, IB).
- **component**: abstract interface describing a transport (`tcp`, `ptl`,
  `ofi`). It can "query" *devices* and "open" *rail interface*. It correspond to
  `lcr_component_t` defined in `lcr_component.h`
- **device**: representation of a physical device by its name (eg `eth0`, `bxi0`).
- **rail interface**: abstract interface to expose capabilities of the network
  used (send/recv operations, connection management, RDMA, Atomics). It defines
  the transport API.  
- **driver**: implementation of the rail interface API for a specific transport.
- **rail (=resource)**: instance of a rail interface.

#### Context initialization

The LCP Context handles the initialization of the network resources based on the
configuration provided by the user. For example, it queries available devices
(TCP, OFI, Portals), it instanciates and initializes resources,... It
centralizes all structures related to communication.

Context initialization has 7 steps:
1. *Load network configuration*: load the configuration provided at context
   creation from `lcp_context_param_t` and from `lowcomm_config.c`
2. *Pre-"Component query" configuration*: check if requested components are
   available. This is a compilation type check since component availability may be
   determined by compilation option (ex: `--with-portals`)
3. *Component filtering*:
  - based on the MPI layout configuration (ex: 1 proc, 1 node, n MPI tasks =>
    only use TBSM)
  - avoid registering rail for polling, which introduce overhead, when we know
    they will not be used
  - TBSM usage **is not configurable**. If MPC is compiled in thread-mode + run
    with less proc than MPI rank => TBSM will be used for inter-thread
    communication.
4. *Component device query*: call `lcp_context_query_component_devices` for all
   requested and filtered components
5. *Device filtering*:
  - based on device name from rail configuration (if not `any` then do `strstr`)
  - compared also with the maximal number of interfaces in rail configuration
  - build a device map for later resource init
6. *Resource allocation and interface opening*.
7. *Interface capabilities*: capabilities may be check using
   `lcr_iface_get_attr` only, so this can be known only after initialization since
   network resources may have runtime specifications (two Portals NIC from two
   differents cluster may have different configurations).

#### Connection management

**Connection management** relies on endpoints which conceptually refer to the
communication channels or connection points through which processes can
exchange messages and data. They have the following properties:
- Each process has a set of endpoints, and each endpoint is uniquely identified
  by a specific identifier (addressing scheme described later).
- They created dynamically or "on-demand" allowing processes to manage them at
  runtime.
- Resource mapping: each endpoint is associated with particular hardware
  resources, like network interfaces. Endpoint supports multirail: one endpoint
  may have multiple network interfaces.
- Asynchronous connection: connection establishment is non-blocking. Operations
  on non-connected endpoints will be set to pending and resumed at a later stage.

Endpoints follows the two layers architecture:
- Protocol endpoints: Encompass multiple rail endpoints and are used to
  implement the multirail feature.
- Rail endpoints: defined in the Rail layer, containing specific network
  connection information (like IP address for TCP, Queue Pairs for InfiniBand, or
  Process Identifiers in Portals4) and an interface object.

**Addressing scheme** is based on `monitor.c`. Each endpoint is identified by a
unique `uint64_t` where:
- 32 most significant bits are the **set id**: used for multi-job runs for example.
- 32 least significant bits are the **process id**: used to identify an UNIX process.

#### Communication protocols

Without going into much detail, we describe impactful design choice in LCP.

##### Datapaths

LCP defines multiple **datapaths** depending on message size, resquest
parameter, endpoint layout (intra-process, intra-node, inter-node), available
networks and their capabilities. For both TAG and AM API, branching is located
in `*start*` functions (`lcp_tag_send_start` and `lcp_am_send_start`).

##### Internal communication model

**Internal communication model** is based on the active message paradigm:
- Handlers are registered and assigned an active message id.
- Upon reception of data from the network, appropriate handler will be called
  and executed.

Internal handler definition is done through the `LCP_DEFINE_AM` macro where we
specify the active message ID, the function handler that has been previously
developed and some flags.

For example in `lcp_tag.c`, `LCP_DEFINE_AM(LCP_AM_ID_EAGER_TAG,
lcp_eager_tag_handler, 0);` defines the handler that processes eager message
for the TAG API. It will be executed by the underlying network transport, see
`tcp.c` in function `lcr_tcp_invoke_am`.

Such tags are defined in `lcp_types.h`. These tags are registered in the 
adequate protocol file (C.F. `lcp_tag.c`, `lcp_rndv.c`) using `LCP_DEFINE_AM`.
This macro requires a callback symbol which prototype is defined in 
`lcr_defs.h` as `lcr_am_callback`. The specified callback is always called on
request receive. However it is not called back if it has been called once. 

If a request has triggered its associated active message callback but is not expected as a MPI receive, it is not being inserted into unexpected matching lists and must be explicitly inserted into such lists. The standard procedure that should be pasted into any callback is the following : 

```c

static int your_message_handler(void *arg, void *data,
                                 size_t length,
                                 __UNUSED__ unsigned flags)
{
        int rc = LCP_SUCCESS;
        lcp_context_h ctx = arg;
        lcp_unexp_ctnr_t *ctnr;
        lcp_request_t *req;
        lcp_tag_hdr_t *hdr = data;
        lcp_task_h task = NULL;

        task = lcp_context_task_get(ctx, hdr->dest_tid);  
        if (task == NULL) {
                mpc_common_errorpoint_fmt("LCP: could not find task with tid=%d", hdr->dest_tid);
                rc = LCP_ERROR;
                goto err;
        }

	LCP_TASK_LOCK(task);
	/* Try to match it with a posted message */
        req = lcp_match_prqueue(task->prqs, 
                                hdr->comm, 
                                hdr->tag,
                                hdr->src_tid);
	/* if request is not matched */
	if (req == NULL) {
                mpc_common_debug_info("LCP: recv unexp tag src=%d, tag=%d, dest=%d, "
                                      "length=%d, sequence=%d", hdr->src_tid, 
                                      hdr->tag, hdr->dest_tid, length - sizeof(lcp_tag_hdr_t), 
                                      hdr->seqn);
		rc = lcp_request_init_unexp_ctnr(task, &ctnr, hdr, length, 
                                                 LCP_RECV_CONTAINER_UNEXP_EAGER_TAG);
		if (rc != LCP_SUCCESS) {
			goto err;
		}
		// add the request to the unexpected messages
		lcp_append_umqueue(task->umqs, &ctnr->elem, 
			       hdr->comm);

		LCP_TASK_UNLOCK(task);
		return LCP_SUCCESS;
	}
	LCP_TASK_UNLOCK(task);

	[further message handling]
}
```

The said MPI receive call does not trigger the adequate active message callback so any code that is after the aforementioned procedure is to be repeated in the `lcp_recv.c:lcp_tag_recv_nb` function. 
 
##### Data layout

Before being sent to the network, data is successively encapsulated by the two
layers, each adding their own protocol data. Indeed, through IOVEC or packing,
user data is preceeded by a header containing protocol data of the current
layer.  Unpacking is performed upon data reception.

Actual memory layout for both TAG and AM API may be found in `lcp_tag.c` and
`lcp_am.c`.

We mention that tag-matching offloading protocols require a completely
different data layout since all protocol data has to be contained in metadata
container provided by the lower layer. For more details, check
`lcp_tag_offload.c`.

##### Software tag matching

Tag matching is implemented by `lcp_tag_match.c` using intrusive queue
datastructures. It implements the Unexpected Message Queues (UMQ) and
Posted Receive Queues (PRQ). There are a unique queue per MPI rank.

Before being posted to the UMQ, the message data is copied to a receive
descriptor (`lcp_unexp_ctnr_t`) so that Rail resources can be released on event
completion.  Receive descriptor also describe the type of protocol from which
the message has been received.

##### Protocol implementation

**EAGER**: TODO

**RENDEZ-VOUS**: TODO

#### Memory management

TODO
