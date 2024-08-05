#ifndef MPC_LOWCOMM_DATATYPES
#define MPC_LOWCOMM_DATATYPES

#include <mpc_lowcomm_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************
*        Unified datatypes        *
***********************************/

/** \brief General datatype used to describe all datatypes
 *
 *  Here a datatype is described as a list of segments which can themselves
 *  gather several types.
 *
 */
typedef struct MPI_ABI_Datatype
{
	/* Context */
	struct _mpc_dt_footprint *  context; /**< Saves the creation context for MPI_get_envelope & MPI_Get_contents */

	/* Attrs */
	struct __mpc_dt_attr_store *attrs; /**< ATTR array for this type */

	/* Basics */
	size_t                      size;  /**< Total size of the datatype */
	size_t                      count; /**< Number of elements in the datatype */

        /* MPITypes */
        void *                      handle;

	/* Content */
	long *                      begins; /**< Begin offsets */
	long *                      ends;   /**< End offsets */

	/* Optimized Content */
	size_t                      opt_count;  /**< Number of blocks with optimization */
	long *                      opt_begins; /**< Begin offsets with optimization */
	long *                      opt_ends;   /**< End offsets with optimization */
	mpc_lowcomm_datatype_t *    datatypes;  /**< Datatypes for each block */

	/* Bounds */
	long                        lb; /**< Lower bound offset  */
	long                        ub; /**< Upper bound offset */

	/* Additionnal informations */
	unsigned int                id;                 /**< Integer ID (useful for debug and searching _mpc_dt_storage) */
	unsigned int                ref_count;          /**< Ref counter to manage freeing */

	bool                        is_lb;              /**< Does type has a lower bound */
	bool                        is_ub;              /**< Does type has an upper bound */
	bool                        is_a_padded_struct; /**< Was the type padded with UB during construction ? */
	bool                        is_commited;        /**< Does type is commited ? */

	char                        padding[19];        /**< Pad to 128 bytes */
} _mpc_lowcomm_general_datatype_t;

/* ******************
 * Common datatypes *
 * *****************/

/* The null handles is defined in \ref mpc_lowcomm_types.h */

/** Integers */
#define MPC_LOWCOMM_CHAR                       ( (mpc_lowcomm_datatype_t)1)
#define MPC_LOWCOMM_SHORT                      ( (mpc_lowcomm_datatype_t)2)
#define MPC_LOWCOMM_INT                        ( (mpc_lowcomm_datatype_t)3)
#define MPC_LOWCOMM_LONG                       ( (mpc_lowcomm_datatype_t)4)
#define MPC_LOWCOMM_LONG_LONG_INT              ( (mpc_lowcomm_datatype_t)5)
#define MPC_LOWCOMM_LONG_LONG                  ( (mpc_lowcomm_datatype_t)6)
#define MPC_LOWCOMM_SIGNED_CHAR                ( (mpc_lowcomm_datatype_t)7)
#define MPC_LOWCOMM_UNSIGNED_CHAR              ( (mpc_lowcomm_datatype_t)8)
#define MPC_LOWCOMM_UNSIGNED_SHORT             ( (mpc_lowcomm_datatype_t)9)
#define MPC_LOWCOMM_UNSIGNED                   ( (mpc_lowcomm_datatype_t)10)
#define MPC_LOWCOMM_UNSIGNED_LONG              ( (mpc_lowcomm_datatype_t)11)
#define MPC_LOWCOMM_UNSIGNED_LONG_LONG         ( (mpc_lowcomm_datatype_t)12)
#define MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT     MPC_LOWCOMM_UNSIGNED_LONG_LONG

/** Floating points */
#define MPC_LOWCOMM_FLOAT                      ( (mpc_lowcomm_datatype_t)13)
#define MPC_LOWCOMM_DOUBLE                     ( (mpc_lowcomm_datatype_t)14)
#define MPC_LOWCOMM_LONG_DOUBLE                ( (mpc_lowcomm_datatype_t)15)

/** stdint and stddef types */
#define MPC_LOWCOMM_WCHAR                      ( (mpc_lowcomm_datatype_t)16)
#define MPC_LOWCOMM_C_BOOL                     ( (mpc_lowcomm_datatype_t)17)
#define MPC_LOWCOMM_INT8_T                     ( (mpc_lowcomm_datatype_t)18)
#define MPC_LOWCOMM_INT16_T                    ( (mpc_lowcomm_datatype_t)19)
#define MPC_LOWCOMM_INT32_T                    ( (mpc_lowcomm_datatype_t)20)
#define MPC_LOWCOMM_INT64_T                    ( (mpc_lowcomm_datatype_t)21)
#define MPC_LOWCOMM_UINT8_T                    ( (mpc_lowcomm_datatype_t)22)
#define MPC_LOWCOMM_UINT16_T                   ( (mpc_lowcomm_datatype_t)23)
#define MPC_LOWCOMM_UINT32_T                   ( (mpc_lowcomm_datatype_t)24)
#define MPC_LOWCOMM_UINT64_T                   ( (mpc_lowcomm_datatype_t)25)

/** MPI integer types */
#define MPC_LOWCOMM_AINT                       ( (mpc_lowcomm_datatype_t)26)
#define MPC_LOWCOMM_COUNT                      ( (mpc_lowcomm_datatype_t)27)
#define MPC_LOWCOMM_OFFSET                     ( (mpc_lowcomm_datatype_t)28)

/** Complex numbers */
#define MPC_LOWCOMM_C_COMPLEX                  ( (mpc_lowcomm_datatype_t)29) /* /!\ derived type */
#define MPC_LOWCOMM_C_FLOAT_COMPLEX            ( (mpc_lowcomm_datatype_t)30) /* /!\ derived type */
#define MPC_LOWCOMM_C_DOUBLE_COMPLEX           ( (mpc_lowcomm_datatype_t)31) /* /!\ derived type */
#define MPC_LOWCOMM_C_LONG_DOUBLE_COMPLEX      ( (mpc_lowcomm_datatype_t)32) /* /!\ derived type */

/** Misc */
#define MPC_LOWCOMM_BYTE                       ( (mpc_lowcomm_datatype_t)33)
#define MPC_LOWCOMM_PACKED                     ( (mpc_lowcomm_datatype_t)34)

/** Predefined MPI datatypes corresponding to C++ datatypes */
#define MPC_LOWCOMM_CXX_BOOL                   ( (mpc_lowcomm_datatype_t)35)
#define MPC_LOWCOMM_CXX_FLOAT_COMPLEX          ( (mpc_lowcomm_datatype_t)36) /* /!\ derived type */
#define MPC_LOWCOMM_CXX_DOUBLE_COMPLEX         ( (mpc_lowcomm_datatype_t)37) /* /!\ derived type */
#define MPC_LOWCOMM_CXX_LONG_DOUBLE_COMPLEX    ( (mpc_lowcomm_datatype_t)38) /* /!\ derived type */

/** FORTRAN types */
#define MPC_LOWCOMM_INTEGER                    ( (mpc_lowcomm_datatype_t)39)
#define MPC_LOWCOMM_REAL                       ( (mpc_lowcomm_datatype_t)40)
#define MPC_LOWCOMM_DOUBLE_PRECISION           ( (mpc_lowcomm_datatype_t)41)
#define MPC_LOWCOMM_COMPLEX                    ( (mpc_lowcomm_datatype_t)42) /* /!\ derived type */
#define MPC_LOWCOMM_LOGICAL                    ( (mpc_lowcomm_datatype_t)43)
#define MPC_LOWCOMM_CHARACTER                  ( (mpc_lowcomm_datatype_t)44)
#define MPC_LOWCOMM_DOUBLE_COMPLEX             ( (mpc_lowcomm_datatype_t)45) /* /!\ derived type */

/** F08 types */
#define MPC_LOWCOMM_INTEGER1                   ( (mpc_lowcomm_datatype_t)46)
#define MPC_LOWCOMM_INTEGER2                   ( (mpc_lowcomm_datatype_t)47)
#define MPC_LOWCOMM_INTEGER4                   ( (mpc_lowcomm_datatype_t)48)
#define MPC_LOWCOMM_INTEGER8                   ( (mpc_lowcomm_datatype_t)49)
#define MPC_LOWCOMM_INTEGER16                  ( (mpc_lowcomm_datatype_t)50)
#define MPC_LOWCOMM_REAL2                      ( (mpc_lowcomm_datatype_t)51)
#define MPC_LOWCOMM_REAL4                      ( (mpc_lowcomm_datatype_t)52)
#define MPC_LOWCOMM_REAL8                      ( (mpc_lowcomm_datatype_t)53)
#define MPC_LOWCOMM_REAL16                     ( (mpc_lowcomm_datatype_t)54)
#define MPC_LOWCOMM_COMPLEX4                   ( (mpc_lowcomm_datatype_t)55) /* /!\ derived type */
#define MPC_LOWCOMM_COMPLEX8                   ( (mpc_lowcomm_datatype_t)56) /* /!\ derived type */
#define MPC_LOWCOMM_COMPLEX16                  ( (mpc_lowcomm_datatype_t)57) /* /!\ derived type */
#define MPC_LOWCOMM_COMPLEX32                  ( (mpc_lowcomm_datatype_t)58) /* /!\ derived type */

/** Predefined derived datatypes */
#define MPC_LOWCOMM_FLOAT_INT                  ( (mpc_lowcomm_datatype_t)59) /* /!\ derived type */
#define MPC_LOWCOMM_DOUBLE_INT                 ( (mpc_lowcomm_datatype_t)60) /* /!\ derived type */
#define MPC_LOWCOMM_LONG_INT                   ( (mpc_lowcomm_datatype_t)61) /* /!\ derived type */
#define MPC_LOWCOMM_2INT                       ( (mpc_lowcomm_datatype_t)62) /* /!\ derived type */
#define MPC_LOWCOMM_SHORT_INT                  ( (mpc_lowcomm_datatype_t)63) /* /!\ derived type */
#define MPC_LOWCOMM_LONG_DOUBLE_INT            ( (mpc_lowcomm_datatype_t)64) /* /!\ derived type */
#define MPC_LOWCOMM_2REAL                      ( (mpc_lowcomm_datatype_t)65) /* /!\ derived type */
#define MPC_LOWCOMM_2DOUBLE_PRECISION          ( (mpc_lowcomm_datatype_t)66) /* /!\ derived type */
#define MPC_LOWCOMM_2INTEGER                   ( (mpc_lowcomm_datatype_t)67) /* /!\ derived type */

/** MPC specific */
#define MPC_LOWCOMM_2FLOAT                     ( (mpc_lowcomm_datatype_t)68) /* /!\ derived type */
#define MPC_LOWCOMM_TYPE_COMMON_LIMIT          69                            /**< Last predefined datatype identifier */
/*!< This should always be 1 plus the last common type */

/* ****************************
 * common datatypes functions *
 * ***************************/

/**
 * \brief initializes common datatypes
 *
 * \return MPI_SUCCESS on success
 */
int mpc_lowcomm_datatype_common_init();

/**
 * \brief get the size of a datatype
 *
 * \param datatype datatype to get the size of
 * \return the size of the datatype
 */
size_t mpc_lowcomm_datatype_common_get_size(const mpc_lowcomm_datatype_t datatype);

/**
 * \brief get the name of a common datatype
 *
 * \param datatype datatype to get the name of
 * \return the name of the datatype
 */
char * mpc_lowcomm_datatype_common_get_name(const mpc_lowcomm_datatype_t datatype);

/**
 * \brief get the unified type structure of a common datatype
 *
 * \param datatype datatype to get the type structure of
 * \return the datatype struct pointer
 */
mpc_lowcomm_datatype_t mpc_lowcomm_datatype_common_get_type_struct(const mpc_lowcomm_datatype_t datatype);

/**
 * \brief set the name of a common datatype
 *
 * \param datatype datatype to set the name of
 * \param name new name of the datatype
 * \return MPI_SUCCESS on success
 */
int mpc_lowcomm_datatype_common_set_name(mpc_lowcomm_datatype_t datatype, const char *const name);

/**
 * \brief set the unified datatype structure of a common datatype
 *
 * \param datatype datatype to set the structure of
 * \param new_struct new structure of the datatype
 * \return MPI_SUCCESS on success
 */
int mpc_lowcomm_datatype_common_set_type_struct(mpc_lowcomm_datatype_t datatype, mpc_lowcomm_datatype_t new_struct);

/**
 * \brief Display a common datatype for debug
 *
 * \param datatype the datatype to display
 */
void mpc_lowcomm_datatype_common_display(const mpc_lowcomm_datatype_t datatype);

/**
 * \brief Check if a given address is the one of a common datatype unified structure
 *
 * \param datatype the datatype to check
 * \return true if common else false
 */
bool mpc_lowcomm_datatype_is_common_addr(const mpc_lowcomm_datatype_t datatype);

/* NOLINTBEGIN(clang-diagnostic-unused-function): False positives */

/**
 * \brief Get the representation of a common datatype with its actual address
 *
 * \param datatype the datatype with address
 * \return datatype with common representation
 */
mpc_lowcomm_datatype_t mpc_lowcomm_datatype_get_common_addr(const mpc_lowcomm_datatype_t datatype);

/**
 * \brief Check if a given datatype is common
 *
 * \param datatype the datatype to check
 * \return true if common else false
 */
static inline bool mpc_lowcomm_datatype_is_common_predefined(const mpc_lowcomm_datatype_t datatype)
{
	if( (MPC_LOWCOMM_DATATYPE_NULL < datatype) && (datatype < (mpc_lowcomm_datatype_t)MPC_LOWCOMM_TYPE_COMMON_LIMIT) )
	{
		return true;
	}

	return false;
}

/**
 * \brief Check if a given datatype is common
 * This function checks if the entry is an address to a common
 * or a predefined constants datatype
 *
 * \param datatype the datatype to check
 * \return true if common else false
 */
static inline bool mpc_lowcomm_datatype_is_common(const mpc_lowcomm_datatype_t datatype)
{
	return mpc_lowcomm_datatype_is_common_predefined(datatype) || mpc_lowcomm_datatype_is_common_addr(datatype);
}

/* NOLINTEND(clang-diagnostic-unused-function) */

#ifdef __cplusplus
}
#endif

#endif
