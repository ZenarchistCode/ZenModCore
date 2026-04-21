modded class OptionsMenuGame
{
	static const string LAYOUT_SECTION = "ZenModCore/gui/layouts/zen_mod_options/zen_mod_section.layout";

	protected ref array<ref ZenOptionsUIRow> m_ZenRows;
	protected bool m_ZenInjected;

	void OptionsMenuGame(Widget parent, Widget details_root, GameOptions options, OptionsMenu menu)
	{
		ZenInitModOptionsUI();
	}

	protected void ZenInitModOptionsUI()
	{
		if (m_ZenInjected)
			return;

		m_ZenInjected = true;

		if (!m_Root)
			return;

		ZenOptions opts = ZenOptions.Get();
		array<ref ZenOptionsCategory> cats = opts.GetCategories();
		if (!cats)
			return;

		if (cats.Count() == 0)
			return;

		Widget gameSettingsRoot = m_Root.FindAnyWidget("game_settings_root");
		if (!gameSettingsRoot)
			return;

		if (!m_ZenRows)
			m_ZenRows = new array<ref ZenOptionsUIRow>;

		for (int c = 0; c < cats.Count(); c++)
		{
			ZenOptionsCategory cat = cats[c];
			if (!cat)
				continue;

			if (!cat.Settings)
				continue;

			if (cat.Settings.Count() == 0)
				continue;
			
			ZenOptionsUIUtils.SortSettingsByDisplayPriority(cat.Settings);

			Widget section = g_Game.GetWorkspace().CreateWidgets(LAYOUT_SECTION, gameSettingsRoot);
			if (!section)
				continue;

			TextWidget headerText = TextWidget.Cast(section.FindAnyWidget("zen_mod_section_text"));
			if (headerText)
				headerText.SetText(cat.DisplayName);

			Widget contentRoot = section.FindAnyWidget("zen_mod_section_content");
			if (!contentRoot)
				contentRoot = section;

			for (int i = 0; i < cat.Settings.Count(); i++)
			{
				ZenModSetting s = cat.Settings[i];
				if (!s)
					continue;

				int userId = ZenOptionsUIUtils.NextUserId();

				if (m_TextMap)
					m_TextMap.Insert(userId, new Param2<string, string>(s.Title, s.Description));

				ZenOptionsUIRow row = new ZenOptionsUIRow(contentRoot, s, userId, this);

				if (row && row.GetSelector())
					row.GetSelector().m_OptionChanged.Insert(ZenOnAnyModOptionChanged);

				m_ZenRows.Insert(row);
			}
		}

		g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ZenUpdateScrollbarVisibility, 1, false);
	}
	
	protected void ZenUpdateScrollbarVisibility()
	{
		if (!m_Root)
			return;

		ScrollWidget scroll = ScrollWidget.Cast(m_Root.FindAnyWidget("game_settings_scroll"));
		Widget contentRoot = m_Root.FindAnyWidget("game_settings_root");
	
		if (!scroll || !contentRoot)
			return;
	
		// Force a layout pass (helps Size-To-Content widgets catch up)
		contentRoot.Update();
		scroll.Update();
		m_Root.Update();
	
		float sx;
		float viewH;
		scroll.GetScreenSize(sx, viewH);
	
		// Compute "real" content height based on top-level children screen bounds
		float contentH = ZenComputeChildrenHeight(contentRoot);
	
		int show;
		if (contentH > (viewH + 1.0)) // 1px epsilon avoids rounding flicker
			show = 1;
		else
			show = 0;
	
		scroll.SetAlpha(show);
	
		// Only draggable when visible
		if (show == 1)
			scroll.ClearFlags(WidgetFlags.IGNOREPOINTER);
		else
			scroll.SetFlags(WidgetFlags.IGNOREPOINTER);
	
		// Debug if you want:
		// ZMPrint("[ZenOptions] viewH=" + viewH + " contentH=" + contentH + " show=" + show);
	}
	
	protected float ZenComputeChildrenHeight(Widget root)
	{
		float rx, ry;
		root.GetScreenPos(rx, ry);
	
		float bottom = ry;
	
		Widget child = root.GetChildren();
		while (child)
		{
			float cx, cy;
			float cw, ch;
	
			child.GetScreenPos(cx, cy);
			child.GetScreenSize(cw, ch);
	
			float b = cy + ch;
			if (b > bottom)
				bottom = b;
	
			child = child.GetSibling();
		}
	
		return bottom - ry;
	}

	protected void ZenOnAnyModOptionChanged(int idx)
	{
		if (m_Menu)
			m_Menu.OnChanged();
	}

	override bool IsChanged()
	{
		if (super.IsChanged())
			return true;

		return ZenOptions.Get().IsAnyChanged();
	}

	override void Apply()
	{
		super.Apply();
		
		ZenOptions.Get().ApplyPending();
	}

	override void Revert()
	{
		super.Revert();

		ZenOptions.Get().RevertPending();

		if (!m_ZenRows)
			return;

		foreach (ZenOptionsUIRow row : m_ZenRows)
		{
			if (row)
				row.RefreshFromSetting();
		}
	}
}
