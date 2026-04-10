modded class ActionBase
{
	// CLIENT ACTION DISPLAY PRIORITY
	// The higher this number is, the more it will be prioritized over other options in the display list when multiple actions are possible.
	// Useful for making an action display first instead of leaving it up to action insert order which can be tricky to make a work around.
	int Zen_GetDisplayPriority()
	{
		return 0;
	}
}