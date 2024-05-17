Module MPC_Lowcomm
======================

SUMMARY:
--------

MPC communication module. It is a thread-safe, task-aware communication layer to
exchange data between threads or processes. The module provides:
- Point-to-point (P2P) communications: support for MPI tag-matching and
  communicators.
- Resource management: automatic discovery of available network interfaces (also
  based on user input).

Architecture with two layers:
- top layer for protocols (LCP): message passing and active message
  communication models supported, eager, rendez-vous, data striping protocols,
  memory registration,...
- lower layer for transports (Rail): abstract interface for underlying specific
  transport such as TCP, OFI or Portals.

Software designs are based on the Active Message paradigm: upon message
reception and identifier, the corresponding registered handler is called and
executed.

CONTENTS:
---------

* **lcp/**        : LowComm Protocol network layer.
* **lcr/**        : LowComm Rail network layer.
* **cplane/**     : TCP-based network backbone for inter-job communication.
* **tbsm/**       : Thread-Based Shared Memory driver to handle inter-task network layer.
* **shm/**        : SHared Memory driver to handle intra-node communications.
* **tcp/**        : Driver sources to handle TCP network layer.
* **portals/**    : Driver sources to handle Portals network layer.
* **ofi/**        : Driver sources to handle OpenFabric network layer

COMPONENTS:
-----------

### Rail: transport layer

TODO: general discussion on the rail abstraction.

#### Send mode

TODO: describe bcopy and zcopy semantics



### LCP: LowComm Protocol Layer

LCP is reponsible for:
- Resource initialization: automatically discover available network resources
  and instanciation based on user input.
- Connection management: endpoint creation, communication establishement,
  multirail.
- Communication protocols: optimized datapath for different communication
  configuration (intra-proc, intra-node, inter-node).
- Communication models: message passing and active message.
- Memory management: register/unregister memory for RDMA communications.

Next, we introduce important definition and then explain implementation details.

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
