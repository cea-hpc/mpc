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
#include <mpc_common_types.h>
#include <mpc_config.h>
#include <sctk_debug.h>
#include <string.h>
#include <errno.h>
#include <libxml/xmlschemas.h>
#include "runtime_config_selectors.h"
#include "runtime_config_sources.h"
#include "libxml_helper.h"

/* optional headers */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /*HAVE_SYS_STAT_H*/

/********************************** GLOBALS *********************************/
/**
 * This is a constant on exported symbol, so the user can redefine it in application source
 * code if wanted.
**/
const char * sctk_runtime_config_default_user_file_path = NULL;

/********************************* FUNCTION *********************************/
/**
 * Checks if a given profile name is not already in the profile name array
 * @param config_sources Define the XML source in which to search for previous profile.
 * @param candidate Define the name of the profile to find.
**/
static int sctk_runtime_config_sources_profile_name_is_unique( struct sctk_runtime_config_sources * config_sources, const xmlChar *candidate )
{
	unsigned int i = 0;

	/* errors */
	assert( candidate != NULL );
	assert( config_sources != NULL );

	/* loop on profiles */
	for( i = 0 ; i < config_sources->cnt_profile_names; i++ ) {
		if( !xmlStrcmp( (const xmlChar *)&config_sources->profile_names[i], candidate ) )
			return 0;
	}

	return 1;
}

/** @TODO check that some assert need to be turned into assume (when checking node names...) **/

/********************************* FUNCTION *********************************/
void sctk_runtime_config_sources_select_profile_name(struct sctk_runtime_config_sources * config_sources,xmlChar * profile_name)
{
	/* errors */
	assert(profile_name != NULL);

	/* Check if the profile name is unique and store its name if true */
	if( sctk_runtime_config_sources_profile_name_is_unique( config_sources, profile_name ) ) {
		config_sources->profile_names[config_sources->cnt_profile_names] = profile_name;
		sctk_nodebug("MPC_Config : Add profile %s\n",config_sources->profile_names[config_sources->cnt_profile_names]);
		config_sources->cnt_profile_names++;
		assume_m(config_sources->cnt_profile_names < SCTK_RUNTIME_CONFIG_MAX_PROFILES,
		         "Reach maximum number of profiles : SCTK_RUNTIME_CONFIG_MAX_PROFILES = %d.",SCTK_RUNTIME_CONFIG_MAX_PROFILES);
	} else {
		xmlFree( profile_name );
	}
}

/********************************* FUNCTION *********************************/
/**
 * Loops through all mapping to store their associated profile name
 * @param config_sources Define the XML source in which to store the profile names.
 * @param mapping XML node containing the mappings ( as extracted in sctk_runtime_config_sources_select_profiles_in_file)
**/
void sctk_runtime_config_sources_select_profiles_in_mapping(struct sctk_runtime_config_sources * config_sources, xmlNodePtr mapping)
{
	/* vars */
	xmlNodePtr selectors;
	xmlNodePtr profile;

	/* errors */
	assert(config_sources != NULL);
	assert(mapping != NULL);
	assert(mapping->type == XML_ELEMENT_NODE);
	assert(xmlStrcmp(mapping->name,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPING) == 0);

	/* check selectors */
	selectors = sctk_libxml_find_child_node(mapping,SCTK_RUNTIME_CONFIG_XML_NODE_SELECTORS);
	if (selectors != NULL) {
		if (sctk_runtime_config_xml_selectors_check(selectors)) {
			/* get profiles */
			profile = sctk_libxml_find_child_node(mapping,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILES);
			/* down to first profile child */
			profile = xmlFirstElementChild(profile);

			assert(profile != NULL);
			assert(xmlStrcmp(profile->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0);

			/* loop on childs */
			while (profile != NULL) {
				/* if profile, ok add it */
				if (xmlStrcmp(profile->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0) {
					xmlChar *profile_name = xmlNodeGetContent(profile);
					sctk_runtime_config_sources_select_profile_name(config_sources,profile_name);
				}
				/* move to next one */
				profile = xmlNextElementSibling(profile);
			}
		}
	}
}

/********************************* FUNCTION *********************************/
/**
 * Select the profiles manually requested by user via --profile= option on mpcrun or via MPC_USER_PROFILES
 * It's preferable to call this after sctk_runtime_config_sources_select_profiles_in_mapping() to ensure to override automatic
 * profile selection with user one.
 * @param config_sources Define the XML source in which to store the profile names.
**/
void sctk_runtime_config_sources_select_user_profiles(struct sctk_runtime_config_sources * config_sources)
{
	/* vars */
	const char * env_var = NULL;
	char * profiles = NULL;
	char * current = NULL;
	char * end = NULL;

	/* errors */
	assert(config_sources != NULL);

	/* read env var MPC_USER_CONFIG_PROFILES */
	env_var = sctk_runtime_config_get_env_or_value("MPC_USER_PROFILES",NULL);

	/* load if */
	if (env_var != NULL) {
		/* clone to rewrite it */
		profiles = strdup(env_var);

		current = profiles;
		end = profiles;

		/* loop on the while chain and split on ',' */
		while (*current != '\0') {
			/* move unitil next ',' */
			while (*end != '\0' && *end != ',')
				end++;

			/* if get ',' => replace by '\0' and move next */
			if (*end == ',') {
				*end = '\0';
				end++;
			}

			/* mark the profile for use */
			if (*current != '\0')
				sctk_runtime_config_sources_select_profile_name(config_sources,BAD_CAST(strdup(current)));

			/* move start to previous end */
			current = end;
		}
	}
}

/********************************* FUNCTION *********************************/
/** @TODO doc **/
void sctk_runtime_config_sources_select_profiles_in_file(struct sctk_runtime_config_sources * config_sources,struct sctk_runtime_config_source_xml * source)
{
	/* vars */
	xmlNodePtr mappings;
	xmlNodePtr mapping;

	/* error */
	assert(config_sources != NULL);
	assert(source != NULL);

	/* search "mappings" balise */
	mappings = sctk_libxml_find_child_node(source->root_node,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPINGS);
	if (mappings != NULL) {
		/* errors */
		assert(mappings->type == XML_ELEMENT_NODE);
		assert(xmlStrcmp(mappings->name,SCTK_RUNTIME_CONFIG_XML_NODE_MAPPINGS) == 0);

		/* loop on mappings to extract names */
		mapping = xmlFirstElementChild(mappings);
		while (mapping != NULL) {
			sctk_runtime_config_sources_select_profiles_in_mapping(config_sources,mapping);
			mapping = xmlNextElementSibling(mapping);
		}
	}
}

/********************************* FUNCTION *********************************/
/**
 * Find the profile node corresponding to the given name.
 * @param source Define the XML source in which to search the node.
 * @param name Define the name of the profile to find.
 * @return Return a pointer to the requested node if found, NULL otherwise.
**/
xmlNodePtr sctk_runtime_config_sources_find_profile_node(struct sctk_runtime_config_source_xml * source,const xmlChar * name)
{
	/* vars */
	xmlNodePtr profiles;
	xmlNodePtr profile = NULL;
	xmlChar * profile_name;

	/* errors */
	assert(name != NULL);

	/* trivial, if no document, nothing to search */
	if (source->document == NULL)
		return NULL;

	/* find node "profiles" */
	profiles = sctk_libxml_find_child_node(source->root_node,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILES);
	if (profiles == NULL)
		return NULL;

	/* loop on childs ("profile" nodes) */
	profile = xmlFirstElementChild(profiles);
	while (profile != NULL) {
		profile_name = sctk_libxml_find_child_node_content(profile,BAD_CAST("name"));
		if (profile_name != NULL ) {
			if( xmlStrcmp(name,profile_name) == 0 ) {
				sctk_nodebug("MPC_Config : ok find node for %s in %p.\n",name,source);
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

/********************************* FUNCTION *********************************/
/**
 * Insert nodes by ensuring unicity. It already present, done nothing.
 * @param config_sources Define the configuration sources to use.
 * @param node Define the profile node to insert.
**/
void sctk_runtime_config_sources_insert_profile_node(struct sctk_runtime_config_sources * config_sources,xmlNodePtr node)
{
	/* vars */
	unsigned int i;
	bool present = false;

	/* trivial cas */
	if (node == NULL)
		return;

	/* errors */
	assert(xmlStrcmp(node->name,SCTK_RUNTIME_CONFIG_XML_NODE_PROFILE) == 0 );

	/* search to check if already in list */
	for (i = 0 ; i < config_sources->cnt_profile_nodes ; ++i)
		if (config_sources->profile_nodes[i] == node)
			present = true;

	/* if not present, can insert */
	if ( ! present )
		config_sources->profile_nodes[config_sources->cnt_profile_nodes++] = node;
}

/********************************* FUNCTION *********************************/
/**
 * Search the profile node correspondig to the given profile name in all XML files. All found profiles
 * are added to the node list in priority order.
 * @param config_sources Define the configuration sources to use.
 * @param name Define the name of the profile to find.
 * @TODO Can replace this function by a simple loop if xml files are in a list instead of being members of a struct.
**/
void sctk_runtime_config_sources_select_profile_nodes(struct sctk_runtime_config_sources * config_sources,const xmlChar * name)
{
	/* vars */
	xmlNodePtr node;
	bool find_once = false;
	int i = 0;

	/* errors */
	assert(config_sources != NULL);
	assert(name != NULL);

	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ ) {
		if( sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]) ) {
			node = sctk_runtime_config_sources_find_profile_node(&config_sources->sources[i],name);
			if (node != NULL) {
				find_once = true;
				sctk_runtime_config_sources_insert_profile_node(config_sources,node);
			}
		}
	}

	/* warning */
	/** @TODO To discuss, this may be a warning. **/
	assume_m(find_once,"Can't find requested profile %s in configuration files.",name);

	/* error */
	assume_m(config_sources->cnt_profile_nodes < SCTK_RUNTIME_CONFIG_MAX_PROFILES,"Reach maximum number of profile : SCTK_RUNTIME_CONFIG_MAX_PROFILES = %d.",SCTK_RUNTIME_CONFIG_MAX_PROFILES);
}

/********************************* FUNCTION *********************************/
/**
 * Function used to find the DOM nodes of profiles to map. It fill the array profile_nodes
 * by using the list of names from profile_names. It ensure that a node is present only one time
 * and sort them in priority order.
 * After this call, we can safely fill the C struct by following the order to support
 * option overriding between config files.
 * @param config_sources Define the configuration sources to use.
**/
void sctk_runtime_config_sources_select_profiles_nodes(struct sctk_runtime_config_sources * config_sources)
{
	/* vars */
	unsigned int i;

	/* errors */
	assert(config_sources != NULL);

	/* search special one : default */
	sctk_runtime_config_sources_select_profile_nodes(config_sources,BAD_CAST("default"));

	/* loop on names to get profile related xml nodes */
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		sctk_runtime_config_sources_select_profile_nodes(config_sources,config_sources->profile_names[i]);
}

/********************************* FUNCTION *********************************/
/**
 * This function ensure the selection of all profiles to load from XML files.
 * It will sort them on priority order in profile_nodes member.
 * @param config_sources Define the configuration sources to use.
**/
void sctk_runtime_config_sources_select_profiles(struct sctk_runtime_config_sources * config_sources)
{
	int i = 0;

	/* errors */
	assert(config_sources != NULL);

	/* select names from all sources <mappings> entries */
	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ )
		if(  sctk_runtime_config_source_xml_is_open(&config_sources->sources[i])  )
			sctk_runtime_config_sources_select_profiles_in_file(config_sources,&config_sources->sources[i]);

	/* select names from command line */
	sctk_runtime_config_sources_select_user_profiles(config_sources);

	/* select all profile nodes from name list */
	sctk_runtime_config_sources_select_profiles_nodes(config_sources);
}

/********************************* FUNCTION *********************************/
/**
 * Check if the file exist before trying to use it with libxml.
 * @param filename Name of the file to check.
 * @return true if it exists, false otherwise.
**/
bool sckt_runtime_config_file_exist(const char * filename)
{
	bool exist = false;
	#ifdef HAVE_SYS_STAT_H
	struct stat value;
	#else
	FILE * fp;
	#endif

	/*Check file presence, fstat may be smartter for filesystem,
	  but for portatbility, fallback on fopen/fclose.*/
	#ifdef HAVE_SYS_STAT_H
	exist = (stat(filename, &value) != -1);
	#else
	fp = fopen(filename,"r");
	exist = (fp != NULL);
	if (fp != NULL)
		fclose(fp);
	#endif

	return exist;
}

/********************************* FUNCTION *********************************/
/**
 * Function used to open a particular XML file and select the DOM root node for future usage.
 * It will abort on errors.
 * @param source Define the struct to setup while openning the file.
 * @param filename Define the XML file to open. File loading can be disable by using "none" as filename.
**/
void sctk_runtime_config_source_xml_open(struct sctk_runtime_config_source_xml * source,const char * filename,enum sctk_runtime_config_open_error_level level)
{
	/* errors */
	assert(source != NULL);
	assert(filename != NULL);

	/* setup default */
	source->document = NULL;
	source->root_node = NULL;

	/* skip if filename is "none" */
	if (strcmp(filename,"none") == 0)
		return;

	/* check if file exist */
	if ( ! sckt_runtime_config_file_exist(filename) ) {
		switch(level) {
			case SCTK_RUNTIME_CONFIG_OPEN_WARNING:
				sctk_warning("Cannot open XML file : %s",filename);
				return;
			case SCTK_RUNTIME_CONFIG_OPEN_ERROR:
				sctk_fatal("Cannot open XML file : %s",filename);
				return;
			case SCTK_RUNTIME_CONFIG_OPEN_SILENT:
				return;
		}
	}

	/* load it */
	source->document = xmlParseFile(filename);
	assume( source->document != NULL );

	/* get root node */
	source->root_node = xmlDocGetRootElement(source->document);
	assume_m (source->root_node != NULL,"Config file is empty : %s",filename);

	/* check root node name */
	assume_m (xmlStrcmp(source->root_node->name,SCTK_RUNTIME_CONFIG_XML_NODE_MPC) == 0,"Bad root node name %s in config file : %s\n",source->root_node->name,filename);
}

/********************************* FUNCTION *********************************/
/**
 * Check if the configuration source if open.
 * @param source Define the configuration source to check.
 * @return true if yes, false otherwise.
**/
bool sctk_runtime_config_source_xml_is_open( struct sctk_runtime_config_source_xml * source )
{
	return (source->document != NULL);
}

/********************************* FUNCTION *********************************/
/**
 * Use the value of an environment variable or a given default value if not defined or if empty.
 * @param env_name The name of the environment variable to use with getenv()
 * @param fallback_value Define the value to return if the environment variable is empty.
 * @return The value for the requested environment variable.
**/
const char * sctk_runtime_config_get_env_or_value(const char * env_name,const char * fallback_value)
{
	/* vars */
	const char * res;

	/* errors */
	assert(env_name != NULL);

	/* select the good value */
	res = getenv(env_name);
	if (res == NULL)
		res = fallback_value;
	else if (*res == '\0')
		res= fallback_value;

	/* return */
	return res;
}

/********************************* FUNCTION *********************************/
/**
 * Validate all XML sources with the given XML schema file.
 * This function can be skiped on cluster nodes if launch with mpcrun as we already checked the config
 * files. It permit to not load the XSD file on every nodes and avoid to spam with error messages.
 * Some help used for implementation : http://julp.lescigales.org/c/libxml2/validation.php
 * @param config_sources Define the structure containing the list of XML source to validate.
 * @param xml_shema_path Define the path to XML schema file to use for validation.
 * @return true if all XML sources are valid, false otherwise.
**/
bool sctk_runtime_config_sources_validate(struct sctk_runtime_config_sources * config_sources,const char * xml_shema_path)
{
	/* vars */
	int i;
	bool status = true;
	bool has_one = false;
	xmlSchemaPtr schema;
	xmlSchemaValidCtxtPtr vctxt;
	xmlSchemaParserCtxtPtr pctxt;

	/* errors */
	assert(config_sources != NULL);
	assert(xml_shema_path != NULL);

	/* check if has one file at least */
	for ( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++)
		has_one |= sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]);

	/* if not can skip */
	if ( ! has_one )
		return true;

	/* Open XML schema file */
	pctxt = xmlSchemaNewParserCtxt(xml_shema_path);
	assume_m(pctxt != NULL,"Fail to open XML schema file to validate config : %s.",xml_shema_path);

	/* Parse the schema */
	schema = xmlSchemaParse(pctxt);
	xmlSchemaFreeParserCtxt(pctxt);
	assume_m(schema != NULL,"Fail to parse the XML schema file to validate config : %s.",xml_shema_path);

	/* Create validation context */
	if ((vctxt = xmlSchemaNewValidCtxt(schema)) == NULL) {
		xmlSchemaFree(schema);
		sctk_fatal("Fail to create validation context from XML schema file : %s.",xml_shema_path);
	}

	/* Create validation output system */
	xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

	/* Validation */
	for ( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++)
		if (sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]))
			status &= (xmlSchemaValidateDoc(vctxt, config_sources->sources[i].document) == 0);

	/* Free the schema */
	xmlSchemaFree(schema);
	xmlSchemaFreeValidCtxt(vctxt);

	return status;
}

/********************************* FUNCTION *********************************/
/**
 * Function used to open and prepare all config sources. It will also select all wanted profiles. After
 * this call, all sources are ready to be mapped to C struct.
 * The function will obort on failure.
 * @param config_sources Define the structure to init.
**/
void sctk_runtime_config_sources_open(struct sctk_runtime_config_sources * config_sources)
{
	/* vars */
	bool status;
	const char * config_silent;
	const char * config_system;
	const char * config_schema;
	const char * config_user;

	const char * mpc_rprefix = sctk_runtime_config_get_env_or_value("MPC_PREFIX", MPC_PREFIX_PATH);

	char * def_sys_path = NULL;
	def_sys_path = (char *)malloc(1024*sizeof(char));
	def_sys_path[0]='\0'; 
	strcat(def_sys_path, mpc_rprefix);
	strcat(def_sys_path, "/share/mpc/config.xml");

	char * def_sys_path_fallback = NULL;
	def_sys_path_fallback = (char *)malloc(1024*sizeof(char));
	def_sys_path_fallback[0]='\0';
	strcat(def_sys_path_fallback, mpc_rprefix);
	strcat(def_sys_path_fallback, "/share/mpc/config.xml.example");

	char * def_schema_path = NULL;
	def_schema_path = (char *)malloc(1024*sizeof(char));
	def_schema_path[0]='\0';
	strcat(def_schema_path, mpc_rprefix);
	strcat(def_schema_path, "/share/mpc/mpc-config.xsd");

	/* errors */
	assert(config_sources != NULL);

	/* set to default */
	memset(config_sources,0,sizeof(*config_sources));

	/* try to load from env */
	config_silent = sctk_runtime_config_get_env_or_value("MPC_CONFIG_SILENT","0");
	config_schema = sctk_runtime_config_get_env_or_value("MPC_CONFIG_SCHEMA",def_schema_path);
	config_system = sctk_runtime_config_get_env_or_value("MPC_SYSTEM_CONFIG",def_sys_path);
	config_user   = sctk_runtime_config_get_env_or_value("MPC_USER_CONFIG",sctk_runtime_config_default_user_file_path);


	/*
	If system file is default and if it didn't exist, fall back onto the .example one.
	This is a trick, but it permit to not create system config.xml at install, so avoid
	trouble in case of future update of the lib and if config file format evolve.
	The problem stay present only if the admin creates system config.xml and modifies it.
	But in that case it's acceptable.
	*/
	if (strcmp(config_system,def_sys_path) == 0 && !sckt_runtime_config_file_exist(config_system))
		config_system = def_sys_path_fallback;

	/* open system and user config */
	sctk_runtime_config_source_xml_open(&config_sources->sources[SCTK_RUNTIME_CONFIG_SYSTEM_LEVEL],config_system,SCTK_RUNTIME_CONFIG_OPEN_WARNING);

	/* if get user config as parameter from --config. */
	if (config_user != NULL )
		if( config_user[0] != '\0')
			sctk_runtime_config_source_xml_open(&config_sources->sources[SCTK_RUNTIME_CONFIG_USER_LEVEL],config_user,SCTK_RUNTIME_CONFIG_OPEN_ERROR);

	/*
	validate XML format with schema
	but skip it if already done by in mpcrun, it avoid to open the .xsd file on each node for nothing
	and avoid to spam the error stream with multiple instance of error messages.
	To disable mpcrun need to define CONFIG_SILENT="1" (as for xml loading errors)
	If launch without mpcrun, it's sequential so need to run it which is okay by default.
	*/
	if (strcmp(config_silent,"1") != 0) {
		status = sctk_runtime_config_sources_validate(config_sources,config_schema);
		assume_m(status,"Fail to validate the given configurations files, please check previous error lines for more details.");
	}

	/* find profiles to use and sort them depeding on priority */
	sctk_runtime_config_sources_select_profiles(config_sources);
}

/********************************* FUNCTION *********************************/
/**
 * Close all XML files and free dynamic memories pointed by config_sources.
 * @param config_sources Define the structure to close and cleanup.
**/
void sctk_runtime_config_sources_close(struct sctk_runtime_config_sources * config_sources)
{
	/* vars */
	unsigned int i;

	/* errors */
	assert(config_sources != NULL);

	/* close XML documents */
	for( i = 0 ; i < SCTK_RUNTIME_CONFIG_LEVEL_COUNT ; i++ )
		if( sctk_runtime_config_source_xml_is_open(&config_sources->sources[i]) )
			xmlFreeDoc(config_sources->sources[i].document);

	/* free selected names (as xmlNodeGetContent done copies) */
	for (i = 0 ; i < config_sources->cnt_profile_names ; i++)
		xmlFree(config_sources->profile_names[i]);

	/* reset to default */
	memset(config_sources,0,sizeof(*config_sources));
}
