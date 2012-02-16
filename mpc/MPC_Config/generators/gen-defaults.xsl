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
#   - VALAT Sébastien sebastien.valat@cea.fr                           #
#                                                                      #
########################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:text>#include &lt;stdlib.h&gt;&#10;</xsl:text>
		<xsl:text>#include "sctk_config_struct.h"&#10;</xsl:text>
		<xsl:apply-templates select='config'/>
		<xsl:call-template name="gen-main-reset-function"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-main-reset-function">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:text>void sctk_config_reset(struct sctk_config * config)&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="modules">
				<xsl:for-each select="module">
					<xsl:value-of select="concat('&#9;sctk_config_module_init_',type,'(&amp;config->modules.',name,');&#10;')"/>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
		<xsl:text>};&#10;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select='usertypes'/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select='struct'/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-struct-name">
		<xsl:value-of select="concat('struct sctk_config_module_',name)"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:value-of select="concat('void sctk_config_module_init_',name,'(void * struct_ptr)&#10;')"/>
		<xsl:text>{&#10;&#09;</xsl:text>
		<xsl:call-template name="gen-struct-name"/>
		<xsl:text> * obj = struct_ptr;&#10;</xsl:text>
		<xsl:text>&#09;//Simple params :&#10;</xsl:text>
		<xsl:apply-templates select="param"/>
 		<xsl:apply-templates select="array"/>
		<xsl:text>}&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:choose>
			<xsl:when test="type = 'int'"><xsl:call-template name='gent-default-param-int'/></xsl:when>
			<xsl:when test="type = 'bool'"><xsl:call-template name='gent-default-param-bool'/></xsl:when>
			<xsl:otherwise><xsl:call-template name="gent-default-param-usertype"/></xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-int">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='default'><xsl:value-of select="default"/></xsl:when>
			<xsl:otherwise>0</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-bool">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='default'><xsl:value-of select="default"/></xsl:when>
			<xsl:otherwise>false</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-usertype">
		<xsl:value-of select="concat('&#09;sctk_config_module_init_',type,'(&amp;obj->',name,');&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:text>&#09;//array&#10;</xsl:text>
		<xsl:choose>
			<xsl:when test="default"><xsl:call-template name="array-default"/></xsl:when>
			<xsl:otherwise><xsl:call-template name="array-empty"/></xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-empty">
		<!-- pointer -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='name'/>
		<xsl:text> = NULL;&#10;</xsl:text>
		<!-- size -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='name'/>
		<xsl:text>_size = 0;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-default">
		<!-- pointer -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='name'/>
		<xsl:value-of select="concat(' = calloc(',count(default/value),',sizeof(',type,'));&#10;')"/>
		<xsl:call-template name="array-default-values"/>
		<!-- size -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='name'/>
		<xsl:text>_size = </xsl:text>
		<xsl:value-of select="count(default/value)"/>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-default-values">
		<xsl:for-each select="default/value">
			<xsl:text>&#09;obj-&gt;</xsl:text>
			<xsl:value-of select="../../name" />
			<xsl:value-of select="concat('[',position()-1,'] = ',.,';&#10;')"/>
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
