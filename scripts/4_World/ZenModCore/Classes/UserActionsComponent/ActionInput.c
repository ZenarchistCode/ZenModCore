// Allow actions to set Zen_GetDisplayPriority() to control their display order.
// LOWER values have HIGHER priority.
//
// Example:
// If skinning and searching a zombie are both valid and skinning normally appears first,
// setting the search action's Zen_GetDisplayPriority() to -1 will make search appear first
// without needing to alter ActionConstructor/Entity.c action insertion order.
modded class StandardActionInput
{
	protected bool m_ZenHasPreviousContext;
	protected Object m_ZenPreviousTargetObject;
	protected Object m_ZenPreviousTargetParent;
	protected ItemBase m_ZenPreviousMainItem;
	protected int m_ZenPreviousTargetComponent = -1;

	override void UpdatePossibleActions(PlayerBase player, ActionTarget target, ItemBase item, int action_condition_mask)
	{
		ActionBase previousSelectedAction;
		array<ActionBase> previousActions = new array<ActionBase>;

		if (m_SelectActions)
		{
			foreach (ActionBase previousAction : m_SelectActions)
			{
				previousActions.Insert(previousAction);
			}

			if (m_selectedActionIndex >= 0 && m_selectedActionIndex < m_SelectActions.Count())
				previousSelectedAction = m_SelectActions[m_selectedActionIndex];
		}

		Object currentTargetObject;
		Object currentTargetParent;
		int currentTargetComponent = -1;

		if (target)
		{
			currentTargetObject = target.GetObject();
			currentTargetParent = target.GetParent();
			currentTargetComponent = target.GetComponentIndex();
		}

		bool sameContext = m_ZenHasPreviousContext;
		
		if (currentTargetObject != m_ZenPreviousTargetObject)
			sameContext = false;

		if (currentTargetParent != m_ZenPreviousTargetParent)
			sameContext = false;

		if (currentTargetComponent != m_ZenPreviousTargetComponent)
			sameContext = false;

		if (item != m_ZenPreviousMainItem)
			sameContext = false;

		// ALWAYS let vanilla/modded action logic run first.
		super.UpdatePossibleActions(player, target, item, action_condition_mask);

		if (m_SelectActions && m_SelectActions.Count() > 1)
		{
			bool sameActionSet = Zen_HasSameActionSet(previousActions, m_SelectActions);

			Zen_SortActionsByDisplayPriority();

			// If we're still looking at exactly the same action context,
			// preserve whatever action the player manually scrolled to.
			if (sameContext && sameActionSet && previousSelectedAction)
			{
				int previousIndex = m_SelectActions.Find(previousSelectedAction);

				if (previousIndex >= 0)
					m_selectedActionIndex = previousIndex;
				else
					m_selectedActionIndex = 0;
			}
			else
			{
				// New target/action context: show highest-priority action.
				m_selectedActionIndex = 0;
			}
		}

		m_ZenPreviousTargetObject = currentTargetObject;
		m_ZenPreviousTargetParent = currentTargetParent;
		m_ZenPreviousTargetComponent = currentTargetComponent;
		m_ZenPreviousMainItem = item;
		m_ZenHasPreviousContext = true;
	}

	protected bool Zen_HasSameActionSet(array<ActionBase> previousActions, array<ActionBase> currentActions)
	{
		if (!previousActions || !currentActions)
			return false;

		if (previousActions.Count() != currentActions.Count())
			return false;

		foreach (ActionBase previousAction : previousActions)
		{
			if (currentActions.Find(previousAction) == -1)
				return false;
		}

		return true;
	}

	protected void Zen_SortActionsByDisplayPriority()
	{
		for (int i = 1; i < m_SelectActions.Count(); i++)
		{
			ActionBase action = m_SelectActions[i];

			if (!action)
				continue;

			int displayPriority = action.Zen_GetDisplayPriority();
			int j = i - 1;

			while (j >= 0)
			{
				ActionBase previousAction = m_SelectActions[j];

				if (!previousAction)
					break;

				if (previousAction.Zen_GetDisplayPriority() <= displayPriority)
					break;

				m_SelectActions[j + 1] = previousAction;
				j--;
			}

			m_SelectActions[j + 1] = action;
		}
	}
}