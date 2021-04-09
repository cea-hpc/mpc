#include "loader.h"

#include "mpc_conf.h"
#include "utils.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

mpc_conf_config_loader_rights_t mpc_conf_config_loader_rights_parse(const char *str)
{
	/* Now check can mod */
	if(!strcmp(str, "none") )
	{
		return MPC_CONF_MOD_NONE;
	}
	else if(!strcmp(str, "system") )
	{
		return MPC_CONF_MOD_SYSTEM;
	}
	else if(!strcmp(str, "user") )
	{
		return MPC_CONF_MOD_USER;
	}
	else if(!strcmp(str, "both") )
	{
		return MPC_CONF_MOD_BOTH;
	}

	return MPC_CONF_MOD_ERROR;
}

mpc_conf_config_type_t *mpc_conf_config_loader_paths(char *conf_name,
                                                     char *system_prefix,
                                                     char *user_prefix,
                                                     char *can_create)
{
	if(!_utils_is_file_or_dir(system_prefix) )
	{
		_utils_verbose_output(0, "Can only set as config prefix files or directories (%s)\n", system_prefix);
		return NULL;
	}

	mpc_conf_config_loader_rights_t rights = mpc_conf_config_loader_rights_parse(can_create);

	if(rights == MPC_CONF_MOD_ERROR)
	{
		_utils_verbose_output(0, "Cannot parse rights for prefixes should be (none|system|user|both) (%s)\n", can_create);
		return NULL;
	}

	if(user_prefix)
	{
		if(!_utils_is_file_or_dir(user_prefix) )
		{
			_utils_verbose_output(0, "Can only set as config prefix files or directories (%s)\n", user_prefix);
			return NULL;
		}
	}
	else
	{
		user_prefix = "";
	}

	char * asystem_prefix = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
	snprintf(asystem_prefix, MPC_CONF_STRING_SIZE, system_prefix);

	char * auser_prefix = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
	snprintf(auser_prefix, MPC_CONF_STRING_SIZE, user_prefix);

	char * acan_create = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
	snprintf(acan_create, MPC_CONF_STRING_SIZE, can_create);


	char * amanual_prefix = malloc(sizeof(char) * MPC_CONF_STRING_SIZE);
	amanual_prefix[0] = '\0';


	mpc_conf_config_type_t *type = mpc_conf_config_type_init(conf_name, PARAM("cancreate",
	                                                                        acan_create,
	                                                                        MPC_CONF_STRING,
	                                                                        "Which config is allowed to create elements (between system and user/manual)"),
																			PARAM("system",
																			asystem_prefix,
																			MPC_CONF_STRING,
																			"System level configuration prefix/file"),
																			PARAM("user",
																			auser_prefix,
																			MPC_CONF_STRING,
																			"User level configuration prefix/file"),
																			PARAM("manual",
																			amanual_prefix,
																			MPC_CONF_STRING,
																			"Manual override configuration prefix/file"),
																		NULL);

	unsigned int i;

	for( i = 0 ; i < type->elem_count; i++)
	{
		 mpc_conf_config_type_elem_set_to_free(type->elems[i], 1);
	}


	mpc_conf_config_type_elem_t *can_create_elem = mpc_conf_config_type_get(type, "cancreate");
	mpc_conf_config_type_elem_set_locked(can_create_elem, 1);

	mpc_conf_config_type_elem_t *system_elem = mpc_conf_config_type_get(type, "system");
	mpc_conf_config_type_elem_set_locked(system_elem, 1);

	return type;
}

mpc_conf_config_type_t *mpc_conf_config_loader_search_path(char *conf_name)
{
	char config_search_path[512];

	snprintf(config_search_path, 512, "conf.paths.%s", conf_name);

	mpc_conf_config_type_elem_t *conf = mpc_conf_root_config_get(config_search_path);

	if(!conf)
	{
		_utils_verbose_output(1, "no search path for %s\n", conf_name);
		return NULL;
	}

	return mpc_conf_config_type_elem_get_inner(conf);
}

#define MAX_KEY_LENGTH    512

int mpc_conf_config_loader_push(char *conf_name, char *key, char *value, char *sep, int can_create)
{
	_utils_verbose_output(1, "PUSH: %s to %s with value %s (can_create %d)\n", conf_name, key, value, can_create);
	
	/* Are we prefixed with conf_name or not (if not add it) */
	char expanded_key[MAX_KEY_LENGTH];

	int len = (strlen(conf_name) + strlen(key) + strlen(sep) );

	if(len >= (MAX_KEY_LENGTH - 1) )
	{
		_utils_verbose_output(0, "cannot parse key is too long %s (%s)\n", conf_name, key);
		return 1;
	}

	snprintf(expanded_key, 512, "%s%s%s", conf_name, sep, key);


	/* Try to get the elem */
	mpc_conf_config_type_elem_t *elem = mpc_conf_root_config_get_sep(expanded_key, sep);

    mpc_conf_type_t type_to_set =MPC_CONF_TYPE_NONE;

	if(elem)
	{
		_utils_verbose_output(0, "%s: set %s = %s\n", conf_name, expanded_key, value);
		type_to_set = elem->type;
	}
	else
	{

		_utils_verbose_output(1, "PUSH NEW %s = %s\n", expanded_key, value);

		if(!can_create)
		{
			_utils_verbose_output(0, "%s: cannot set %s = %s (no such key)\n", conf_name, expanded_key, value);
			return 1;
		}

        mpc_conf_type_t infered_type = mpc_conf_type_infer_from_string(value);

        if(infered_type ==MPC_CONF_TYPE_NONE)
        {
            _utils_verbose_output(0, "%s: could not infer type for %s = %s\n", conf_name, expanded_key, value);
            return 1;
        }

        type_to_set = infered_type;

		/* Prepare new element storage if needed */

		ssize_t elem_size = mpc_conf_type_size(type_to_set);
		void * elem_data = (void *)value;

		if(0 < elem_size)
		{
			elem_data = malloc(elem_size);
			if(!elem_data)
			{
				perror("malloc");
				return 1;
			}

			memset(elem_data, 0, elem_size);
		}

		/* Now create the new element */
		elem = mpc_conf_root_config_set_sep(expanded_key,
											sep,
											type_to_set,
											elem_data,
											1);
		if(!elem)
		{
			_utils_verbose_output(0, "%s: could not insert new elem for %s = %s\n", conf_name, expanded_key, value);
			free(elem_data);
			return 1;
		}

		mpc_conf_config_type_elem_set_to_free(elem, 1);
	}

	_utils_verbose_output(1, "PARSING %s = %s as %s\n", expanded_key, value, mpc_conf_type_name(type_to_set));

    return mpc_conf_config_type_elem_set_from_string(elem, type_to_set, value);
}

static inline int ___mpc_conf_config_load_eq_list(char *conf_name, char *data, char *sep, int can_create, char *path, int line_start, char * extra_key_prefix)
{
	/* Now read A = B knowing that all keys are not prefixed withMPC_CONF_NAME */
	char *cur_line = data;
	char *nl       = _utils_split_next_newline(cur_line);

	int line = line_start;

	int ret = 0;

	do
	{
		char *trimmed_line = _utils_trim(cur_line);

		_utils_verbose_output(3, "%s:%d:%s\n", path, line, trimmed_line);

		if(strlen(trimmed_line) )
		{
			if(trimmed_line[0] != '#')
			{
				char *equal = strchr(trimmed_line, '=');

				if(!equal)
				{
					_utils_verbose_output(0, "%s:%d: mising '=' skipping '%s'\n", path, line, trimmed_line);
				}
				else
				{
					/* Can parse */
					*equal = '\0';

					char *key   = _utils_trim(trimmed_line);
					char *value = _utils_trim(equal + 1);

					if(!strlen(key) )
					{
						_utils_verbose_output(0, "%s:%d: empty key name\n", path);
					}
					else if(!strlen(value) )
					{
						_utils_verbose_output(0, "%s:%d: empty value\n", path);
					}
					else
					{
						if(extra_key_prefix)
						{
							/* We need to extend key with the extra prefix */
							char tmp_key[MAX_KEY_LENGTH];
							snprintf(tmp_key,MAX_KEY_LENGTH, "%s%s%s", extra_key_prefix, sep, key);
							key = tmp_key;
						}

						_utils_verbose_output(2, "%s:%d: %s = %s\n", path, line, key, value);

						if(_utils_strcasestarts_with(key, conf_name) )
						{
							/* Make sure there is not 'conf_name' in conf_name */
							char double_conf_name[512];
							snprintf(double_conf_name, 512, "%s.%s", conf_name, conf_name);

							if(!mpc_conf_root_config_get(double_conf_name) )
							{
								_utils_verbose_output(0, "%s:%d: it is not needed to prefix keys with '%s_'\n", path, line, conf_name);
							}
						}

						int loc_ret = mpc_conf_config_loader_push(conf_name, key, value, sep, can_create);
						ret |= loc_ret;

						if(loc_ret)
						{
							_utils_verbose_output(0, "%s:%d: %s = %s (error parsing)\n", path, line, key, value);
						}
					}
				}
			}
			else
			{
				_utils_verbose_output(2, "%s:%d: %s\n", path, line, trimmed_line);
			}
		}

		line++;

		if(!nl)
		{
			break;
		}
	
		cur_line = nl + 1;
		nl       = _utils_split_next_newline(cur_line);

	} while(1);

	return ret;
}

static inline int ___mpc_conf_config_load_file_conf(char *conf_name, char *path, int can_create)
{
	size_t fsize;
	char * file_content = _utils_read_whole_file(path, &fsize);

	if(!file_content)
	{
		return 1;
	}


	int ret = ___mpc_conf_config_load_eq_list(conf_name, file_content, "_", can_create, path, 1, NULL);


	free(file_content);

	return ret;
}

#define MAX_SECTION_COUNT 1024

static inline int ___mpc_conf_config_load_file_ini(char * conf_name, char * path, int can_create)
{
	size_t fsize;
	char * file_content = _utils_read_whole_file(path, &fsize);

	if(!file_content)
	{
		return 1;
	}

	/* We now want to extract ini sections if present */
	char * dup = strdup(file_content);
	if(!dup)
	{
		perror("strdup");
		free(file_content);
		return 1;
	}

	/* This will store section pointers and names */
	char * section_headers[MAX_SECTION_COUNT];
	/* This stores the offset of each section in dup */
	char * sections_starts[MAX_SECTION_COUNT];
	/* This store the line start number for each section */
	int line_starts[MAX_SECTION_COUNT];
	/* This is the number of sections found */
	int section_count = 0;

	/* We now parse line by line and determine section offsets */
	char * line_start = dup;

	int line_number = 1;

	while(line_start < (dup + fsize))
	{
		char * line_end = strchr(line_start, '\n');

		if(line_end)
		{
			*line_end = '\0';
		}
		else
		{
			break;
		}

		/* Now try  to detect a section header */
		char * line = _utils_trim(line_start);

		if(line[0] == '[' && line[strlen(line) - 1] == ']')
		{
			_utils_verbose_output(1, "%s:%d: found section header %s\n", path, line_number, line);
			/* Extract section header name */
			line[strlen(line) - 1] = '\0';
			section_headers[section_count] = strdup(_utils_trim(line + 1));
			sections_starts[section_count]=line_start;
			line_starts[section_count]=line_number;
			section_count++;
		}

		line_start = line_end + 1;

		line_number++;
	}

	int ret = 0;

	int i;

	/* No sections parse directly with conf_name as prefix and start from LINE 1 */
	if(section_count == 0)
	{
		_utils_verbose_output(1, "%s: no sections headers parsing whole file\n", path);
		ret = ___mpc_conf_config_load_eq_list(conf_name, file_content, ".", can_create, path, 1, NULL);
	}
	else
	{
		/* If first section does not start at line 1 parse the start */
		if(line_starts[0] != 1)
		{
			_utils_verbose_output(1, "%s: loading content before first section\n", path);
			size_t first_sec_offset = (sections_starts[0] - dup);
			file_content[first_sec_offset - 1] = '\0';
			
			_utils_trim(file_content);
			if(strlen(file_content))
			{
				if( ___mpc_conf_config_load_eq_list(conf_name, file_content, ".", can_create, path, 1, NULL) )
				{
					_utils_verbose_output(0, "%s: errors when loading content before first section\n", path);
				}
			}
		}

		/* We will need to generate prefixes for each sections and then parse them separately */
		for(i = 0 ; i < section_count ; i++)
		{
			size_t start_offset = (sections_starts[i] - dup);
			size_t end_offset = fsize;

			if(i < (section_count-1))
			{
				/* End just at next section note that we write \0 on header name */
				end_offset = (sections_starts[i + 1] - dup - 1);
			}

			file_content[end_offset] = '\0';

			char * section_start = file_content + start_offset;
			char * endm = strchr(section_start, ']');
			if(endm)
			{
				section_start = endm + 1;
			}

			_utils_trim(section_start);

			_utils_verbose_output(1, "%s: loading section [%s] %d :\n%s\n", path, section_headers[i],strlen(section_start), section_start);

			if(strlen(section_start))
			{
				if(___mpc_conf_config_load_eq_list(conf_name, section_start, ".", can_create, path, line_starts[i], section_headers[i]))
				{
					_utils_verbose_output(0, "%s: errors when loading content in [%s]\n", path, section_headers[i]);
				}
			}
		}
	}

	free(file_content);
	free(dup);

	for( i = 0 ; i  < section_count ; i++)
	{
		free(section_headers[i]);
	}

	return ret;
}

static inline int __mpc_conf_config_load_file(char *conf_name, char *path, int can_create)
{
	mpc_conf_output_type_t type = mpc_conf_output_type_infer_from_file(path);

	_utils_verbose_output(1, "loading %s for %s\n", path, conf_name);

	switch(type)
	{
		case MPC_CONF_FORMAT_CONF:
			return ___mpc_conf_config_load_file_conf(conf_name, path, can_create);
		case MPC_CONF_FORMAT_INI:
			return ___mpc_conf_config_load_file_ini(conf_name, path, can_create);
		default:
			_utils_verbose_output(0, "could not infer configuration format for %s \n", path);
			return 1;

			break;
	}

	return 1;
}

static inline int __mpc_conf_config_load_prefix(char *conf_name, char *path, int can_create)
{
	if(!_utils_is_file_or_dir(path) )
	{
		return 1;
	}

	int ret = 0;

	if(_utils_is_dir(path) )
	{
		struct dirent *elem = NULL;
		DIR *          dir  = opendir(path);

		if(!dir)
		{
			perror("opendir");
			return 1;
		}

		while( (elem = readdir(dir) ) != NULL)
		{
			if(!strcmp(elem->d_name, ".") || !strcmp(elem->d_name, "..") )
			{
				continue;
			}

			char cpath[1024];
			snprintf(cpath, 1024, "%s/%s", path, elem->d_name);

			if(!_utils_is_file(cpath) )
			{
				_utils_verbose_output(0, "when loading config for %s only regular files are expected for %s \n", conf_name, cpath);
				continue;
			}

			int lret = __mpc_conf_config_load_file(conf_name, cpath, can_create);

			if(lret)
			{
				_utils_verbose_output(0, "failed loading %s\n", cpath);
			}

			ret |= lret;
		}

		closedir(dir);
	}
	else
	{
		return __mpc_conf_config_load_file(conf_name, path, can_create);
	}

	return ret;
}

int mpc_conf_config_load(char *conf_name)
{
	mpc_conf_config_type_t *paths = mpc_conf_config_loader_search_path(conf_name);

	if(!paths)
	{
		_utils_verbose_output(1, "no search path for %s\n", conf_name);
		return 1;
	}

	mpc_conf_config_type_elem_t *erights = mpc_conf_config_type_get(paths, "cancreate");
	mpc_conf_config_type_elem_t *esystem = mpc_conf_config_type_get(paths, "system");
	mpc_conf_config_type_elem_t *euser   = mpc_conf_config_type_get(paths, "user");
	mpc_conf_config_type_elem_t *emanual   = mpc_conf_config_type_get(paths, "manual");


	if(!erights || !esystem || !euser)
	{
		_utils_verbose_output(1, "Incoherent search path for %s\n", conf_name);
		return 1;
	}

	const char *srights = mpc_conf_type_elem_get_as_string(erights);
	mpc_conf_config_loader_rights_t rights = mpc_conf_config_loader_rights_parse(srights);

	char *system = mpc_conf_type_elem_get_as_string(esystem);
	char *user   = mpc_conf_type_elem_get_as_string(euser);

	if(rights == MPC_CONF_MOD_ERROR)
	{
		_utils_verbose_output(1, "eights for can_create seem to be erroneous in %s\n", conf_name);
		return 1;
	}

	int ret = 0;

	if(__mpc_conf_config_load_prefix(conf_name, system, (rights == MPC_CONF_MOD_SYSTEM) || (rights == MPC_CONF_MOD_BOTH) ) )
	{
		ret |= _utils_verbose_output(0, "failed to load %s for %s\n", system, conf_name);
	}

	if(strlen(user) && _utils_is_file_or_dir(user) )
	{
		if(__mpc_conf_config_load_prefix(conf_name, user, (rights == MPC_CONF_MOD_USER) || (rights == MPC_CONF_MOD_BOTH) ) )
		{
			ret |= _utils_verbose_output(0, "failed to load part of the config in %s for %s\n", user, conf_name);
		}
	}


	/* Now load the manual prefix if present */
	char overrive_env_var[200];
	char * conf_name_upper = strdup(conf_name);
	_utils_upper_string(conf_name_upper);
	snprintf(overrive_env_var, 200, "CONF_PATHS_%s_MANUAL", conf_name_upper);
	free(conf_name_upper);

	/* We do a manual getenv as env parsing is done after this phase */
	char * manual_conf = getenv(overrive_env_var);

	/* This is for retrocompat when not invoking mpcrun */
	if(!manual_conf)
	{
		manual_conf = getenv("MPC_USER_CONFIG");
	}

	if(manual_conf)
	{
		/* We do a set as MPC_USER_CONFIG would not override automagically */
		char mconf[MPC_CONF_STRING_SIZE];
		snprintf(mconf, MPC_CONF_STRING_SIZE, manual_conf);
		mpc_conf_config_type_elem_set(emanual, MPC_CONF_STRING, mconf);

		/* Now try to load the manual config / prefix */

		if(__mpc_conf_config_load_prefix(conf_name, manual_conf, (rights == MPC_CONF_MOD_USER) || (rights == MPC_CONF_MOD_BOTH) ) )
		{
			ret |= _utils_verbose_output(0, "failed to load part of the config in %s for %s\n", manual_conf, conf_name);
		}
	}

	return ret;
}
