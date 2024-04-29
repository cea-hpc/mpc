Networks
========

MPC supports tcp, shm, portals and infiniband networks. Networks drivers are organized in "rails" which are the following :

- **tbsmmpi** : :term:`TBSM`
- **shmmpi** : :term:`SHM`
- **verbsofirail** : verbs driver using :term:`OFI`
- **tcpofirail** : tcp driver using :term:`OFI`
- **portalsmpi** : portals driver

 The following configurations with their used rails are available :

- shm
    - tbsmmpi
    - shmmpi
- tcpshm
    - tbsmmpi
    - shmmpi
    - tcpofirail
- tcp
    - tbsmmpi
    - verbsofirail
- verbs
    - tbsmmpi
    - verbsofirail
- verbsshm
    - tbsmmpi
    - verbsofirail
    - shmmpi
- portals4
    - tbsmmpi
    - portalsmpi

