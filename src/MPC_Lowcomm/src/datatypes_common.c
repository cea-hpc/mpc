#include "datatypes_common.h"

#include "mpc_common_debug.h"

#include <stdio.h>
#include <string.h>
#include <sctk_alloc.h>


/**
 * \brief initializes a datatype
 *
 * \param datatype datatype name
 * \param t type (registering its size)
 */

#define mpc_lowcomm_datatype_init(datatype)                                                                         \
	_mpc_lowcomm_datatype_init_struct_common(&(mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].type_struct), \
	                                         (intptr_t)datatype - 1, sizeof(datatype ## _TYPE) );               \
	sprintf(mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].name, "%s_%s", "MPI", #datatype)

_mpc_lowcomm_datatype_common_t mpc_lowcomm_datatypes_list[MPC_LOWCOMM_TYPE_COMMON_LIMIT];

/** \brief Fill a unified datatype struct for a common datatype */
static inline void _mpc_lowcomm_datatype_init_struct_common(mpc_lowcomm_datatype_t datatype,
                                                            const unsigned int id,
                                                            const size_t size)
{
	datatype->id   = id;
	datatype->size = size;

	datatype->count = 1;

	datatype->begins    = sctk_malloc(sizeof(long) );
	datatype->ends      = sctk_malloc(sizeof(long) );
	datatype->datatypes = sctk_malloc(sizeof(mpc_lowcomm_datatype_t) );

	if(!datatype->begins || !datatype->ends || !datatype->datatypes)
	{
		mpc_common_debug_fatal("Failled to allocate common type content");
	}

	datatype->begins[0]    = 0;
	datatype->ends[0]      = size - 1; //! Inclusive bounds
	datatype->datatypes[0] = datatype;

	datatype->opt_count  = datatype->count;
	datatype->opt_begins = datatype->begins;
	datatype->opt_ends   = datatype->ends;

	datatype->lb    = 0;
	datatype->is_lb = false;
	datatype->ub    = 0;
	datatype->is_ub = false;

	datatype->is_a_padded_struct = false;
	datatype->is_commited        = true;

	datatype->ref_count = 1; /**< Common datatype cannot be freed */
	datatype->context   = NULL;
	datatype->attrs     = NULL;
}

int _mpc_lowcomm_datatype_init_common()
{
	/* Integers */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_CHAR);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_SHORT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_LONG);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_LONG_LONG_INT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_LONG_LONG);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_SIGNED_CHAR);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UNSIGNED_CHAR);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UNSIGNED_SHORT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UNSIGNED);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UNSIGNED_LONG);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UNSIGNED_LONG_LONG);

	/* Floats */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_FLOAT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_DOUBLE);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_LONG_DOUBLE);

	/* C Types */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_WCHAR);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_C_BOOL);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INT8_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INT16_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INT32_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INT64_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UINT8_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UINT16_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UINT32_T);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_UINT64_T);

	/* MPI Types */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_AINT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_COUNT);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_OFFSET);

	/* Misc */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_BYTE);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_PACKED);

	/* FORTRAN Types */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_REAL);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_DOUBLE_PRECISION);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_LOGICAL);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_CHARACTER);

	/* F08 Types */
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER1);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER2);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER4);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER8);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_INTEGER16);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_REAL2);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_REAL4);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_REAL8);
	mpc_lowcomm_datatype_init(MPC_LOWCOMM_REAL16);

	return MPC_LOWCOMM_SUCCESS;
}

size_t mpc_lowcomm_datatype_common_get_size(const mpc_lowcomm_datatype_t datatype)
{
	if(mpc_lowcomm_datatype_is_common_predefined(datatype) )
	{
		return mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].type_struct.size;
	}

	if(mpc_lowcomm_datatype_is_common_addr(datatype) )
	{
		_mpc_lowcomm_datatype_common_t *common = (_mpc_lowcomm_datatype_common_t *)datatype;
		return common->type_struct.size;
	}

	return 0;
}

char * mpc_lowcomm_datatype_common_get_name(const mpc_lowcomm_datatype_t datatype)
{
	if(mpc_lowcomm_datatype_is_common_predefined(datatype) )
	{
		return mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].name;
	}

	if(mpc_lowcomm_datatype_is_common_addr(datatype) )
	{
		_mpc_lowcomm_datatype_common_t *common = (_mpc_lowcomm_datatype_common_t *)datatype;
		return common->name;
	}

	return NULL;
}

mpc_lowcomm_datatype_t mpc_lowcomm_datatype_common_get_type_struct(const mpc_lowcomm_datatype_t datatype)
{
	if(datatype < (mpc_lowcomm_datatype_t)MPC_LOWCOMM_TYPE_COMMON_LIMIT)
	{
		return &(mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].type_struct);
	}

	return datatype;
}

int mpc_lowcomm_datatype_common_set_name(mpc_lowcomm_datatype_t datatype, const char *const name)
{
	if(datatype < (mpc_lowcomm_datatype_t)MPC_LOWCOMM_TYPE_COMMON_LIMIT)
	{
		sprintf(mpc_lowcomm_datatypes_list[(intptr_t)datatype - 1].name, "%s", name);
		return MPC_LOWCOMM_SUCCESS;
	}
	return MPC_LOWCOMM_ERR_TYPE;
}

int mpc_lowcomm_datatype_common_set_type_struct(mpc_lowcomm_datatype_t datatype, const mpc_lowcomm_datatype_t new_struct)
{
	if(mpc_lowcomm_datatype_is_common_predefined(datatype) )
	{
		memcpy(mpc_lowcomm_datatype_common_get_type_struct(datatype), new_struct, sizeof(_mpc_lowcomm_general_datatype_t) );
		return MPC_LOWCOMM_SUCCESS;
	}
	if(mpc_lowcomm_datatype_is_common_addr(datatype) )
	{
		memcpy(datatype, new_struct, sizeof(_mpc_lowcomm_general_datatype_t) );
		return MPC_LOWCOMM_SUCCESS;
	}
	return MPC_LOWCOMM_ERR_TYPE;
}

bool mpc_lowcomm_datatype_is_common_addr(const mpc_lowcomm_datatype_t datatype)
{
	if(datatype >= (mpc_lowcomm_datatype_t)mpc_lowcomm_datatypes_list &&
	   datatype <= (mpc_lowcomm_datatype_t)(mpc_lowcomm_datatypes_list + MPC_LOWCOMM_TYPE_COMMON_LIMIT) )
	{
		return true;
	}

	return false;
}

void mpc_lowcomm_datatype_common_display(const mpc_lowcomm_datatype_t datatype)
{
	if(!mpc_lowcomm_datatype_is_common(datatype) )
	{
		mpc_common_debug_error("Unknown datatype provided to %s\n", __FUNCTION__);
		abort();
	}

	if(mpc_lowcomm_datatype_is_common_predefined(datatype) )
	{
		mpc_common_debug_error("=============COMMON=================");
		mpc_common_debug_error("NAME %s", mpc_lowcomm_datatype_common_get_name(datatype) );
		mpc_common_debug_error("SIZE %ld", mpc_lowcomm_datatype_common_get_size(datatype) );
		mpc_common_debug_error("====================================");
	}

	else
	{
		_mpc_lowcomm_datatype_common_t *predefined_cast = (_mpc_lowcomm_datatype_common_t *)datatype;
		mpc_common_debug_error("=============COMMON=================");
		mpc_common_debug_error("NAME %s", predefined_cast->name);
		mpc_common_debug_error("SIZE %ld", datatype->size);
		mpc_common_debug_error("====================================");
	}
}
