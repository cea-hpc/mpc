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
#include "libxml_helper.h"

/******************************** FUNCTIONS *********************************/
/**
 * Helper function to find a node based on his tag name.
 * @param node Define the node in which to search.
 * @param tagname Define the tag to find in direct childs.
 * @return Return a pointer to the related node, or NULL if not found.
**/
xmlNodePtr sctk_libxml_find_child_node(xmlNodePtr node,const xmlChar * tagname)
{
	/* vars */
	xmlNodePtr res = NULL;

	/* errors */
	assert(tagname != NULL);
	if (node == NULL)
		return NULL;

	/* search first child */
	if ((node->type == XML_ELEMENT_NODE) && (node->children != NULL)) {
		res = xmlFirstElementChild(node);
		while(res != NULL) {
			if (res->type == XML_ELEMENT_NODE && xmlStrcmp(res->name,tagname) == 0)
				break;
			res = xmlNextElementSibling(res);
		}
	}

	/* return */
	return res;
}

/******************************** FUNCTIONS *********************************/
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
	/* find node */
	node = sctk_libxml_find_child_node(node,tagname);

	/* return */
	if (node == NULL)
		return NULL;
	else
		return xmlNodeGetContent(node);
}

/******************************** FUNCTIONS *********************************/
/**
 * Count the number of direct child with the given tag name.
 * @param node Define the XML node in which to search.
 * @param tagname Define the tag name to search.
 * @return Return the number of requested child. If node is NULL, return 0.
**/
int sctk_libxml_count_child_nodes(xmlNodePtr node,const xmlChar * tagname)
{
	/* vars */
	xmlNodePtr cur = NULL;
	int cnt = 0;

	/* errors */
	assert(tagname != NULL);
	if (node == NULL)
		return 0;

	/* search first child */
	if ((node->type == XML_ELEMENT_NODE) && (node->children != NULL)) {
		cur = xmlFirstElementChild(node);
		while(cur != NULL) {
			if (cur->type == XML_ELEMENT_NODE && xmlStrcmp(cur->name,tagname) == 0)
				cnt++;
			cur = xmlNextElementSibling(cur);
		}
	}

	/* return */
	return cnt;
}

/******************************** FUNCTIONS *********************************/
/**
 * IMPORTED FROM LIBXML 2.7.8 AS IT WAS NOT PROVIDED BY < 2.7.3.
 * 
 * Finds the first child node of that element which is a Element node
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * @param parent the parent node
 * @return Returns the first element child or NULL if not available
 */
#ifndef xmlFirstElementChild
xmlNodePtr xmlFirstElementChild(xmlNodePtr parent)
{
	xmlNodePtr cur = NULL;

	if (parent == NULL)
		return(NULL);

	switch (parent->type) {
		case XML_ELEMENT_NODE:
		case XML_ENTITY_NODE:
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
			cur = parent->children;
			break;
		default:
			return(NULL);
	}

	while (cur != NULL) {
		if (cur->type == XML_ELEMENT_NODE)
			return(cur);
		cur = cur->next;
	}

	return(NULL);
}
#endif /* xmlFirstElementChild */

/******************************** FUNCTIONS *********************************/
/**
 * IMPORTED FROM LIBXML 2.7.8 AS NOT PROVIDED BY < 2.7.3.
 *
 * Finds the first closest next sibling of the node which is an
 * element node.
 * Note the handling of entities references is different than in
 * the W3C DOM element traversal spec since we don't have back reference
 * from entities content to entities references.
 *
 * @param node the current node
 * @return Returns the next element sibling or NULL if not available
 */
#ifndef xmlNextElementSibling
xmlNodePtr xmlNextElementSibling(xmlNodePtr node)
{
	if (node == NULL)
		return(NULL);

	switch (node->type) {
		case XML_ELEMENT_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		case XML_DTD_NODE:
		case XML_XINCLUDE_START:
		case XML_XINCLUDE_END:
			node = node->next;
			break;
		default:
			return(NULL);
	}

	while (node != NULL) {
		if (node->type == XML_ELEMENT_NODE)
			return(node);
		node = node->next;
	}

	return(NULL);
}
#endif /* xmlNextElementSibling */
