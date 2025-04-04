<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <!-- Identity transform - copy everything by default -->
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <!-- Skip markAttachmentType comments -->
  <xsl:template match="comment()[contains(., 'markAttachmentType')]">
    <!-- Skip these comments as they will be generated based on LookupFlag value -->
  </xsl:template>

  <!-- Create the root element -->
  <xsl:template match="/">
    <LookupList>
      <xsl:apply-templates select="LookupList/Lookup"/>
    </LookupList>
  </xsl:template>

  <!-- Remove index attributes from Lookup elements -->
  <xsl:template match="Lookup">
    <Lookup>
      <!-- Copy LookupType and LookupFlag attributes -->
      <xsl:copy-of select="@LookupType"/>
      <xsl:copy-of select="@LookupFlag"/>
      <!-- Copy all child nodes -->
      <xsl:apply-templates select="node()"/>
    </Lookup>
  </xsl:template>

  <!-- Remove index attributes from other elements -->
  <xsl:template match="@index"/>

  <!-- Handle LookupFlag elements and normalize both value and markAttachmentType comments -->
  <xsl:template match="LookupFlag">
    <xsl:variable name="flag_value" select="number(@value)"/>
    <xsl:variable name="upper_bytes" select="floor($flag_value div 256)"/>

    <LookupFlag>
      <!-- If upper bytes are non-zero, normalize to a fixed value (256) -->
      <xsl:choose>
        <xsl:when test="$upper_bytes &gt; 0">
          <xsl:attribute name="value">256</xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
          <xsl:copy-of select="@value"/>
        </xsl:otherwise>
      </xsl:choose>
    </LookupFlag>

    <!-- Add markAttachmentType comment if upper bytes are non-zero -->
    <xsl:if test="$upper_bytes &gt; 0">
      <xsl:comment> markAttachmentType[1] </xsl:comment>
    </xsl:if>
  </xsl:template>

  <!-- Handle ExtensionSubst elements -->
  <xsl:template match="ExtensionSubst">
    <ExtensionSubst>
      <xsl:copy-of select="@Format"/>
      <xsl:apply-templates select="node()"/>
    </ExtensionSubst>
  </xsl:template>

  <!-- Handle Coverage elements -->
  <xsl:template match="BacktrackCoverage|InputCoverage|LookAheadCoverage|Coverage">
    <xsl:element name="{name()}">
      <xsl:apply-templates select="node()"/>
    </xsl:element>
  </xsl:template>

  <!-- Handle SubstLookupRecord elements -->
  <xsl:template match="SubstLookupRecord">
    <SubstLookupRecord>
      <xsl:apply-templates select="node()"/>
    </SubstLookupRecord>
  </xsl:template>

  <!-- Normalize LookupListIndex values to a fixed value -->
  <xsl:template match="LookupListIndex">
    <LookupListIndex>
      <!-- Copy all attributes except value -->
      <xsl:for-each select="@*[name() != 'value']">
        <xsl:copy/>
      </xsl:for-each>
      <!-- Set a fixed value for all LookupListIndex elements -->
      <xsl:attribute name="value">0</xsl:attribute>
    </LookupListIndex>
  </xsl:template>
</xsl:stylesheet>
