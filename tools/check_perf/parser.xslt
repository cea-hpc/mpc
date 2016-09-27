<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  	<xsl:template match="*">
		<xsl:for-each select="profile" > 
		<xsl:for-each select="probe" > 
		  <xsl:value-of select="@name" /><xsl:text disable-output-escaping="yes"><![CDATA[| ]]></xsl:text>
		  <xsl:value-of select="hits" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="total" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="min" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="max" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
  <xsl:text>&#10;</xsl:text>
		<xsl:for-each select="probe" > 
		  <xsl:value-of select="@name" /><xsl:text disable-output-escaping="yes"><![CDATA[| ]]></xsl:text>
		  <xsl:value-of select="hits" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="total" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="min" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="max" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
  <xsl:text>&#10;</xsl:text>
		<xsl:for-each select="probe" > 
		  <xsl:value-of select="@name" /><xsl:text disable-output-escaping="yes"><![CDATA[| ]]></xsl:text>
		  <xsl:value-of select="hits" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="total" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="min" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
		  <xsl:value-of select="max" /><xsl:text disable-output-escaping="yes"><![CDATA[ ]]></xsl:text>
  <xsl:text>&#10;</xsl:text>
		</xsl:for-each>
		</xsl:for-each>
		</xsl:for-each>
		</xsl:for-each>
			</xsl:template>

</xsl:stylesheet>
