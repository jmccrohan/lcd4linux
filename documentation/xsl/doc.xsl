<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!--
  XSL stylesheet for processing documentation for the lcd4linux project
    Copyright 2004 Xavier Vello <xavier66@free.fr>
    Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
-->

<xsl:output omit-xml-declaration="yes" method="xml" encoding="UTF-8" indent="yes" doctype-public="-//W3C//DTD XHTML 1.1//EN" doctype-system="http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd"/>

<xsl:variable name="references" select="document('../data/references.xml')/references"/>
<xsl:param name="class" select="''"/>
<xsl:param name="root" select="''"/>

<!--Includes-->
<xsl:include href="head.xsl"/>
<xsl:include href="body.xsl"/>
<xsl:include href="helpers.xsl"/>
<xsl:include href="xhtml.xsl"/>
<xsl:include href="references.xsl"/>

<!--The start point-->
<xsl:template match="doc">
  <xsl:element name="html">
    <xsl:apply-templates select="head"/>
    <xsl:apply-templates select="body"/>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
