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
#include "sctk_runtime_config_debug.h"
#include "sctk_runtime_config_selectors.h"
#include "sctk_libxml_helper.h"
#include "sctk_runtime_config_sources.h"

/*******************  FUNCTION  *********************/
/**
 * Checks if a given profile name is not already in the profile name array
 * @param config_sources Define the XML source in which to search for previous profile.
 * @param candidate Define the name of the profile to find.
**/
static int sctk_runtime_config_sources_profile_name_is_unique( struct sctk_runtime_config_sources * config_sources, const xmlChar *candidate )
{
	assert( candidate != NULL );
	assert( config_sources != NULL );

	int i = 0;
	
	for( i = 0 ; i < config_sources->cnt_profile_names; i++ )
	{
		if( !xmlStrcmp( (const xmlChar *)&config_sources->profile_names[i], candidate ) )
			return 0;
	}
	
	return 1;
}



/** @TODO check that some assert need to be turned into assume (when checking node names...) **/

/*******************  FUNCTION  *********************/
/**
 * Loops through all mapping to store their associated profile name
 * @param config_sources Define the XML source in which to store the profile names.
 * @param mapping XML node containing the mappings ( as extracted in sctk_runtime_config_sources_select_profiles_in_file)
**/
void sctk_runtime_config_sources_select_profiles_in_mapping(struct sctk_runtime_config_sources * config_sources, xmlNodePtr mapping)
{
	//vars
	xmlNodePtr selectors;
	xmlNodePtr profile;

	//errors
	assert(config_sources != NULL);
	assert(mapping != NULL);
	assert(mapping->type == XML_ELEMENT_NODE);
	assert(xmlStrcmp(mapping->name,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPING) == 0);

	//check selectors
	selectors = sctk_libxml_find_child_node(mapping,SCTK_RUNTIME_CONFIG_XML_NODE_SELECTORS);
	if (selectors != NULL)
	{
		if (sctk_runtime_config_xml_selectors_check(selectors))
		{
			//get profiles
			profile = sctk_libxml_find_child_node(mapping,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILES);
			//down to first profile child
			profile = xmlFirstElementChild(profile);

			assert(profile != NULL);
			assert(xmlStrcmp(profile->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0);

			//loop on childs
			while (profile != NULL)
			{
				//if profile, ok add it
				if (xmlStrcmp(profile->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0)
				{

					xmlChar *profile_name = xmlNodeGetContent(profile);

					//Check if the profile name is unique and store its name if so
					if( sctk_runtime_config_sources_profile_name_is_unique( config_sources, profile_name ) )
					{
						config_sources->profile_names[config_sources->cnt_profile_names] = profile_name;
						printf("DEBUG : add profile %s\n",config_sources->profile_names[config_sources->cnt_profile_names]);
						config_sources->cnt_profile_names++;
						assume(config_sources->cnt_profile_names < SCTK_RUNTIME_CONFIG_MAX_PROFILES,
						       "Reach maximum number of profiles : SCTK_RUNTIME_CONFIG_MAX_PROFILES = %d.",SCTK_RUNTIME_CONFIG_MAX_PROFILES);
					} else {
						xmlFree( profile_name );
					}

				}
				//move to next one
				profile = xmlNextElementSibling(profile);
			}
		}
	}
}

/*******************  FUNCTION  *********************/
/** @TODO doc **/
void sctk_runtime_config_sources_select_profiles_in_file(struct sctk_runtime_config_sources * config_sources,struct sctk_runtime_config_source_xml * source)
{
	//vars
	xmlNodePtr mappings;
	xmlNodePtr mapping;

	//error
	assert(config_sources != NULL);
	assert(source != NULL);

	//search "mappings" balise
	mappings = sctk_libxml_find_child_node(source->root_node,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPINGS);
	if (mappings != NULL)
	{
		//errors
		assert(mappings->type == XML_ELEMENT_NODE);
		assert(xmlStrcmp(mappings->name,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPINGS) == 0);

		//loop on mappings to extract names
		mapping = xmlFirstElementChild(mappings);
		while (mapping != NULL)
		{
			sctk_runtime_config_sources_select_profiles_in_mapping(config_sources,mapping);
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
xmlNodePtr sctk_runtime_config_sources_find_profile_node(struct sctk_runtime_config_source_xml * source,const xmlChar * name)
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
	profiles = sctk_libxml_find_child_node(source->root_node,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILES);
	if (profiles == NULL)
		return NULL;

	//loop on childs ("profile" nodes)
	profile = xmlFirstElementChild(profiles);
	while (profile != NULL)
	{
		profile_name = sctk_libxml_find_child_node_content(profile,BAD_CAST("name"));
		if (profile_name != NULL )
		{
			if( xmlStrcmp(name,profile_name) == 0 )
			{
				printf("DEBUG : ok find node for %s in %p.\n",name,source);
				free(profile_name);
				break;
			}
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
void sctk_runtime_config_sources_insert_profile_node(struct sctk_runtime_config_sources * config_sources,xmlNodePtr node)
{
	//vars
	int i;
	bool present = false;

	//trivial cas
	if (node == NULL)
		return;

	//errors
	assert(xmlStrcmp(node->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0 );

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
void sctk_runtime_config_sources_select_profile_nodes(struct sctk_runtime_config_sources * config_sources,const xmlChar * name)
{
	//vars
	xmlNodePtr node;
	bool find_once = false;

	//errors
	assert(config_sources != NULL);
	assert(name != NULL);

	int i = 0;
	
	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ )
	{
		if( sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]) )
		{
			node = sctk_runtime_config_sources_find_profile_node(&config_sources->sources[i],name);
			if (node != NULL)
			{
				find_once = true;
				sctk_runtime_config_sources_insert_profile_node(config_sources,node);
			}
		}	
	}


	//warning
	/** @TODO To discuss, this may be an error. **/
	if (!find_once)
		warning("Can't find requested profile %s in configuration files.\n",name);

	//error
	assume(config_sources->cnt_profile_nodes < SCTK_RUNTIME_CONFIG_MAX_PROFILES,"Reach maximum number of profile : SCTK_RUNTIME_CONFIG_MAX_PROFILES = %d.",SCTK_RUNTIME_CONFIG_MAX_PROFILES);
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
void sctk_runtime_config_sources_select_profiles_nodes(struct sctk_runtime_config_sources * config_sources)
{
	//vars
	unsigned int i;

	//errors
	assert(config_sources != NULL);

	//search special one : default
	sctk_runtime_config_sources_select_profile_nodes(config_sources,BAD_CAST("default"));

	//loop on names to get profile related xml nodes
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		sctk_runtime_config_sources_select_profile_nodes(config_sources,config_sources->profile_names[i]);
}

/*******************  FUNCTION  *********************/
/**
 * This function ensure the selection of all profiles to load from XML files.
 * It will sort them on priority orider in profile_nodes member.
 * @param config_sources Define the configuration sources to use.
**/
void sctk_runtime_config_sources_select_profiles(struct sctk_runtime_config_sources * config_sources)
{
	//errors
	assert(config_sources != NULL);

	int i = 0;
	
	//select in all sources
	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ )
	{
		if(  sctk_runtime_config_source_xml_is_open(&config_sources->sources[i])  )
		{
			sctk_runtime_config_sources_select_profiles_in_file(config_sources,&config_sources->sources[i]);
		}	
	}


	//select all profile nodes
	sctk_runtime_config_sources_select_profiles_nodes(config_sources);
}

/*******************  FUNCTION  *********************/
/**
 * Function used to open a particular XML file and select the DOM root node for future usage.
 * It will abort on errors.
 * @param source Define the struct to setup while openning the file.
 * @param filename Define the XML file to open.
**/
void sctk_runtime_config_source_xml_open(struct sctk_runtime_config_source_xml * source,const char * filename,enum sctk_runtime_config_open_error_level level)
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
			case SCTK_RUNTIME_CONFIG_OPEN_WARNING:
				warning("Cannot open XML file : %s\n",filename);
				return;
			case SCTK_RUNTIME_CONFIG_OPEN_ERROR:
				fatal("Cannot open XML file : %s\n",filename);
				return;
			case SCTK_RUNTIME_CONFIG_OPEN_SILENT:
				return;
		}
	}

	//get root node
	source->root_node = xmlDocGetRootElement(source->document);
	assume (source->root_node != NULL,"Config file is empty : %s\n",filename);

	//check root node name
	assume (xmlStrcmp(source->root_node->name,SCTK_RUNTIME_CONFIG_XML_NODE_MPC) == 0,"Bad root node name %s in config file : %s\n",source->root_node->name,filename);
}


int sctk_runtime_config_source_xml_is_open( struct sctk_runtime_config_source_xml * source )
{
	return (source->document == NULL)?0:1;
}

/*******************  FUNCTION  *********************/
/**
 * Function used to open and prepare all config sources. It will also select all wanted profiles. After
 * this call, all sources are ready to be mapped to C struct.
 * The function will obort on failure.
 * @param config_sources Define the structure to init.
 * @param application_config_file Define the application config filename if any. Use NULL for none.
**/
void sctk_runtime_config_sources_open(struct sctk_runtime_config_sources * config_sources, const char * application_config_file)
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
	sctk_runtime_config_source_xml_open(&config_sources->sources[SCTK_RUNTIME_CONFIG_SYSTEM_LEVEL],SCTK_INSTALL_PREFIX "/share/mpc/system.xml",SCTK_RUNTIME_CONFIG_OPEN_WARNING);
	sctk_runtime_config_source_xml_open(&config_sources->sources[SCTK_RUNTIME_CONFIG_USER_LEVEL],user_file,SCTK_RUNTIME_CONFIG_OPEN_SILENT);

	if (application_config_file != NULL )
	{
		if( application_config_file[0] != '\0')
			sctk_runtime_config_source_xml_open(&config_sources->sources[SCTK_RUNTIME_CONFIG_APPLICATION_LEVEL],application_config_file,SCTK_RUNTIME_CONFIG_OPEN_ERROR);
	}

	//find profiles to use and sort them depeding on priority
	sctk_runtime_config_sources_select_profiles(config_sources);
}

/*******************  FUNCTION  *********************/
/**
 * Close all XML files and free dynamic memories pointed by config_sources.
 * @param config_sources Define the structure to close and cleanup.
**/
void sctk_runtime_config_sources_close(struct sctk_runtime_config_sources * config_sources)
{
	//vars
	int i;

	//errors
	assert(config_sources != NULL);

	//close XML documents
	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ )
	{
		if( sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]) )
		{
			xmlFreeDoc(config_sources->sources[i].document);
		}	
	}

	//free selected names (as xmlNodeGetContent done copies)
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		xmlFree(config_sources->profile_names[i]);

	//reset to default
	memset(config_sources,0,sizeof(*config_sources));
}
