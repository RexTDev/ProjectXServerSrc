#include "../../common/CommonDefines.h"
#ifdef ENABLE_BOOK_COMBINATION
#define _item_combination_cpp_
#include "stdafx.h"
#include "config.h"
#include "constants.h"
#include "utils.h"
#include "log.h"
#include "char.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include <stdlib.h>
#include <sstream>
// #define RETURN_IF_COMB_IS_NOT_OPENED(ch) if (!(ch)->IsCombOpen()) return

extern int test_server;

void Comb_open_skillbook(LPCHARACTER ch)
{
	if (NULL == ch)
		return;

	LPCHARACTER	npc = ch->GetQuestNPC();
	if (NULL == npc)
	{
		if (test_server)
			dev_log(LOG_DEB0, "combination_npc is NULL");
		
		return;
	}

	if (ch->IsCombOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Das Kombinationsfenster ist bereits geöffnet.");
		return;
	}

	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() 
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		|| ch->IsAcceOpen() 
#endif
	|| ch->IsCombOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	long distance = DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY());
	if (distance >= COMB_MAX_DISTANCE)
	{
		sys_log(1, "Combination: TOO_FAR: %s distance %d", ch->GetName(), distance);
		return;
	}

	Comb_clean_item(ch);
	ch->SetCombNpc(npc);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "combination open_skillbook %d", npc->GetRaceNum());
}

void Comb_add_item(LPCHARACTER ch, int combination_index, int inven_index)
{
	LPITEM	item;
	LPITEM	*combination_item;

	// RETURN_IF_COMB_IS_NOT_OPENED(ch);

	if (inven_index < 0 || INVENTORY_MAX_NUM <= inven_index)
		return;

	if (combination_index < 0 || COMB_MAX_NUM <= combination_index)
		return;

	item = ch->GetInventoryItem(inven_index);
	if (item == NULL)
		return;

	combination_item = ch->GetCombItem();
	for (int i = 0; i < COMB_MAX_NUM; ++i)
	{
		if (item == combination_item[i])
		{
			combination_item[i] = NULL;
			break;
		}
	}

	combination_item[combination_index] = item;

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "combination[%d]: inventory[%d]: %s added", combination_index, inven_index, item->GetName());

	return;
}

void Comb_add_item2(LPCHARACTER ch, int combination_index, int inven_index)
{
	LPITEM	item;
	LPITEM	*combination_item;

	// RETURN_IF_COMB_IS_NOT_OPENED(ch);

	if (inven_index < 0 || INVENTORY_MAX_NUM <= inven_index)
		return;

	if (combination_index < 0 || COMB_MAX_NUM <= combination_index)
		return;

	item = ch->GetInventoryItem(inven_index);
	if (item == NULL)
		return;

	combination_item = ch->GetCombItem();
	for (int i = 0; i < COMB_MAX_NUM; ++i)
	{
		if (item == combination_item[i])
		{
			combination_item[i] = NULL;
			break;
		}
	}

	combination_item[combination_index] = item;

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "combination[%d]: inventory[%d]: %s added", combination_index, inven_index, item->GetName());

	ch->ChatPacket(CHAT_TYPE_INFO, "combination[%d]: inventory[%d]: %s added", combination_index, inven_index, item->GetName());

	return;
}

void Comb_del_item(LPCHARACTER ch, int combination_index)
{
	LPITEM	item;
	LPITEM	*combination_item;

	// RETURN_IF_COMB_IS_NOT_OPENED(ch);
	if (combination_index < 0 || COMB_MAX_NUM <= combination_index)
		return;

	combination_item = ch->GetCombItem();
	if (combination_item[combination_index] == NULL)
		return;

	item = combination_item[combination_index];

	combination_item[combination_index] = NULL;
	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "combination[%d]: combination[%d]: %s deleted", combination_index, item->GetCell(), item->GetName());

	return;
}

bool Comb_make_skillbook(LPCHARACTER ch)
{
	LPCHARACTER	npc;
	LPITEM	*items;
	int	i_number = 0;

	if (!(ch)->IsCombOpen())
	{
		(ch)->ChatPacket(CHAT_TYPE_INFO, "Das Kombinationsfenster ist nicht geöffnet.");
		return false;
	}

	npc = ch->GetQuestNPC();
	if (NULL == npc)
	{
		return false;
	}

	items = ch->GetCombItem();

	if (ch->GetGold() < 1000000)
	{
		(ch)->ChatPacket(CHAT_TYPE_INFO, "Du hast nicht genug Yang.");
		return false;
	}
	
	else if (ch->GetSkillGroup() == 0)
	{
		(ch)->ChatPacket(CHAT_TYPE_INFO, "Du hast keine Lehre ausgewählt.");
		return false;
	}

	for (int i = 0; i < COMB_MAX_NUM; ++i)
	{
		if (items[i] == NULL)
		{
			(ch)->ChatPacket(CHAT_TYPE_INFO, "Du benötigst mindestens 10 zufällige Fertigkeitsbücher.");
			return false;
		}
		else if
		(
			items[i]->GetVnum() != 50300
//			&& items[i]->GetVnum() !>= 50401 && items[i]->GetVnum() !<= 50511		// Warrior, Sura, Shaman, Ninja
#ifdef ENABLE_WOLFMAN_CHARACTER
//			&& items[i]->GetVnum() !>= 50530 && items[i]->GetVnum() !<= 50535		// Wolfman
#endif
#ifdef ENABLE_KEYTO_CHARACTER
//			&& items[i]->GetVnum() >= XXXXX && items[i]->GetVnum() <= XXXXX		// Elfe
#endif
		)
		{
			(ch)->ChatPacket(CHAT_TYPE_INFO, "Du kannst nur Fertigkeitsbücher verwenden.");
			return false;
		}
#ifdef ENABLE_SOULBIND_SYSTEM
		else if (items[i]->IsBind() || items[i]->IsUntilBind())
		{
			(ch)->ChatPacket(CHAT_TYPE_INFO, "Du kannst keinen Seelengebunden Gegenstand verwenden.");
			return false;
		}
#endif
	}

	if (ch->GetJob() == JOB_WARRIOR)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(1,6);
		}
		else if (ch->GetSkillGroup() == 2)
		{
			i_number = number(16,21);
		}
	}
	else if (ch->GetJob() == JOB_ASSASSIN)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(31,36);
		}
		else if (ch->GetSkillGroup() == 2)
		{
			i_number = number(46,51);
		}
	}
	else if (ch->GetJob() == JOB_SURA)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(61,66);
		}
		else if (ch->GetSkillGroup() == 2)
		{
			i_number = number(76,81);
		}
	}
	else if (ch->GetJob() == JOB_SHAMAN)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(91,96);
		}
		else if (ch->GetSkillGroup() == 2)
		{
			i_number = number(106,111);
		}
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	else if (ch->GetJob() == JOB_WOLFMAN)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(170,175);
		}
	}
#endif
#ifdef ENABLE_KEYTO_CHARACTER
	else if (ch->GetJob() == JOB_ELFE)
	{
		if (ch->GetSkillGroup() == 1)
		{
			i_number = number(188,193);
		}
	}
#endif

	items[0]->RemoveFromCharacter();
	items[1]->RemoveFromCharacter();
	items[2]->RemoveFromCharacter();
	items[3]->RemoveFromCharacter();
	items[4]->RemoveFromCharacter();
	items[5]->RemoveFromCharacter();
	items[6]->RemoveFromCharacter();
	items[7]->RemoveFromCharacter();
	items[8]->RemoveFromCharacter();
	items[9]->RemoveFromCharacter();
	ch->PointChange(POINT_GOLD, -1000000, false);
	ch->AutoGiveItem(50300)->SetSocket(0, i_number);

	ch->ChatPacket(CHAT_TYPE_INFO, "Die Kombination war erfolgreich.");
	ch->ChatPacket(CHAT_TYPE_COMMAND, "combination success_skillbook");

	return true;
}

void Comb_close(LPCHARACTER ch)
{
	// RETURN_IF_COMB_IS_NOT_OPENED(ch);
	Comb_clean_item(ch);
	ch->SetCombNpc(NULL);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "combination close");
	dev_log(LOG_DEB0, "<combination> close (%s)", ch->GetName());
}

void Comb_clean_item(LPCHARACTER ch)
{
	LPITEM	*combination_item;

	combination_item = ch->GetCombItem();
	for (int i = 0; i < COMB_MAX_NUM; ++i)
	{
		if (combination_item[i] == NULL)
			continue;

		combination_item[i] = NULL;
	}
}
#endif
