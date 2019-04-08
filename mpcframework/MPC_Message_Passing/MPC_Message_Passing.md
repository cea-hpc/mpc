Module MPC_Message_Passing
======================

SUMMARY:
--------

MPC message passing support. It represents the whole message communication
layer between any MPI process.

CONTENTS:
---------
* **sctk_driver_tcp/**        : Driver sources to handle TCP/IP network layer
* **sctk_driver_ib/**         : Driver sources to handle Infiniband network layer
* **sctk_low_level_comm/**    : Handles communication layer between processes
* **include/**                : Main header files used by other MPC modules
* **sctk_driver_portals/**    : Driver sources to handle Portals network layer
* **sctk_driver_shm/**        : Driver sources to handle SHMEM as a network layer
* **sctk_inter_thread_comm/** : Defines communication interface between threads
    

COMPONENTS:
-----------

### INTER\_THREAD\_COM
This module will receive requests from the MPI interface. Its purpose is to
create messages (or MPC requests) and to ensure the tracking from one side to
another. Especially, the inter\_thread\_comm is optimised for inter-task
communication when source and target tasks are located in the same UNIX process
(thread-based mode). In that case, the network is never involved. In other
cases, handlers are set to transmit the request through the 'network' component:
the LOW\_LEVEL\_COMM.

### LOW\_LEVEL\_COMM
This module is in charge of point-to-point communication between processes. Even
if threads are not considered here, this interface should stay thread-safe. This
interface is designed to handle the principle of routes (=endpoint describing
metadata beteween a pair or processes), rails (data structure representing a
network technology). Each rail is specifically implementd (see below) and expose
entry points to the low\_level\_comm.

To select the best route to use for sending a message to a peer, the function
`sctk_network_ellect_endpoint` will iterate over existing routes (at
initialiation, a ring is generally created). If one route is found (=an endpoint
to the destination, of a route has been dynamically created to it), the
`send_message` handler is called for the associated rail.

### Driver interface
A rail is always mapped to a network type (TCP,SHM,IB...). Each type defines a
initialization function, in charge of setting up the network. Rails are
completely independent, allowing MPC to expose a efficient multirail approach.

The following functions are defined in the rail and should be set by the
initialization, for each networks:
* `send_message`: the given message(parameter) has to send throught the given
  route (parameter).
* `notify_recv_message`: a RECV request has been posted locally and a net message
  will be expected. This does not guarantee the message to be received on a
  specific rail (all rails are notified) in case of multirail.
* `notify_matching_message`: A RECV request matched with a messaged received
  from the network.
* `notify_perform_message`: A message is waited from a specific origin.
* `notify_idle_message`: The most used function (very intensively). When a
  thread is idle, this function is called to progress net messages. Not
  implementing msg progression here can lead to deadlocks (exepf it dedicated
  polling thread for this network exists).
* `notify_any_source_message`: an ANY\_SOURCE RECV msg has been posted
* `send_message_from_network`: Function to notify upper-layer that a new message
  arrived. This function should always call `sctk_send_message` or its
  derivatives

In addition to these main message management functions, a driver should
implement the following:
* connect\_to: Used when creating a more complex topology than the initial ring
* connect\_fom: Used when creating a more complex topology than the initial ring
* connect_on_demand: if the rail support it, this function is called when
  upper-layers request this rail to create a route to a new destination.
* control_message_handler: Used to managed control-messages to the destination
  of this rail (some rails used CMs to create new routes).

### TCP

* TCP creates a polling thread for each created route.
* TCP keep an listening socket to wait for new route creation.

### INFINIBAND

* TBW

### PORTALS

Portals support in MPC is exposing two different "drivers". One is full-MPI
based, and can be found under `sctk_driver_portals` directory. The second one is
supporting the Active Message interface and bypass most of the standard
communication layer from MPI-specific entry points. This is mainly used for the
support of ARPC inside MPC. It can be found under `sctk_driver_portals_am`. As
they can be really similar, we will only provide documentatio for the first one,
changes for Active-Messages will be detailed also.

#### MPI-based interface

It is not intended to list everything here to replace the furnished Doxygen
already present in source code, but more to create a global view of the
interface and why it has been designed that way.

The interface is split into multiple files :
* `sctk_portals.*`: Intended to be the bridge between upper layers and the
  internal implementation. It exposes routines registered into a rail.
* `sctk_ptl_types.h` : Declare any type required for this driver. We also
  created a lot of typedefs to map Portals types to our own, helping to quickly
  replace any change in the interface. **All types should be declared here and
  not anywhere else**.
* `sctk_ptl_iface.*`: Intended to be the bridge between lower layer (i.e Portals
  API). It exposses routines easy to use for the different protocols (ex:
  `sctk_ptl_me_create()`, `sctk_ptl_pte_init()`, etc...), clearly easier to deal
  with when implementing message passing algorithms.
* `sctk_ptl_eager.*`: Defines routines to support EAGER two-sided messaging protocol.
* `sctk_ptl_rdv.*`: Defines routines to support RDV two-sided messaging protocol
* `sctk_ptl_cm.*`: Defines the interface to deal with Control-messages, the
  one-sided signalization network from MPC.
* `sctk_ptl_rdma.*`: Implement the MPI RDMA layer on top of Portals interface,
  a one-sided interface as well.
* `sctk_ptl_toolkit.*`: Manager between protocols (which one to use in which
  situation). Also contains the polling algorith on both sides (ME & MD).

The Portals driver is built as follows:
1. Each Portals entry (PTE) is associated with a communicator. It gives space in
   the match_bits to store other information. Particularly, as almost everything
   can fit into a single match_bits and/or immediate data, our protocol does not
   need any message header to be processed. When reveived a Portals request, one
   can get the whole message (or the *READY* notification in case of RDV)
2. The first three PTEs are reserved for resilience/recovery(WIP),
   control-messages and one-sided. The fourth and fifth entries are created in
   advance, and will be mapped to `MPI_COMM_WORLD` and `MPI_COMM_SELF`. The
   latter need to be kept to avoid comm_id shifting but obviously, there won't
   be any entry initialisation to avoid consuming useless resources.
3. An arbitrary number of generic ME (ME-PUT) are attached to each PTE to
   anticipate potential eager messages. These pre-posted blocs are stored in the
   OVERFLOW_LIST.
4. When a message has to be sent, an MD is posted to send the message. In the
   case of a RDV message, an ME is also set to receive a subsequent GET. Then a
   PUT is emitted to the target (with the data in eager, without in rdv).
5. When a message has to be received, an ME-PUT is set, the match_bits provided
   will identify this unique message (messages are sent in-order by Portals and
   two Put() requests cannot overtake each other). 
6. Considering two-sided messaging, when the local RECV will be posted, the
   Portals API will handle the matching by itself. When an event is polled, data
   has already been copied into the targeted buffer. In case of eager, an
   extra-copy might be necessary.

Things to note when working on this module :

* If the error `PTL_NI_OP_VIOLATION` occurs, it is higly probable the reason is
  a request like a `GET` reached the pre-posted buffers (for eager purpose)
  located at the end of each PTL entries. The reason is up to the developer to
  determine, but the error is symptomatic of this kind of situation. For
  debugging purposes, the match_bits should be the first thing to check, as if a
  supposed ME-GET did not match, it will catch the generic ME-PUT, raising this
  error.
* the match_bits is a combination o f the MPC rank, the MPI tag, the atomic usage
  ID of the used route and the type of messages. For RDV messages, an additional
  flag for detection, as in some situations (ANY_SOURCE/ANY_TAG) the protocol
  above won't create unique match_bits to differenciate PUT and GET from two
  different successive requests.
* The point above leads to the following limitations :
    * MPI tag can't go beyond 32 bit encoding (not a limitation as the standard
      defines the tag as an integer)
    * MPI rank can't go beyond 16 bit encoding, meaning that a mismatch could
      occur if there is more than 65536 processes exchanging with the current
      one at the same time. The real rank is retrieved from as local map to
      levereage the initial hard limit (because the rank was initally exchanged
      trought the match_bits)
    * The sequence number on the endpoint cannot exeed 8-bit encoding, meaning
      that two given processes cannot have more than 256 on-the-fly messages
      without any mismatch occuring.
    * A message type should be defined by inter_thread_comm and not go beyond a
      8-bit encoding, or 256 values. For RDV messages, this type is truncated by
      one bit and is limited to 128 types only (clerly enough by the time this
      documentation is written).


#### AM-based interface

TBW
