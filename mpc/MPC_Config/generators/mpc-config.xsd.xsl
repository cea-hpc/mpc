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
	<xsl:output method="xml" indent="yes"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
			<xsl:apply-templates select="config"/>
			<xsl:call-template name="modules"/>
			<xsl:call-template name="gen-static-part"/>
		</xs:schema>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select="usertypes"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="modules">
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="modules">
			<xs:all>
				<xsl:apply-templates select="config/modules/module"/>
			</xs:all>	
		</xs:complexType>
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
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType>
			<xsl:attribute name="name"><xsl:value-of select="concat('user_type_',@name)"/></xsl:attribute>
			<xs:all>
				<xsl:apply-templates select="param|array"/>
			</xs:all>
		</xs:complexType>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xs:element minOccurs="0">
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
		</xs:element>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-xs-type-name">
		<xsl:choose>
			<xsl:when test="@type = 'string'">xs:string</xsl:when>
			<xsl:when test="@type = 'int'">xs:integer</xsl:when>
			<xsl:when test="@type = 'bool'">xs:boolean</xsl:when>
			<xsl:when test="@type = 'float'">xs:decimal</xsl:when>
			<xsl:when test="@type = 'double'">xs:decimal</xsl:when>
			<xsl:when test="@type = 'size'">xs:string</xsl:when>
			<xsl:otherwise><xsl:value-of select="concat('user_type_',@type)"/></xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xs:element minOccurs="0">
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xs:complexType>
				<xs:sequence>
					<xs:element minOccurs="0" maxOccurs="unbounded">
						<xsl:attribute name="name"><xsl:value-of select="@entry-name"/></xsl:attribute>
						<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
					</xs:element>
				</xs:sequence>
			</xs:complexType>
		</xs:element>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType>
			<xsl:attribute name="name"><xsl:value-of select="concat('user_type_',@name)"/></xsl:attribute>
			<xs:choice>
				<xsl:apply-templates select="choice"/>
			</xs:choice>
		</xs:complexType>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="choice">
		<xs:element>
			<xsl:attribute name="name"><xsl:value-of select="@name"/></xsl:attribute>
			<xsl:attribute name="type"><xsl:call-template name="gen-xs-type-name"/></xsl:attribute>
		</xs:element>
	</xsl:template>
	
	<!-- ********************************************************* -->
	<xsl:template name="gen-static-part">
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mapping_selector_env">
			<xs:simpleContent>
				<xs:extension base="xs:string">
					<xs:attribute name="name" type="xs:string" use="required"/>
				</xs:extension>
			</xs:simpleContent>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mapping_selectors">
			<xs:sequence>
				<xs:element name="env" type="static_part_mapping_selector_env" minOccurs="0" maxOccurs="unbounded"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mapping_profiles">
			<xs:sequence>
				<xs:element name="profile" type="xs:string" minOccurs="0" maxOccurs="unbounded"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mapping">
			<xs:all>
				<xs:element name="selectors" type="static_part_mapping_selectors" minOccurs="0"/>
				<xs:element name="profiles" type="static_part_mapping_profiles" minOccurs="0"/>
			</xs:all>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mappings">
			<xs:sequence>
				<xs:element name="mapping" type="static_part_mapping" minOccurs="0" maxOccurs="unbounded"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_profile">
			<xs:sequence>
				<xs:element name="name" type="xs:string"/>
				<xs:element name="modules" type="modules" minOccurs="0"/>
				<xs:element name="networks" type="user_type_networks" minOccurs="0"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_profiles">
			<xs:sequence>
				<xs:element name="profile" type="static_part_profile" minOccurs="0" maxOccurs="unbounded"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:complexType name="static_part_mpc">
			<xs:sequence>
				<xs:element name="profiles" type="static_part_profiles" minOccurs="0"/>
				<xs:element name="mappings" type="static_part_mappings" minOccurs="0"/>
			</xs:sequence>
		</xs:complexType>
		<xsl:comment> ********************************************************* </xsl:comment>
		<xs:element name="mpc" type="static_part_mpc"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-mpc-header">
<xsl:comment>
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
#   - AUTOMATIC GENERATION                                             #
#                                                                      #
########################################################################
</xsl:comment>
	</xsl:template>

</xsl:stylesheet>