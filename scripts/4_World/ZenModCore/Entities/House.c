modded class House
{
	protected bool m_WasZenHologrammed;

	void SetZenHologrammed(bool hologram, string textureOverride = "")
	{
		m_WasZenHologrammed = hologram;

		array<string> config_textures = GetHiddenSelectionsTextures();
		if (!config_textures || config_textures.Count() == 0)
			return;

		if (m_WasZenHologrammed)
		{
			string textureAlpha = "#(argb,8,8,3)color(1,1,1,0.1,ca)";
			if (textureOverride != "")
				textureAlpha = textureOverride;

			SetObjectTexture(0, textureAlpha);
		}
		else
		{
			string textureNoAlpha = config_textures.Get(0);
			SetObjectTexture(0, textureNoAlpha);
		}
	}

	bool IsZenHologrammed()
	{
		return m_WasZenHologrammed;
	}

	bool ShouldZenHologram()
	{
		#ifdef ZenModPack
		return ZenModEnabled("ZenHologram");
		#endif
		
		return false;
	}
}