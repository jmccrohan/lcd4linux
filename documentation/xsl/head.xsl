<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!--
  XSL stylesheet for processing documentation for the lcd4linux project
    Copyright 2004 Xavier Vello <xavier66@free.fr>
    Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
-->

<!--The info node, to generate headers-->

<xsl:template match="head" node="doc">
  <xsl:element name="head">
    <xsl:apply-templates select="title"/>
    <xsl:apply-templates select="ref"/>
    <xsl:element name="link">
      <xsl:attribute name="rel">stylesheet</xsl:attribute>
       <xsl:attribute name="href">
         <xsl:copy-of select="$root"/>
         <xsl:text>doc.css</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="type">text/css</xsl:attribute>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="title" node="head">
  <xsl:element name="title">
    <xsl:value-of select="."/>
  </xsl:element>
</xsl:template>

<xsl:template match="ref" node="head">
</xsl:template>

<xsl:template match="history" node="head">
  <table border="1">
  <xsl:for-each select="revision">
    <tr>
      <td><xsl:value-of select="@version"/></td>
      <td><xsl:value-of select="."/></td>
    </tr>
  </xsl:for-each>
  </table>
</xsl:template> 

<xsl:template match="links" node="head">
  <xsl:element name="div">
    <xsl:attribute name="class">links</xsl:attribute>
    <xsl:element name="div">
      <xsl:attribute name="class">title</xsl:attribute>
      See Also
    </xsl:element>
      
    <xsl:apply-templates select="link" mode="head"/>
      
    <xsl:element name="div"> 
      <xsl:attribute name="class">index</xsl:attribute>
      <xsl:element name="a">
        <xsl:attribute name="href">
          <xsl:copy-of select="$root"/>
          <xsl:text>index.html</xsl:text>
        </xsl:attribute>
        <xsl:text>&lt; Index</xsl:text>
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template> 

</xsl:stylesheet>
