<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<!--
  XSL stylesheet for processing documentation for the lcd4linux project
    Copyright 2004 Xavier Vello <xavier66@free.fr>
    Copyright 2004 The LCD4Linux Team <lcd4linux-devel@users.sourceforge.net>
-->

<!--The special helpers-->

<xsl:template match="cmd">
  <xsl:element name="div">
    <xsl:attribute name="class">cmd</xsl:attribute>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="conf">
  <xsl:element name="div">
    <xsl:attribute name="class">conf</xsl:attribute>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="note">
  <xsl:element name="div">
    <xsl:attribute name="class">note</xsl:attribute>
    <xsl:element name="img">
      <xsl:attribute name="src">
        <xsl:copy-of select="$root"/>
        <xsl:text>images/note-icon.png</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="alt">
        <xsl:text>Note : </xsl:text>
      </xsl:attribute>  
      <xsl:attribute name="class">icon</xsl:attribute>
    </xsl:element>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="warn">
  <xsl:element name="div">
    <xsl:attribute name="class">warn</xsl:attribute>
    <xsl:element name="img">
      <xsl:attribute name="src">
        <xsl:copy-of select="$root"/>
        <xsl:text>images/warn-icon.png</xsl:text>
      </xsl:attribute>
      <xsl:attribute name="alt">
        <xsl:text>Warning : </xsl:text>
      </xsl:attribute> 
      <xsl:attribute name="class">icon</xsl:attribute>
    </xsl:element>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="new">  <!--This last will be more complicated-->
  <xsl:element name="div">
    <xsl:attribute name="class">new</xsl:attribute>
    <xsl:apply-templates/>
  </xsl:element>
</xsl:template>

<xsl:template match="link" mode="head">
  <xsl:apply-templates select="."/>
  <xsl:element name="br"/>
</xsl:template>

<xsl:template match="link">
  <xsl:choose>
  <xsl:when test="@ref">
    <xsl:variable name="refid" select="@ref"/>
    <xsl:choose>
      <xsl:when test="$references/ref[@id=$refid]">
        <xsl:element name="a">
        <xsl:attribute name="href">
          <xsl:choose>
            <xsl:when test="$references/ref[@id=$refid]/class = $class">
              <xsl:value-of select="concat($references/ref[@id=$refid]/file, '.html', $references/ref[@id=$refid]/anchor)"/>
            </xsl:when> 
            <xsl:when test="not(string-length($references/ref[@id=$refid]/class))">
              <xsl:copy-of select="$root"/>
              <xsl:value-of select="concat($references/ref[@id=$refid]/file, '.html', $references/ref[@id=$refid]/anchor)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="$root"/>
              <xsl:value-of select="concat($references/ref[@id=$refid]/class, '/', $references/ref[@id=$refid]/file, '.html', $references/ref[@id=$refid]/anchor)"/>
            </xsl:otherwise>
          </xsl:choose>              
        </xsl:attribute>
        <xsl:choose>
          <xsl:when test="string-length(.)">
            <xsl:value-of select="."/>
          </xsl:when>
          <xsl:when test="$references/ref[@id=$refid]/label">
            <xsl:value-of select="$references/ref[@id=$refid]/label"/>
           </xsl:when> 
          <xsl:otherwise>
            <xsl:text>NO LABEL</xsl:text> 
          </xsl:otherwise>
        </xsl:choose>
        </xsl:element>   
      </xsl:when>
    </xsl:choose>
  </xsl:when>
  <xsl:when test="@url">
    <xsl:element name="a">
      <xsl:attribute name="href">
        <xsl:value-of select="@url"/>
      </xsl:attribute>
      <xsl:choose>
        <xsl:when test="string-length(.)">
          <xsl:value-of select="."/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="@url"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:element> 
  </xsl:when>
  <xsl:otherwise>
    <xsl:message><xsl:value-of select="current()"/> : invalid link !</xsl:message>
  </xsl:otherwise> 
  </xsl:choose>
</xsl:template>

<xsl:template match="index">
  <xsl:choose>
    <xsl:when test="@class">
      <xsl:apply-templates select="$references/ref">
       <xsl:with-param name="class"><xsl:value-of select="@class"/></xsl:with-param>
     </xsl:apply-templates>      
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="$references/ref"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
