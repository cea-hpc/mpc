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

/********************  HEADERS  *********************/
#include <assert.h>
#include "sctk_libxml_helper.h"

/*******************  FUNCTION  *********************/
/**
 * Helper function to find a node based on his tag name.
 * @param node Define the node in which to search.
 * @param tagname Define the tag to find in direct childs.
 * @return Return a pointer to the related node, or NULL if not found.
**/
xmlNodePtr sctk_libxml_find_child_node(xmlNodePtr node,const xmlChar * tagname)
{
	//vars
	xmlNodePtr res = NULL;

	//errors
	assert(tagname != NULL);
	if (node == NULL)
		return NULL;

	//search first child
	if ((node->type == XML_ELEMENT_NODE) && (node->children != NULL))
	{
		res = xmlFirstElementChild(node);
		while(res != NULL)
		{
			if (res->type == XML_ELEMENT_NODE && xmlStrcmp(res->name,tagname) == 0)
				break;
			res = xmlNextElementSibling(res);
		}
	}

	//return
	return res;
}

/*******************  FUNCTION  *********************/
/**
 * Helper function to find a node based on his tag name.
 * @param node Define the node in which to search.
 * @param tagname Define the tag to find in direct childs.
 * @return Return a pointer to the content, or NULL if not found. Caution you get a copy, so you need to call free
 * when finish to use it.
 * @TODO avoid the copy by wrapping an equivalent of xmlNodeGetContent.
**/
xmlChar * sctk_libxml_find_child_node_content(xmlNodePtr node,const xmlChar * tagname)
{
	//find node
	node = sctk_libxml_find_child_node(node,tagname);

	//return
	if (node == NULL)
		return NULL;
	else
		return xmlNodeGetContent(node);
}

/*******************  FUNCTION  *********************/
/**
 * Count the number of direct child with the given tag name.
 * @param node Define the XML node in which to search.
 * @param tagname Define the tag name to search.
 * @return Return the number of requested child. If node is NULL, return 0.
**/
int sctk_libxml_count_child_nodes(xmlNodePtr node,const xmlChar * tagname)
{
	//vars
	xmlNodePtr cur = NULL;
	int cnt = 0;

	//errors
	assert(tagname != NULL);
	if (node == NULL)
		return 0;

	//search first child
	if ((node->type == XML_ELEMENT_NODE) && (node->children != NULL))
	{
		cur = xmlFirstElementChild(node);
		while(cur != NULL)
		{
			if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name,tagname) == 0)
				cnt++;
			cur = xmlNextElementSibling(cur);
		}
	}

	//return
	return cnt;
}
