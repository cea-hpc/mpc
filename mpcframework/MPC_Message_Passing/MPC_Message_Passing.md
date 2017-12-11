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

* Portals implements two protocols: EAGER and RDV
* It aims to maximize zero-copy & OS bypass by limiting application
  notification. When an event occurs, data has already been copied/retrieved.
* the match_bits is a combination of the MPC rank, the MPI tag the atomic usage
  ID of the used route.
