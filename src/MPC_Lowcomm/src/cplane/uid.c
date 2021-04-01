#include "uid.h"

#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>

#include <mpc_common_debug.h>

#include "set.h"

static char  __set_dir[512]        = { 0 };
static short __set_dir_initialized = 0;


#define __UID_WIDTH     20
#define __RANK_WIDTH    12

typedef union
{
	mpc_lowcomm_set_uid_t value;
	struct
	{
		unsigned int uid  : __UID_WIDTH;
		unsigned int rank : __RANK_WIDTH;
	}                      members;
}__set_uid_t;

void __set_descriptor_fill(struct _mpc_lowcomm_uid_descriptor_s *sd, char *uri, mpc_lowcomm_set_uid_t set_uid)
{
	memset(sd, 0, sizeof(struct _mpc_lowcomm_uid_descriptor_s));
	time_t atime = time(NULL);

	srand(atime);

	sd->cookie  = rand();
	sd->set_uid = set_uid;
	snprintf(sd->leader_uri, MPC_LOWCOMM_PEER_URI_SIZE, uri);
}

int _mpc_lowcomm_uid_descriptor_save(struct _mpc_lowcomm_uid_descriptor_s *sd, char *path)
{
	FILE *desc = fopen(path, "w");

	if(!desc)
	{
		/* Failed */
		return -1;
	}

	int ret = fwrite(sd, sizeof(struct _mpc_lowcomm_uid_descriptor_s), 1, desc);

	if(ret != 1)
	{
		/* failed */
		fclose(desc);
		return -1;
	}

	fclose(desc);

	return 0;
}

int _mpc_lowcomm_uid_descriptor_load(struct _mpc_lowcomm_uid_descriptor_s *sd, char *path)
{
	FILE *desc = fopen(path, "r");

	if(!desc)
	{
		/* Failed */
		return -1;
	}

	int ret = fread(sd, sizeof(struct _mpc_lowcomm_uid_descriptor_s), 1, desc);

	if(ret != 1)
	{
		/* failed */
		perror("fread");
		fclose(desc);
		return -1;
	}

	fclose(desc);

	return 0;
}

static inline int __check_for_dir_or_create(char *path)
{
	struct stat statbuf;

	if(stat(path, &statbuf) < 0)
	{
		/* Try to create */
		if(mkdir(path, 0700) < 0)
		{
			perror("mkdir");
			return -1;
		}

		return 0;
	}

	if(S_ISDIR(statbuf.st_mode) )
	{
		/* All ok it is a directory */
		return 0;
	}

	return -1;
}

static inline int __is_file(char *path)
{
	struct stat statbuf;

	if(stat(path, &statbuf) < 0)
	{
		/* Something bad happened */
		return 0;
	}

	if(S_ISREG(statbuf.st_mode) )
	{
		/* All ok it is a file */
		return 1;
	}

	return 0;
}

static inline char *__get_set_directory(void)
{
	if(__set_dir_initialized)
	{
		assert(0 < strlen(__set_dir) );
		return __set_dir;
	}

	uid_t my_uid = getuid();

	struct passwd *pwd_info = getpwuid(my_uid);

	if(!pwd_info)
	{
		mpc_common_debug_fatal("Could not retrieve user's home directory");
	}

	snprintf(__set_dir, 512, "%s/.mpc/", pwd_info->pw_dir);

	if(__check_for_dir_or_create(__set_dir) < 0)
	{
		mpc_common_debug_fatal("Could not create %s directory", __set_dir);
	}

	snprintf(__set_dir, 512, "%s/.mpc/sets", pwd_info->pw_dir);

	if(__check_for_dir_or_create(__set_dir) < 0)
	{
		mpc_common_debug_fatal("Could not create %s directory", __set_dir);
	}

	__set_dir_initialized = 1;

	return __set_dir;
}

static inline char *__set_path(char *buff, int len, uint32_t set_uid)
{
	char *set_path = __get_set_directory();

	/* First check that the UID matches */
	__set_uid_t sid;

	sid.value = set_uid;

	uid_t uid = getuid();

	if(sid.members.uid != uid)
	{
		mpc_common_debug_warning("UID mismatch in set name %d != %d", sid.members.uid, uid);
	}

	uint32_t r = sid.members.rank;
	snprintf(buff, len, "%s/%u", set_path, r);


	//mpc_common_debug_error("PATH : %s", buff);

	return buff;
}

static inline int32_t __uid_valid(char *uid, int *valid)
{
	*valid = 0;
	int len = strlen(uid);

	int i;

	for(i = 0; i < len; i++)
	{
		if(!isdigit(uid[i]) )
		{
			return 0;
		}
	}

	int id = atoi(uid);

	/* Now check that it fits in UID range */
	if( ( (1 << __RANK_WIDTH) - 1) <= id)
	{
		return 0;
	}

	*valid = 1;

	return id;
}

int _mpc_lowcomm_uid_scan(int (*uid_cb)(uint32_t uid, char *path) )
{
	if(!uid_cb)
	{
		return -1;
	}

	char *set_path = __get_set_directory();

	DIR *          set_dir = opendir(set_path);
	struct dirent *file    = NULL;

	if(!set_dir)
	{
		perror("opendir");
		mpc_common_debug_fatal("Failed scanning the set directory");
	}

	char path[1024];

	while( (file = readdir(set_dir) ) ) // if dp is null, there's no more content to read
	{
		if(!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..") )
		{
			continue;
		}

		snprintf(path, 1024, "%s/%s", set_path, file->d_name);

		/* Check for uid and sanity */
		int     is_valid = 0;
		int32_t id       = __uid_valid(file->d_name, &is_valid);

		if(!is_valid)
		{
			mpc_common_debug_warning("Set list %s seems invalid", path);
			continue;
		}


		int ret = (uid_cb)(id, path);

		if(ret != 0)
		{
			/* Done */
			break;
		}
	}

	closedir(set_dir);

	return 0;
}

static inline int32_t __get_free_rank_slot(__set_uid_t *set_id, char *uri)
{
	uid_t my_uid = getuid();

	if( ( (1 << __UID_WIDTH) - 1) <= my_uid)
	{
		mpc_common_debug_warning("UID overflows the %d bits allowed using truncated value", __UID_WIDTH);
	}

	set_id->members.uid = my_uid;

	char *   set_path    = __get_set_directory();
	uint32_t max_rank_id = (1 << __RANK_WIDTH) - 1;

	uint32_t i;

	/* To limit concurency we spread
	 * on the id space first */
	pid_t pid = getpid();

	srand(pid);
	int offset = rand();

	for(i = 0; i < max_rank_id; i++)
	{
		uint32_t target_id = 1 + (offset + i) % (max_rank_id - 1);

		char path[1024];
		snprintf(path, 1024, "%s/%d", set_path, target_id);

		if(__is_file(path) )
		{
			continue;
		}

		set_id->members.rank = target_id;

		/* Try to acquire the file */
		struct _mpc_lowcomm_uid_descriptor_s sd;
		__set_descriptor_fill(&sd, uri, set_id->value);

		if(_mpc_lowcomm_uid_descriptor_save(&sd, path) < 0)
		{
			/* Failed to write */
			continue;
		}

		/* Now check for race */
		struct _mpc_lowcomm_uid_descriptor_s loadsd;
		if(_mpc_lowcomm_uid_descriptor_load(&loadsd, path) < 0)
		{
			continue;
		}

		if(loadsd.cookie != sd.cookie)
		{
			/* We had a race skip */
			continue;
		}
		else
		{
			break;
		}
	}

	return 0;
}

uint32_t _mpc_lowcomm_uid_new(char *rank_zero_uri)
{
	__set_uid_t set_id = { 0 };

	assume(sizeof(set_id.members) == sizeof(set_id.value) );


	if(__get_free_rank_slot(&set_id, rank_zero_uri) != 0)
	{
		mpc_common_debug_fatal("Could not allocate a slot in UID set directory");
	}

	//mpc_common_debug_warning("==> %u (%u, %u)", set_id.value, set_id.members.uid, set_id.members.rank);

	return set_id.value;
}

int _mpc_lowcomm_uid_clear(uint64_t uid)
{
	char pathbuf[517];

	__set_path(pathbuf, 517, uid);

	mpc_common_debug_error("CLEARRR ==> %s", pathbuf);

	if(__is_file(pathbuf) )
	{
		unlink(pathbuf);
		return 0;
	}

	return 1;
}
