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
#include <string.h>
#include <stdbool.h>
#include <sctk_config.h>
#include "sctk_config_debug.h"
#include "sctk_config_selectors.h"
#include "sctk_libxml_helper.h"
#include "sctk_config_sources.h"

/*********************  CONSTS  *********************/
/** Tag name of the root node of XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_MPC = BAD_CAST("mpc");
/** Tag name of profile node in XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_PROFILES = BAD_CAST("profiles");
/** Tag name of profile node in XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_PROFILE = BAD_CAST("profile");
/** Tag name of mappings node in XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_MAPPINGS = BAD_CAST("mappings");
/** Tag name of mapping node in XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_MAPPING = BAD_CAST("mapping");
/** Tag name of selectors node in XML document. **/
static const xmlChar * SCKT_CONFIG_XML_NODE_SELECTORS = BAD_CAST("selectors");

/** @TODO check that some assert need to be turned into assume (when checking node names...) **/

/*******************  FUNCTION  *********************/
/** @TODO doc **/
void sctk_config_sources_select_profiles_in_mapping(struct sctk_config_sources * config_sources,struct sctk_config_source_xml * source,xmlNodePtr mapping)
{
	//vars
	xmlNodePtr selectors;
	xmlNodePtr profile;

	//errors
	assert(config_sources != NULL);
	assert(source != NULL);
	assert(mapping != NULL);
	assert(mapping->type == XML_ELEMENT_NODE);
	assert(xmlStrcmp(mapping->name,BAD_CAST(SCKT_CONFIG_XML_NODE_MAPPING)) == 0);

	//check selectors
	selectors = sctk_libxml_find_child_node(mapping,SCKT_CONFIG_XML_NODE_SELECTORS);
	if (selectors != NULL)
	{
		if (sctk_config_xml_selectors_check(selectors))
		{
			//get profiles
			profile = sctk_libxml_find_child_node(mapping,SCKT_CONFIG_XML_NODE_PROFILES);
			//down to first profile child
			profile = xmlFirstElementChild(profile);
			assert(profile != NULL);
			assert(xmlStrcmp(profile->name,SCKT_CONFIG_XML_NODE_PROFILE) == 0);
			//loop on childs
			while (profile != NULL)
			{
				//if profile, ok add it
				if (xmlStrcmp(profile->name,SCKT_CONFIG_XML_NODE_PROFILE) == 0)
				{
					/** @TODO manage unicity at this point instead of manage it when inserting profile DOM nodes later.**/
					config_sources->profile_names[config_sources->cnt_profile_names] = xmlNodeGetContent(profile);
					printf("DEBUG : add profile %s\n",config_sources->profile_names[config_sources->cnt_profile_names]);
					config_sources->cnt_profile_names++;
					assume(config_sources->cnt_profile_names < SCTK_CONFIG_MAX_PROFILES,"Reach maximum number of profiles : SCTK_CONFIG_MAX_PROFILES = %d.",SCTK_CONFIG_MAX_PROFILES);
				}
				//move to next one
				profile = xmlNextElementSibling(profile);
			}
		}
	}
}

/*******************  FUNCTION  *********************/
/** @TODO doc **/
void sctk_config_sources_select_profiles_in_file(struct sctk_config_sources * config_sources,struct sctk_config_source_xml * source)
{
	//vars
	xmlNodePtr mappings;
	xmlNodePtr mapping;

	//error
	assert(config_sources != NULL);
	assert(source != NULL);

	//search "mappings" balise
	mappings = sctk_libxml_find_child_node(source->root_node,SCKT_CONFIG_XML_NODE_MAPPINGS);
	if (mappings != NULL)
	{
		//errors
		assert(mappings->type == XML_ELEMENT_NODE);
		assert(xmlStrcmp(mappings->name,SCKT_CONFIG_XML_NODE_MAPPINGS) == 0);

		//loop on mappings to extract names
		mapping = xmlFirstElementChild(mappings);
		while (mapping != NULL)
		{
			sctk_config_sources_select_profiles_in_mapping(config_sources,source,mapping);
			mapping = xmlNextElementSibling(mapping);
		}
	}
}

/*******************  FUNCTION  *********************/
/**
 * Find the profile node corresponding to the given name.
 * @param source Define the XML source in which to search the node.
 * @param name Define the name of the profile to find.
 * @return Return a pointer to the requested node if found, NULL otherwise.
**/
xmlNodePtr sctk_config_sources_find_profile_node(struct sctk_config_source_xml * source,const xmlChar * name)
{
	//vars
	xmlNodePtr profiles;
	xmlNodePtr profile = NULL;
	xmlChar * profile_name;

	//errors
	assert(name != NULL);

	//trivial, if no document, nothing to search
	if (source->document == NULL)
		return NULL;

	//find node "profiles"
	profiles = sctk_libxml_find_child_node(source->root_node,SCKT_CONFIG_XML_NODE_PROFILES);
	if (profiles == NULL)
		return NULL;

	//loop on childs ("profile" nodes)
	profile = xmlFirstElementChild(profiles);
	while (profile != NULL)
	{
		profile_name = sctk_libxml_find_child_node_content(profile,BAD_CAST("name"));
		if (profile_name != NULL && xmlStrcmp(name,profile_name) == 0)
		{
			printf("DEBUG : ok find node for %s in %p.\n",name,source);
			free(profile_name);
			break;
		} else {
			free(profile_name);
		}
		profile = xmlNextElementSibling(profile);
	}

	return profile;
}

/*******************  FUNCTION  *********************/
/**
 * Insert nodes by ensuring unicity. It already present, done nothing.
 * @param config_sources Define the configuration sources to use.
 * @param node Define the profile node to insert.
**/
void sctk_config_sources_insert_profile_node(struct sctk_config_sources * config_sources,xmlNodePtr node)
{
	//vars
	int i;
	bool present = false;

	//trivial cas
	if (node == NULL)
		return;

	//errors
	assert(xmlStrcmp(node->name,SCKT_CONFIG_XML_NODE_PROFILE) == 0 );

	//search to check if already in list
	for (i = 0 ; i < config_sources->cnt_profile_nodes ; ++i)
		if (config_sources->profile_nodes[i] == node)
			present = true;

	//if not present, can insert
	if ( ! present )
		config_sources->profile_nodes[config_sources->cnt_profile_nodes++] = node;
}

/*******************  FUNCTION  *********************/
/**
 * Search the profile node correspondig to the given profile name in all XML files. All found profiles
 * are added to the node list in priority order.
 * @param config_sources Define the configuration sources to use.
 * @param name Define the name of the profile to find.
 * @TODO Can replace this function by a simple loop if xml files are in a list instead of being members of a struct.
**/
void sctk_config_sources_select_profile_nodes(struct sctk_config_sources * config_sources,const xmlChar * name)
{
	//vars
	xmlNodePtr node;
	bool find_once = false;

	//errors
	assert(config_sources != NULL);
	assert(name != NULL);

	//find system
	node = sctk_config_sources_find_profile_node(&config_sources->system,name);
	if (node != NULL)
	{
		find_once = true;
		sctk_config_sources_insert_profile_node(config_sources,node);
	}

	//find user
	node = sctk_config_sources_find_profile_node(&config_sources->user,name);
	if (node != NULL)
	{
		find_once = true;
		sctk_config_sources_insert_profile_node(config_sources,node);
	}

	//find application
	node = sctk_config_sources_find_profile_node(&config_sources->application,name);
	if (node != NULL)
	{
		find_once = true;
		sctk_config_sources_insert_profile_node(config_sources,node);
	}

	//warning
	/** @TODO To discuss, this may be an error. **/
	if (!find_once)
		warning("Can't find requested profile %s in configuration files.\n",name);

	//error
	assume(config_sources->cnt_profile_nodes < SCTK_CONFIG_MAX_PROFILES,"Reach maximum number of profile : SCTK_CONFIG_MAX_PROFILES = %d.",SCTK_CONFIG_MAX_PROFILES);
}

/*******************  FUNCTION  *********************/
/**
 * Function used to find the DOM nodes of profiles to map. It fill the array profile_nodes
 * by using the list of names from profile_names. It ensure that a node is present only one time
 * and sort them in priority order.
 * After this call, we can safetely fill the C struct by following the order to support
 * option overriding between config files.
 * @param config_sources Define the configuration sources to use.
**/
void sctk_config_sources_select_profiles_nodes(struct sctk_config_sources * config_sources)
{
	//vars
	unsigned int i;

	//errors
	assert(config_sources != NULL);

	//search special one : default
	sctk_config_sources_select_profile_nodes(config_sources,BAD_CAST("default"));

	//loop on names to get profile related xml nodes
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		sctk_config_sources_select_profile_nodes(config_sources,config_sources->profile_names[i]);
}

/*******************  FUNCTION  *********************/
/**
 * This function ensure the selection of all profiles to load from XML files.
 * It will sort them on priority orider in profile_nodes member.
 * @param config_sources Define the configuration sources to use.
**/
void sctk_config_sources_select_profiles(struct sctk_config_sources * config_sources)
{
	//errors
	assert(config_sources != NULL);

	//select in all sources
	sctk_config_sources_select_profiles_in_file(config_sources,&config_sources->system);
	sctk_config_sources_select_profiles_in_file(config_sources,&config_sources->user);
	if (config_sources->application.root_node != NULL)
		sctk_config_sources_select_profiles_in_file(config_sources,&config_sources->application);

	//select all profile nodes
	sctk_config_sources_select_profiles_nodes(config_sources);
}

/*******************  FUNCTION  *********************/
/**
 * Function used to open a particular XML file and select the DOM root node for future usage.
 * It will abort on errors.
 * @param source Define the struct to setup while openning the file.
 * @param filename Define the XML file to open.
**/
void sctk_config_source_xml_open(struct sctk_config_source_xml * source,const char * filename,enum sctk_config_open_error_level level)
{
	//errors
	assert(source != NULL);
	assert(filename != NULL);

	//setup default
	source->document = NULL;
	source->root_node = NULL;

	//open file
	source->document = xmlParseFile(filename);
	if (source->document == NULL)
	{
		switch(level)
		{
			case SCTK_CONFIG_OPEN_WARNING:
				warning("Cannot open XML file : %s\n",filename);
				return;
			case SCTK_CONFIG_OPEN_ERROR:
				fatal("Cannot open XML file : %s\n",filename);
				return;
			case SCTK_CONFIG_OPEN_SILENT:
				return;
		}
	}

	//get root node
	source->root_node = xmlDocGetRootElement(source->document);
	assume (source->root_node != NULL,"Config file is empty : %s\n",filename);

	//check root node name
	assume (xmlStrcmp(source->root_node->name,SCKT_CONFIG_XML_NODE_MPC) == 0,"Bad root node name %s in config file : %s\n",source->root_node->name,filename);
}

/*******************  FUNCTION  *********************/
/**
 * Function used to open and prepare all config sources. It will also select all wanted profiles. After
 * this call, all sources are ready to be mapped to C struct.
 * The function will obort on failure.
 * @param config_sources Define the structure to init.
 * @param application_config_file Define the application config filename if any. Use NULL for none.
**/
void sctk_config_sources_open(struct sctk_config_sources * config_sources, const char * application_config_file)
{
	//vars
	#warning Allocate dynamically
	char user_file[1024];

	//errors
	assert(config_sources != NULL);

	//set to default
	memset(config_sources,0,sizeof(*config_sources));

	//calc user file path
	sprintf(user_file,"%s/.mpc/config.xml",getenv("HOME"));

	//open system config
	sctk_config_source_xml_open(&config_sources->system,SCTK_INSTALL_PREFIX "/share/mpc/system.xml",SCTK_CONFIG_OPEN_WARNING);
	sctk_config_source_xml_open(&config_sources->user,user_file,SCTK_CONFIG_OPEN_SILENT);
	if (application_config_file != NULL && application_config_file[0] != '\0')
		sctk_config_source_xml_open(&config_sources->application,application_config_file,SCTK_CONFIG_OPEN_ERROR);

	//find profiles to use and sort them depeding on priority
	sctk_config_sources_select_profiles(config_sources);
}

/*******************  FUNCTION  *********************/
/**
 * Close all XML files and free dynamic memories pointed by config_sources.
 * @param config_sources Define the structure to close and cleanup.
**/
void sctk_config_sources_close(struct sctk_config_sources * config_sources)
{
	//vars
	int i;

	//errors
	assert(config_sources != NULL);

	//close XML documents
	xmlFreeDoc(config_sources->system.document);
	xmlFreeDoc(config_sources->user.document);
	if (config_sources->application.document != NULL)
		xmlFreeDoc(config_sources->application.document);

	//free selected names (as xmlNodeGetContent done copies)
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		free(config_sources->profile_names[i]);

	//reset to default
	memset(config_sources,0,sizeof(*config_sources));
}
