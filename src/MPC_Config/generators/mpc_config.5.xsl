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
########################################################################-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xs="http://www.w3.org/2001/XMLSchema">

	<!-- ********************************************************* -->
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:call-template name="gen-mpc-man-base"/>
		<xsl:call-template name="gen-mpc-man-module"/>
		<xsl:apply-templates select="config"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select="usertypes"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-man-module">
		<xsl:text>.Sh AVAILABLE MODULES&#10;</xsl:text>
		<xsl:text>.Pp The &lt;modules&gt; node can contain the following fields :&#10;</xsl:text>
		<xsl:text>.Bl -tag -width Ds&#10;</xsl:text>
		<xsl:apply-templates select="config/modules/module"/>
		<xsl:text>.El&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="print-doc-for-user-type">
		<xsl:param name="type"/>
		<xsl:for-each select="//struct[@name = $type]">
			<xsl:value-of select="concat(@doc,'&#10;')"/>
		</xsl:for-each>
		<xsl:for-each select="//union[@name = $type]">
			<xsl:value-of select="concat(@doc,'&#10;')"/>
		</xsl:for-each>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="module">
		<xsl:value-of select="concat('.It Cm ',@name,'&#10;')"/>
		<xsl:call-template name="print-doc-for-user-type">
			<xsl:with-param name="type"><xsl:value-of select='@type'/></xsl:with-param>
		</xsl:call-template>
		<xsl:value-of select="concat('.br&#10;See type ',@type,'.&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select="struct|union"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:value-of select="concat('.Sh ENTRIES OF TYPE ',@name,'&#10;')"/>
		<xsl:value-of select="concat(@doc,'&#10;','.Pp&#10;')"/>
		<xsl:text>It support parameters :&#10;.Pp&#10;</xsl:text>
		<xsl:text>.Bl -tag -width Ds&#10;</xsl:text>
		<xsl:apply-templates select="param|array"/>
		<xsl:text>.El&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:variable name="type_enum" select="@type"/>
		<xsl:choose>
			<xsl:when test="//enum[@name = $type_enum]">
				<xsl:value-of select="concat('.It Cm ',@name,'&#10;','Type is enum ',@type,'. ')"/>
				<xsl:text>Possible values are : </xsl:text>
				<xsl:for-each select="//enum[@name = $type_enum]/value">
					<xsl:value-of select="."/>
					<xsl:if test="position() != last()">
						<xsl:text>, </xsl:text>
					</xsl:if>
					<xsl:if test="position() = last()">
						<xsl:text>.&#10;</xsl:text>
					</xsl:if>
				</xsl:for-each>
				<xsl:text>&#10;</xsl:text>
			</xsl:when>
			<xsl:when test="@type = 'funcptr'">
				<xsl:value-of select="concat('.It Cm ',@name,'&#10;','Type is function pointer. ')"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="concat('.It Cm ',@name,'&#10;','Type is ',@type,'. ')"/>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:if test='@default'><xsl:value-of select="concat('Default value is ',@default,'. ')"/></xsl:if>
		<xsl:text>&#10;</xsl:text>
		<xsl:value-of select="concat('.Pp ',@name,'&#10;',@doc,'&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:param name="type"/>
		<xsl:value-of select="concat('.It Cm ',@name,'&#10;','Type is array of ',$type,'. ')"/>
		<xsl:if test='default and default/value'>
			<xsl:text>Default value is {</xsl:text>
			<xsl:for-each select='default/value'>
				<xsl:value-of select="."/>
				<xsl:if test="position() != count(../value)">
				<xsl:text>, </xsl:text>
				</xsl:if>
			</xsl:for-each>
			<xsl:text>}.</xsl:text>
		</xsl:if>
		<xsl:text>&#10;</xsl:text>
		<xsl:value-of select="concat('.Pp ',@name,'&#10;',@doc,'&#10;')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
		<xsl:value-of select="concat('.Sh OPTIONS OF NODE ',@name,'&#10;')"/>
		<xsl:value-of select="concat(@doc,'&#10;','.Pp&#10;')"/>
		<xsl:text>It can contain a node of type :&#10;.Pp&#10;</xsl:text>
		<xsl:text>.Bl -tag -width Ds&#10;</xsl:text>
		<xsl:apply-templates select="choice"/>
		<xsl:text>.El&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="choice">
		<xsl:value-of select="concat('.It Cm ',@name,'&#10; of type ',@type,'. &#10;')"/>
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
.Nm ${PREFIX}/share/mpcframework/config.xml or ${MPC_SYSTEM_CONFIG}
.Nm File given to ${MPC_USER_CONFIG} or to option --config of
.Xr mpcrun 1
.Sh DESCRIPTION
Their is three important concepts in MPC configuration system :
.Bl -tag -width Ds
.It Cm Profile
Each configuration file provide some profiles. A profile has a name and provide a set of values.
.It Cm Profile mapping
At loading time, we can define which profiles to apply to generate the final configuration (mpcrun --profiles=a,b,c...).
.It Cm Overriding
Each profile can override values of a previous loaded one of the onces from a file previously loaded. So in your profile you can redefine only the specific values you want to change, the others keep their values.
.El
.Pp
Files are loaded in priority order :
.Bl -enum -offset indent -compact
.It
System configuration (${PREFIX}/share/mpcframework/config.xml or $MPC_SYSTEM_CONFIG).
.It
User configuration given to mpcrun with option --config of $MPC_USER_CONFIG.
.El
.Pp
You can find XML schema file for validation in ${PREFIX}/share/mpcframework/mpc-config.xsd. You can validate with one of the two solutions :
.Bl -enum -offset indent -compact
.It
mpc_print_config --user={YOUR_XML_FILE}
.It
xmllint --noout --schema ${PREFIX}/share/mpcframework/mpc-config.xsd {YOUR_XML_FILE}
.El
.Pp
.Sh GLOBAL LAYOUT
The XML file may look like this :
.PP
.br
	&lt;mpc&gt;
.br
		&lt;profiles&gt;
.br
			&lt;profile&gt;
.br
				&lt;name&gt;default&lt;/name&gt;
.br
				&lt;modules&gt;
.br
					...
.br
				&lt;/modules&gt;
.br
			&lt;/profile&gt;
.br
			&lt;profile&gt;
.br
				&lt;name&gt;debug&lt;/name&gt;
.br
				&lt;modules&gt;
.br
					...
.br
				&lt;/modules&gt;
.br
			&lt;/profile&gt;
.br
		&lt;/profiles&gt;
.br
	&lt;/mpc&gt;
.Pp
MPC always use the profile with name "default", others are only enabled with option --profiles= or with automatic mappings by using the &lt;mappins&gt; section.
</xsl:text>
	</xsl:template>

</xsl:stylesheet>
