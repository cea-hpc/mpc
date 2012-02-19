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

#ifndef SCTK_RUNTIME_CONFIG_FILES_H
#define SCTK_RUNTIME_CONFIG_FILES_H

/********************  HEADERS  *********************/
#include <libxml/tree.h>
#include <libxml/parser.h>

/********************  MACRO  ***********************/
/**
 * Define the maximum number of profile to manage in lists.
 * @todo Maybe replace those static arrays by UTList elements.
**/
#define SCTK_RUNTIME_CONFIG_MAX_PROFILES 16

/*********************  STRUCT  *********************/
/**
 * Define an XML source which is a root node and an XML document.
 * @brief Handler for an XML configuration file.
**/
struct sctk_runtime_config_source_xml
{
	/** XML document manage by libxml. **/
	xmlDocPtr document;
	/** DOM root node in XML document. **/
	xmlNodePtr root_node;
};

/*********************  ENUMS  *********************/
enum sctk_runtime_config_open_error_level
{
	SCTK_CONFIG_OPEN_WARNING,
	SCTK_CONFIG_OPEN_ERROR,
	SCTK_CONFIG_OPEN_SILENT
};

enum sctk_xml_config_type
{
	/** System level XML configuration file identifier. **/
	SCTK_CONFIG_SYSTEM_LEVEL,
	/** User level XML configuration file identifier. **/
	SCTK_CONFIG_USER_LEVEL,
	/** Application level XML configuration file identifier. **/
	SCTK_CONFIG_APPLICATION_LEVEL,
	/** Number of configuration types. **/
	SCTK_CONFIG_LEVEL_COUNT
};


/*********************  STRUCT  *********************/
/**
 * Structure to aggregate pointers to all source of configuration.
 * It will be used to fill the configuration C structure at end point.
 * @brief Aggregate handler of all source of configuration.
**/
struct sctk_runtime_config_sources
{
	/** Array storing the config source associated with each level **/
	struct sctk_runtime_config_source_xml sources[SCTK_CONFIG_LEVEL_COUNT];
	/** List of selected XML profile names. **/
	xmlChar * profile_names[SCTK_RUNTIME_CONFIG_MAX_PROFILES];
	/** List of selected XML profile nodes. **/
	xmlNodePtr profile_nodes[SCTK_RUNTIME_CONFIG_MAX_PROFILES];
	/** Number of selected XML profile names. **/
	unsigned int cnt_profile_names;
	/** Number of selected XML profile nodes. **/
	unsigned int cnt_profile_nodes;
};

/*******************  FUNCTION  *********************/
//functions to manage the sctk_runtime_config_sources structure
void sctk_runtime_config_sources_open(struct sctk_runtime_config_sources * config_sources, const char * application_config_file);
void sctk_runtime_config_sources_close(struct sctk_runtime_config_sources * config_sources);

/*******************  FUNCTION  *********************/
//function to manage open operations of a specific xml file.
void sctk_runtime_config_source_xml_open(struct sctk_runtime_config_source_xml * source,const char * filename,enum sctk_runtime_config_open_error_level level);
int sctk_runtime_config_source_xml_is_open( struct sctk_runtime_config_source_xml * source );

/*******************  FUNCTION  *********************/
//functions to manage selection of profiles in XML DOM tree
void sctk_runtime_config_sources_select_profiles(struct sctk_runtime_config_sources * config_sources);
void sctk_runtime_config_sources_select_profiles_nodes(struct sctk_runtime_config_sources * config_sources);
void sctk_runtime_config_sources_select_profile_nodes(struct sctk_runtime_config_sources * config_sources,const xmlChar * name);
void sctk_runtime_config_sources_insert_profile_node(struct sctk_runtime_config_sources * config_sources,xmlNodePtr node);
xmlNodePtr sctk_runtime_config_sources_find_profile_node(struct sctk_runtime_config_source_xml * source,const xmlChar * name);
void sctk_runtime_config_sources_select_profiles_in_file(struct sctk_runtime_config_sources * config_sources,struct sctk_runtime_config_source_xml * source);
void sctk_runtime_config_sources_select_profiles_in_mapping(struct sctk_runtime_config_sources * config_sources, xmlNodePtr mapping);

#endif //SCTK_CONFIG_FILES_H
