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

#ifndef SCTK_RUNTIME_CONFIG_SOURCES_H
#define SCTK_RUNTIME_CONFIG_SOURCES_H

/********************  HEADERS  *********************/
#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

/********************  MACRO  ***********************/
/**
 * Define the maximum number of profile to manage in lists.
 * @todo Maybe replace those static arrays by UTList elements.
**/
#define SCTK_RUNTIME_CONFIG_MAX_PROFILES 16

/*********************  CONSTS  *********************/
/** Tag name of the root node of XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_MPC BAD_CAST("mpc")
/** Tag name of profile node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_PROFILES BAD_CAST("profiles")
/** Tag name of profile node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE BAD_CAST("profile")
/** Tag name of mappings node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_MAPPINGS BAD_CAST("mappings")
/** Tag name of mapping node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_MAPPING BAD_CAST("mapping")
/** Tag name of selectors node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_SELECTORS BAD_CAST("selectors")
/** Tag name of modules node in XML document. **/
#define SCTK_RUNTIME_CONFIG_XML_NODE_MODULES BAD_CAST("modules")

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
/**
 * Define some warning levels for errors appening in configuration routines.
 * This is mostly for file not found at loading time depending on the level.
**/
enum sctk_runtime_config_open_error_level
{
	/** Only print a warning and continue. **/
	SCTK_RUNTIME_CONFIG_OPEN_WARNING,
	/** This is an error, print and exit. **/
	SCTK_RUNTIME_CONFIG_OPEN_ERROR,
	/** Do not display anything and continue. **/
	SCTK_RUNTIME_CONFIG_OPEN_SILENT
};

/*********************  ENUMS  *********************/
/**
 * Define all configuration levels. Loaded in direct order.
**/
enum sctk_xml_config_type
{
	/** System level XML configuration file identifier. **/
	SCTK_RUNTIME_CONFIG_SYSTEM_LEVEL,
	/** User level XML configuration file identifier. **/
	SCTK_RUNTIME_CONFIG_USER_LEVEL,
	/** Application level XML configuration file identifier. **/
	SCTK_RUNTIME_CONFIG_APPLICATION_LEVEL,
	/** Number of configuration types. **/
	SCTK_RUNTIME_CONFIG_LEVEL_COUNT
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
	struct sctk_runtime_config_source_xml sources[SCTK_RUNTIME_CONFIG_LEVEL_COUNT];
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
void sctk_runtime_config_sources_open(struct sctk_runtime_config_sources * config_sources);
void sctk_runtime_config_sources_close(struct sctk_runtime_config_sources * config_sources);

/*******************  FUNCTION  *********************/
//function to manage open operations of a specific xml file.
void sctk_runtime_config_source_xml_open(struct sctk_runtime_config_source_xml * source,const char * filename,enum sctk_runtime_config_open_error_level level);
bool sctk_runtime_config_source_xml_is_open( struct sctk_runtime_config_source_xml * source );

/*******************  FUNCTION  *********************/
//functions to manage selection of profiles in XML DOM tree
void sctk_runtime_config_sources_select_profiles(struct sctk_runtime_config_sources * config_sources);
void sctk_runtime_config_sources_select_profiles_nodes(struct sctk_runtime_config_sources * config_sources);
void sctk_runtime_config_sources_select_profile_nodes(struct sctk_runtime_config_sources * config_sources,const xmlChar * name);
void sctk_runtime_config_sources_insert_profile_node(struct sctk_runtime_config_sources * config_sources,xmlNodePtr node);
xmlNodePtr sctk_runtime_config_sources_find_profile_node(struct sctk_runtime_config_source_xml * source,const xmlChar * name);
void sctk_runtime_config_sources_select_profiles_in_file(struct sctk_runtime_config_sources * config_sources,struct sctk_runtime_config_source_xml * source);
void sctk_runtime_config_sources_select_profiles_in_mapping(struct sctk_runtime_config_sources * config_sources, xmlNodePtr mapping);

/*******************  FUNCTION  *********************/
//some helper functions
const char * sctk_runtime_config_get_env_or_value(const char * env_name,const char * fallback_value);

#endif //SCTK_RUNTIME_CONFIG_FILES_H
