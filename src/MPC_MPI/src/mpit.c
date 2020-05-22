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
/* #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # */
/* #                                                                      # */
/* ######################################################################## */

#include "mpit_internal.h"

#include "comm_lib.h"
#include "mpc_thread.h"
#include "mpc_common_debug.h"

#include <mpc_common_flags.h>

#include <sctk_alloc.h>

#include <string.h>

#include <mpc_runtime_config.h>

/************************************************************************/
/* INTERNAL MPI_T Init and Finalize                                     */
/************************************************************************/

static void __set_config_bind_trampoline()
{
        sctk_runtime_config_mpit_bind_variable_set_trampoline(mpc_MPI_T_bind_variable);
}


void mpc_MPI_T_init() __attribute__((constructor));

void mpc_MPI_T_init()
{
        MPC_INIT_CALL_ONLY_ONCE

        mpc_common_init_callback_register("Before Config INIT",
                                          "Initialize MPIT variable defaults",
                                          MPI_T_cvars_array_init, 1);

        mpc_common_init_callback_register("Before Config INIT",
                                          "Initialize MPIT bind trampoline",
                                          __set_config_bind_trampoline, 2);
}

volatile int ___mpi_t_is_running = 0;
static mpc_common_spinlock_t mpit_init_lock = SCTK_SPINLOCK_INITIALIZER;

int mpc_MPI_T_init_thread(__UNUSED__ int required, int *provided) {
  if (!provided) {
    return MPI_ERR_ARG;
  }

  mpc_common_spinlock_lock(&mpit_init_lock);

  if (___mpi_t_is_running == 0) {
    MPI_T_pvars_array_init();
    MPI_T_cvars_array_init();
    MPC_T_session_array_init();
  }

  ___mpi_t_is_running++;

  mpc_common_spinlock_unlock(&mpit_init_lock);

  *provided = MPI_THREAD_MULTIPLE;

  return MPI_SUCCESS;
}

int mpc_MPI_T_finalize(void) {
  mpc_common_spinlock_lock(&mpit_init_lock);

  ___mpi_t_is_running--;

  if (___mpi_t_is_running == 0) {
    MPI_T_pvars_array_release();
    MPI_T_cvars_array_release();
    MPC_T_session_array_release();
  }

  mpc_common_spinlock_unlock(&mpit_init_lock);

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Enum                                                  */
/************************************************************************/

/** This is the internal enum data-structure */

int MPC_T_enum_init(struct MPC_T_enum *en, char *name, int num_entries,
                    char *names[], int *values) {
  en->name = strdup(name);
  en->number_of_entries = num_entries;

  en->entry_names = sctk_calloc(num_entries, sizeof(char *));

  assume(en->entry_names != NULL);

  en->entry_values = sctk_calloc(num_entries, sizeof(int));

  assume(en->entry_values != NULL);

  int i;

  for (i = 0; i < num_entries; ++i) {
    en->entry_names[i] = strdup(names[i]);
  }

  if (values) {
    for (i = 0; i < num_entries; ++i) {
      en->entry_values[i] = values[i];
    }
  }

  return MPI_SUCCESS;
}

int MPC_T_enum_release(struct MPC_T_enum *en) {
  free(en->name);
  en->name = NULL;

  int i;

  for (i = 0; i < en->number_of_entries; ++i) {
    free(en->entry_names[i]);
    en->entry_values[i] = -1;
  }

  sctk_free(en->entry_names);
  en->entry_names = NULL;

  sctk_free(en->entry_values);
  en->entry_values = NULL;

  en->number_of_entries = -1;

  return MPI_SUCCESS;
}

int mpc_MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name,
                            int *name_len) {
  struct MPC_T_enum *menu = (struct MPC_T_enum *)enumtype;

  if (!num || !name || !name_len) {
    return MPI_ERR_ARG;
  }

  *num = menu->number_of_entries;
  *((char **)name) = menu->name;
  *name_len = strlen(menu->name);

  return MPI_SUCCESS;
}

int mpc_MPI_T_enum_get_item(MPI_T_enum enumtype, int index, int *value,
                            char *name, int *name_len) {
  struct MPC_T_enum *menu = (struct MPC_T_enum *)enumtype;

  if (!value || !name || !name_len) {
    return MPI_ERR_ARG;
  }

  if (menu->number_of_entries <= index) {
    return MPI_T_ERR_INVALID_ITEM;
  }

  *value = menu->entry_values[index];
  *((char **)name) = menu->entry_names[index];
  *name_len = strlen(*((char **)name));

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T storage                                               */
/************************************************************************/

int MPC_T_data_init(struct MPC_T_data *tdata, MPI_Datatype type, int continuous,
                    int readonly) {
  memset(tdata, 0, sizeof(struct MPC_T_data));

  tdata->refcount = 0;
  tdata->type = type;
  mpc_common_spinlock_init(&tdata->lock, 0);
  tdata->enabled = continuous;
  tdata->continuous = continuous;
  tdata->readonly = readonly;
  tdata->pcontent = NULL;

  return MPI_SUCCESS;
}

int MPC_T_data_release(struct MPC_T_data *tdata) {
  return MPC_T_data_init(tdata, MPI_DATATYPE_NULL, 0, 0);
}

size_t MPC_T_data_get_size(struct MPC_T_data *tdata) {
  size_t ret = 0;

  switch (tdata->type) {
  case MPI_INT:
    ret = sizeof(int);
    break;
  case MPI_UNSIGNED:
    ret = sizeof(unsigned int);
    break;
  case MPI_UNSIGNED_LONG:
    ret = sizeof(unsigned long int);
    break;
  case MPI_UNSIGNED_LONG_LONG:
    ret = sizeof(unsigned long long);
    break;
  case MPI_DOUBLE:
    ret = sizeof(double);
    break;
  case MPI_COUNT:
    ret = sizeof(MPI_Count);
    break;
  case MPI_CHAR:
    ret = sizeof(char);
    break;
  default:
    mpc_common_debug_fatal("No such datatype for MPI_T_DATA");
  }

  return ret;
}

static inline void *MPC_T_data_get_ptr_size(struct MPC_T_data *tdata,
                                            size_t *size) {
  void *alias = tdata->pcontent;

  switch (tdata->type) {
  case MPI_INT:
    *size = sizeof(int);
    return (void *)alias ? alias : &tdata->content.i;
    break;
  case MPI_UNSIGNED:
    *size = sizeof(unsigned int);
    return (void *)alias ? alias : &tdata->content.ui;
    break;
  case MPI_UNSIGNED_LONG:
    *size = sizeof(unsigned long int);
    return (void *)alias ? alias : &tdata->content.lu;
    break;
  case MPI_UNSIGNED_LONG_LONG:
    *size = sizeof(unsigned long long);
    return (void *)alias ? alias : &tdata->content.llu;
    break;
  case MPI_DOUBLE:
    *size = sizeof(double);
    return (void *)alias ? alias : &tdata->content.d;
    break;
  case MPI_COUNT:
    *size = sizeof(MPI_Count);
    return (void *)alias ? alias : &tdata->content.count;
    break;
  case MPI_CHAR:
    *size = sizeof(char);
    return (void *)alias ? alias : &tdata->content.c;
    break;
  default:
    mpc_common_debug_fatal("No such datatype for MPI_T_DATA");
    return 0;
  }
}

int MPC_T_data_write(struct MPC_T_data *tdata, void *data) {
  if (tdata->enabled == 0) {
    return MPI_ERR_ARG;
  }

  size_t size;
  void *p = MPC_T_data_get_ptr_size(tdata, &size);

  memcpy(p, data, size);

  return MPI_SUCCESS;
}

int MPC_T_data_read(struct MPC_T_data *tdata, void *data) {
  size_t size;
  void *p = MPC_T_data_get_ptr_size(tdata, &size);

  mpc_common_nodebug("READ is from %p in %p for size %d", p, tdata, size);

  memcpy(data, p, size);

  return MPI_SUCCESS;
}

int MPC_T_data_alias(struct MPC_T_data *tdata, void *data) {
  mpc_common_nodebug("NOW SET ALLIAS TO %p in %p", data, tdata);
  tdata->pcontent = data;
  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS                               */
/************************************************************************/

/** This is where the CVARS (meta) are being stored */

static mpc_common_spinlock_t cvar_array_lock = SCTK_SPINLOCK_INITIALIZER;
static int cvar_array_initialized = 0;
static struct MPC_T_cvars_array __cvar_array;

#define CATEGORIES(cat, parent, desc)
#define CVAR(name, verbosity, type, desc, bind, scope, cat)                    \
  mpit_concat(MPI_T_CAT_, cat),
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)

/** This is the pvars categories */
static const MPC_T_category_t __mpi_t_cvars_categories[MPI_T_CVAR_COUNT] = {
#include "sctk_mpit.h"
};

#undef CATEGORIES
#undef CVAR
#undef PVAR

/** NOTE : This function is generated by the config */
int MPI_T_cvar_fill_info_from_config() {
#define CATEGORIES(cat, parent, desc)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)
#define CVAR(name, verbosity, type, desc, bind, scope, cat)                    \
  MPC_T_cvar_register_on_slot(name, #name, verbosity, type, NULL, desc, bind,  \
                              scope);
#include "sctk_mpit.h"

#undef CATEGORIES
#undef CVAR
#undef PVAR
  return 0;
}

void MPI_T_cvars_array_init() {
  mpc_common_spinlock_lock(&cvar_array_lock);

  if (cvar_array_initialized == 0) {
    cvar_array_initialized = 1;
  } else {
    mpc_common_spinlock_unlock(&cvar_array_lock);
    return;
  }

  __cvar_array.dyn_cvar_count = 0;
  __cvar_array.dyn_cvars = NULL;
  mpc_common_spinlock_init(&__cvar_array.lock, 0);

  MPI_T_cvar_fill_info_from_config();

  mpc_common_spinlock_unlock(&cvar_array_lock);
}

void MPI_T_cvars_array_release() {
  mpc_common_spinlock_lock(&cvar_array_lock);

  if (cvar_array_initialized == 1) {
    cvar_array_initialized = 0;
  } else {
    mpc_common_spinlock_unlock(&cvar_array_lock);
    return ;
  }

  __cvar_array.dyn_cvar_count = 0;
  sctk_free(__cvar_array.dyn_cvars);
  __cvar_array.dyn_cvars = NULL;

  mpc_common_spinlock_unlock(&cvar_array_lock);
}

struct MPC_T_cvar *MPI_T_cvars_array_get(MPC_T_cvar_t slot) {
  mpc_common_nodebug("GET SLOT %d over %d == %p", slot, MPI_T_CVAR_COUNT,
               &__cvar_array.st_vars[slot]);

  if (slot < MPI_T_CVAR_COUNT) {
    return &__cvar_array.st_vars[slot];
  } else {
    struct MPC_T_cvar *cv = NULL;

    mpc_common_spinlock_lock(&cvar_array_lock);

    if ((slot - MPI_T_CVAR_COUNT) < __cvar_array.dyn_cvar_count) {
      cv = &__cvar_array.dyn_cvars[slot - MPI_T_CVAR_COUNT];
    }

    mpc_common_spinlock_unlock(&cvar_array_lock);

    return cv;
  }
}

struct MPC_T_cvar *MPI_T_cvars_array_new(int *cvar_index) {
  if (!cvar_index) {
    return NULL;
  }

  mpc_common_spinlock_lock(&cvar_array_lock);

  int my_index = __cvar_array.dyn_cvar_count;
  __cvar_array.dyn_cvar_count++;

  __cvar_array.dyn_cvars =
      sctk_realloc(__cvar_array.dyn_cvars,
                   __cvar_array.dyn_cvar_count * sizeof(struct MPC_T_cvar));

  if (__cvar_array.dyn_cvars == NULL) {
    mpc_common_spinlock_unlock(&cvar_array_lock);
    return NULL;
  }

  mpc_common_spinlock_unlock(&cvar_array_lock);

  *cvar_index = my_index;
  return &__cvar_array.dyn_cvars[my_index];
}

/** This is how to manipulate CVARS */

static int MPC_T_cvar_init(struct MPC_T_cvar *cv, int event_key, char *name,
                           MPC_T_verbosity verbosity, MPI_Datatype datatype,
                           struct MPC_T_enum *enumtype, char *desc,
                           MPC_T_binding bind, MPC_T_cvar_scope scope) {
  cv->index = event_key;
  cv->name = strdup(name);
  cv->verbosity = verbosity;
  cv->datatype = datatype;
  if (enumtype) {
    memcpy(&cv->enumtype, enumtype, sizeof(struct MPC_T_enum));
  } else {
    MPC_T_enum_init_empty(&cv->enumtype);
  }
  cv->desc = strdup(desc);
  cv->bind = bind;
  cv->scope = scope;

  int readonly = 0;

  if ((scope == MPI_T_SCOPE_CONSTANT) || (scope == MPI_T_SCOPE_READONLY))
    readonly = 1;

  MPC_T_data_init(&cv->data, cv->datatype, 1, readonly);

  return 0;
}

int MPC_T_cvar_register_on_slot(int event_key, char *name,
                                MPC_T_verbosity verbosity,
                                MPI_Datatype datatype,
                                struct MPC_T_enum *enumtype, char *desc,
                                MPC_T_binding bind, MPC_T_cvar_scope scope) {
  struct MPC_T_cvar *cv = NULL;

  if (event_key < MPI_T_CVAR_COUNT) {
    cv = MPI_T_cvars_array_get(event_key);
    assume(cv != NULL);
  } else {
    return -1;
  }

  return MPC_T_cvar_init(cv, event_key, name, verbosity, datatype, enumtype,
                         desc, bind, scope);
}

int MPC_T_cvar_register(char *name, MPC_T_verbosity verbosity,
                        MPI_Datatype datatype, struct MPC_T_enum *enumtype,
                        char *desc, MPC_T_binding bind,
                        MPC_T_cvar_scope scope) {
  int my_index = -1;
  struct MPC_T_cvar *cv = MPI_T_cvars_array_new(&my_index);

  assume(cv != NULL);

  return MPC_T_cvar_init(cv, my_index, name, verbosity, datatype, enumtype,
                         desc, bind, scope);
}

/** This is the MPI Inteface */

int mpc_MPI_T_cvar_get_num(int *num_cvar) {
  if (!num_cvar)
    return 1;

  mpc_common_spinlock_lock(&cvar_array_lock);
  *num_cvar = __cvar_array.dyn_cvar_count + MPI_T_CVAR_COUNT;
  mpc_common_spinlock_unlock(&cvar_array_lock);

  return MPI_SUCCESS;
}

int mpc_MPI_T_cvar_get_info(int cvar_index, char *name, int *name_len,
                            int *verbosity, MPI_Datatype *datatype,
                            MPI_T_enum *enumtype, char *desc, int *desc_len,
                            int *bind, int *scope) {
  struct MPC_T_cvar *cvar = MPI_T_cvars_array_get(cvar_index);

  if (!cvar) {
    return MPI_T_ERR_INVALID_INDEX;
  }

  if (name && name_len)
    if (*name_len)
      snprintf(name, *name_len, "%s", cvar->name);
  if (name_len)
    *name_len = strlen(cvar->name) + 1;
  if (verbosity)
    *verbosity = (int)cvar->verbosity;
  if (datatype)
    *datatype = cvar->datatype;
  if (enumtype)
    *enumtype = (MPI_T_enum)&cvar->enumtype;
  if (desc && desc_len)
    if (*desc_len)
      snprintf(desc, *desc_len, "%s", cvar->desc);
  if (desc_len)
    *desc_len = strlen(cvar->desc) + 1;
  if (bind)
    *bind = (int)cvar->bind;
  if (scope)
    *scope = (int)cvar->scope;

  return MPI_SUCCESS;
}

int mpc_MPI_T_cvar_get_index(const char *name, int *cvar_index) {
  int num_cvar;
  mpc_MPI_T_cvar_get_num(&num_cvar);

  int found = 0;
  int i = -1;

  for (i = 0; i < num_cvar; ++i) {
    struct MPC_T_cvar *cvar = MPI_T_cvars_array_get(i);

    if (!strcmp(cvar->name, name)) {
      *cvar_index = i;
      found = 1;
      mpc_common_nodebug("%s is at %d", name, i);
      break;
    }
  }

  return found ? MPI_SUCCESS : MPI_T_ERR_INVALID_INDEX;
}

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS HANDLES                       */
/************************************************************************/

int mpc_MPI_T_cvar_handle_alloc(int cvar_index, __UNUSED__ void *obj_handle,
                                MPI_T_cvar_handle *handle, __UNUSED__ int *count) {
  /* Note we only support CVARS which are of "global" type
   * ie not attached to an MPI handle */

  struct MPC_T_cvar *cvar = MPI_T_cvars_array_get(cvar_index);

  if (!cvar) {
    return MPI_ERR_ARG;
  }

  *handle = cvar;

  return MPI_SUCCESS;
}

int mpc_MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle) {
  *handle = MPI_T_CVAR_HANDLE_NULL;
  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Control variables CVARS ACCESS                        */
/************************************************************************/

int mpc_MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buff) {
  struct MPC_T_cvar *cvar = handle;

  if (!cvar) {
    return MPI_ERR_ARG;
  }

  MPC_T_data_read(&cvar->data, buff);

  return MPI_SUCCESS;
}

int mpc_MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buff) {
  struct MPC_T_cvar *cvar = handle;

  if (!cvar) {
    return MPI_ERR_ARG;
  }

  MPC_T_data_write(&cvar->data, (void *)buff);

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS                           */
/************************************************************************/

/** This is the PVAR storage */

static struct MPC_T_pvars_array __pvar_array;
static int pvar_array_initialized = 0;
static mpc_common_spinlock_t pvar_array_lock = SCTK_SPINLOCK_INITIALIZER;

#define CATEGORIES(cat, parent, desc)
#define CVAR(name, verbosity, type, desc, bind, scope, cat)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)                                                      \
  mpit_concat(MPI_T_CAT_, cat),

/** This is the pvars categories */
static const MPC_T_category_t __mpi_t_pvars_categories[MPI_T_PVAR_COUNT] = {
#include "sctk_mpit.h"
};

#undef CATEGORIES
#undef CVAR
#undef PVAR

int MPI_T_pvar_fill_info_from_config() {
#define CATEGORIES(cat, parent, desc)
#define CVAR(name, verbosity, type, desc, bind, scope, cat)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)                                                      \
  mpc_MPI_T_pvars_array_register_on_slot(name, #name, verbosity, class, type,  \
                                         NULL, desc, bind, readonly,           \
                                         continuous, atomic);

#include "sctk_mpit.h"

#undef CATEGORIES
#undef CVAR
#undef PVAR
  return 0;
}

int MPI_T_pvars_array_init() {
  mpc_common_spinlock_lock(&pvar_array_lock);

  if (pvar_array_initialized == 0) {
    pvar_array_initialized = 1;
  } else {
    mpc_common_spinlock_unlock(&pvar_array_lock);
    return 0;
  }

  __pvar_array.dyn_pvar_count = 0;
  __pvar_array.dyn_pvars = NULL;
  mpc_common_spinlock_init(&__pvar_array.lock, 0);

  MPI_T_pvar_fill_info_from_config();

  mpc_common_spinlock_unlock(&pvar_array_lock);
  return 0;
}

/** Release the CVAR array storage (done once)
 * @return Non-zero on error
 */
int MPI_T_pvars_array_release() {
  mpc_common_spinlock_lock(&pvar_array_lock);

  if (pvar_array_initialized == 1) {
    pvar_array_initialized = 0;
  } else {
    mpc_common_spinlock_unlock(&pvar_array_lock);
    return 0;
  }

  __pvar_array.dyn_pvar_count = 0;
  sctk_free(__pvar_array.dyn_pvars);
  __pvar_array.dyn_pvars = NULL;

  mpc_common_spinlock_unlock(&pvar_array_lock);
  return 0;
}

struct MPC_T_pvar *MPI_T_pvars_array_get(int slot) {

  if (slot < MPI_T_PVAR_COUNT) {
    return &__pvar_array.st_pvar[slot];
  } else {

    struct MPC_T_pvar *pv = NULL;

    mpc_common_spinlock_lock(&pvar_array_lock);

    if ((slot - MPI_T_PVAR_COUNT) < __pvar_array.dyn_pvar_count) {
      pv = &__pvar_array.dyn_pvars[slot - MPI_T_PVAR_COUNT];
    }

    mpc_common_spinlock_unlock(&pvar_array_lock);

    return pv;
  }

  return NULL;
}

struct MPC_T_pvar *MPI_T_pvars_array_new(int *pvar_index) {
  if (!pvar_index) {
    return NULL;
  }

  mpc_common_spinlock_lock(&pvar_array_lock);

  int my_index = __pvar_array.dyn_pvar_count;
  __pvar_array.dyn_pvar_count++;

  __pvar_array.dyn_pvars =
      sctk_realloc(__pvar_array.dyn_pvars,
                   __pvar_array.dyn_pvar_count * sizeof(struct MPC_T_pvar));

  if (__pvar_array.dyn_pvars == NULL) {
    mpc_common_spinlock_unlock(&pvar_array_lock);
    return NULL;
  }

  mpc_common_spinlock_unlock(&pvar_array_lock);

  *pvar_index = my_index;
  return &__pvar_array.dyn_pvars[my_index];
}

/** This is how to manipulate PVARS */

static int mpc_MPI_T_pvar_init(struct MPC_T_pvar *pv, int pvar_index,
                               char *name, MPC_T_verbosity verbosity,
                               MPC_T_pvar_class var_class,
                               MPI_Datatype datatype, MPI_T_enum enumtype,
                               char *desc, MPC_T_binding bind, int readonly,
                               int continuous, int atomic) {

  pv->pvar_index = pvar_index;
  pv->name = strdup(name);
  pv->verbosity = verbosity;
  pv->pvar_class = var_class;
  pv->datatype = datatype;
  pv->enumtype = enumtype;
  pv->desc = strdup(desc);
  pv->bind = bind;
  pv->readonly = readonly;
  pv->continuous = continuous;
  pv->atomic = atomic;

  return 0;
}

int mpc_MPI_T_pvars_array_register_on_slot(
    int pvar_index, char *name, MPC_T_verbosity verbosity,
    MPC_T_pvar_class var_class, MPI_Datatype datatype, MPI_T_enum enumtype,
    char *desc, MPC_T_binding bind, int readonly, int continuous, int atomic) {
  // mpc_common_debug_warning("REGISTER %s on slot %d", name , pvar_index);
  struct MPC_T_pvar *pv = NULL;

  if (pvar_index < MPI_T_PVAR_COUNT) {
    pv = MPI_T_pvars_array_get(pvar_index);
    assume(pv != NULL);
  } else {
    return -1;
  }

  return mpc_MPI_T_pvar_init(pv, pvar_index, name, verbosity, var_class,
                             datatype, enumtype, desc, bind, readonly,
                             continuous, atomic);
}

int mpc_MPI_T_pvars_array_register(char *name, MPC_T_verbosity verbosity,
                                   MPC_T_pvar_class var_class,
                                   MPI_Datatype datatype, MPI_T_enum enumtype,
                                   char *desc, MPC_T_binding bind, int readonly,
                                   int continuous, int atomic) {
  int my_index = -1;
  struct MPC_T_pvar *pv = MPI_T_pvars_array_new(&my_index);

  assume(pv != NULL);

  return mpc_MPI_T_pvar_init(pv, my_index, name, verbosity, var_class, datatype,
                             enumtype, desc, bind, readonly, continuous,
                             atomic);
}

/** This is the MPI interface */

int mpc_MPI_T_pvar_get_num(int *num_pvar) {
  int ret = 0;

  mpc_common_spinlock_lock(&pvar_array_lock);
  ret = __pvar_array.dyn_pvar_count + MPI_T_PVAR_COUNT;
  mpc_common_spinlock_unlock(&pvar_array_lock);

  *num_pvar = ret;

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_get_info(int pvar_index, char *name, int *name_len,
                            int *verbosity, int *var_class,
                            MPI_Datatype *datatype, MPI_T_enum *enumtype,
                            char *desc, int *desc_len, int *bind, int *readonly,
                            int *continuous, int *atomic) {
  struct MPC_T_pvar *pv = MPI_T_pvars_array_get(pvar_index);

  if (!pv) {
    return MPI_T_ERR_INVALID_INDEX;
  }

  if (name && name_len)
    if (*name_len)
      snprintf(name, *name_len, "%s", pv->name);
  if (name_len)
    *name_len = strlen(pv->name) + 1;
  if (verbosity)
    *verbosity = (int)pv->verbosity;
  if (var_class)
    *var_class = (int)pv->pvar_class;
  if (datatype)
    *datatype = pv->datatype;
  if (enumtype)
    *enumtype = pv->enumtype;
  if (desc && desc_len)
    if (*desc_len)
      snprintf(desc, *desc_len, "%s", pv->desc);
  if (desc_len)
    *desc_len = strlen(pv->desc) + 1;
  if (bind)
    *bind = pv->bind;
  if (readonly)
    *readonly = pv->readonly;
  if (continuous)
    *continuous = pv->continuous;
  if (atomic)
    *atomic = pv->atomic;

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_get_index(char *name, int *pvar_class, int *pvar_index) {
  int num_pvar;
  mpc_MPI_T_pvar_get_num(&num_pvar);

  int found = 0;
  int i;

  for (i = 0; i < num_pvar; i++) {
    struct MPC_T_pvar *pvar = MPI_T_pvars_array_get(i);

    if (!pvar)
      continue;

    if (!strcmp(pvar->name, name)) {
      *pvar_index = i;
      *pvar_class = pvar->pvar_class;
      found = 1;
      break;
    }
  }

  return found ? MPI_SUCCESS : MPI_T_ERR_INVALID_INDEX;
}

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS SESSIONS                  */
/************************************************************************/

int mpc_MPI_T_pvar_session_create(MPI_T_pvar_session *session) {
  if (MPC_T_session_array_init_session(session)) {
    return MPI_ERR_INTERN;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_session_free(MPI_T_pvar_session *session) {
  if (MPC_T_session_array_release_session(session)) {
    return MPI_ERR_INTERN;
  }

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Handle Allocation         */
/************************************************************************/

int mpc_MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index,
                                void *obj_handle, MPI_T_pvar_handle *handle,
                                int *count) {
  if (!count || !handle) {
    return MPI_ERR_ARG;
  }

  *count = 1;

  struct MPC_T_pvar_handle *new_h =
      MPC_T_session_array_alloc(session, obj_handle, pvar_index);

  if (!new_h) {
    return MPI_ERR_ARG;
  }

  *handle = new_h;

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_handle_free(MPI_T_pvar_session session,
                               MPI_T_pvar_handle *handle) {
  MPC_T_session_array_free(session, *handle);
  return 0;
}

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Start and Stop            */
/************************************************************************/

int mpc_MPI_T_pvar_start(__UNUSED__ MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  struct MPC_T_pvar_handle *th = handle;

  if (handle == MPI_T_PVAR_ALL_HANDLES) {
    return MPI_T_ERR_PVAR_NO_STARTSTOP;
    //MPC_T_session_start(session, MPI_T_PVAR_ALL_HANDLES, th->index);
  } else {
    MPC_T_data_start(&th->data);
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_stop(__UNUSED__ MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  struct MPC_T_pvar_handle *th = handle;

  if (handle == MPI_T_PVAR_ALL_HANDLES) {
    return MPI_T_ERR_PVAR_NO_STARTSTOP;
    //return MPC_T_session_stop(session, MPI_T_PVAR_ALL_HANDLES, th->index);
  } else {
    MPC_T_data_stop(&th->data);
  }

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T Performance variables PVARS Read and Write            */
/************************************************************************/

int mpc_MPI_T_pvar_read(__UNUSED__ MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                        void *buff) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  struct MPC_T_pvar_handle *th = handle;

  MPC_T_data_read(&th->data, buff);

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_readreset(MPI_T_pvar_session session,
                             MPI_T_pvar_handle handle, void *buff) {
  mpc_MPI_T_pvar_read(session, handle, buff);
  mpc_MPI_T_pvar_reset(session, handle);
  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_write(__UNUSED__ MPI_T_pvar_session session, MPI_T_pvar_handle handle,
                         const void *buff) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  struct MPC_T_pvar_handle *th = handle;

  if (th->data.readonly) {
    return MPI_ERR_ACCESS;
  }

  MPC_T_data_write(&th->data, (void *)buff);

  return MPI_SUCCESS;
}

int mpc_MPI_T_pvar_reset(__UNUSED__ MPI_T_pvar_session session, MPI_T_pvar_handle handle) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  struct MPC_T_pvar_handle *th = handle;

  char buff[512];
  memset(buff, 0, 512);

  if (th->data.readonly) {
    return MPI_ERR_ACCESS;
  }

  MPC_T_data_write(&th->data, buff);

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T variables categorization                              */
/************************************************************************/

/** This is the category hierarchy */
#define CATEGORIES(cat, parent, desc) mpit_concat(MPI_T_CAT_, parent),
#define CVAR(name, verbosity, type, desc, bind, scope, cat)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)

static const MPC_T_category_t __mpi_t_category_parent[MPI_T_CATEGORY_COUNT] = {
    MPI_T_CAT_NULL,
#include "sctk_mpit.h"
    MPI_T_CAT_NULL};

#undef CATEGORIES
#undef CVAR
#undef PVAR

/** This is the category descriptions */
#define CATEGORIES(cat, parent, desc) desc,
#define CVAR(name, verbosity, type, desc, bind, scope, cat)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)

static const char *const __mpi_t_category_desc[MPI_T_CATEGORY_COUNT] = {
    "MPI_T_CAT_VARS",
#include "sctk_mpit.h"
    "MPI_T_CAT_NULL"};

#undef CATEGORIES
#undef CVAR
#undef PVAR

/** This is the category names */
#define CATEGORIES(cat, parent, desc) #cat,
#define CVAR(name, verbosity, type, desc, bind, scope, cat)
#define PVAR(name, verbosity, class, type, desc, bind, readonly, continuous,   \
             atomic, cat)

static const char *const __mpi_t_category_name[MPI_T_CATEGORY_COUNT] = {
    "MPI_T_CAT_VARS",
#include "sctk_mpit.h"
    "MPI_T_CAT_NULL"};

#undef CATEGORIES
#undef CVAR
#undef PVAR

int mpc_MPI_T_category_get_num(int *num_cat) {
  if (!num_cat) {
    return MPI_ERR_ARG;
  }

  *num_cat = MPI_T_CATEGORY_COUNT;

  return MPI_SUCCESS;
}

int mpc_MPI_T_category_get_info(int cat_index, char *name, int *name_len,
                                char *desc, int *desc_len, int *num_cvars,
                                int *num_pvars, int *num_categories) {
  if (MPI_T_CATEGORY_COUNT <= cat_index) {
    return MPI_ERR_ARG;
  }

  if (name && name_len) {
    if (*name_len)
      snprintf(name, *name_len, "%s", __mpi_t_category_name[cat_index]);
  }

  if (name_len) {
    *name_len = strlen(__mpi_t_category_name[cat_index]) + 1;
  }

  if (desc && desc_len) {
    if (*desc_len)
      snprintf(desc, *desc_len, "%s", __mpi_t_category_desc[cat_index]);
  }

  if (desc_len) {
    *desc_len = strlen(__mpi_t_category_desc[cat_index]) + 1;
  }

  int __num_pvars = 0;
  int __num_cvars = 0;
  int __num_cat = 0;

  int i;

  if (num_pvars) {

    for (i = 0; i < MPI_T_PVAR_COUNT; i++) {
      if (__mpi_t_pvars_categories[i] == (MPC_T_category_t)cat_index) {
        __num_pvars++;
      }
    }

    *num_pvars = __num_pvars;
  }

  if (num_cvars) {

    for (i = 0; i < MPI_T_CVAR_COUNT; i++) {
      if (__mpi_t_cvars_categories[i] ==(MPC_T_category_t)cat_index) {
        __num_cvars++;
      }
    }

    *num_cvars = __num_cvars;
  }

  if (num_categories) {

    for (i = 0; i < MPI_T_CATEGORY_COUNT; i++) {
      if (__mpi_t_category_parent[i] == (MPC_T_category_t)cat_index) {
        __num_cat++;
      }
    }

    *num_categories = __num_cat;
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_category_get_index(char *name, int *cat_index) {
  int i;

  for (i = 0; i < MPI_T_CATEGORY_COUNT; i++) {
    if (!strcmp(__mpi_t_category_name[i], name)) {
      *cat_index = i;
      return MPI_SUCCESS;
    }
  }

  return MPC_T_ERR_INVALID_NAME;
}

int mpc_MPI_T_category_get_cvars(int cat_index, int len, int indices[]) {
  if (MPI_T_CATEGORY_COUNT <= cat_index) {
    return MPI_ERR_ARG;
  }

  int i;
  int num_cvars = 0;

  for (i = 0; i < MPI_T_CVAR_COUNT; i++) {
    if (__mpi_t_cvars_categories[i] == (MPC_T_category_t)cat_index) {
      if (num_cvars < len) {
        indices[num_cvars] = i;
      } else {
        break;
      }
      num_cvars++;
    }
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_category_get_pvars(int cat_index, int len, int indices[]) {
  if (MPI_T_CATEGORY_COUNT <= cat_index) {
    return MPI_ERR_ARG;
  }

  int i;
  int num_pvars = 0;

  for (i = 0; i < MPI_T_PVAR_COUNT; i++) {
    if (__mpi_t_pvars_categories[i] == (MPC_T_category_t)cat_index) {
      if (num_pvars < len) {
        indices[num_pvars] = i;
      } else {
        break;
      }
      num_pvars++;
    }
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_category_get_categories(int cat_index, int len, int indices[]) {
  if (MPI_T_CATEGORY_COUNT <= cat_index) {
    return MPI_ERR_ARG;
  }

  int i;
  int num_cat = 0;

  for (i = 0; i < MPI_T_CATEGORY_COUNT; i++) {
    if (__mpi_t_category_parent[i] == (MPC_T_category_t)cat_index) {
      if (num_cat < len) {
        indices[num_cat] = i;
      } else {
        break;
      }
      num_cat++;
    }
  }

  return MPI_SUCCESS;
}

int mpc_MPI_T_category_changed(int *stamp) {
  if (!stamp) {
    return MPI_ERR_ARG;
  }

  /* No change expected for us */

  *stamp = 1337;

  return MPI_SUCCESS;
}

/************************************************************************/
/* INTERNAL MPI_T session                                               */
/************************************************************************/

struct MPC_T_pvar_handle *MPC_T_pvar_handle_new(MPI_T_pvar_session session,
                                                int mpi_handle,
                                                struct MPC_T_pvar *pvar) {
  struct MPC_T_pvar_handle *ret = sctk_malloc(sizeof(struct MPC_T_pvar_handle));

  if (!ret) {
    perror("malloc");
    return NULL;
  }

  MPC_T_data_init(&ret->data, pvar->datatype, pvar->continuous, pvar->readonly);

  ret->index = pvar->pvar_index;
  ret->session = session;
  ret->mpi_handle = mpi_handle;
  mpc_common_spinlock_init(&ret->lock, 0);

  ret->next = NULL;

  return ret;
}

int MPC_T_pvar_handle_free(struct MPC_T_pvar_handle *handle) {
  MPC_T_data_release(&handle->data);
  memset(handle, 0, sizeof(struct MPC_T_pvar_handle));
  sctk_free(handle);
  return 0;
}

struct MPC_T_data *MPC_T_pvar_handle_data(struct MPC_T_pvar_handle *data) {
  return &data->data;
}

int MPC_T_pvar_handle_read(struct MPC_T_pvar_handle *data, void *dataptr) {
  struct MPC_T_data *d = MPC_T_pvar_handle_data(data);
  MPC_T_data_read(d, dataptr);
  return 0;
}

static struct MPC_T_session_array ___session_array;

int MPC_T_session_array_init() {
  ___session_array.current_session = 0;
  sctk_spin_rwlock_init(&___session_array.lock);

  int i;

  for (i = 0; i < MPI_T_PVAR_COUNT; i++) {
    ___session_array.handle_lists[i] = NULL;
  }

  return 0;
}

int MPC_T_session_array_release() {
  ___session_array.current_session = -2;

  int i;

  for (i = 0; i < MPI_T_PVAR_COUNT; i++) {
    struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[i];

    while (tmp) {
      MPC_T_pvar_handle_free(tmp);
      tmp = tmp->next;
    }
  }

  return 0;
}

int MPC_T_session_array_init_session(MPI_T_pvar_session *session) {
  mpc_common_spinlock_write_lock(&___session_array.lock);
  int sess = ___session_array.current_session++;
  mpc_common_spinlock_write_unlock(&___session_array.lock);

  *session = sess;

  return 0;
}

int MPC_T_session_array_release_session(MPI_T_pvar_session *session) {

  int sess_id = *session;

  int i;

  // mpc_common_spinlock_write_lock( &___session_array.lock );

  /* Remove probes from the session */
  for (i = 0; i < MPI_T_PVAR_COUNT; i++) {
    struct MPC_T_pvar_handle *prev = NULL;
    struct MPC_T_pvar_handle *to_free = NULL;
    struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[i];

    while (tmp) {
      /* Only if right session */
      if (tmp->session == sess_id) {
        to_free = tmp;
        if (prev) {
          /* In list */
          prev->next = tmp->next;
        } else {
          /* First element */
          ___session_array.handle_lists[i] = tmp->next;
        }
      } else {
        to_free = NULL;
      }

      prev = tmp;
      tmp = tmp->next;

      if (to_free) {
        MPC_T_pvar_handle_free(to_free);
      }
    }
  }

  // mpc_common_spinlock_write_unlock( &___session_array.lock );

  *session = MPI_T_PVAR_SESSION_NULL;

  return 0;
}

struct MPC_T_pvar_handle *MPC_T_session_array_alloc(MPI_T_pvar_session session,
                                                    void *handle,
                                                    MPC_T_pvar_t pvar) {

  struct MPC_T_pvar *var = MPI_T_pvars_array_get(pvar);

  if (!var) {
    return NULL;
  }

  int mpihandle = -1;

  if (handle && (var->bind != MPI_T_BIND_NO_OBJECT)) {
    mpihandle = *((int *)handle);
  }

  struct MPC_T_pvar_handle *varhandle =
      MPC_T_pvar_handle_new(session, mpihandle, var);

  /* Now insert in array */
  mpc_common_spinlock_write_lock(&___session_array.lock);

  /* Head is next */
  varhandle->next = ___session_array.handle_lists[pvar];
  /* Replace head */
  ___session_array.handle_lists[pvar] = varhandle;

  mpc_common_spinlock_write_unlock(&___session_array.lock);

  return varhandle;
}

int MPC_T_session_array_free(__UNUSED__ MPI_T_pvar_session session,
                             struct MPC_T_pvar_handle *handle) {
  if (!handle) {
    return MPI_ERR_ARG;
  }

  mpc_common_spinlock_write_lock(&___session_array.lock);

  int pvar = handle->index;

  struct MPC_T_pvar_handle *prev = NULL;
  struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[pvar];

  while (tmp) {
    /* Only if right session */
    if (tmp == handle) {
      if (prev) {
        /* In list */
        prev->next = tmp->next;
      } else {
        /* First element */
        ___session_array.handle_lists[pvar] = tmp->next;
      }

      MPC_T_pvar_handle_free(tmp);
    }

    prev = tmp;
    tmp = tmp->next;
  }

  mpc_common_spinlock_write_unlock(&___session_array.lock);

  return 0;
}

int MPC_T_session_set(MPI_T_pvar_session session, void *handle,
                      MPC_T_pvar_t pvar, void *dataptr) {
  if (!mpc_MPI_T_initialized()) {
    mpc_common_debug_error("NOT INIT");
    return MPI_ERR_ARG;
  }

  /* We may set multiple handles
   *
   * -1 session means all sessions
   * MPI_T_PVAR_ALL_HANDLES handle means all handles matching a pvar
   *
   **/

  int mpi_handle = -1;

  if (handle && (handle != MPI_T_PVAR_ALL_HANDLES)) {
    mpi_handle = *((int *)handle);
  }

  mpc_common_spinlock_read_lock(&___session_array.lock);

  struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[pvar];

  while (tmp) {
    if ((tmp->session == session) || (session == -1)) {
      if ((tmp->mpi_handle == mpi_handle) ||
          (handle == MPI_T_PVAR_ALL_HANDLES)) {
        MPC_T_data_write(&tmp->data, dataptr);
      }
    }

    tmp = tmp->next;
  }

  mpc_common_spinlock_read_unlock(&___session_array.lock);

  return MPI_SUCCESS;
}

int MPC_T_session_start(MPI_T_pvar_session session, void *handle,
                        MPC_T_pvar_t pvar) {
  if (!mpc_MPI_T_initialized()) {
    mpc_common_debug_error("NOT INIT");
    return MPI_ERR_ARG;
  }

  /* We may set multiple handles
   *
   * -1 session means all sessions
   * MPI_T_PVAR_ALL_HANDLES handle means all handles matching a pvar
   *
   **/

  int mpi_handle = -1;

  if (handle && (handle != MPI_T_PVAR_ALL_HANDLES)) {
    mpi_handle = *((int *)handle);
  }

  mpc_common_spinlock_read_lock(&___session_array.lock);

  struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[pvar];

  while (tmp) {
    if ((tmp->session == session) || (session == -1)) {
      if ((tmp->mpi_handle == mpi_handle) ||
          (handle == MPI_T_PVAR_ALL_HANDLES)) {
        MPC_T_data_start(&tmp->data);
      }
    }

    tmp = tmp->next;
  }

  mpc_common_spinlock_read_unlock(&___session_array.lock);

  return MPI_SUCCESS;
}

int MPC_T_session_stop(MPI_T_pvar_session session, void *handle,
                       MPC_T_pvar_t pvar) {
  if (!mpc_MPI_T_initialized()) {
    mpc_common_debug_error("NOT INIT");
    return MPI_ERR_ARG;
  }

  /* We may set multiple handles
   *
   * -1 session means all sessions
   * MPI_T_PVAR_ALL_HANDLES handle means all handles matching a pvar
   *
   **/

  int mpi_handle = -1;

  if (handle && (handle != MPI_T_PVAR_ALL_HANDLES)) {
    mpi_handle = *((int *)handle);
  }

  mpc_common_spinlock_read_lock(&___session_array.lock);

  struct MPC_T_pvar_handle *tmp = ___session_array.handle_lists[pvar];

  while (tmp) {
    if ((tmp->session == session) || (session == -1)) {
      if ((tmp->mpi_handle == mpi_handle) ||
          (handle == MPI_T_PVAR_ALL_HANDLES)) {
        MPC_T_data_stop(&tmp->data);
      }
    }

    tmp = tmp->next;
  }

  mpc_common_spinlock_read_unlock(&___session_array.lock);

  return MPI_SUCCESS;
}


/********************
 * VARIABLE BINDING *
 ********************/

void mpc_MPI_T_bind_variable(char *name, size_t size, void * data_addr)
{
        int the_temp_index;

        if( mpc_MPI_T_cvar_get_index( name , &the_temp_index ) == MPI_SUCCESS )
        {
                struct MPC_T_cvar * the_cvar = MPI_T_cvars_array_get( the_temp_index );

                if( MPC_T_data_get_size( &the_cvar->data ) != size )
                {
                        fprintf(stderr,"Error size mismatch for %s", name);
                        abort();
                }

                if( the_cvar )
                {
                        MPC_T_data_alias(&the_cvar->data, data_addr);
                }
                else
                {
                        fprintf(stderr,"ERROR in CONFIG : MPIT var was found but no entry for %s", name);
                        abort();
                }
        }
        else
        {
                fprintf(stderr,"ERROR in CONFIG : No such MPIT CVAR alias for %s", name);
                abort();
        }
}
