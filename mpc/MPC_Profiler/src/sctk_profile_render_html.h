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
/* #   - BESNARD Jean-Baptiste jean-baptiste.besnard@cea.fr               # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_PROFILE_RENDER_HTML_H
#define SCTK_PROFILE_RENDER_HTML_H

#include "sctk_profile_render.h"


	void sctk_profile_render_html_register( struct sctk_profile_renderer *rd );
	void sctk_profile_render_html_setup( struct sctk_profile_renderer *rd );
	void sctk_profile_render_html_teardown( struct sctk_profile_renderer *rd );

	void sctk_profile_render_html_setup_profile( struct sctk_profile_renderer *rd );
	void sctk_profile_render_html_render_profile( struct sctk_profiler_array *array, int id, int parent_id, int depth, int going_up, struct sctk_profile_renderer *rd );
	void sctk_profile_render_html_teardown_profile( struct sctk_profile_renderer *rd );

	void sctk_profile_render_html_setup_meta( struct sctk_profile_renderer *rd );
	void sctk_profile_render_html_render_meta( struct sctk_profile_renderer *rd, struct sctk_profile_meta *meta );
	void sctk_profile_render_html_teardown_meta( struct sctk_profile_renderer *rd );

#endif /* SCTK_PROFILE_RENDER_HTML_H */
