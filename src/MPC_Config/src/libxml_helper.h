/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - VALAT Sebastien sebastien.valat@cea.fr                           # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_LIBXML_HELPER_H
#define SCTK_LIBXML_HELPER_H

/********************************* INCLUDES *********************************/
#include <libxml/tree.h>
#include <libxml/parser.h>

/********************************* FUNCTION *********************************/
xmlNodePtr sctk_libxml_find_child_node(xmlNodePtr node,const xmlChar * tagname);
xmlChar * sctk_libxml_find_child_node_content(xmlNodePtr node,const xmlChar * tagname);
int sctk_libxml_count_child_nodes(xmlNodePtr node,const xmlChar * tagname);

/********************************* FUNCTION *********************************/
/* Those once came from libxml by redefined as no available for old versions (<2.7.3) */
#ifndef xmlFirstElementChild
xmlNodePtr xmlFirstElementChild(xmlNodePtr parent);
#endif
#ifndef xmlNextElementSibling
xmlNodePtr xmlNextElementSibling(xmlNodePtr node);
#endif

#endif /*SCTK_LIBXML_HELPER_H*/