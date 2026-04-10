class ZenGUIBase_Game extends UIScriptedMenu
{
	static const string ZEN_LAYOUT_NOT_SET = "ERROR: NOT SET!";
	string Zen_GetLayoutFile() { return ZEN_LAYOUT_NOT_SET; }
	void Zen_SetItem(notnull EntityAI entity);
	void Zen_LoadWidgets();
	bool Zen_LockMouseAndKeyboardOnShow() { return true; }
}