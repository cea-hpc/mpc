# include "mpc_omp_task_scheduling.h"

# include <assert.h>
# include <fcntl.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>

/***************** FILE FUNCTIONS ***********************/
static inline int
mkdir_if_not_exists(char * path)
{
    struct stat sb;
    if (mkdir(path, S_IRWXU) == -1)
    {
        if (stat(path, &sb) == -1 || !S_ISDIR(sb.st_mode))
        {
            return -1;
        }
    }
    return 0;
}

static inline int
mkdir_p(char * directory)
{
    assert(directory);
    assert(*directory);

    char * buffer = strdup(directory);
    char * tmp = buffer;
    while (*tmp)
    {
        if (*tmp == '/')
        {
            *tmp = 0;
            if (mkdir_if_not_exists(buffer) == -1) return -1;
            *tmp = '/';
        }
        ++tmp;
    }
    if (mkdir_if_not_exists(buffer)) goto err;

    free(buffer);
    return 0;

err:
    free(buffer);
    return -1;
}

int
mpc_omp_task_scheduling_open(
    mpc_omp_task_scheduling_t * scheduling,
    char * directory,
    int nthreads,
    mpc_omp_task_scheduling_file_mode_t mode)
{
    assert(nthreads);
    scheduling->nthreads = nthreads;

    assert(directory);
    assert(*directory);
    size_t dirlen = strlen(directory);
    size_t max_process_dir_namelen = 16;
    size_t max_core_file_namelen = 16;
    size_t max_len = dirlen + max_process_dir_namelen + max_core_file_namelen + 1;
    char * buffer = (char *) malloc(max_len);

    if (mode == MPC_OMP_TASK_SCHEDULING_FILE_MODE_WRITER)
    {
        /* ensure directory exists */
        snprintf(buffer, max_len, "%s/%d", directory, pid);
        if (mkdir_p(buffer) == -1) return -1;

        /* create core file */
        for (int tid = 0 ; tid < nthreads ; ++tid)
        {
            snprintf(buffer, max_len, "%s/%d/%d", directory, pid, tid);
            file->fd = open(buffer, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            free(buffer);
            if (file->fd == -1) goto err;

            /* write file header */
            mpc_omp_task_scheduling_file_header_t header;
            header.magic = MPC_OMP_TASK_SCHEDULE_MAGIC;
            header.version = MPC_OMP_TASK_SCHEDULE_VERSION;
            if (write(file->fd, &header, sizeof(mpc_omp_task_scheduling_file_header_t)) == -1) goto err;
            file->cursor = 0;
        }
    }
    else
    {
        assert(mode == MPC_OMP_TASK_SCHEDULING_FILE_MODE_READER);
        file->cursor        = 0;
        file->cursor_max    = 0;
    }

    return 0;

err:
    /* TODO : release all */
    return -1;
}

static inline int
__task_scheduling_file_flush(mpc_omp_task_scheduling_file_t * file)
{
    if (file->cursor)
    {
        size_t count = file->cursor * sizeof(mpc_omp_task_scheduling_file_entry_t);
        if (write(file->fd, file->entries, count) != count) return -1;
        file->cursor = 0;
    }
    return 0;
}

/* write an entry into the scheduling: the task 'uid' runs into the thread 'tid_next' and was created or ran previously on thread 'tid_prev' */
int
mpc_omp_task_scheduling_next_write(
    mpc_omp_task_scheduling_t * scheduling,
    int tid_prev,
    int tid_next,
    int uid)
{
    assert(tid >= 0);
    assert(tid < scheduling->nthreads);
    assert(scheduling->mode == MPC_OMP_TASK_SCHEDULING_FILE_MODE_WRITER);

    mpc_omp_task_scheduling_file_t * file = scheduling->files + tid_prev;
    if (file->cursor == MPC_OMP_TASK_SCHEDULE_ENTRY_BUFF_SIZE)
    {
        if (__task_scheduling_file_flush(file) == -1) return NULL;
        file->cursor = 0;
    }

    mpc_omp_task_scheduling_file_entry_t * entry = file->entries + file->cursor;
    entry->uid = uid;
    entry->tid = tid_next;
}

/* read an entry from the scheduling */
int
mpc_omp_task_scheduling_next_read(
    mpc_omp_task_scheduling_t * scheduling,
    int tid_prev,
    int * tid_next
    int * uid)
{
    assert(tid >= 0);
    assert(tid < scheduling->nthreads);
    assert(scheduling->mode == MPC_OMP_TASK_SCHEDULING_FILE_MODE_READER);

    mpc_omp_task_scheduling_file_t * file = scheduling->files + tid;
    if (file->cursor == file->cursor_max)
    {
        int count = (int) (MPC_OMP_TASK_SCHEDULE_ENTRY_BUFF_SIZE * sizeof(mpc_omp_task_scheduling_file_entry_t));
        int r = read(file->fd, file->entries, count);
        if (r == -1)    return NULL;
        file->cursor_max = r / sizeof(mpc_omp_task_scheduling_file_entry_t);
        file->cursor = 0;
    }
    return file->entries + file->cursor++;
}

void
mpc_omp_task_scheduling_close(mpc_omp_task_scheduling_t * scheduling)
{
    /* FINAL FLUSH (> kamehameha) */
    for (int tid = 0 ; tid < scheduling->nthreads ; ++tid)
    {
        if (scheduling->mode == MPC_OMP_TASK_SCHEDULING_FILE_MODE_WRITER)
        {
            if (__task_scheduling_file_flush(file) == -1)
            {
                // TODO : handle error ?
            }
        }
        close(file->fd);
    }
}
