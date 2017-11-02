#ifndef SCTK_PTL_TYPES_H_
#define SCTK_PTL_TYPES_H_

#include <portals4.h>
#include "sctk_io_helper.h"

#define SCTK_PTL_IGNORE_NONE (sctk_ptl_matchbits_t){.data.comm = 0, .data.tag = 0 }
#define SCTK_PTL_IGNORE_ALL (sctk_ptl_matchbits_t){.data.comm = UINT32_MAX, .data.tag = UINT32_MAX }
#define SCTK_PTL_IGNORE_COMM (sctk_ptl_matchbits_t){.data.comm = UINT32_MAX, .data.tag = 0 }
#define SCTK_PTL_IGNORE_TAG (sctk_ptl_matchbits_t){.data.comm = 0, .data.tag = UINT32_MAX }


#define sctk_ptl_meh_t ptl_handle_me_t
#define sctk_ptl_me_t ptl_me_t
#define SCTK_PTL_ME_FLAGS PTL_ME_EVENT_LINK_DISABLE
#define SCTK_PTL_ME_PUT_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_PUT
#define SCTK_PTL_ME_GET_FLAGS SCTK_PTL_ME_FLAGS | PTL_ME_OP_GET
#define SCTK_PTL_ME_OVERFLOW_FLAGS SCTK_PTL_ME_GET_FLAGS | PTL_ME_OP_GET

#define sctk_ptl_mdh_t ptl_handle_md_t
#define sctk_ptl_md_t ptl_md_t
#define SCTK_PTL_MD_FLAGS 0
#define SCTK_PTL_MD_PUT_FLAGS SCTK_PTL_MD_FLAGS
#define SCTK_PTL_MD_GET_FLAGS SCTK_PTL_MD_FLAGS

#define sctk_ptl_nih_t ptl_handle_ni_t
#define sctk_ptl_limits_t ptl_ni_limits_t

#define sctk_ptl_eq_t ptl_handle_eq_t

#define sctk_ptl_event_t ptl_event_t

#define sctk_ptl_cnt_t ptl_ct_event_t
#define sctk_ptl_cnth_t ptl_handle_ct_t

#define sctk_ptl_id_t ptl_process_t

#define SCTK_PTL_PTE_FLAGS PTL_PT_FLOWCTRL
#define SCTK_PTL_PTE_GET 0
#define SCTK_PTL_PTE_PUT 1
#define SCTK_PTL_PTE_CM  2
#define SCTK_PTL_EQ_PTE_SIZE 512
#define SCTK_PTL_EQ_MDS_SIZE 128


struct sctk_ptl_bits_content_s
{
	uint32_t comm;
	uint32_t tag;
};

/* should all be sized to raw 64 bits */
typedef union sctk_ptl_matchbits_t
{
	ptl_match_bits_t raw;
	struct sctk_ptl_bits_content_s data;
} sctk_ptl_matchbits_t;

typedef struct sctk_ptl_pte_s
{
	ptl_pt_index_t idx;
	sctk_ptl_eq_t eq;
	sctk_ptl_meh_t uniq_meh;
} sctk_ptl_pte_t;

/**
 * Portals-specific information specializing a rail.
 */
typedef struct sctk_ptl_rail_info_s
{
	sctk_ptl_limits_t max_limits;           /**< container for Portals thresholds */
	sctk_ptl_nih_t iface;                   /**< Interface handler for the device */
	sctk_ptl_id_t id;                       /**< Local id identifying this rail */
	sctk_ptl_pte_t* pt_entries;             /**< Portals table for this rail */
	size_t nb_entries;                      /**< Number of Portals entries */
	sctk_ptl_eq_t mds_eq;                   /**< EQ for all MDs emited from this NI */

	char connection_infos[MAX_STRING_SIZE]; /**< string identifying this rail over the PMI */
	size_t connection_infos_size;           /**< Size of the above string */
} sctk_ptl_rail_info_t;

#endif
