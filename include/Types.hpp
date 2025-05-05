#pragma once

namespace secret_hitler
{

	enum class Policy
	{
		Liberal,
		Fascist
	};
	enum class Role
	{
		Liberal,
		Fascist,
		Hitler
	};
	enum class ActionType
	{
		Nominate,
		Vote,
		DrawDiscard,
		Enact,
		Veto,
		Execute
	};

	struct Action
	{
		ActionType type;
		int actor;
		int target;
		bool voteYes;
		int index;
	};

}