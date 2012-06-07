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

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xs="http://www.w3.org/2001/XMLSchema">

	<!-- ********************************************************* -->
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:call-template name="gen-mpc-man-base"/>
		<xsl:apply-templates select="config"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select="usertypes"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="modules">
	
		<!--<xs:complexType name="modules">
			<xs:all>
				<xsl:apply-templates select="config/modules/module"/>
			</xs:all>
		</xs:complexType>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="module">
		<xs:element minOccurs="0">
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
		</xs:element>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct|union"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:value-of select="concat('.Sh OPTIONS OF NODE ',@name,'&#10;')"/>
		<xsl:value-of select="concat(@doc,'&#10;','.Pp&#10;')"/>
		<xsl:text>It support parameters :&#10;.Pp&#10;</xsl:text>
		<xsl:text>.Bl -tag -width Ds&#10;</xsl:text>
		<xsl:apply-templates select="param|array"/>
		<xsl:text>.El&#10;</xsl:text>
<!--		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType>
			<xsl:attribute name="name"><xsl:value-of select="concat('user_type_',@name)"/></xsl:attribute>
			<xs:all>
				<xsl:apply-templates select="param|array"/>
			</xs:all>
		</xs:complexType>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:value-of select="concat('.It Cm ',@name,'&#10;','Type is ',@type,'. ')"/>
		<xsl:if test='@default'><xsl:value-of select="concat('Default value if ',@default,'. ')"/></xsl:if>
		<xsl:text>&#10;</xsl:text>
		<xsl:value-of select="concat('.Pp ',@name,'&#10;',@doc,'&#10;')"/>
<!--		<xs:element minOccurs="0">
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
		</xs:element>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-xs-type-name">
<!--		<xsl:choose>
			<xsl:when test="@type = 'string'">xs:string</xsl:when>
			<xsl:when test="@type = 'int'">xs:integer</xsl:when>
			<xsl:when test="@type = 'bool'">xs:boolean</xsl:when>
			<xsl:when test="@type = 'float'">xs:decimal</xsl:when>
			<xsl:when test="@type = 'double'">xs:decimal</xsl:when>
			<xsl:when test="@type = 'size'">xs:string</xsl:when>
			<xsl:otherwise><xsl:value-of select="concat('user_type_',@type)"/></xsl:otherwise>
		</xsl:choose>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
<!--		<xs:element minOccurs="0">
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xs:complexType>
				<xs:sequence>
					<xs:element minOccurs="0" maxOccurs="unbounded">
						<xsl:attribute name="name"><xsl:value-of select="@entry-name"/></xsl:attribute>
						<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
					</xs:element>
				</xs:sequence>
			</xs:complexType>
		</xs:element>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
<!--		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType>
			<xsl:attribute name="name"><xsl:value-of select="concat('user_type_',@name)"/></xsl:attribute>
			<xs:choice>
				<xsl:apply-templates select="choice"/>
			</xs:choice>
		</xs:complexType>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="choice">
<!--		<xs:element>
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
		</xs:element>-->
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-header">
<xsl:text>
.\" ############################# MPC License ##############################
.\" # Wed Nov 19 15:19:19 CET 2008                                         #
.\" # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          #
.\" #                                                                      #
.\" # IDDN.FR.001.230040.000.S.P.2007.000.10000                            #
.\" # This file is part of the MPC Runtime.                                #
.\" #                                                                      #
.\" # This software is governed by the CeCILL-C license under French law   #
.\" # and abiding by the rules of distribution of free software.  You can  #
.\" # use, modify and/ or redistribute the software under the terms of     #
.\" # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     #
.\" # following URL http://www.cecill.info.                                #
.\" #                                                                      #
.\" # The fact that you are presently reading this means that you have     #
.\" # had knowledge of the CeCILL-C license and that you accept its        #
.\" # terms.                                                               #
.\" #                                                                      #
.\" # Authors:                                                             #
.\" #   - VALAT Sebastien sebastien.valat@cea.fr                           #
.\" #   - AUTOMATIC GENERATION                                             #
.\" #                                                                      #
.\" ########################################################################
</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-man-base">
	<xsl:text>
.Dd $Mdocdate: June 6 2012 $
.Dt MPC_CONFIG 5
.Os
.Sh NAME
.Nm mpc_config
.Nd MPC runtime configuration files
.Sh SYNOPSIS
.Nm ~/.mpc/config or ${MPC_USER_CONFIG}
.Nm ${PREFIX}/share/mpc/config.xml or ${MPC_SYSTEM_CONFIG}
.Nm File given to option --config of
.Xr mpcrun 1
.Sh DESCRIPTION
Their is three important concepts in MPC configuration system :
.Bl -tag -width Ds

.It Cm Profile
Each configuration file provide some profiles. A profile has a name and provide a set of values.
.It Cm Profile mapping
At loading time, we can define which profiles to apply to generate the final configuration.
.It Cm Overriding
Each profile can override values of a previous loaded one of the onces from a file previously loaded. So in your profile you can redefine only the specific values you want to change, the others keep their values.
.El
.Pp
Files are loaded in priority order :
.Bl -enum -offset indent -compact
.It
System configuration (${PREFIX}/share/mpc/config.xml or $MPC_SYSTEM_CONFIG)
.It
User configuration (~/.mpc/config or ${MPC_USER_CONFIG})
.It
Application configuration given to mpcrun with option --config
.El
.Pp
.Sh OPTIONS
MPC configuration system support the given options :
	</xsl:text>
	</xsl:template>
</xsl:stylesheet>
