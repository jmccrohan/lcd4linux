<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!--
  XSL stylesheet for processing documentation for the lcd4linux project
    Copyright 2004 Xavier Vello <xavier66@free.fr>
    Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
-->

<!--The content of the page-->

<xsl:template match="body" node="doc">
  <xsl:element name="body">
  <!--Here will be added html code for the layout-->
    <xsl:apply-templates select="/doc/head/links"/>  
    <xsl:element name="h1">
      <xsl:value-of select="/doc/head/title"/>
    </xsl:element>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>
</xsl:stylesheet>
