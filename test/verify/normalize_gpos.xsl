<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
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

  <!-- Normalize MarkFilteringSet values to a fixed value "0" -->
  <xsl:template match="MarkFilteringSet">
    <MarkFilteringSet>
      <xsl:attribute name="value">0</xsl:attribute>
    </MarkFilteringSet>
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
