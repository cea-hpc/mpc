<?xml version="1.0"?>
<!--
############################# MPC License ##############################
# Wed Nov 19 15:19:19 CET 2008                                         #
# Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
#                                                                      #
# IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
# This file is part of the MPC Runtime.                                #
#                                                                      #
# This software is governed by the CeCILL-C license under French law   #
# and abiding by the rules of distribution of free software.  You can  #
# use, modify and/ or redistribute the software under the terms of     #
# the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
# following URL http://www.cecill.info.                                #
#                                                                      #
# The fact that you are presently reading this means that you have     #
# had knowledge of the CeCILL-C license and that you accept its        #
# terms.                                                               #
#                                                                      #
# Authors:                                                             #
#   - VALAT Sebastien sebastien.valat@cea.fr                           #
#                                                                      #
########################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:text>#include &lt;stdbool.h&gt;&#10;</xsl:text>
		<xsl:text>#include "sctk_runtime_config_struct_defaults.h"&#10;&#10;</xsl:text>
		<xsl:text>#ifndef SCTK_RUNTIME_CONFIG_STRUCT_H&#10;</xsl:text>
		<xsl:text>#define SCTK_RUNTIME_CONFIG_STRUCT_H&#10;</xsl:text>

		<xsl:apply-templates select="config"/>
		<xsl:call-template name="gen-final-module-struct"/>
		<xsl:call-template name="gen-config-struct"/>
		<xsl:text>&#10;#endif // SCTK_RUNTIME_CONFIG_STRUCT_H&#10;</xsl:text>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-config-struct">
		<xsl:text>&#10;/*********************  STRUCT  *********************/&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#09;struct sctk_runtime_config_modules modules;&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-final-module-struct">
		<xsl:text>&#10;/*********************  STRUCT  *********************/&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config_modules&#10;</xsl:text>
		<xsl:text>{</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="modules">
				<xsl:for-each select="module">
					<xsl:value-of select="concat('&#10;&#9;struct sctk_runtime_config_module_',type,' ',name,';')"/>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
		<xsl:text>&#10;};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select="usertypes"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:text>&#10;/*********************  STRUCT  *********************/&#10;</xsl:text>
		<xsl:value-of select="concat('/**',doc,'**/&#10;')"/>
		<xsl:value-of select="concat('struct sctk_runtime_config_module_',name,'&#10;')"/>
		<xsl:text>{</xsl:text>
		<xsl:apply-templates select="param"/>
		<xsl:apply-templates select="array"/>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:value-of select="concat('&#9;/**',doc,'**/&#10;')"/>
		<xsl:text>&#9;</xsl:text>
		<xsl:call-template name="gen-type-name"/>
		<xsl:value-of select="concat(' ',name)"/>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:value-of select="concat('&#9;/**',doc,'**/&#10;')"/>
		<xsl:text>&#9;</xsl:text>
		<xsl:call-template name="gen-type-name"/>
		<xsl:value-of select="concat(' * ',name)"/>
		<xsl:text>;&#10;</xsl:text>
		<xsl:value-of select="concat('&#9;/** Number of elements in ',name,' array. **/&#10;')"/>
		<xsl:value-of select="concat('&#9;int ',name,'_size;&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-type-name">
		<xsl:choose>
			<xsl:when test="type = 'int'">int</xsl:when>
			<xsl:when test="type = 'bool'">bool</xsl:when>
			<xsl:when test="type = 'float'">float</xsl:when>
			<xsl:when test="type = 'double'">double</xsl:when>
			<xsl:when test="type = 'string'">char *</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gen-user-type-name">
					<xsl:with-param name="type"><xsl:value-of select='type'/></xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-user-type-name">
		<xsl:param name="type"/>
		<xsl:for-each select="//struct[name = $type]">
			<xsl:value-of select="concat('struct sctk_runtime_config_module_',$type)"/>
		</xsl:for-each>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-header">
<xsl:text>/* ############################# MPC License ############################## */
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
/* #   - AUTOMATIC GENERATION                                             # */
/* #                                                                      # */
/* ######################################################################## */

</xsl:text>
	</xsl:template>
</xsl:stylesheet>
