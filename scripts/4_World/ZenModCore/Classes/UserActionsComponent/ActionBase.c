modded class ActionBase
{
	// CLIENT ACTION DISPLAY PRIORITY
	// The LOWER this number is, the more it will be prioritized over other options
	// in the display list when multiple actions are possible.
	//
	// Default priority is 0.
	// Example:
	//	-10 = displayed before 0
	//	0   = default
	//	10  = displayed after 0
	//
	// Useful for making an action display first instead of relying on action insert order.
	int Zen_GetDisplayPriority()
	{
		return 0;
	}
}