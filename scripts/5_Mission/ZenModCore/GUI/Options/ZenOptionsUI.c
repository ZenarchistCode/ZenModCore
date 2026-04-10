class ZenOptionsUIUtils
{
	static int s_NextUserId = 909090;

	static int NextUserId()
	{
		s_NextUserId++;
		return s_NextUserId;
	}
}

class ZenOptionsUIRow
{
	static const string LAYOUT_ROW = "ZenModCore/gui/layouts/zen_mod_options/zen_mod_setting_row.layout";

	protected ref ZenModSetting m_Setting;

	protected Widget m_RowRoot;
	protected TextWidget m_Label;
	protected Widget m_OptionRoot;

	protected ref OptionSelectorMultistate m_Selector;

	// Our own value label (clips correctly)
	protected TextWidget m_ValueText;

	void ZenOptionsUIRow(Widget parent, ZenModSetting setting, int userId, ScriptedWidgetEventHandler owner)
	{
		m_Setting = setting;

		m_RowRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT_ROW, parent);
		if (!m_RowRoot)
			return;

		m_Label = TextWidget.Cast(m_RowRoot.FindAnyWidget("zen_mod_setting_label"));
		if (m_Label && m_Setting)
			m_Label.SetText(m_Setting.Title);

		m_OptionRoot = m_RowRoot.FindAnyWidget("zen_mod_setting_option");
		if (!m_OptionRoot)
			return;

		m_OptionRoot.SetUserID(userId);

		// Grab our own value label from the layout
		m_ValueText = TextWidget.Cast(m_OptionRoot.FindAnyWidget("zen_mod_setting_value"));
		if (m_ValueText)
			m_ValueText.SetText(GetPendingValueSafe());

		// Create vanilla selector (for arrows + input)
		m_Selector = new OptionSelectorMultistate(m_OptionRoot, m_Setting.PendingIndex, owner, false, m_Setting.Options);
		if (!m_Selector)
			return;

		// THIS is the leaking widget -> hide it
		HideVanillaSelectorLabel();

		m_Selector.m_OptionChanged.Insert(OnSelectorChanged);

		// Ensure our label matches initial state
		UpdateValueText();
	}

	protected void HideVanillaSelectorLabel()
	{
		if (!m_OptionRoot)
			return;

		Widget w;

		// Most builds
		w = m_OptionRoot.FindAnyWidget("option_label");
		if (w)
			w.Show(false);

		// Some layouts have extra text nodes
		w = m_OptionRoot.FindAnyWidget("option_label_shadow");
		if (w)
			w.Show(false);

		// Some builds may parent it under the selector wrapper instead
		if (m_Selector)
		{
			Widget selRoot = m_Selector.GetParent();
			if (selRoot)
			{
				w = selRoot.FindAnyWidget("option_label");
				if (w)
					w.Show(false);

				w = selRoot.FindAnyWidget("option_label_shadow");
				if (w)
					w.Show(false);
			}
		}
	}

	protected string GetPendingValueSafe()
	{
		if (!m_Setting)
			return "";

		if (!m_Setting.Options)
			return "";

		int count = m_Setting.Options.Count();
		if (count <= 0)
			return "";

		int idx = m_Setting.PendingIndex;

		if (idx < 0)
			idx = 0;

		if (idx >= count)
			idx = count - 1;

		return m_Setting.Options[idx];
	}

	protected void UpdateValueText()
	{
		if (!m_ValueText)
			return;

		m_ValueText.SetText(GetPendingValueSafe());
	}

	void OnSelectorChanged(int idx)
	{
		if (m_Setting)
			m_Setting.SetPendingIndex(idx);

		UpdateValueText();
	}

	OptionSelectorMultistate GetSelector()
	{
		return m_Selector;
	}

	void RefreshFromSetting()
	{
		if (!m_Setting)
			return;

		if (!m_Selector)
			return;

		m_Selector.SetValue(m_Setting.PendingIndex, false);
		UpdateValueText();
	}
}