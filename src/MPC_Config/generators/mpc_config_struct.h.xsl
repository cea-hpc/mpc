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
		<xsl:text>#include &lt;mpc_common_types.h&gt;&#10;</xsl:text>
		<xsl:text>#include "mpc_config_struct_defaults.h"&#10;&#10;</xsl:text>
		<xsl:text>#ifndef SCTK_RUNTIME_CONFIG_STRUCT_H&#10;</xsl:text>
		<xsl:text>#define SCTK_RUNTIME_CONFIG_STRUCT_H&#10;</xsl:text>
		<xsl:text>&#10;#define SCTK_RUNTIME_CONFIG_MAX_PROFILES 16&#10;</xsl:text>

		<xsl:call-template name="gen-funcptr-struct"/>
		<xsl:apply-templates select="config"/>
		<xsl:call-template name="gen-final-module-struct"/>
		<xsl:call-template name="gen-config-struct"/>
		<xsl:text>&#10;#endif /* SCTK_RUNTIME_CONFIG_STRUCT_H */&#10;</xsl:text>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-config-struct">
		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#09;int number_profiles;&#10;</xsl:text>
		<xsl:text>&#09;char* profiles_name_list[SCTK_RUNTIME_CONFIG_MAX_PROFILES];&#10;</xsl:text>
		<xsl:text>&#09;struct sctk_runtime_config_modules modules;&#10;</xsl:text>
		<xsl:text>&#09;struct sctk_runtime_config_struct_networks networks;&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-funcptr-struct">
		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config_funcptr&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#09;char * name;&#10;</xsl:text>
		<xsl:text>&#09;void (* value)();&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-final-module-struct">
		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:text>struct sctk_runtime_config_modules&#10;</xsl:text>
		<xsl:text>{</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="modules">
				<xsl:for-each select="module">
					<xsl:value-of select="concat('&#10;&#9;struct sctk_runtime_config_struct_',@type,' ',@name,';')"/>
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
		<xsl:apply-templates select="enum"/>
		<xsl:apply-templates select="struct|union"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="enum">
		<xsl:text>&#10;/********************************** ENUM ************************************/&#10;</xsl:text>
		<xsl:value-of select="concat('/**',@doc,'**/&#10;')"/>
		<xsl:value-of select="concat('enum ',@name,'&#10;')"/>
		<xsl:text>{&#10;</xsl:text>
		<xsl:for-each select="value">
			<xsl:value-of select="concat('&#9;', .)"/>
			<xsl:if test="position() != last()">
				<xsl:text>,</xsl:text>
			</xsl:if>
			<xsl:text>&#10;</xsl:text>
		</xsl:for-each>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
		<xsl:text>&#10;/********************************** ENUM ************************************/&#10;</xsl:text>
		<xsl:value-of select="concat('/**',@doc,'**/&#10;')"/>
		<xsl:value-of select="concat('enum sctk_runtime_config_struct_',@name,'_type&#10;')"/>
		<xsl:text>{&#10;</xsl:text>
		<xsl:value-of select="concat('&#09;SCTK_RTCFG_',@name,'_NONE,&#10;')"/>
		<xsl:for-each select="choice">
			<xsl:value-of select="concat('&#09;SCTK_RTCFG_',../@name,'_',@name,',&#10;')"/>
		</xsl:for-each>
		<xsl:text>};&#10;</xsl:text>

		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:value-of select="concat('/**',@doc,'**/&#10;')"/>
		<xsl:value-of select="concat('struct sctk_runtime_config_struct_',@name,'&#10;')"/>
		<xsl:text>{&#10;</xsl:text>
			<xsl:value-of select="concat('&#9;enum sctk_runtime_config_struct_',@name,'_type type;&#10;')"/>
			<xsl:text>&#09;union {&#10;</xsl:text>
			<xsl:apply-templates select="choice"/>
			<xsl:text>&#09;} value;&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="choice">
		<xsl:text>&#9;</xsl:text>
		<xsl:call-template name="param"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:text>&#10;/******************************** STRUCTURE *********************************/&#10;</xsl:text>
		<xsl:value-of select="concat('/**',@doc,'**/&#10;')"/>
		<xsl:value-of select="concat('struct sctk_runtime_config_struct_',@name,'&#10;')"/>
		<xsl:text>{</xsl:text>
		<xsl:text>&#09;int init_done;&#10;</xsl:text>
		<xsl:apply-templates select="param"/>
		<xsl:apply-templates select="array"/>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param" name="param">
		<xsl:if test="@doc">
			<xsl:value-of select="concat('&#9;/**',@doc,'**/&#10;')"/>
		</xsl:if>
		<xsl:text>&#9;</xsl:text>
		<xsl:choose>
			<xsl:when test="@type = 'funcptr'">
				<xsl:value-of select="concat('struct sctk_runtime_config_funcptr ',@name)"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gen-type-name"/>
				<xsl:value-of select="concat(' ',@name)"/>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:value-of select="concat('&#9;/**',@doc,'**/&#10;')"/>
		<xsl:text>&#9;</xsl:text>
		<xsl:call-template name="gen-type-name"/>
		<xsl:value-of select="concat(' * ',@name)"/>
		<xsl:text>;&#10;</xsl:text>
		<xsl:value-of select="concat('&#9;/** Number of elements in ',@name,' array. **/&#10;')"/>
		<xsl:value-of select="concat('&#9;int ',@name,'_size;&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-funcptr">
		<xsl:choose>
			<xsl:when test="return"><xsl:apply-templates select="gen-return" /></xsl:when>
			<xsl:otherwise><xsl:text>void</xsl:text></xsl:otherwise>
		</xsl:choose>
		<xsl:value-of select="concat(' (*',@name, ') (')"/>
		<xsl:choose>
			<xsl:when test="arg"><xsl:call-template name="gen-arg"/></xsl:when>
			<xsl:otherwise><xsl:text>void</xsl:text></xsl:otherwise>
		</xsl:choose>
		<xsl:text>)</xsl:text>
	</xsl:template>
	
	<!-- ********************************************************* -->
	<xsl:template match="return" name="gen-return">
		<xsl:call-template name="gen-type-name"/>
	</xsl:template>
	
    <!-- ********************************************************* -->
    <xsl:template name="gen-arg">
	    <xsl:for-each select="arg">
		    <xsl:call-template name="gen-type-name"/>
		    <xsl:value-of select="concat(' ',@name)"/>
		    <xsl:if test="not(position() = last())">, </xsl:if>
	    </xsl:for-each>
    </xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-type-name">
		<xsl:choose>
			<xsl:when test="@type = 'int'">int</xsl:when>
			<xsl:when test="@type = 'bool'">short int</xsl:when>
			<xsl:when test="@type = 'float'">float</xsl:when>
			<xsl:when test="@type = 'double'">double</xsl:when>
			<xsl:when test="@type = 'string'">char *</xsl:when>
			<xsl:when test="@type = 'size'">size_t</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gen-user-type-name">
					<xsl:with-param name="type"><xsl:value-of select='@type'/></xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-user-type-name">
		<xsl:param name="type"/>
		<xsl:choose>
			<xsl:when test="//struct[@name = $type]">
				<xsl:value-of select="concat('struct sctk_runtime_config_struct_',$type)"/>
			</xsl:when>
			<xsl:when test="//union[@name = $type]">
				<xsl:value-of select="concat('struct sctk_runtime_config_struct_',$type)"/>
			</xsl:when>
			<xsl:when test="//enum[@name = $type]">
				<xsl:value-of select="concat('enum ',$type)"/>
			</xsl:when>
		</xsl:choose>
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
