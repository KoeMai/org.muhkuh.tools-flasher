<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text" encoding="UTF-8"/>
	
	<xsl:include href="../targets/version/flasher_version.xsl"/>
	
	<xsl:template match="/">
		<xsl:text>
This is a list of all known SPI flashes for the flasher <xsl:value-of select="$FLASHER_VERSION_ALL"/>:

</xsl:text>
		<xsl:apply-templates select="KnownSerialFlashes"/>
	</xsl:template>

	<xsl:template match="KnownSerialFlashes">
		<xsl:for-each select="SerialFlash">
			<xsl:sort select="@name"/>
			<xsl:text>
    * <xsl:value-of select="@name"/>
</xsl:text>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
