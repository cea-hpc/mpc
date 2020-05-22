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

/********************************* INCLUDES *********************************/
#include <mpc_common_debug.h>
#include "runtime_config_selectors.h"

/********************************  CONSTS  **********************************/
/** @TODO place all of them at same place. **/
/** Tag name of the root node of XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_SELECTOR_ENV = BAD_CAST("env");

/********************************* FUNCTION *********************************/
/**
 * Check status of &lt;env name='name'&gt;value&lt;/env&gt; selector based on environment variable.
 * @param selector Define the XML node to validation as a selector.
 * @return True if valid, false otherwise.
**/
bool sctk_runtime_config_xml_selector_env_check(xmlNodePtr selector)
{
	/* vars */
	xmlChar * env_name;
	xmlChar * expected_value;
	bool res;

	/* error */
	assert(xmlStrcmp(selector->name,SCKT_CONFIG_XML_NODE_SELECTOR_ENV) == 0);

	/* fetch values */
	env_name = xmlGetProp(selector,BAD_CAST("name"));
	expected_value = xmlNodeGetContent(selector);
	const char * current_value = getenv((char *)env_name);

	/* debug */
	/* mpc_common_debug("MPC_Config : compare env[%s] : %s == %s\n",env_name,expected_value,current_value); */

	/* check status */
	res = (xmlStrcmp(BAD_CAST(current_value),expected_value) == 0);

	/* free temp memory */
	free(env_name);
	free(expected_value);

	/* return result */
	return res;
}

/********************************* FUNCTION *********************************/
/**
 * Check status on a given selector. It currently support :
 *    - env : Check value of an environment variable, see sctk_runtime_config_xml_selector_env_check().
 * @param selector Define the XML node to validation as a selector.
 * @return True if valid, false otherwise.
**/
bool sctk_runtime_config_xml_selector_check(xmlNodePtr selector)
{
	/* errors */
	assert(selector != NULL);
	assert(selector->type == XML_ELEMENT_NODE);

	/* switches */
	if (xmlStrcmp(selector->name,BAD_CAST("env")) == 0)
	{
		return sctk_runtime_config_xml_selector_env_check(selector);
	}
	else
	{
		mpc_common_debug_fatal("Invalid selector in mappings : %s.",selector->name);
		return false;
	}
}

/********************************* FUNCTION *********************************/
/**
 * Check if one of the given selector is valid.
 * @param selectors Define the &lt;selectors&gt; node to check. It must contain some selector elements.
 * @return True if find one valid, false otherwise.
**/
bool sctk_runtime_config_xml_selectors_check(xmlNodePtr selectors)
{
	/* vars */
	xmlNodePtr selector;

	/* errors */
	assert(selectors != NULL);
	assert(selectors->type == XML_ELEMENT_NODE);
	assert(xmlStrcmp(selectors->name,BAD_CAST("selectors")) == 0);

	/* loop on all selectors */
	selector = xmlFirstElementChild(selectors);
	while (selector != NULL) {
		if (sctk_runtime_config_xml_selector_check(selector) == 0)
			return false;
		selector = xmlNextElementSibling(selector);
	}

	return true;
}
