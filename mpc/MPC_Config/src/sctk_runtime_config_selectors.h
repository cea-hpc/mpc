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

#ifndef SCTK_RUNTIME_CONFIG_SELECTORS_H
#define SCTK_RUNTIME_CONFIG_SELECTORS_H

/********************  HEADERS  *********************/
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <stdbool.h>

/*******************  FUNCTION  *********************/
//selector functions
bool sctk_runtime_config_xml_selectors_check(xmlNodePtr selectors);
bool sctk_runtime_config_xml_selector_check(xmlNodePtr selector);

/*******************  FUNCTION  *********************/
//specific selector rules implementation.
bool sctk_runtime_config_xml_selector_env_check(xmlNodePtr selector);

#endif
