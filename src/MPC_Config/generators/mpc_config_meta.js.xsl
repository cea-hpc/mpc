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
		<xsl:text>var meta = new Array();&#10;</xsl:text>
		<xsl:text>meta.types = {&#10;</xsl:text>
		<xsl:apply-templates select="config/usertypes"/>
		<xsl:text>};&#10;</xsl:text>
		<xsl:text>&#10;</xsl:text>
		<xsl:text>meta.modules = {&#10;</xsl:text>
		<xsl:apply-templates select="config/modules"/>
		<xsl:text>};&#10;</xsl:text>
		<xsl:text>&#10;</xsl:text>
		<xsl:text>meta.networks = {&#10;</xsl:text>
		<xsl:text>&#9;networks: {type: "networks", name: "networks"},&#10;</xsl:text>
		<xsl:text>};&#10;</xsl:text>
		<xsl:text>&#10;</xsl:text>
		<xsl:text>meta.enum = {&#10;</xsl:text>
		<xsl:apply-templates select="config/usertypes/enum"/>
		<xsl:text>};&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct|union"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:value-of select="concat('&#09;',@name,' : {')"/>
		<xsl:text>type: 'struct', </xsl:text>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:text>childs: {&#10;</xsl:text>
		<xsl:apply-templates select="param|array"/>
		<xsl:text>&#09;}</xsl:text>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:value-of select="concat('&#09;&#09;',@name,': {')"/>
		<xsl:text>mode: 'param', </xsl:text>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:value-of select="concat('type: &quot;',@type,'&quot;, ')"/>
		<xsl:value-of select="concat('doc: &quot;',@doc,'&quot;, ')"/>
		<xsl:choose>
			<xsl:when test="@default">
				<xsl:value-of select="concat('dflt: &quot;',@default,'&quot;, ')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>dflt: null</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:value-of select="concat('&#09;&#09;',@name,': {')"/>
		<xsl:text>mode: 'array', </xsl:text>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:value-of select="concat('type: &quot;',@type,'&quot;, ')"/>
		<xsl:value-of select="concat('entryname: &quot;',@entry-name,'&quot;, ')"/>
		<xsl:choose>
			<xsl:when test="@default">
				<xsl:value-of select="concat('dflt: &quot;',@default,'&quot;, ')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>dflt: null</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
		<xsl:value-of select="concat('&#09;',@name,' : {')"/>
		<xsl:text>type: 'union', </xsl:text>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:text>choice: {&#10;</xsl:text>
		<xsl:apply-templates select="choice"/>
		<xsl:text>&#09;}</xsl:text>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="choice">
		<xsl:value-of select="concat('&#09;&#09;',@name,': {')"/>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:value-of select="concat('type: &quot;',@type,'&quot;')"/>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="enum">
		<xsl:value-of select="concat('&#09;',@name,' : {')"/>
		<xsl:text>type: 'enum', </xsl:text>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:value-of select="concat('doc: &quot;',@doc,'&quot;, ')"/>
		<xsl:text>values: {&#10;</xsl:text>
		<xsl:apply-templates select="value"/>
		<xsl:text>&#09;}</xsl:text>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="value">
		<xsl:value-of select="concat('&#09;&#09;',.,': &quot;', ., '&quot;,&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="modules">
		<xsl:apply-templates select="module"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="module">
		<xsl:value-of select="concat('&#09;',@name,': {')"/>
		<xsl:value-of select="concat('name: &quot;',@name,'&quot;, ')"/>
		<xsl:value-of select="concat('type: &quot;',@type,'&quot;')"/>
		<xsl:text>},&#10;</xsl:text>
	</xsl:template>
</xsl:stylesheet>