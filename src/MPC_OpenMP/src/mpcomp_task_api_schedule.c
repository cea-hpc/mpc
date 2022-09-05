# ifndef __MPC_OMP_TASK_SCHEDULE_H__
#  define __MPC_OMP_TASK_SCHEDULE_H__

/**
 * This file describe various data structures to export/import an
 * entire application « task scheduling »
 *
 * -------------------------------------------
 *                  VERSION 1
 * -------------------------------------------
 *  This version assumes that
 *      - all the tasks are created by the same threads and in the same order
 *      - the scheduling files entries are sorted by their order of execution on the associated thread
 */

# define MPC_OMP_TASK_SCHEDULE_MAGIC            0x7461736b
# define MPC_OMP_TASK_SCHEDULE_VERSION          1
# define MPC_OMP_TASK_SCHEDULE_ENTRY_BUFF_SIZE  1024


/* a scheduling file header */
typedef struct
{
    /* magic */
    int magic;

    /* file version */
    int version;

} mpc_omp_task_scheduling_file_header_t;

/* scheduling events */
typedef enum
{
    MPC_OMP_TASK_SCHEDULING_EVENT_CREATE,
    MPC_OMP_TASK_SCHEDULING_EVENT_SCHEDULE,
    MPC_OMP_TASK_SCHEDULING_EVENT_MAX
} mpc_omp_task_scheduling_event_t;

/* a scheduling file entry */
typedef struct
{
    /* event (0 = create, 1 = schedule) */
    unsigned event : 1;

    /* the task ID */
    unsigned uid : 31;

} mpc_omp_task_scheduling_file_entry_t;

/* a scheduling file writer */
typedef struct
{
    /* the file descriptor */
    int fd;

    /* the read/write buffer */
    mpc_omp_task_scheduling_file_entry_t buffer[MPC_OMP_TASK_SCHEDULE_ENTRY_BUFF_SIZE];

    /* the cursor position in the buffer */
    unsigned short cursor;

} mpc_omp_task_scheduling_file_writer_t;

/* a scheduling file reader */
typedef mpc_omp_task_scheduling_file_reader_t mpc_omp_task_scheduling_file_writer_t;

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

/***************** WRITER FUNCTIONS ********************/
int
mpc_omp_task_scheduling_file_writer_open(
    mpc_omp_task_scheduling_t * scheduling,
    mpc_omp_task_scheduling_file_writer_t * writer,
    int pid,
    int tid)
{
    size_t dirlen = strlen(scheduling->directory);
    size_t max_process_dir_namelen = 16;
    size_t max_core_file_namelen = 16;
    size_t max_len = dirlen + max_process_dir_namelen + max_core_file_namelen + 1;
    char * buffer = (char *) malloc(max_len);

    /* ensure directory exists */
    snprintf(buffer, max_len, "%s/%d", directory, core.pid);
    if (mkdir_p(buffer) == -1) return -1;

    /* create core file */
    snprintf(buffer, max_len, "%s/%d/%d", directory, pid, tid);
    writer->fd = open(buffer, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    free(buffer);
    if (writer->fd == -1) return -1;

    /* write file header */
    mpc_omp_task_scheduling_file_header_t header;
    header.magic = MPC_OMP_TASK_SCHEDULE_MAGIC;
    header.version = MPC_OMP_TASK_SCHEDULE_VERSION;
    if (write(writer->fd, &header, sizeof(mpc_omp_task_scheduling_file_header_t)) == -1)
    {
        close(writer->fd);
        return -1;
    }

    writer->cursor = 0;
    return 0;
}

static inline int
__task_scheduling_file_writer_flush(mpc_omp_task_scheduling_file_writer_t * writer)
{
    if (writer->cursor == 0) return ;
    size_t count = writer->cursor * sizeof(mpc_omp_task_scheduling_file_entry_t);
    if (write(writer->fd, writer->buffer, count) != count) return -1;
    writer->cursor = 0;
    return 0;
}

int
mpc_omp_task_scheduling_file_writer_close(mpc_omp_task_scheduling_file_writer_t * writer)
{
    if (__task_scheduling_file_writer_flush(writer) == -1) return -1;
    close(reader->fd);
}

mpc_omp_task_scheduling_file_entry_t *
mpc_omp_task_scheduling_file_writer_next_entry(mpc_omp_task_scheduling_file_writer_t * writer)
{
    if (reader->cursor == MPC_OMP_TASK_SCHEDULE_ENTRY_BUFF_SIZE)
    {
        if (__task_scheduling_file_writer_flush(writer) == -1) return NULL;
        reader->cursor = 0;
    }
    return writer->buff + reader->cursor++;
}

/***************** READER FUNCTIONS ********************/
void mpc_omp_task_scheduling_file_reader_open(
    mpc_omp_task_scheduling_t * scheduling,
    mpc_omp_task_scheduling_file_reader_t * reader,
    int nthread);

void mpc_omp_task_scheduling_file_reader_close(mpc_omp_task_scheduling_file_reader_t * reader);

void mpc_omp_task_scheduling_file_reader_next_entry(
    mpc_omp_task_scheduling_file_reader_t * reader,
    mpc_omp_task_scheduling_file_entry_t * entry);

# endif /* __MPC_OMP_TASK_SCHEDULE_H__ */
