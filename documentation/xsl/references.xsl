<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!--
  XSL stylesheet for processing documentation for the lcd4linux project
    Copyright 2004 Xavier Vello <xavier66@free.fr>
    Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
-->

<xsl:template match="ref" node="references">
  <xsl:param name="class" select="''"/>
  <xsl:choose>
    <xsl:when test="class=$class">
      <xsl:apply-templates select="." mode="link"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="ref" node="references" mode="link">
  <xsl:choose>
  <xsl:when test="not(hiden)">
  <xsl:element name="a">
    <xsl:attribute name="href">
      <xsl:choose>
        <xsl:when test="not(string-length(class))">
          <xsl:copy-of select="$root"/>
          <xsl:value-of select="concat(file, '.html')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:copy-of select="$root"/>
          <xsl:value-of select="concat(class, '/', file, '.html', anchor)"/>
        </xsl:otherwise>
      </xsl:choose>              
    </xsl:attribute>
    <xsl:choose>
      <xsl:when test="label">
        <xsl:value-of select="label"/>
      </xsl:when> 
      <xsl:otherwise>
        <xsl:value-of select="file"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:element>  
  <xsl:element name="br"/>
  </xsl:when>
  </xsl:choose>  
</xsl:template>

</xsl:stylesheet>
