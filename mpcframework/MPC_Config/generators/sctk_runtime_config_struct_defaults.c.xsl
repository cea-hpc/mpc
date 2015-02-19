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
#   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     #
#                                                                      #
########################################################################
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>

	<!-- ********************************************************* -->
	<xsl:template match="all">
		<xsl:call-template name="gen-mpc-header"/>
		<xsl:text>#include &lt;stdlib.h&gt;&#10;</xsl:text>
		<xsl:text>#include &lt;string.h&gt;&#10;</xsl:text>
		<xsl:text>#include &lt;dlfcn.h&gt;&#10;</xsl:text>
		<xsl:text>#include "sctk_runtime_config_struct.h"&#10;</xsl:text>
		<xsl:text>#include "sctk_runtime_config_struct_defaults.h"&#10;</xsl:text>
		<xsl:text>#include "sctk_runtime_config_mapper.h"&#10;</xsl:text>
		<xsl:apply-templates select='config'/>
		<xsl:call-template name="gen-main-reset-function"/>
		<xsl:call-template name="gen-struct-default"/>
		<xsl:call-template name="gen-struct-offset"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-struct-name">
		<xsl:value-of select="concat('struct sctk_runtime_config_struct_',@name)"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-main-reset-function">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:text>void sctk_runtime_config_reset(struct sctk_runtime_config * config)&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#9;memset(config, 0, sizeof(struct sctk_runtime_config));&#10;</xsl:text>
		<xsl:text>&#9;sctk_handler = dlopen(0, RTLD_LAZY | RTLD_GLOBAL);&#10;</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="modules">
				<xsl:for-each select="module">
					<xsl:value-of select="concat('&#9;sctk_runtime_config_struct_init_',@type,'(&amp;config->modules.',@name,');&#10;')"/>
				</xsl:for-each>
			</xsl:for-each>
			<xsl:for-each select="usertypes">
				<xsl:for-each select="enum">
					<xsl:value-of select="concat('&#9;sctk_runtime_config_enum_init_',@name,'();&#10;')"/>
				</xsl:for-each>
			</xsl:for-each>
			
		</xsl:for-each>
		<xsl:text>&#09;sctk_runtime_config_struct_init_networks(&amp;config->networks);&#10;</xsl:text>
		<xsl:text>&#09;dlclose(sctk_handler);&#10;</xsl:text>
		<xsl:text>}&#10;&#10;</xsl:text>

		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:text>void sctk_runtime_config_clean_hash_tables()&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#09;struct enum_type * current_enum, * tmp_enum;&#10;</xsl:text>
		<xsl:text>&#09;struct enum_value * current_value, * tmp_value;&#10;&#10;</xsl:text>

		<xsl:text>&#09;HASH_ITER(hh, enums_types, current_enum, tmp_enum) {&#10;</xsl:text>
		<xsl:text>&#09;&#09;HASH_ITER(hh, current_enum->values, current_value, tmp_value) {&#10;</xsl:text>
		<xsl:text>&#09;&#09;&#09;HASH_DEL(current_enum->values, current_value);&#10;</xsl:text>
		<xsl:text>&#09;&#09;&#09;free(current_value);&#10;</xsl:text>
		<xsl:text>&#09;&#09;}&#10;</xsl:text>
		<xsl:text>&#09;&#09;HASH_DEL(enums_types, current_enum);&#10;</xsl:text>
		<xsl:text>&#09;&#09;free(current_enum);&#10;</xsl:text>
		<xsl:text>&#09;}&#10;</xsl:text>
		<xsl:text>}&#10;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-struct-default">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:text>void sctk_runtime_config_reset_struct_default_if_needed(char * structname, void * ptr )&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="usertypes">
				<xsl:for-each select="struct">
					<xsl:value-of select="concat('&#9;if( !strcmp( structname , &quot;sctk_runtime_config_struct_',@name,'&quot;) )&#10;&#9;{&#10;')"/>
					<xsl:value-of select="concat('&#9;&#9;sctk_runtime_config_struct_init_',@name,'( ptr );&#10;')"/>
					<xsl:text>&#9;&#9;return;&#10;</xsl:text>
					<xsl:text>&#9;}&#10;&#10;</xsl:text>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
		<xsl:text>}&#10;&#10;</xsl:text>

	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gen-struct-offset">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:text>void * sctk_runtime_config_get_union_value_offset(char * unionname, void * ptr )&#10;</xsl:text>
		<xsl:text>{&#10;</xsl:text>
		<xsl:for-each select="config">
			<xsl:for-each select="usertypes">
				<xsl:for-each select="union">
					<xsl:value-of select="concat('&#9;if( !strcmp( unionname , &quot;sctk_runtime_config_struct_',@name,'&quot;) )&#10;&#9;{&#10;')"/>
					<xsl:text>&#9;&#9;</xsl:text>
					<xsl:call-template name="gen-struct-name"/>
					<xsl:value-of select="concat('&#9;* obj_',@name,' = ptr;&#10;')"/>
					<xsl:value-of select="concat('&#9;&#9;return &amp;(obj_',@name,'->value);&#10;')"/>

					<xsl:text>&#9;}&#10;&#10;</xsl:text>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
		<xsl:text>&#9;/* If not found assume sizeof (int) */&#10;</xsl:text>
		<xsl:text>&#9;return (char *)ptr + sizeof(int);&#10;</xsl:text>
		<xsl:text>}&#10;&#10;</xsl:text>

	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="config">
		<xsl:apply-templates select='usertypes'/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="usertypes">
		<xsl:apply-templates select='struct|union|enum'/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="struct">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:value-of select="concat('void sctk_runtime_config_struct_init_',@name,'(void * struct_ptr)&#10;')"/>
		<xsl:text>{&#10;&#09;</xsl:text>
		<xsl:call-template name="gen-struct-name"/>
		<xsl:text> * obj = struct_ptr;&#10;</xsl:text>
		
		<xsl:text>&#09;/* Make sure this element is not initialized yet       */&#10;</xsl:text>
		<xsl:text>&#09;/* It allows us to know when we are facing dynamically */&#10;</xsl:text>
		<xsl:text>&#09;/* allocated objects requiring an init                 */&#10;</xsl:text>
		<xsl:text>&#09;if( obj->init_done != 0 ) return;&#10;&#10;</xsl:text>
		
		<xsl:text>&#09;/* Simple params : */&#10;</xsl:text>
		<xsl:apply-templates select="param"/>
 		<xsl:apply-templates select="array"/>
		<xsl:text>&#09;obj->init_done = 1;&#10;</xsl:text>
		<xsl:text>}&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="enum">
		<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:value-of select="concat('void sctk_runtime_config_enum_init_',@name,'()&#10;')"/>
		<xsl:text>{&#10;</xsl:text>
		<xsl:text>&#09;struct enum_type * current_enum = (struct enum_type *) malloc(sizeof(struct enum_type));&#10;</xsl:text>
		<xsl:text>&#09;struct enum_value * current_value, * values = NULL;&#10;</xsl:text>
		<xsl:value-of select="concat('&#10;&#09;strncpy(current_enum->name, &quot;enum ', @name, '&quot;, 50);&#10;')"/>
		<xsl:apply-templates select="value"/>
		<xsl:text>&#10;&#09;current_enum->values = values;&#10;</xsl:text>
		<xsl:text>&#09;HASH_ADD_STR(enums_types, name, current_enum);&#10;</xsl:text>
		<xsl:text>}&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="value">
		<xsl:text>&#10;&#09;current_value = (struct enum_value *) malloc(sizeof(struct enum_value));&#10;</xsl:text>
		<xsl:value-of select="concat('&#09;strncpy(current_value->name, &quot;', ., '&quot;, 50);&#10;')"/>
		<xsl:value-of select="concat('&#09;current_value->value = ', ., ';&#10;')"/>
		<xsl:text>&#09;HASH_ADD_STR(values, name, current_value);&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="union">
			<xsl:text>&#10;/*******************  FUNCTION  *********************/&#10;</xsl:text>
		<xsl:value-of select="concat('void sctk_runtime_config_struct_init_',@name,'(void * struct_ptr)&#10;')"/>
		<xsl:text>{&#10;&#09;</xsl:text>
		<xsl:call-template name="gen-struct-name"/>
		<xsl:text> * obj = struct_ptr;&#10;</xsl:text>
		<xsl:call-template name="union-no-default"/>
		<xsl:text>}&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="union-no-default">
		<xsl:value-of select="concat('&#09;obj->type = SCTK_RTCFG_',@name,'_NONE;&#10;')"/>
		<xsl:text>&#09;memset(&amp;obj->value,0,sizeof(obj->value));&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="param">
		<xsl:choose>
			<xsl:when test="@type = 'int'"><xsl:call-template name='gent-default-param-int'/></xsl:when>
			<xsl:when test="@type = 'bool'"><xsl:call-template name='gent-default-param-bool'/></xsl:when>
			<xsl:when test="@type = 'string'"><xsl:call-template name='gent-default-param-string'/></xsl:when>
			<xsl:when test="@type = 'float'"><xsl:call-template name='gent-default-param-decimal'/></xsl:when>
			<xsl:when test="@type = 'double'"><xsl:call-template name='gent-default-param-decimal'/></xsl:when>
			<xsl:when test="@type = 'size'"><xsl:call-template name='gent-default-param-size'/></xsl:when>
			<xsl:when test="@type = 'funcptr'"><xsl:call-template name='gent-default-param-funcptr'/></xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="gent-default-param-usertype">
					<xsl:with-param name="type"><xsl:value-of select='@type'/></xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
			<xsl:value-of select="concat('&#09;sctk_runtime_config_struct_init_',@type,'(&amp;obj->',@name,');&#10;')"/>	
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-size">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='@default'><xsl:value-of select="concat('sctk_runtime_config_map_entry_parse_size(&quot;',@default, '&quot;)' )"/></xsl:when>
			<xsl:otherwise>0</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-int">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='@default'><xsl:value-of select="@default"/></xsl:when>
			<xsl:otherwise>0</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-bool">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='@default'><xsl:value-of select="@default"/></xsl:when>
			<xsl:otherwise>false</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-decimal">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='@default'><xsl:value-of select="@default"/></xsl:when>
			<xsl:otherwise>0.0</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-string">
		<xsl:text>&#09;obj-></xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = </xsl:text>
		<xsl:choose>
			<xsl:when test='@default'><xsl:text>"</xsl:text><xsl:value-of select="@default"/><xsl:text>"</xsl:text></xsl:when>
			<xsl:otherwise>NULL</xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-funcptr">
		<xsl:choose>
			<xsl:when test="@default"><xsl:call-template name="funcptr-default-value"/></xsl:when>
			<xsl:otherwise><xsl:call-template name="funcptr-empty-value"/></xsl:otherwise>
		</xsl:choose>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="funcptr-default-value">
		<xsl:value-of select="concat('&#09;obj->', @name, '.name = &quot;', @default, '&quot;;&#10;')"/>
		<xsl:text>&#09;*(void **) &amp;(obj-&gt;</xsl:text>
		<xsl:value-of select="concat(@name,'.value) = sctk_runtime_config_get_symbol(&quot;', @default, '&quot;)')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="funcptr-empty-value">
		<xsl:value-of select="concat('&#09;obj->', @name, '.name = &quot;&quot;;&#10;')"/>
		<xsl:value-of select="concat('&#09;obj->', @name, '.value = NULL')"/>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="gent-default-param-usertype">
		<xsl:param name="type"/>
		<xsl:choose>
			<xsl:when test="//enum[@name = $type]">
				<xsl:choose>
					<xsl:when test="@default">
						<xsl:value-of select="concat('&#09;obj->',@name,' = ', @default, ';&#10;')"/>					
					</xsl:when>
					<xsl:otherwise>
						<xsl:message terminate="yes">
							<xsl:value-of select="concat('Error : enum ', $type, ' ', @name, ' must have a default value!!')"/>
						</xsl:message>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="concat('&#09;sctk_runtime_config_struct_init_',@type,'(&amp;obj->',@name,');&#10;')"/>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template match="array">
		<xsl:text>&#09;/* array */&#10;</xsl:text>
		<xsl:choose>
			<xsl:when test="default"><xsl:call-template name="array-default"/></xsl:when>
			<xsl:otherwise><xsl:call-template name="array-empty"/></xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-empty">
		<!-- pointer -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text> = NULL;&#10;</xsl:text>
		<!-- size -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text>_size = 0;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-default">
		<!-- pointer -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:choose>
			<xsl:when test="(@type='string')"> <xsl:value-of select="concat(' = calloc(',count(default/value),',sizeof(char *));&#10;')"/> </xsl:when>
			<xsl:otherwise> <xsl:value-of select="concat(' = calloc(',count(default/value),',sizeof(',@type,'));&#10;')"/> </xsl:otherwise>
		</xsl:choose>
		
		<xsl:call-template name="array-default-values"/>
		<!-- size -->
		<xsl:text>&#09;obj-&gt;</xsl:text>
		<xsl:value-of select='@name'/>
		<xsl:text>_size = </xsl:text>
		<xsl:value-of select="count(default/value)"/>
		<xsl:text>;&#10;</xsl:text>
	</xsl:template>

	<!-- ********************************************************* -->
	<xsl:template name="array-default-values">
		<xsl:for-each select="default/value">
			<xsl:text>&#09;obj-&gt;</xsl:text>
			<xsl:value-of select="../../@name" />
			<xsl:choose>
				<xsl:when test="../../@type = 'string'">
					<xsl:value-of select="concat('[',position()-1,'] = &quot;',.,'&quot;;&#10;')"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="concat('[',position()-1,'] = ',.,';&#10;')"/>
				</xsl:otherwise>
			</xsl:choose>
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
