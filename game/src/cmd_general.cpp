#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#ifdef NEW_PET_SYSTEM
	#include "New_PetSystem.h"
#endif
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "threeway_war.h"
#include "log.h"
#ifdef ENABLE_TELEPORT_COMMAND
	#include "commandTeleporter.h"
#endif
#ifdef ENABLE_TITLE_SYSTEM
	#include "title.h"
#endif
#ifdef SHOP_SEARCH
	#include "shop.h"
#endif
#include "../../common/VnumHelper.h"
#ifdef __AUCTION__
	#include "auction_manager.h"
#endif

ACMD(do_user_horse_ride)
{
	if (ch->IsObserverMode())
		return;

	if (ch->IsDead() || ch->IsStun())
		return;

	if (ch->IsHorseRiding() == false)
	{
		// ���� �ƴ� �ٸ�Ż���� Ÿ���ִ�.
		if (ch->GetMountVnum())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�̹� Ż���� �̿����Դϴ�."));
			return;
		}

		if (ch->GetHorse() == NULL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� ��ȯ���ּ���."));
			return;
		}

		ch->StartRiding();
	}
	else
	{
		ch->StopRiding();
	}
}

ACMD(do_user_horse_back)
{
	if (ch->GetHorse() != NULL)
	{
		ch->HorseSummon(false);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� �������½��ϴ�."));
	}
	else if (ch->IsHorseRiding() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("������ ���� ������ �մϴ�."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� ��ȯ���ּ���."));
	}
}

ACMD(do_user_horse_feed)
{
	// ���λ����� �� ���¿����� �� ���̸� �� �� ����.
	if (ch->GetMyShop())
		return;

	if (ch->GetHorse() == NULL)
	{
		if (ch->IsHorseRiding() == false)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� ��ȯ���ּ���."));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ź ���¿����� ���̸� �� �� �����ϴ�."));
		return;
	}

	DWORD dwFood = ch->GetHorseGrade() + 50054 - 1;

	if (ch->CountSpecifyItem(dwFood) > 0)
	{
		ch->RemoveSpecifyItem(dwFood, 1);
		ch->FeedHorse();
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("������ %s%s �־����ϴ�."),
				ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName,
				"");
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s �������� �ʿ��մϴ�"), ITEM_MANAGER::instance().GetTable(dwFood)->szLocaleName);
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int         	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo()
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

struct SendDisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetCharacter())
		{
			if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
				d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
		}
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
	: seconds( 0 )
	{
	}
};

EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), LC_TEXT("�˴ٿ��� %d�� ���ҽ��ϴ�."), *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), LC_TEXT("%d�� �� ������ �˴ٿ� �˴ϴ�."), iSec);

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}
	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}

#ifdef ENABLE_TITLE_SYSTEM
ACMD(do_prestige_title)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "your argument is invalid");
		return;
	}

	if (!strcmp(arg1, "prestige_0")){
		TitleManager::instance().SetTitle(ch, "disable");}

	if (!strcmp(arg1, "prestige_1")){
		TitleManager::instance().SetTitle(ch, "title1");}

	if (!strcmp(arg1, "prestige_2")){
		TitleManager::instance().SetTitle(ch, "title2");}

	if (!strcmp(arg1, "prestige_3")){
		TitleManager::instance().SetTitle(ch, "title3");}

	if (!strcmp(arg1, "prestige_4")){
		TitleManager::instance().SetTitle(ch, "title4");}

	if (!strcmp(arg1, "prestige_5")){
		TitleManager::instance().SetTitle(ch, "title5");}

	if (!strcmp(arg1, "prestige_6")){
		TitleManager::instance().SetTitle(ch, "title6");}

	if (!strcmp(arg1, "prestige_7")){
		TitleManager::instance().SetTitle(ch, "title7");}

	if (!strcmp(arg1, "prestige_8")){
		TitleManager::instance().SetTitle(ch, "title8");}

	if (!strcmp(arg1, "prestige_9")){
		TitleManager::instance().SetTitle(ch, "title9");}

	if (!strcmp(arg1, "prestige_10")){
		TitleManager::instance().SetTitle(ch, "title10");}

	if (!strcmp(arg1, "prestige_11")){
		TitleManager::instance().SetTitle(ch, "title11");}

	if (!strcmp(arg1, "prestige_12")){
		TitleManager::instance().SetTitle(ch, "title12");}

	if (!strcmp(arg1, "prestige_13")){
		TitleManager::instance().SetTitle(ch, "title13");}

	if (!strcmp(arg1, "prestige_14")){
		TitleManager::instance().SetTitle(ch, "title14");}

	if (!strcmp(arg1, "prestige_15")){
		TitleManager::instance().SetTitle(ch, "title15");}

	if (!strcmp(arg1, "prestige_16")){
		TitleManager::instance().SetTitle(ch, "title16");}

	if (!strcmp(arg1, "prestige_17")){
		TitleManager::instance().SetTitle(ch, "title17");}

	if (!strcmp(arg1, "prestige_18")){
		TitleManager::instance().SetTitle(ch, "title18");}

	if (!strcmp(arg1, "prestige_19")){
		TitleManager::instance().SetTitle(ch, "title19");}

	if (!strcmp(arg1, "buy_prestige_1")){
		TitleManager::instance().BuyPotion(ch, "buy_potion_1");}

	if (!strcmp(arg1, "buy_prestige_2")){
		TitleManager::instance().BuyPotion(ch, "buy_potion_2");}

	if (!strcmp(arg1, "buy_prestige_3")){
		TitleManager::instance().BuyPotion(ch, "buy_potion_3");}

	if (!strcmp(arg1, "vegas_transform_title")){
		TitleManager::instance().TransformTitle(ch);}
}
#endif

EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );

	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
			case SCMD_QUIT:
			case SCMD_PHASE_SELECT:
				{
					TPacketNeedLoginLogInfo acc_info;
					acc_info.dwPlayerID = ch->GetDesc()->GetAccountTable().id;

					db_clientdesc->DBPacket( HEADER_GD_VALID_LOGOUT, 0, &acc_info, sizeof(acc_info) );

					LogManager::instance().DetailLoginLog( false, ch );
				}
				break;
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				break;

			case SCMD_PHASE_SELECT:
				{
					ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

					if (d)
					{
						d->SetPhase(PHASE_SELECT);
					}
				}
				break;
		}

		return 0;
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d�� ���ҽ��ϴ�."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	/* RECALL_DELAY
	   if (ch->m_pkRecallEvent != NULL)
	   {
	   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("��� �Ǿ����ϴ�."));
	   event_cancel(&ch->m_pkRecallEvent);
	   return;
	   }
	// END_OF_RECALL_DELAY */

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("��� �Ǿ����ϴ�."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�α��� ȭ������ ���� ���ϴ�. ��ø� ��ٸ�����."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("������ ���� �մϴ�. ��ø� ��ٸ�����."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ĳ���͸� ��ȯ �մϴ�. ��ø� ��ٸ�����."));
			break;
	}

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
		false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();

				{
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
						info->left_second = 3;
				}

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

ACMD(do_mount)
{
	/*
	   char			arg1[256];
	   struct action_mount_param	param;

	// �̹� Ÿ�� ������
	if (ch->GetMountingChr())
	{
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	return;

	param.x		= atoi(arg1);
	param.y		= atoi(arg2);
	param.vid	= ch->GetMountingChr()->GetVID();
	param.is_unmount = true;

	float distance = DISTANCE_SQRT(param.x - (DWORD) ch->GetX(), param.y - (DWORD) ch->GetY());

	if (distance > 600.0f)
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�� �� ������ ���� ��������."));
	return;
	}

	action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
	return;
	}

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	return;

	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(atoi(arg1));

	if (!tch->IsNPC() || !tch->IsMountable())
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ű⿡�� Ż �� �����."));
	return;
	}

	float distance = DISTANCE_SQRT(tch->GetX() - ch->GetX(), tch->GetY() - ch->GetY());

	if (distance > 600.0f)
	{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�� �� ������ ���� Ÿ����."));
	return;
	}

	param.vid		= tch->GetVID();
	param.is_unmount	= false;

	action_enqueue(ch, ACTION_TYPE_MOUNT, &param, 0.0f, true);
	 */
}

ACMD(do_fishing)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	ch->SetRotation(atof(arg1));
	ch->fishing();
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (subcmd != SCMD_RESTART_TOWN && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		if (!test_server)
		{
			if (ch->IsHack())
			{
				//���� ���ϰ�쿡�� üũ ���� �ʴ´�.
				if (false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ����� �� �� �����ϴ�. (%d�� ����)"), iTimeToDead - (180 - g_nPortalLimitTime));
					return;
				}
			}
#define eFRS_HERESEC	170
			if (iTimeToDead > eFRS_HERESEC)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ����� �� �� �����ϴ�. (%d�� ����)"), iTimeToDead - eFRS_HERESEC);
				return;
			}
		}
	}

	//PREVENT_HACK
	//DESC : â��, ��ȯ â �� ��Ż�� ����ϴ� ���׿� �̿�ɼ� �־
	//		��Ÿ���� �߰�
	if (subcmd == SCMD_RESTART_TOWN)
	{
		if (ch->IsHack())
		{
			//����, �����ʿ����� üũ ���� �ʴ´�.
			if ((!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG) ||
			   	false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ����� �� �� �����ϴ�. (%d�� ����)"), iTimeToDead - (180 - g_nPortalLimitTime));
				return;
			}
		}

#define eFRS_TOWNSEC	173
		if (iTimeToDead > eFRS_TOWNSEC)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� �������� ����� �� �� �����ϴ�. (%d �� ����)"), iTimeToDead - eFRS_TOWNSEC);
			return;
		}
	}
	//END_PREVENT_HACK

	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();

	//FORKED_LOAD
	//DESC: ��Ÿ� ������ ��Ȱ�� �Ұ�� ���� �Ա��� �ƴ� ��Ÿ� ������ ������������ �̵��ϰ� �ȴ�.
	if (1 == quest::CQuestManager::instance().GetEventFlag("threeway_war"))
	{
		if (subcmd == SCMD_RESTART_TOWN || subcmd == SCMD_RESTART_HERE)
		{
			if (true == CThreeWayWar::instance().IsThreeWayWarMapIndex(ch->GetMapIndex()) &&
					false == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

				ch->ReviveInvisible(5);
				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

				return;
			}

			//����
			if (true == CThreeWayWar::instance().IsSungZiMapIndex(ch->GetMapIndex()))
			{
				if (CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()) <= 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�������� ��Ȱ ��ȸ�� ��� �Ҿ����ϴ�! ������ �̵��մϴ�!"));
					ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
				{
					ch->Show(ch->GetMapIndex(), GetSungziStartX(ch->GetEmpire()), GetSungziStartY(ch->GetEmpire()));
				}

				ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				ch->ReviveInvisible(5);

				return;
			}
		}
	}
	//END_FORKED_LOAD

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;
			}

			return;
		}
	}
	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;

			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(1);
			break;

		case SCMD_RESTART_HERE:
			sys_log(0, "do_restart: restart here");
			ch->RestartAtSamePos();
			//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(0);
			ch->ReviveInvisible(5);
			break;
	}
}

#define MAX_STAT g_iStatusPointSetMaxValue

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

#ifdef ENABLE_TELEPORT_COMMAND
ACMD(do_teleport)
{
	char szNumber[3];
	one_argument(argument, szNumber, sizeof(szNumber));

	if (!*szNumber || !isdigit(*szNumber) || !strcmp(szNumber, "show_list"))
		goto TELEPORT_COMMAND_INFORMATION;

	if (!ch->IsPC() || NULL == ch)
		return;

	if (ch->IsObserverMode())
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[0]);
		return;
	}

	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[1]);
		return;
	}

	if (ch->IsStun())
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[2]);
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[4]);
		return;
	}

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->IsCubeOpen() || ch->GetShop() || ch->GetMyShop()
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	|| ch->IsAcceOpen()
#endif
		)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[5]);
		return;
	}

	if (!ch->CanWarp()) // or ch->CanWarp() == false
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[13]);
		return;	
	}

	else if (isdigit(*szNumber))
	{
		int iRequestIndexMap = 0;
		str_to_number(iRequestIndexMap, szNumber);

		if (iRequestIndexMap < 1)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[6]);
			return;
		}

		if (iRequestIndexMap > NUMBER_MAX_MAPS)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[7], NUMBER_MAX_MAPS);
			return;
		}

		if (ch->GetLevel() < szTeleportConfig[iRequestIndexMap][2])
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[8], szTeleportConfig[iRequestIndexMap][2]);
			return;
		}

		if (ch->GetLevel() > szTeleportConfig[iRequestIndexMap][3])
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, szTableTranslate[9], szTeleportConfig[iRequestIndexMap][3]);
			return;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, szTableTranslate[10], iRequestIndexMap);
		ch->WarpSet(szTeleportConfig[iRequestIndexMap][0], szTeleportConfig[iRequestIndexMap][1]);
	}

TELEPORT_COMMAND_INFORMATION:
	ch->ChatPacket(CHAT_TYPE_INFO, "___________________________________________________________________________________________________");
	ch->ChatPacket(CHAT_TYPE_INFO,  szTableTranslate[11]);
	ch->ChatPacket(CHAT_TYPE_INFO,  szTableTranslate[12]);
	ch->ChatPacket(CHAT_TYPE_INFO, "");

	for (int iRequest = 0; iRequest < _countof(szTeleportConfig); iRequest++)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, szTableNameMaps[iRequest], szTeleportConfig[iRequest][2], szTeleportConfig[iRequest][3]);
	}
}
#endif

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�а� �߿��� �ɷ��� �ø� �� �����ϴ�."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�а� �߿��� �ɷ��� �ø� �� �����ϴ�."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE idx = 0;

	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	// ch->ChatPacket(CHAT_TYPE_INFO, "%s GRP(%d) idx(%u), MAX_STAT(%d), expr(%d)", __FUNCTION__, ch->GetRealPoint(idx), idx, MAX_STAT, ch->GetRealPoint(idx) >= MAX_STAT);
	if (ch->GetRealPoint(idx) >= MAX_STAT)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + 1);
	ch->SetPoint(idx, ch->GetPoint(idx) + 1);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -1);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����忡�� ����Ͻ� �� �����ϴ�."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

#ifdef ENABLE_DUELL_BLOCK_SYSTEM
	if (pkVictim->IsBlockMode(BLOCK_DUELL_MODE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, ("Dieser Spieler hat duellieren geblockt"));
		return;
	}
#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("������ ������Դϴ�."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ��忡 �������� �ʽ��ϴ�."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ��� ��ų ������ ������ ������ �����ϴ�."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

#ifndef ENABLE_7AND8TH_SKILLS
			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:
#endif

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:
#ifdef ENABLE_7AND8TH_SKILLS
			case SKILL_ANTI_PALBANG:
			case SKILL_ANTI_AMSEOP:
			case SKILL_ANTI_SWAERYUNG:
			case SKILL_ANTI_YONGBI:
			case SKILL_ANTI_GIGONGCHAM:
			case SKILL_ANTI_HWAJO:
			case SKILL_ANTI_MARYUNG:
			case SKILL_ANTI_BYEURAK:
	#ifdef ENABLE_WOLFMAN_CHARACTER_78
			case SKILL_ANTI_SALPOONG:
	#endif
	#ifdef ENABLE_KEYTO_CHARACTER_78
			// reserved
	#endif
			case SKILL_HELP_PALBANG:
			case SKILL_HELP_AMSEOP:
			case SKILL_HELP_SWAERYUNG:
			case SKILL_HELP_YONGBI:
			case SKILL_HELP_GIGONGCHAM:
			case SKILL_HELP_HWAJO:
			case SKILL_HELP_MARYUNG:
			case SKILL_HELP_BYEURAK:
	#ifdef ENABLE_WOLFMAN_CHARACTER_78
			case SKILL_HELP_SALPOONG:
	#endif
	#ifdef ENABLE_KEYTO_CHARACTER_78
			// reserved
	#endif
#endif
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
// @version	05/06/20 Bang2ni - Ŀ�ǵ� ó�� Delegate to CHARACTER class
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
// @version	05/06/20 Bang2ni - Ŀ�ǵ� ó�� Delegate to CHARACTER class
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<â��> �߸��� ��ȣ�� �Է��ϼ̽��ϴ�."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<â��> �߸��� ��ȣ�� �Է��ϼ̽��ϴ�."));
		return;
	}

	TSafeboxChangePasswordPacket p;

	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szOldPassword, arg1, sizeof(p.szOldPassword));
	strlcpy(p.szNewPassword, arg2, sizeof(p.szNewPassword));

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_PASSWORD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<â��> �߸��� ��ȣ�� �Է��ϼ̽��ϴ�."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<â��> â�� �̹� �����ֽ��ϴ�."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10) // 10�ʿ� �ѹ��� ��û ����
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<â��> â�� ������ 10�� �ȿ��� �� �� �����ϴ�."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	TSafeboxLoadPacket p;
	p.dwID = ch->GetDesc()->GetAccountTable().id;
	strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
	strlcpy(p.szPassword, arg1, sizeof(p.szPassword));

	db_clientdesc->DBPacket(HEADER_GD_MALL_LOAD, ch->GetDesc()->GetHandle(), &p, sizeof(p));
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<��Ƽ> ���� ������ ��Ƽ ���� ó���� �� �� �����ϴ�."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<��Ƽ> ���� �ȿ����� ��Ƽ���� ���� �� �����ϴ�."));
		return;
	}

	LPPARTY pParty = ch->GetParty();

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<��Ƽ> ��Ƽ���� �����̽��ϴ�."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

#ifdef GIFT_SYSTEM
#include "db.h"
#include <string>
#include <boost/algorithm/string.hpp>
ACMD(do_gift_show)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift_show");
}
bool GetGift(LPCHARACTER ch, DWORD id,bool all=false)
{
	char szSockets[1024] = { '\0' };
	char *tempSockets = szSockets;
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		tempSockets += sprintf(tempSockets, "socket%d", i);

		if (i<ITEM_SOCKET_MAX_NUM - 1)
			tempSockets += sprintf(tempSockets, ",");
	}
	char szAttrs[1024] = { '\0' };
	char *tempAttrs = szAttrs;
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
	{
		if (i < 7)
			tempAttrs += sprintf(tempAttrs, "attrtype%d,attrvalue%d", i, i);
		else
			tempAttrs += sprintf(tempAttrs, "applytype%d,applyvalue%d", i - 7, i - 7);
		if (i<ITEM_ATTRIBUTE_MAX_NUM - 1)
			tempAttrs += sprintf(tempAttrs, ",");
	}
	char query[8192];
	if (!all)
		snprintf(query, sizeof(query), "SELECT id,vnum,count,%s,%s from player_gift where id='%d' and owner_id=%d and status='WAIT'", szSockets, szAttrs, id, ch->GetPlayerID());
	else
		snprintf(query, sizeof(query), "SELECT id,vnum,count,%s,%s from player_gift where owner_id=%d and status='WAIT'", szSockets, szAttrs, ch->GetPlayerID());
	SQLMsg * pkMsg(DBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows > 0)
	{
		ch->SetQuestFlag("gift.time", get_global_time()+(1*pRes->uiNumRows)+2);
		MYSQL_ROW row;
		bool force = false;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			DWORD vnum, socket[ITEM_SOCKET_MAX_NUM], attr[ITEM_ATTRIBUTE_MAX_NUM][2];
			int col = 0;
			long long count = 0;
			str_to_number(id, row[col++]);
			str_to_number(vnum, row[col++]);
			str_to_number(count, row[col++]);
			if (vnum == 1)
			{
#ifndef FULL_YANG
				long long nTotalMoney = ch->GetGold()+count;

				if (GOLD_MAX <= nTotalMoney)
				{
					sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", ch->GetPlayerID(), ch->GetName());
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20?? ��E?�� ??��uC??�� ��oA��?�� ?��?o�Ƣ� ??��?��?��?"));
					return true;
				}
#endif
#ifdef FULL_YANG
				ch->ChangeGold(count);
#else
				ch->PointChange(POINT_GOLD, count, false);
#endif

			}
			else {
				if (force)
					continue;
				for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
					str_to_number(socket[s], row[col++]);

				for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM; a++)
				{
					str_to_number(attr[a][0], row[col++]);
					str_to_number(attr[a][1], row[col++]);
				}
				LPITEM item = ITEM_MANAGER::instance().CreateItem(vnum, count, 0, true);
				if (item)
				{
					for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
						item->SetSocket(s, socket[s], false);
					item->ClearAttribute();
					for (int a = 0; a < ITEM_ATTRIBUTE_MAX_NUM; a++)
						item->SetForceAttribute(a, attr[a][0], attr[a][1]);


					if (item->IsDragonSoul())
					{
						int iEmptyPos = ch->GetEmptyDragonSoulInventory(item);

						if (iEmptyPos != -1)
						{
							item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
						}
						else
						{
							M2_DESTROY_ITEM(item);
							if (!ch->DragonSoul_IsQualified())
							{
								ch->ChatPacket(CHAT_TYPE_INFO, "<Gift> %s #3", LC_TEXT( "UNKNOW_ERROR"));
								force = true;
								continue;
							}
							else {
								ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "INVENTORY_FULL_ERROR"));
								force = true;
								continue;
							}
						}
					}
					else
					{
						int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

						if (iEmptyPos != -1)
						{
							item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
						}
						else
						{
							M2_DESTROY_ITEM(item);
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "INVENTORY_FULL_ERROR"));
							force = true;
							continue;
						}
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "<Gift> %s #4", LC_TEXT( "UNKNOW_ERROR"));
					force = true;
					continue;
				}

			}
			DBManager::instance().DirectQuery("UPDATE player_gift SET status='OK',date_get=NOW() where id=%d;", id);
		}
		if (force)
			return true;
		if (all)
			ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("GIFT_ADD_ALL_SUCCESS"));
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "GIFT_ADD_SUCCESS"));
		ch->SetQuestFlag("gift.time", get_global_time()+2);
		return true;
	}
	return false;
}
ACMD(do_gift_get)
{
	if(ch->GetQuestFlag("gift.time") > get_global_time())
		return;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	bool full = !isdigit(*arg1);
	DWORD id;
	str_to_number(id, arg1);
	if (GetGift(ch, id,full))
	{
		ch->RefreshGift();
		ch->LoadGiftPage(ch->GetLastGiftPage());
	}

}
ACMD(do_gift_refresh)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		if (ch->GetGiftPages() > 0)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "gift_info %d", ch->GetGiftPages());
	}
	else{
		int page;
		str_to_number(page, arg1);
		ch->LoadGiftPage(page);
		
	}

	
}
#endif

#ifdef OFFLINE_SHOP
void DeleteShop(DWORD id)
{
	CharacterVectorInteractor i;
	if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(30000, i))
	{
		CharacterVectorInteractor::iterator it = i.begin();

		while (it != i.end()) {
			LPCHARACTER pc = *it++;
			if (pc)
				if (pc->GetRaceNum() == 30000 && pc->GetPrivShop() == id) {
					pc->DeleteMyShop();
					return;
				}


		}
	}
	TPacketShopClose packet;
	packet.shop_id = id;
	packet.pid = 0;
	db_clientdesc->DBPacket(HEADER_GD_SHOP_CLOSE, 0, &packet, sizeof(packet));
}

ACMD(do_close_shop)
{
#ifdef ACCOUNT_SHIELD
	if (ch->IsBlockAccount())
		return;
#endif
	if (ch->IsObserverMode() || ch->GetExchange())
		return;
	DWORD id;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
	{
		ch->CloseMyShop();
	}else{
		str_to_number(id, arg1);
		char pid[4096];
		sprintf(pid, "and player_id=%d", ch->GetPlayerID());
		if (id > 0)
		{
			SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT player_id,channel from player_shop WHERE id = %d %s", id, (ch->GetGMLevel() >= SHOP_GM_PRIVILEGES ? "" : pid)));
			SQLResult * pRes = pkMsg->Get();
			if (pRes->uiNumRows > 0)
				DeleteShop(id);
		}



	}
}
#include "banword.h"
ACMD(do_set_name_shop)
{
#ifdef ACCOUNT_SHIELD
	if (ch->IsBlockAccount())
		return;
#endif
	if (ch->IsObserverMode() || ch->GetExchange())
		return;
	DWORD id;
	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !*arg2)
		return;
	str_to_number(id, arg1);
	char pid[4096];
	sprintf(pid, "and player_id=%d", ch->GetPlayerID());
	std::string m_stShopSign(arg2);
	boost::algorithm::replace_all(m_stShopSign, "\\", " ");
	boost::algorithm::replace_all(m_stShopSign, "%", "%%");
	if (m_stShopSign.length()>SHOP_SIGN_MAX_LEN)
		m_stShopSign.resize(SHOP_SIGN_MAX_LEN);
	if (m_stShopSign.length()<1) {
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("???O?i��? ???i�Ƣ� ?��CO��? ��oA�� ??����?����I ��oA��?�� ?�� ?o ??��?��?��?."));
		return;
	}

#ifdef STRING_PROTECTION
	if (CBanwordManager::instance().CheckString(m_stShopSign.c_str(), m_stShopSign.length()) != "")
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("???O?i��? ???i�Ƣ� ?��CO��? ��oA�� ??����?����I ��oA��?�� ?�� ?o ??��?��?��?."));
		return;
	}
#else
	if (CBanwordManager::instance().CheckString(m_stShopSign.c_str(), m_stShopSign.length()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("???O?i��? ???i�Ƣ� ?��CO��? ��oA�� ??����?����I ��oA��?�� ?�� ?o ??��?��?��?."));
		return;
	}
#endif

	if (id>0)
	{
		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id,player_id,channel from player_shop WHERE id = %d %s", id, (ch->GetGMLevel() >= SHOP_GM_PRIVILEGES ? "" : pid)));
		SQLResult * pRes = pkMsg->Get();
		if (pRes->uiNumRows > 0)
		{
			char szName[256];
			DBManager::instance().EscapeString(szName, 256, m_stShopSign.c_str(), m_stShopSign.length());
			DBManager::Instance().DirectQuery("UPDATE player_shop SET name='%s' WHERE id=%d", szName, id);
			ch->LoadPrivShops();
			CharacterVectorInteractor i;
			if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(30000, i))
			{
				CharacterVectorInteractor::iterator it = i.begin();

				while (it != i.end()) {
					LPCHARACTER pc = *it++;
					if (pc)
						if (pc->GetMyShop() && pc->GetPrivShop() == id) {
							pc->SetShopSign(m_stShopSign.c_str());
							return;
						}


				}
			}
			TPacketShopName packet;
			packet.shop_id = id;
			strlcpy(packet.szSign, m_stShopSign.c_str(), sizeof(packet.szSign) - 1);
			db_clientdesc->DBPacket(HEADER_GD_SHOP_NAME, 0, &packet, sizeof(packet));


		}
	}
}

ACMD(do_shop_refresh)
{
	ch->SendShops();
}
ACMD(do_shop_yang)
{
	if (ch->IsObserverMode() || ch->GetExchange())
		return;
	DWORD id;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;
	str_to_number(id, arg1);
	if (*arg1)
	{
//		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT gold from player_shop WHERE id = %d", id));
		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT gold from player_shop WHERE id = %d and player_id=%d", id,ch->GetPlayerID()));
		SQLResult * pRes = pkMsg->Get();
		if (pRes->uiNumRows>0)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
			{
				long long gold;
				str_to_number(gold, row[0]);
				if (gold >0)
				{
#ifndef FULL_YANG
					long long nTotalMoney = ch->GetGold() + gold;

					if (GOLD_MAX <= nTotalMoney)
					{
						sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", ch->GetPlayerID(), ch->GetName());
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20?? ��E?�� ??��uC??�� ��oA��?�� ?��?o�Ƣ� ??��?��?��?"));
						return;
					}
#endif
#ifdef FULL_YANG
					ch->ChangeGold(gold);
#else
					ch->PointChange(POINT_GOLD,gold, false);
#endif

					TPrivShop s = ch->GetPrivShopTable(id);
					s.gold = s.gold - gold;
					s.rest_count = s.item_count - 1;
					ch->UpdatePrivShopTable(s.shop_id, s);
					ch->SendShops();
					DBManager::instance().DirectQuery("UPDATE player_shop SET gold=gold - %lld WHERE id = %d", gold, id);
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("SHOP_YANG_ADD"));
				}
				}
			}
		}
	}
#include <boost/algorithm/string.hpp>
ACMD(do_shop_update_item)
{

	if (ch->IsObserverMode() || ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->GetMyShop())
		return;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;
	std::vector<std::string> args;
	boost::split(args, arg1, boost::is_any_of("|"));
	if (args.size()<3) {
		return;
	}
	DWORD shop_id;
	str_to_number(shop_id, args[1].c_str());
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id from player_shop WHERE id = %d and player_id=%d", shop_id, ch->GetPlayerID()));
	SQLResult * pRes = pkMsg->Get();
	if (pRes->uiNumRows>0)
	{
		/*
		/update_shop_item price|%d|%d|%d)
		/update_shop_item remove|%d|%d
		/update_shop_item add|%d|%d|%d|%d|%s
		*/
		if (args.size() == 4)
		{
			DWORD item_id;
			str_to_number(item_id, args[2].c_str());
			long long price;
			str_to_number(price, args[3].c_str());
			if (price <= 0 || item_id <= 0)
				return;
			{
				SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id from player_shop_items WHERE id = %d and shop_id=%d", item_id, shop_id));
				SQLResult * pRes = pkMsg->Get();
				if (pRes->uiNumRows > 0)
					DBManager::instance().DirectQuery("UPDATE player_shop_items SET price=%lld WHERE id = %d and shop_id=%d", price, item_id, shop_id);



			}

		}
		else if (args.size() == 3)
		{
			DWORD item_id;
			str_to_number(item_id, args[2].c_str());
			if (item_id <= 0)
				return;
			{

				std::string shop_name(LC_TEXT("SHOP_NAME"));
				boost::replace_all(shop_name, "#PLAYER_NAME#", ch->GetName());
				boost::replace_all(shop_name, "#ID#", "");


				char szSockets[1024] = { '\0' };
				char *tempSockets = szSockets;
				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
				{
					tempSockets += sprintf(tempSockets, "socket%d", i);

					if (i<ITEM_SOCKET_MAX_NUM - 1)
						tempSockets += sprintf(tempSockets, ",");
				}
				char szAttrs[1024] = { '\0' };
				char *tempAttrs = szAttrs;
				for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
				{
					if (i < 7)
						tempAttrs += sprintf(tempAttrs, "attrtype%d,attrvalue%d", i, i);
					else
						tempAttrs += sprintf(tempAttrs, "applytype%d,applyvalue%d", i - 7, i - 7);
					if (i<ITEM_ATTRIBUTE_MAX_NUM - 1)
						tempAttrs += sprintf(tempAttrs, ",");
				}


				SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT vnum,count,%s,%s from player_shop_items where shop_id='%d' and id=%d", szSockets, szAttrs, shop_id, item_id));

				SQLResult * pRes = pkMsg->Get();
				if (pRes->uiNumRows>0)
				{
					MYSQL_ROW row;
					while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
					{

						int col = 0;
						char query[8192];
						sprintf(query, "INSERT INTO player_gift SET owner_id=%d,reason='%s',`from`=replace(\"%s\",' ','_'),status='WAIT',date_add=NOW()", ch->GetPlayerID(), LC_TEXT( "SHOP_ITEM_REASON"), shop_name.c_str());
						sprintf(query, "%s, vnum='%s'", query, row[col++]);
						sprintf(query, "%s, count='%s'", query, row[col++]);

						for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
							sprintf(query, "%s, socket%d='%s'", query, s, row[col++]);

						for (int ia = 0; ia < ITEM_ATTRIBUTE_MAX_NUM; ia++)
						{
							if (ia < 7)
							{
								sprintf(query, "%s, attrtype%d='%s'", query, ia, row[col++]);
								sprintf(query, "%s, attrvalue%d='%s'", query, ia, row[col++]);
							}
							else
							{
								sprintf(query, "%s, applytype%d='%s'", query, ia-7,row[col++]);
								sprintf(query, "%s, applyvalue%d='%s'", query, ia-7,row[col++]);
							}
						}
						SQLMsg * pkMsg(DBManager::instance().DirectQuery(query));
						SQLResult * pRes = pkMsg->Get();
						DWORD gift_id = pRes->uiInsertID;
						if (gift_id > 0)
						{
							GetGift(ch, gift_id);
							DBManager::instance().DirectQuery("delete from player_shop_items where id='%d'", item_id);
							DBManager::instance().DirectQuery("UPDATE player_shop SET item_count=item_count-1 WHERE id = %d", shop_id);
						}
					}
				}

			}
		}

		else if (args.size() == 6)
		{
			BYTE display_pos;
			str_to_number(display_pos, args[2].c_str());

			if (display_pos < 0)
				return;
			WORD pos;
			str_to_number(pos, args[3].c_str());
			if (pos < 0)
				return;

			BYTE window_type;
			str_to_number(window_type, args[4].c_str());
			if (window_type < 0)
				return;

#ifdef ACCE_LENNT_FIX
			if(ch->IsAcceOpen())
				Acce_close(ch);
#endif
			long long price;

			str_to_number(price, args[5].c_str());
			if (price <= 0)
				return;
			LPITEM item = ch->GetItem(TItemPos(window_type, pos));
			if (item)
			{
				const TItemTable * item_table = item->GetProto();

				if (item_table && (IS_SET(item_table->dwAntiFlags, ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP)))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "??��a?�� ???????? �Ʃ�?I��oA��?��?�� ?C��?C? ?o ??��?��?��?."));
					return;
				}

				if (item->IsEquipped())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "????A��?I ???????? �Ʃ�?I��oA��?��?�� ?C��?C? ?o ??��?��?��?."));
					return;
				}

				if (item->isLocked())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT( "��c?eA��?I ???????? �Ʃ�?I��oA��?��?�� ?C��?C? ?o ??��?��?��?."));
					return;
				}
				if (item->GetOwner() != ch)
				{
					return;
				}
#ifdef SOULBIND_SYSTEM
				if (item->IsSoulBind())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't sell in private shop item with soul bind."));
					return;
				}
#endif
				char query[1024];
				sprintf(query, "INSERT INTO player_shop_items SET");
				sprintf(query, "%s player_id='%d'", query, ch->GetPlayerID());
				sprintf(query, "%s, shop_id='%d'", query, shop_id);
				sprintf(query, "%s, vnum='%d'", query, item->GetVnum());
				sprintf(query, "%s, count='%d'", query, item->GetCount());
				sprintf(query, "%s, price='%lld'", query, price);

				sprintf(query, "%s, display_pos='%u'", query, display_pos);
				for (int s = 0; s < ITEM_SOCKET_MAX_NUM; s++)
				{
					sprintf(query, "%s, socket%d='%ld'", query, s, item->GetSocket(s));

				}

				for (int ia = 0; ia < ITEM_ATTRIBUTE_MAX_NUM; ia++)
				{
					const TPlayerItemAttribute& attr = item->GetAttribute(ia);
					if (ia < 7)
					{
						sprintf(query, "%s, attrtype%d='%u'", query, ia, attr.bType);
						sprintf(query, "%s, attrvalue%d='%d'", query, ia, attr.sValue);
					}
					else
					{
						sprintf(query, "%s, applytype%d='%u'", query, ia-7, attr.bType);
						sprintf(query, "%s, applyvalue%d='%d'", query, ia-7, attr.sValue);
					}
				}

				DBManager::instance().DirectQuery(query);
				ITEM_MANAGER::Instance().RemoveItem(item, "Priv shop");
				DBManager::instance().DirectQuery("UPDATE player_shop SET item_count=item_count +1 WHERE id = %d", shop_id);
			}
			else
				return;



		}
		else
			return;
		ch->LoadPrivShops();
		char buf[32];
		sprintf(buf, "refresh_shop_items %d", shop_id);
		interpret_command(ch, buf, strlen(buf));
		CharacterVectorInteractor i;
		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(30000, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();
			while (it != i.end()) {
				LPCHARACTER pc = *it++;
				if (pc)
					if (pc->GetRaceNum() == 30000 && pc->GetPrivShop() == shop_id) {
						pc->UpdateShopItems();
						return;
					}


			}
		}

		TPacketShopUpdateItem packet;
		packet.shop_id = shop_id;
		db_clientdesc->DBPacket(HEADER_GD_SHOP_UPDATE_ITEM, 0, &packet, sizeof(packet));
	}
}

ACMD(do_shop_refresh_items)
{
	if (ch->IsObserverMode() || ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen() || ch->GetMyShop())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "shop_item_clear");
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't edit shop while you have opened normal shop."));
		return;
	}

	DWORD id;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;
	str_to_number(id, arg1);
	char szSockets[1024] = { '\0' };
	char *tempSockets = szSockets;
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		tempSockets += sprintf(tempSockets, "socket%d", i);

		if (i<ITEM_SOCKET_MAX_NUM - 1)
			tempSockets += sprintf(tempSockets, ",");
	}
	char szAttrs[1024] = { '\0' };
	char *tempAttrs = szAttrs;
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
	{
		if (i < 7)
			tempAttrs += sprintf(tempAttrs, "attrtype%d,attrvalue%d", i, i);
		else
			tempAttrs += sprintf(tempAttrs, "applytype%d,applyvalue%d", i - 7, i - 7);
		if (i<ITEM_ATTRIBUTE_MAX_NUM - 1)
			tempAttrs += sprintf(tempAttrs, ",");
	}
	SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id,vnum,count,display_pos,price,%s,%s from player_shop_items where shop_id='%d'", szSockets,szAttrs,id));

	SQLResult * pRes = pkMsg->Get();
	BYTE bItemCount = pRes->uiNumRows;
	std::vector<TShopItemTable *> map_shop;
	ch->ChatPacket(CHAT_TYPE_COMMAND, "shop_item_clear");
	if (bItemCount>0)
	{
		bItemCount = 0;
		MYSQL_ROW row;
		int c = 0;
		while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
		{
			int col = 5;
			char attrs[1024] = { '\0' };
			char sockets[1024] = { '\0' };
			for (int i = 0; i<ITEM_SOCKET_MAX_NUM; i++)
				sprintf(sockets, "%s%s%s", sockets, row[col++], (i<ITEM_SOCKET_MAX_NUM-1 ? "|" : ""));
			//col--;

			for (int i = 0; i<ITEM_ATTRIBUTE_MAX_NUM; i++)
				sprintf(attrs, "%s%s,%s%s", attrs, row[col++], row[col++], (i<ITEM_ATTRIBUTE_MAX_NUM-1 ? "|" : ""));
			ch->ChatPacket(CHAT_TYPE_COMMAND, "shop_item %s#%s#%s#%s#%s#%s#%s", row[0], row[1], row[2], row[3], row[4], sockets, attrs);
		}
	}

}
ACMD(do_shop_update)
{
	if (ch->IsObserverMode() || ch->GetExchange())
		return;

	DWORD id;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;
	str_to_number(id, arg1);
	if (*arg1)
	{
		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT player_id from player_shop WHERE id = %d", id));
		SQLResult * pRes = pkMsg->Get();
		if (pRes->uiNumRows>0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Shop %s has been updated", arg1);
			CharacterVectorInteractor i;
			if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(30000, i))
			{
				CharacterVectorInteractor::iterator it = i.begin();

				while (it != i.end()) {
					LPCHARACTER pc = *it++;
					if (pc)
						if (pc->GetRaceNum() == 30000 && pc->GetPrivShop() == id) {
							pc->UpdateShopItems();
							return;
						}


				}
			}
			TPacketShopUpdateItem packet;
			packet.shop_id = id;
			db_clientdesc->DBPacket(HEADER_GD_SHOP_UPDATE_ITEM, 0, &packet, sizeof(packet));
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Shop %s does exists", arg1);
	}
}
ACMD(do_shop_delete)
{
	if (ch->IsObserverMode() || ch->GetExchange())
		return;
	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage:");
		ch->ChatPacket(CHAT_TYPE_INFO, "/delete_shop <type> <arg> ");
		ch->ChatPacket(CHAT_TYPE_INFO, "Types:");
		ch->ChatPacket(CHAT_TYPE_INFO, "		shopid - Delete shop using ID");
		ch->ChatPacket(CHAT_TYPE_INFO, "		player - Delete all player shops by player name");
		ch->ChatPacket(CHAT_TYPE_INFO, "Example:");
		ch->ChatPacket(CHAT_TYPE_INFO, "		/delete_shop player Best4ever");
		ch->ChatPacket(CHAT_TYPE_INFO, "		/delete_shop shopid 1");
		return;
	}
	if (!strcmp(arg1, "player"))
	{
		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id from player_shop WHERE player_id=(select id from player where name='%s')", arg2));
		SQLResult * pRes = pkMsg->Get();
		if (pRes->uiNumRows>0)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
			{
				DWORD id;
				str_to_number(id, row[0]);
				DeleteShop(id);
			}
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Player %s does have any shop", arg2);
	}
	if (!strcmp(arg1, "shopid"))
	{
		SQLMsg * pkMsg(DBManager::instance().DirectQuery("SELECT id from player_shop WHERE id='%s'", arg2));
		SQLResult * pRes = pkMsg->Get();
		if (pRes->uiNumRows>0)
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(pRes->pSQLResult)) != NULL)
			{
				DWORD id;
				str_to_number(id, arg2);
				DeleteShop(id);
			}
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Shop %s does exists", arg2);
	}
}
#else
ACMD(do_close_shop)
{
	if (ch->IsObserverMode())
		return;
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}
#endif

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

#ifdef ENABLE_CHANNEL_SWITCHER
		ACMD(do_channel_switch)
		{
			char arg1[256];
			one_argument(argument, arg1, sizeof(arg1));

			if (!*arg1)
				return;
				
			int new_ch;
			str_to_number(new_ch, arg1);
			if( new_ch <1 || new_ch >4)   // REPLACE 2 WITH YOUR MAX_CHANNEL 
				return;
			if (!ch->IsPC())
				return;
			ch->ChannelSwitch(new_ch);
		}
#endif

ACMD(do_war)
{
	//�� ��� ������ ������
	CGuild * g = ch->GetGuild();

	if (!g)
		return;

	//���������� üũ�ѹ�!
	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> �̹� �ٸ� ���￡ ���� �� �Դϴ�."));
		return;
	}

	//�Ķ���͸� �ι�� ������
	char arg1[256], arg2[256];
	DWORD type = GUILD_WAR_TYPE_FIELD; //fixme102 base int modded uint
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	}

	//����� ������ ���̵� ���µ�
	DWORD gm_pid = g->GetMasterPID();

	//���������� üũ(������ ����常�� ����)
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ������� ���� ������ �����ϴ�."));
		return;
	}

	//��� ��带 ������
	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> �׷� ��尡 �����ϴ�."));
		return;
	}

	//�������� ���� üũ
	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ��尡 �̹� ���� �� �Դϴ�."));
					return;
				}

				int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� �����Ͽ� ������� �� �� �����ϴ�."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ����� ���� �����Ͽ� ������� �� �� �����ϴ�."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�̹� �������� ���� ����Դϴ�."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ��尡 �̹� ���� �� �Դϴ�."));
					g->RequestRefuseWar(opp_g->GetID());
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> �̹� ������ ����� ��� �Դϴ�."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> �̹� ���� ���� ����Դϴ�."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		// ������� �� �� �ִ� ������ ���������ʴ´�.
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ������ ���ڶ� ������� �� �� �����ϴ�."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ������� �ϱ� ���ؼ� �ּ��� %d���� �־�� �մϴ�."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	// �ʵ��� üũ�� �ϰ� ������ üũ�� ������ �³��Ҷ� �Ѵ�.
	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ����� ���� ������ ���ڶ� ������� �� �� �����ϴ�."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ����� ���� ���� �����Ͽ� ������� �� �� �����ϴ�."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ����� ������� �������� �ƴմϴ�."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ���� ����� ������� �������� �ƴմϴ�."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> ������� ���� ������ �����ϴ�."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<���> �׷� ��尡 �����ϴ�."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < PK_PROTECT_LEVEL && mode != 0)
		return;

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����忡�� ����Ͻ� �� �����ϴ�."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);
	// @fixme130 AuthToAdd void -> bool
	bool bIsDenied = answer != 'y';
	bool bIsAdded = MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, bIsDenied); // DENY
	if (bIsAdded && bIsDenied)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ������ ���� ģ�� ����� �ź� ���߽��ϴ�."), ch->GetName());
	}

}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
	if (true == ch->UnEquipSpecialRideUniqueItem())
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
		}
	}
	else
	{
		ch->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("�κ��丮�� �� ���� ���� �� �����ϴ�."));
	}

}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
		{
			ch->ExitToSavedLocation();
		}
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;
#ifdef ENABLE_EXTENDED_EQUIP_VIEW
		if (tch->IsBlockMode(BLOCK_VIEW_EQUIPMENT)) 
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Dieser Spieler erlaubt kein Blick in die Ausrustung."));
			return;
		}
#endif
		/*
		   int iSPCost = ch->GetMaxSP() / 3;

		   if (ch->GetSP() < iSPCost)
		   {
		   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ŷ��� �����Ͽ� �ٸ� ����� ��� �� �� �����ϴ�."));
		   return;
		   }
		   ch->PointChange(POINT_SP, -iSPCost);
		 */
		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����忡�� ����Ͻ� �� �����ϴ�."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�̹� ��Ƽ�� ���� �����Ƿ� ���Խ�û�� �� �� �����ϴ�."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

ACMD(do_monarch_warpto)
{
	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ָ��� ��� ������ ����Դϴ�"));
		return;
	}

	//���� ��Ÿ�� �˻�
	if (!ch->IsMCOK(CHARACTER::MI_WARP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d �ʰ� ��Ÿ���� �������Դϴ�."), ch->GetMCLTime(CHARACTER::MI_WARP));
		return;
	}

	//���� �� ��ȯ ���
	const int WarpPrice = 10000;

	//���� ���� �˻�
	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� �����մϴ�. ���� : %u �ʿ�ݾ� : %u"), NationMoney, WarpPrice);
		return;
	}

	int x = 0, y = 0;
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����: warpto <character name>"));
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("Ÿ���� �������Դ� �̵��Ҽ� �����ϴ�"));
				return;
			}

			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ش� ������ %d ä�ο� �ֽ��ϴ�. (���� ä�� %d)"), pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� �̵��� �� �����ϴ�."));
				return;
			}

			PIXEL_POSITION pos;

			if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
			else
			{
				//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ���Է� �̵��մϴ�"), arg1);
				ch->WarpSet(pos.x, pos.y);

				//���� �� �谨
				CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				//��Ÿ�� �ʱ�ȭ
				ch->SetMC(CHARACTER::MI_WARP);
			}
		}
		else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
		}

		return;
	}
	else
	{
		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ÿ���� �������Դ� �̵��Ҽ� �����ϴ�"));
			return;
		}
		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� �̵��� �� �����ϴ�."));
			return;
		}
		x = tch->GetX();
		y = tch->GetY();
	}

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ���Է� �̵��մϴ�"), arg1);
	ch->WarpSet(x, y);
	ch->Stop();

	//���� �� �谨
	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

	//��Ÿ�� �ʱ�ȭ
	ch->SetMC(CHARACTER::MI_WARP);
}

ACMD(do_monarch_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����: transfer <name>"));
		return;
	}

	if (!CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ָ��� ��� ������ ����Դϴ�"));
		return;
	}

	//���� ��Ÿ�� �˻�
	if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d �ʰ� ��Ÿ���� �������Դϴ�."), ch->GetMCLTime(CHARACTER::MI_TRANSFER));
		return;
	}

	//���� ���� ���
	const int WarpPrice = 10000;

	//���� ���� �˻�
	if (!CMonarch::instance().IsMoneyOk(WarpPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� �����մϴ�. ���� : %u �ʿ�ݾ� : %u"), NationMoney, WarpPrice);
		return;
	}


	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bEmpire != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ٸ� ���� ������ ��ȯ�� �� �����ϴ�."));
				return;
			}
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ���� %d ä�ο� ���� �� �Դϴ�. (���� ä��: %d)"), arg1, pkCCI->bChannel, g_bChannel);
				return;
			}
			if (!IsMonarchWarpZone(pkCCI->lMapIndex))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� �̵��� �� �����ϴ�."));
				return;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� ��ȯ�� �� �����ϴ�."));
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s ���� ��ȯ�Ͽ����ϴ�."), arg1);

			//���� �� �谨
			CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
			//��Ÿ�� �ʱ�ȭ
			ch->SetMC(CHARACTER::MI_TRANSFER);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�Է��Ͻ� �̸��� ���� ����ڰ� �����ϴ�."));
		}

		return;
	}


	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ڽ��� ��ȯ�� �� �����ϴ�."));
		return;
	}

	if (tch->GetEmpire() != ch->GetEmpire())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ٸ� ���� ������ ��ȯ�� �� �����ϴ�."));
		return;
	}
	if (!IsMonarchWarpZone(tch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� �̵��� �� �����ϴ�."));
		return;
	}
	if (!IsMonarchWarpZone(ch->GetMapIndex()))
	{
		ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("�ش� �������� ��ȯ�� �� �����ϴ�."));
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

	//���� �� �谨
	CMonarch::instance().SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
	//��Ÿ�� �ʱ�ȭ
	ch->SetMC(CHARACTER::MI_TRANSFER);
}

ACMD(do_monarch_info)
{
	if (CMonarch::instance().IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� ����"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			if (n == ch->GetEmpire())
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s����] : %s  �����ݾ� %lld "), EMPIRE_NAME(n), p->name[n], p->money[n]);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s����] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ����"));
		TMonarchInfo * p = CMonarch::instance().GetMonarch();
		for (int n = 1; n < 4; ++n)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[%s����] : %s  "), EMPIRE_NAME(n), p->name[n]);

		}
	}

}

ACMD(do_elect)
{
	db_clientdesc->DBPacketHeader(HEADER_GD_COME_TO_VOTE, ch->GetDesc()->GetHandle(), 0);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

ACMD(do_monarch_tax)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: monarch_tax <1-50>");
		return;
	}

	// ���� �˻�
	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ָ��� ����Ҽ� �ִ� ����Դϴ�"));
		return;
	}

	// ���ݼ���
	int tax = 0;
	str_to_number(tax,  arg1);

	if (tax < 1 || tax > 50)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("1-50 ������ ��ġ�� �������ּ���"));

	quest::CQuestManager::instance().SetEventFlag("trade_tax", tax);

	// ���ֿ��� �޼��� �ϳ�
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("������ %d %�� �����Ǿ����ϴ�"));

	// ����
	char szMsg[1024];

	snprintf(szMsg, sizeof(szMsg), "������ ������ ������ %d %% �� ����Ǿ����ϴ�", tax);
	BroadcastNotice(szMsg);

	snprintf(szMsg, sizeof(szMsg), "�����δ� �ŷ� �ݾ��� %d %% �� ����� ���Ե˴ϴ�.", tax);
	BroadcastNotice(szMsg);

	// ��Ÿ�� �ʱ�ȭ
	ch->SetMC(CHARACTER::MI_TAX);
}

static const DWORD cs_dwMonarchMobVnums[] =
{
	191, //	��߽�
	192, //	����
	193, //	����
	194, //	ȣ��
	391, //	����
	392, //	����
	393, //	����
	394, //	����
	491, //	��ȯ
	492, //	����
	493, //	����
	494, //	����
	591, //	����ܴ���
	691, //	���� ����
	791, //	�б�����
	1304, // ��������
	1901, // ����ȣ
	2091, // ���հŹ�
	2191, // �Ŵ�縷�ź�
	2206, // ȭ����i
	0,
};

ACMD(do_monarch_mob)
{
	char arg1[256];
	LPCHARACTER	tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (!ch->IsMonarch())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���ָ��� ����Ҽ� �ִ� ����Դϴ�"));
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mmob <mob name>");
		return;
	}

#ifdef ENABLE_MONARCH_MOB_CMD_MAP_CHECK // @warme006
	BYTE pcEmpire = ch->GetEmpire();
	BYTE mapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(ch->GetMapIndex());
	if (mapEmpire != pcEmpire && mapEmpire != 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�ڱ� ���信���� ����� �� �ִ� ����Դϴ�"));
		return;
	}
#endif

	// ���� �� ��ȯ ���
	const int SummonPrice = 5000000;

	// ���� ��Ÿ�� �˻�
	if (!ch->IsMCOK(CHARACTER::MI_SUMMON))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d �ʰ� ��Ÿ���� �������Դϴ�."), ch->GetMCLTime(CHARACTER::MI_SUMMON));
		return;
	}

	// ���� ���� �˻�
	if (!CMonarch::instance().IsMoneyOk(SummonPrice, ch->GetEmpire()))
	{
		int NationMoney = CMonarch::instance().GetMoney(ch->GetEmpire());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� �����մϴ�. ���� : %u �ʿ�ݾ� : %u"), NationMoney, SummonPrice);
		return;
	}

	const CMob * pkMob;
	DWORD vnum = 0;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	DWORD count;

	// ��ȯ ���� �� �˻�
	for (count = 0; cs_dwMonarchMobVnums[count] != 0; ++count)
		if (cs_dwMonarchMobVnums[count] == vnum)
			break;

	if (0 == cs_dwMonarchMobVnums[count])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("��ȯ�Ҽ� ���� ���� �Դϴ�. ��ȯ������ ���ʹ� Ȩ�������� �����ϼ���"));
		return;
	}

	tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum,
			ch->GetMapIndex(),
			ch->GetX() - number(200, 750),
			ch->GetY() - number(200, 750),
			ch->GetX() + number(200, 750),
			ch->GetY() + number(200, 750),
			true,
			pkMob->m_table.bType == CHAR_TYPE_STONE,
			true);

	if (tch)
	{
		// ���� �� �谨
		CMonarch::instance().SendtoDBDecMoney(SummonPrice, ch->GetEmpire(), ch);

		// ��Ÿ�� �ʱ�ȭ
		ch->SetMC(CHARACTER::MI_SUMMON);
	}
}

static const char* FN_point_string(int apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return LC_TEXT("�ִ� ����� +%d");
		case POINT_MAX_SP:	return LC_TEXT("�ִ� ���ŷ� +%d");
		case POINT_HT:		return LC_TEXT("ü�� +%d");
		case POINT_IQ:		return LC_TEXT("���� +%d");
		case POINT_ST:		return LC_TEXT("�ٷ� +%d");
		case POINT_DX:		return LC_TEXT("��ø +%d");
		case POINT_ATT_SPEED:	return LC_TEXT("���ݼӵ� +%d");
		case POINT_MOV_SPEED:	return LC_TEXT("�̵��ӵ� %d");
		case POINT_CASTING_SPEED:	return LC_TEXT("��Ÿ�� -%d");
		case POINT_HP_REGEN:	return LC_TEXT("����� ȸ�� +%d");
		case POINT_SP_REGEN:	return LC_TEXT("���ŷ� ȸ�� +%d");
		case POINT_POISON_PCT:	return LC_TEXT("������ %d");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_PCT:	return LC_TEXT("������ %d");
#endif
		case POINT_STUN_PCT:	return LC_TEXT("���� +%d");
		case POINT_SLOW_PCT:	return LC_TEXT("���ο� +%d");
		case POINT_CRITICAL_PCT:	return LC_TEXT("%d%% Ȯ���� ġ��Ÿ ����");
		case POINT_RESIST_CRITICAL:	return LC_TEXT("����� ġ��Ÿ Ȯ�� %d%% ����");
		case POINT_PENETRATE_PCT:	return LC_TEXT("%d%% Ȯ���� ���� ����");
		case POINT_RESIST_PENETRATE: return LC_TEXT("����� ���� ���� Ȯ�� %d%% ����");
		case POINT_ATTBONUS_HUMAN:	return LC_TEXT("�ΰ��� ���� Ÿ��ġ +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return LC_TEXT("������ ���� Ÿ��ġ +%d%%");
		case POINT_ATTBONUS_ORC:	return LC_TEXT("������ Ÿ��ġ +%d%%");
		case POINT_ATTBONUS_MILGYO:	return LC_TEXT("�б��� Ÿ��ġ +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return LC_TEXT("��ü�� Ÿ��ġ +%d%%");
		case POINT_ATTBONUS_DEVIL:	return LC_TEXT("�Ǹ��� Ÿ��ġ +%d%%");
		case POINT_STEAL_HP:		return LC_TEXT("Ÿ��ġ %d%% �� ��������� ���");
		case POINT_STEAL_SP:		return LC_TEXT("Ÿ��ġ %d%% �� ���ŷ����� ���");
		case POINT_MANA_BURN_PCT:	return LC_TEXT("%d%% Ȯ���� Ÿ�ݽ� ��� ���ŷ� �Ҹ�");
		case POINT_DAMAGE_SP_RECOVER:	return LC_TEXT("%d%% Ȯ���� ���ؽ� ���ŷ� ȸ��");
		case POINT_BLOCK:			return LC_TEXT("����Ÿ�ݽ� �� Ȯ�� %d%%");
		case POINT_DODGE:			return LC_TEXT("Ȱ ���� ȸ�� Ȯ�� %d%%");
		case POINT_RESIST_SWORD:	return LC_TEXT("�Ѽհ� ��� %d%%");
		case POINT_RESIST_TWOHAND:	return LC_TEXT("��հ� ��� %d%%");
		case POINT_RESIST_DAGGER:	return LC_TEXT("�μհ� ��� %d%%");
		case POINT_RESIST_BELL:		return LC_TEXT("��� ��� %d%%");
		case POINT_RESIST_FAN:		return LC_TEXT("��ä ��� %d%%");
		case POINT_RESIST_BOW:		return LC_TEXT("Ȱ���� ���� %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_CLAW:		return LC_TEXT("�μհ� ��� %d%%");
#endif
#ifdef ENABLE_KEYTO_CHARACTER
		case POINT_RESIST_POLE:		return LC_TEXT("�μհ� ��� %d%%");
#endif
		case POINT_RESIST_FIRE:		return LC_TEXT("ȭ�� ���� %d%%");
		case POINT_RESIST_ELEC:		return LC_TEXT("���� ���� %d%%");
		case POINT_RESIST_MAGIC:	return LC_TEXT("���� ���� %d%%");
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
		case POINT_RESIST_MAGIC_REDUCTION:	return LC_TEXT("Anti Magic Resistance: %d%%");
#endif
		case POINT_RESIST_WIND:		return LC_TEXT("�ٶ� ���� %d%%");
		case POINT_RESIST_ICE:		return LC_TEXT("�ñ� ���� %d%%");
		case POINT_RESIST_EARTH:	return LC_TEXT("���� ���� %d%%");
		case POINT_RESIST_DARK:		return LC_TEXT("��� ���� %d%%");
		case POINT_REFLECT_MELEE:	return LC_TEXT("���� Ÿ��ġ �ݻ� Ȯ�� : %d%%");
		case POINT_REFLECT_CURSE:	return LC_TEXT("���� �ǵ����� Ȯ�� %d%%");
		case POINT_POISON_REDUCE:	return LC_TEXT("�� ���� %d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_BLEEDING_REDUCE:	return LC_TEXT("�� ���� %d%%");
#endif
		case POINT_KILL_SP_RECOVER:	return LC_TEXT("%d%% Ȯ���� ����ġ�� ���ŷ� ȸ��");
		case POINT_EXP_DOUBLE_BONUS:	return LC_TEXT("%d%% Ȯ���� ����ġ�� ����ġ �߰� ���");
		case POINT_GOLD_DOUBLE_BONUS:	return LC_TEXT("%d%% Ȯ���� ����ġ�� �� 2�� ���");
		case POINT_ITEM_DROP_BONUS:	return LC_TEXT("%d%% Ȯ���� ����ġ�� ������ 2�� ���");
		case POINT_POTION_BONUS:	return LC_TEXT("���� ���� %d%% ���� ����");
		case POINT_KILL_HP_RECOVERY:	return LC_TEXT("%d%% Ȯ���� ����ġ�� ����� ȸ��");
//		case POINT_IMMUNE_STUN:	return LC_TEXT("�������� ���� %d%%");
//		case POINT_IMMUNE_SLOW:	return LC_TEXT("�������� ���� %d%%");
//		case POINT_IMMUNE_FALL:	return LC_TEXT("�Ѿ����� ���� %d%%");
//		case POINT_SKILL:	return LC_TEXT("");
//		case POINT_BOW_DISTANCE:	return LC_TEXT("");
		case POINT_ATT_GRADE_BONUS:	return LC_TEXT("���ݷ� +%d");
		case POINT_DEF_GRADE_BONUS:	return LC_TEXT("���� +%d");
		case POINT_MAGIC_ATT_GRADE:	return LC_TEXT("���� ���ݷ� +%d");
		case POINT_MAGIC_DEF_GRADE:	return LC_TEXT("���� ���� +%d");
//		case POINT_CURSE_PCT:	return LC_TEXT("");
		case POINT_MAX_STAMINA:	return LC_TEXT("�ִ� ������ +%d");
		case POINT_ATTBONUS_WARRIOR:	return LC_TEXT("���翡�� ���� +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return LC_TEXT("�ڰ����� ���� +%d%%");
		case POINT_ATTBONUS_SURA:		return LC_TEXT("���󿡰� ���� +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return LC_TEXT("���翡�� ���� +%d%%");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_ATTBONUS_WOLFMAN:	return LC_TEXT("���翡�� ���� +%d%%");
#endif
#ifdef ENABLE_KEYTO_CHARACTER
		case POINT_ATTBONUS_ELFE:	return LC_TEXT("���翡�� ���� +%d%%");
#endif
		case POINT_ATTBONUS_MONSTER:	return LC_TEXT("���Ϳ��� ���� +%d%%");
		case POINT_MALL_ATTBONUS:		return LC_TEXT("���ݷ� +%d%%");
		case POINT_MALL_DEFBONUS:		return LC_TEXT("���� +%d%%");
		case POINT_MALL_EXPBONUS:		return LC_TEXT("����ġ %d%%");
		case POINT_MALL_ITEMBONUS:		return LC_TEXT("������ ����� %.1f��");
		case POINT_MALL_GOLDBONUS:		return LC_TEXT("�� ����� %.1f��");
		case POINT_MAX_HP_PCT:			return LC_TEXT("�ִ� ����� +%d%%");
		case POINT_MAX_SP_PCT:			return LC_TEXT("�ִ� ���ŷ� +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return LC_TEXT("��ų ������ %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return LC_TEXT("��Ÿ ������ %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return LC_TEXT("��ų ������ ���� %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return LC_TEXT("��Ÿ ������ ���� %d%%");
//		case POINT_PC_BANG_EXP_BONUS:	return LC_TEXT("");
//		case POINT_PC_BANG_DROP_BONUS:	return LC_TEXT("");
//		case POINT_EXTRACT_HP_PCT:	return LC_TEXT("");
		case POINT_RESIST_WARRIOR:	return LC_TEXT("������ݿ� %d%% ����");
		case POINT_RESIST_ASSASSIN:	return LC_TEXT("�ڰ����ݿ� %d%% ����");
		case POINT_RESIST_SURA:		return LC_TEXT("������ݿ� %d%% ����");
		case POINT_RESIST_SHAMAN:	return LC_TEXT("������ݿ� %d%% ����");
#ifdef ENABLE_WOLFMAN_CHARACTER
		case POINT_RESIST_WOLFMAN:	return LC_TEXT("������ݿ� %d%% ����");
#endif
#ifdef ENABLE_KEYTO_CHARACTER
		case POINT_RESIST_ELFE:	return LC_TEXT("������ݿ� %d%% ����");
#endif
		default:					return NULL;
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int	year, mon, day;
	int	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	expire = ch->GetQuestFlag("hair.limit_time");

	if (expire < get_global_time())
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, LC_TEXT(" (������ : %d�� %d�� %d��)"), year, mon, day);

	return true;
}

ACMD(do_costume)
{
	char buf[512];
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	CItem* pMount = ch->GetWear(WEAR_COSTUME_MOUNT);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	CItem* pAcce = ch->GetWear(WEAR_COSTUME_ACCE);
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);
#endif

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	if (pBody)
	{
		const char* itemName = pBody->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

	if (pHair)
	{
		const char* itemName = pHair->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pHair->GetAttribute(i);
			if (0 < attr.bType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (pMount)
	{
		const char* itemName = pMount->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  MOUNT : %s", itemName);

		if (pMount->IsEquipped() && arg1[0] == 'm')
			ch->UnequipItem(pMount);
	}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (pAcce)
	{
		const char* itemName = pAcce->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  Acce : %s", itemName);

		for (int i = 0; i < pAcce->GetAttributeCount(); ++i)
		{
			const TPlayerItemAttribute& attr = pAcce->GetAttribute(i);
			if (0 < attr.bType)
			{
				snprintf(buf, bufferSize, FN_point_string(attr.bType), attr.sValue);
				ch->ChatPacket(CHAT_TYPE_INFO, "     %s", buf);
			}
		}

		if (pAcce->IsEquipped() && arg1[0] == 'a')
			ch->UnequipItem(pAcce);
	}
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (pWeapon)
	{
		const char* itemName = pWeapon->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  WEAPON : %s", itemName);

		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
#endif
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
ACMD(do_acce)
{
	if (!ch->CanDoAcce())
		return;
	
	dev_log(LOG_DEB0, "Acce command <%s>: %s", ch->GetName(), argument);
	int acce_index = 0, inven_index = 0;
	const char *line;
	
	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));
	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: acce open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce delete <acce_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       acce make [all]");
		return;
	}
	
	const std::string& strArg1 = std::string(arg1);
	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Acce_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);
				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);
				
				Acce_request_material_info(ch, listIndex, requestCount);
			}
		}
		
		return;
	}
	else if (strArg1 == "absorption")
	{
		Acce_absorption_make(ch);
		return;
	}
	else if (strArg1 == "open_absorption")
	{
		Acce_absorption_open(ch);
		return;
	}
	
	switch (LOWER(arg1[0]))
	{
		case 'o':
			Acce_open(ch);
			break;
		case 'c':
			Acce_close(ch);
			break;
		case 'l':
			Acce_show_list(ch);
			break;
		case 'a':
			{
				if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
					return;
				
				str_to_number(acce_index, arg2);
				str_to_number(inven_index, arg3);
				Acce_add_item (ch, acce_index, inven_index);
			}
			break;
		case 'd':
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;
				
				str_to_number(acce_index, arg2);
				Acce_delete_item (ch, acce_index);
			}
			break;
		case 'm':
			if (0 != arg2[0])
			{
				while (true == Acce_make(ch))
					dev_log(LOG_DEB0, "Acce make success!");
			}
			else
				Acce_make(ch);
			break;
		default:
			return;
	}
}
#endif

ACMD(do_inventory)
{
	int	index = 0;
	int	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s",
						index, item ? item->GetName() : "<NONE>");
		++index;
	}
}

//gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}

#ifdef NEW_PET_SYSTEM
ACMD(do_CubePetAdd)
{
	int pos = 0;
	int invpos = 0;

	const char *line;
	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
		return;
	const std::string& strArg1 = std::string(arg1);
	switch (LOWER(arg1[0]))
	{
	case 'a':	// add cue_index inven_index
	{
		if (0 == arg2[0] || !isdigit(*arg2) ||
			0 == arg3[0] || !isdigit(*arg3))
			return;

		str_to_number(pos, arg2);
		str_to_number(invpos, arg3);

	}
	break;

	default:
		return;
	}

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->SetItemCube(pos, invpos);
	else
		return;

}

ACMD(do_PetSkill)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	DWORD skillslot = 0;
	str_to_number(skillslot, arg1);
	if (skillslot > 2 || skillslot < 0)
		return;

	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->DoPetSkill(skillslot);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Du musst zuerst dein Pet rufen um fortzufahren!");
}

ACMD(do_FeedCubePet)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1)
		return;

	DWORD feedtype = 0;
	str_to_number(feedtype, arg1);
	if (ch->GetNewPetSystem()->IsActivePet())
		ch->GetNewPetSystem()->ItemCubeFeed(feedtype);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Du musst zuerst dein Pet rufen, bevor du das tun kannst!");
}

ACMD(do_PetEvo)
{
	if (ch->GetExchange() || ch->GetMyShop() || ch->GetShopOwner() || ch->IsOpenSafebox() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Du kannst dein Pet jetzt nicht entwickeln!");
		return;
	}
	if (ch->GetNewPetSystem()->IsActivePet())
	{
		int it[3][7] =
		{
			//EVOLUTION_STUFE_1
			{
				55003,		// Junges Pet-Buch
				30083,		// Spinnen-Eiersack
				30058,		// Wei��es Haarband+
				30073,		// Shuriken
				30041,		// Schmuckende Haarnadel
				30074,		// Schwarze Uniform+
				30088		// Eisstuck+
			},

			//EVOLUTION_STUFE_2
			{
				55004,		// Wildes Pet-Buch
				27894,		// Blutrote Perle
				30035,		// Gesichtscreme
				30089,		// Yetifell+
				30031,		// Schmuckgegenstand
				30011,		// Knauel
				30080		// Fluchsammlung+
			},

			//EVOLUTION_STUFE_2
			{
				55005,		// Tapferes Pet-Buch
				30083,		// Unbekannte Medizin+
				27892,		// Wei��e Perle
				27893,		// Blaue Perle
				30086,		// Damonenandenken+
				30077,		// Orkzahn+
				30550		// Blauer Riemen
			}
		};

		int ic[3][7] =
		{
			//EVOLUTION_STUFE_1
			{ 10, 10, 10, 10, 10, 5, 5 },

			//EVOLUTION_STUFE_2
			{ 10, 10, 10, 10, 10, 10, 5 },

			//EVOLUTION_STUFE_3
			{ 10, 10, 5, 5, 10, 10, 5 }
		};

		int tmpevo = ch->GetNewPetSystem()->GetEvolution();

		if (ch->GetNewPetSystem()->GetLevel() >= 40 && tmpevo == 0 ||
			ch->GetNewPetSystem()->GetLevel() >= 60 && tmpevo == 1 ||
			ch->GetNewPetSystem()->GetLevel() >= 80 && tmpevo == 2)
		{
			for (int b = 0; b < 7; b++)
			{
				if (ch->CountSpecifyItem(it[tmpevo][b]) < ic[tmpevo][b])
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "[PetEvolution] Benotigte Items:");
					for (int c = 0; c < 7; c++)
					{
						DWORD vnum = it[tmpevo][c];
						ch->ChatPacket(CHAT_TYPE_INFO, "%s X%d", ITEM_MANAGER::instance().GetTable(vnum)->szLocaleName, ic[tmpevo][c]);
					}
					return;
				}
			}
			for (int c = 0; c < 7; c++)
			{
				ch->RemoveSpecifyItem(it[tmpevo][c], ic[tmpevo][c]);
			}
			ch->GetNewPetSystem()->IncreasePetEvolution();

		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Du kannst dein Pet noch nicht entwickeln!");
			return;
		}

	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Du musst zuerst dein Pet rufen!");

}
#endif

#ifdef USE_CHANGE_ATTRIBUTE_PLUS
ACMD(do_enchant_new)
{
	if (!ch)
		return;
	
	if (!ch->GetEnchantAttr())
		return;
	
	LPITEM itemTarget = ch->GetInventoryItem(ch->GetEnchantAttr());
	if (itemTarget == NULL)
		return;
	
	ch->SetEnchantAttr(5555);
	itemTarget->SetForceAttribute(0, ch->GetEnchantType1(), ch->GetEnchantValue1());
	itemTarget->SetForceAttribute(1, ch->GetEnchantType2(), ch->GetEnchantValue2());
	itemTarget->SetForceAttribute(2, ch->GetEnchantType3(), ch->GetEnchantValue3());
	itemTarget->SetForceAttribute(3, ch->GetEnchantType4(), ch->GetEnchantValue4());
	itemTarget->SetForceAttribute(4, ch->GetEnchantType5(), ch->GetEnchantValue5());
	itemTarget->Lock(false);
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You choose to take the new bonuses."));
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%u", itemTarget->GetID());
		LogManager::instance().ItemLog(ch, itemTarget, "CHANGE_ATTRIBUTE3 ANSWER_YES", buf);
	}
	
	ch->Save();
}

ACMD(do_enchant_old)
{
	if (!ch)
		return;
	
	if (!ch->GetEnchantAttr())
		return;
	
	LPITEM itemTarget = ch->GetInventoryItem(ch->GetEnchantAttr());
	if (itemTarget == NULL)
		return;
	
	ch->SetEnchantAttr(5555);
	itemTarget->Lock(false);
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You choose to take the old bonuses."));
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%u", itemTarget->GetID());
		LogManager::instance().ItemLog(ch, itemTarget, "CHANGE_ATTRIBUTE3 ANSWER_NO", buf);
	}
}
#endif
#ifdef USE_CHANGE_ATTRIBUTE_MINUS
ACMD(do_enchant2)
{
	char arg2[256];
	one_argument(argument, arg2, sizeof(arg2));
	if (!*arg2)
		return;
	
	if (!ch)
		return;
	
	if (ch->GetEnchant2Slot() == 5555 && ch->GetEnchant2Attr() == 5555)
		return;
	
	LPITEM item = ch->GetInventoryItem(ch->GetEnchant2Slot());
	if (item == NULL)
		return;
	
	LPITEM itemTarget = ch->GetInventoryItem(ch->GetEnchant2Attr());
	if (itemTarget == NULL)
		return;
	
	item->SetCount(item->GetCount() - 1);
	item->Lock(false);
	ch->SetEnchant2Slot(5555);
	ch->SetEnchant2Attr(5555);
	
	BYTE iBonusNumber = 0;
	str_to_number(iBonusNumber, arg2);
	int bType = itemTarget->GetAttributeType(iBonusNumber);
	int iValue = itemTarget->GetAttributeValue(iBonusNumber);
	itemTarget->ChangeAttributeExcept(bType);
	itemTarget->SetForceAttribute(iBonusNumber, bType, iValue);
	itemTarget->Lock(false);
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("�Ӽ��� �����Ͽ����ϴ�."));
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%u", itemTarget->GetID());
		LogManager::instance().ItemLog(ch, itemTarget, "CHANGE_ATTRIBUTE4 ANSWER_YES", buf);
	}
	
	ch->Save();
}

ACMD(do_enchant2_close)
{
	if (!ch)
		return;
	
	if (ch->GetEnchant2Attr() == 5555 && ch->GetEnchant2Slot() == 5555)
		return;
	
	LPITEM item = ch->GetInventoryItem(ch->GetEnchant2Slot());
	if (item == NULL)
		return;
	
	LPITEM itemTarget = ch->GetInventoryItem(ch->GetEnchant2Attr());
	if (itemTarget == NULL)
		return;
	
	item->Lock(false);
	itemTarget->Lock(false);
	ch->SetEnchant2Slot(5555);
	ch->SetEnchant2Attr(5555);
	
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%u", itemTarget->GetID());
		LogManager::instance().ItemLog(ch, itemTarget, "CHANGE_ATTRIBUTE4 ANSWER_NO", buf);
	}
	
	ch->Save();
}
#endif

ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	const char *line;

	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		// print usage
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "       cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	// r_info (request information)
	// /cube r_info     ==> (Client -> Server) ���� NPC�� ���� �� �ִ� ������ ��û
	//					    (Server -> Client) /cube r_list npcVNUM resultCOUNT 123,1/125,1/128,1/130,5
	//
	// /cube r_info 3   ==> (Client -> Server) ���� NPC�� ����� �ִ� ������ �� 3��° �������� ����� �� �ʿ��� ������ ��û
	// /cube r_info 3 5 ==> (Client -> Server) ���� NPC�� ����� �ִ� ������ �� 3��° �����ۺ��� ���� 5���� �������� ����� �� �ʿ��� ��� ������ ��û
	//					   (Server -> Client) /cube m_info startIndex count 125,1|126,2|127,2|123,5&555,5&555,4/120000@125,1|126,2|127,2|123,5&555,5&555,4/120000
	//
	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			Cube_open(ch);
			break;

		case 'c':	// close
			Cube_close(ch);
			break;

		case 'l':	// list
			Cube_show_list(ch);
			break;

		case 'a':	// add cue_index inven_index
			{
				if (0 == arg2[0] || !isdigit(*arg2) ||
					0 == arg3[0] || !isdigit(*arg3))
					return;

				str_to_number(cube_index, arg2);
				str_to_number(inven_index, arg3);
				Cube_add_item (ch, cube_index, inven_index);
			}
			break;

		case 'd':	// delete
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;

				str_to_number(cube_index, arg2);
				Cube_delete_item (ch, cube_index);
			}
			break;

		case 'm':	// make
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					dev_log (LOG_DEB0, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}

#ifdef ENABLE_OKAY_CARD
ACMD(do_cards)
{
	const char *line;

	char arg1[256], arg2[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			if (isdigit(*arg2))
			{
				DWORD safemode;
				str_to_number(safemode, arg2);
				ch->Cards_open(safemode);
			}
			break;
		case 'p':	// open
			ch->Cards_pullout();
			break;
		case 'e':	// open
			ch->CardsEnd();
			break;
		case 'd':	// open
			if (isdigit(*arg2))
			{
				DWORD destroy_index;
				str_to_number(destroy_index, arg2);
				ch->CardsDestroy(destroy_index);
			}
			break;
		case 'a':	// open
			if (isdigit(*arg2))
			{
				DWORD accpet_index;
				str_to_number(accpet_index, arg2);
				ch->CardsAccept(accpet_index);
			}
			break;
		case 'r':	// open
			if (isdigit(*arg2))
			{
				DWORD restore_index;
				str_to_number(restore_index, arg2);
				ch->CardsRestore(restore_index);
			}
			break;
		default:
			return;
	}
}
#endif

#ifdef ENABLE_BOOK_COMBINATION
ACMD(do_item_comb)
{
	if (!ch->CanDoComb())
		return;

	dev_log(LOG_DEB0, "item_comb command <%s>: %s", ch->GetName(), argument);

	int combination_index = 0, inven_index = 0;
	const char *line;

	char arg1[256], arg2[256], arg3[256];
	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item_comb open_skillbook");
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item_comb close");
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item_comb add_item <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item_comb del_item <combination_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item_comb make_skillbook");
		return;
	}
	
	const std::string& strArg1 = std::string(arg1);
	if (strArg1 == "open_skillbook")
	{
		Comb_open_skillbook(ch);
		return;
	}
	else if (strArg1 == "add_item")
	{
		if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
			return;
		str_to_number(combination_index, arg2);
		str_to_number(inven_index, arg3);
		Comb_add_item(ch, combination_index, inven_index);
	}
	else if (strArg1 == "add_item2")
	{
		if (0 == arg2[0] || !isdigit(*arg2) || 0 == arg3[0] || !isdigit(*arg3))
			return;
		str_to_number(combination_index, arg2);
		str_to_number(inven_index, arg3);
		Comb_add_item2(ch, combination_index, inven_index);
	}
	else if (strArg1 == "del_item")
	{
		if (0 == arg2[0] || !isdigit(*arg2))
			return;
		str_to_number(combination_index, arg2);
		Comb_del_item(ch, combination_index);
	}
	else if (strArg1 == "make_skillbook")
	{
		Comb_make_skillbook(ch);
		return;
	}
	else if (strArg1 == "close")
	{
		Comb_close(ch);
		return;
	}
}
#endif

ACMD(do_in_game_mall)
{
	if (LC_IsEurope() == true)
	{
		char country_code[3];

		switch (LC_GetLocalType())
		{
			case LC_GERMANY:	country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_FRANCE:		country_code[0] = 'f'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_ITALY:		country_code[0] = 'i'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_SPAIN:		country_code[0] = 'e'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_UK:			country_code[0] = 'e'; country_code[1] = 'n'; country_code[2] = '\0'; break;
			case LC_TURKEY:		country_code[0] = 't'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_POLAND:		country_code[0] = 'p'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_PORTUGAL:	country_code[0] = 'p'; country_code[1] = 't'; country_code[2] = '\0'; break;
			case LC_GREEK:		country_code[0] = 'g'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_RUSSIA:		country_code[0] = 'r'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_DENMARK:	country_code[0] = 'd'; country_code[1] = 'k'; country_code[2] = '\0'; break;
			case LC_BULGARIA:	country_code[0] = 'b'; country_code[1] = 'g'; country_code[2] = '\0'; break;
			case LC_CROATIA:	country_code[0] = 'h'; country_code[1] = 'r'; country_code[2] = '\0'; break;
			case LC_MEXICO:		country_code[0] = 'm'; country_code[1] = 'x'; country_code[2] = '\0'; break;
			case LC_ARABIA:		country_code[0] = 'a'; country_code[1] = 'e'; country_code[2] = '\0'; break;
			case LC_CZECH:		country_code[0] = 'c'; country_code[1] = 'z'; country_code[2] = '\0'; break;
			case LC_ROMANIA:	country_code[0] = 'r'; country_code[1] = 'o'; country_code[2] = '\0'; break;
			case LC_HUNGARY:	country_code[0] = 'h'; country_code[1] = 'u'; country_code[2] = '\0'; break;
			case LC_NETHERLANDS: country_code[0] = 'n'; country_code[1] = 'l'; country_code[2] = '\0'; break;
			case LC_USA:		country_code[0] = 'u'; country_code[1] = 's'; country_code[2] = '\0'; break;
			case LC_CANADA:	country_code[0] = 'c'; country_code[1] = 'a'; country_code[2] = '\0'; break;
			default:
				if (test_server == true)
				{
					country_code[0] = 'd'; country_code[1] = 'e'; country_code[2] = '\0';
				}
				break;
		}

		char buf[512+1];
		char sas[33];
		MD5_CTX ctx;
		const char sas_key[] = "GF9001";

		snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

		MD5Init(&ctx);
		MD5Update(&ctx, (const unsigned char *) buf, strlen(buf));
#ifdef __FreeBSD__
		MD5End(&ctx, sas);
#else
		static const char hex[] = "0123456789abcdef";
		unsigned char digest[16];
		MD5Final(digest, &ctx);
		int i;
		for (i = 0; i < 16; ++i) {
			sas[i+i] = hex[digest[i] >> 4];
			sas[i+i+1] = hex[digest[i] & 0x0f];
		}
		sas[i+i] = '\0';
#endif

		snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
				g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

		ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
	}
}

// �ֻ���
ACMD(do_dice)
{
	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = number(start, end);

#ifdef ENABLE_DICE_SYSTEM
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_DICE_INFO, LC_TEXT("%s���� �ֻ����� ���� %d�� ���Խ��ϴ�. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_DICE_INFO, LC_TEXT("����� �ֻ����� ���� %d�� ���Խ��ϴ�. (%d-%d)"), n, start, end);
#else
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMember(CHAT_TYPE_INFO, LC_TEXT("%s���� �ֻ����� ���� %d�� ���Խ��ϴ�. (%d-%d)"), ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("����� �ֻ����� ���� %d�� ���Խ��ϴ�. (%d-%d)"), n, start, end);
#endif
}

#ifdef ENABLE_NEWSTUFF
ACMD(do_click_safebox)
{
	if ((ch->GetGMLevel() <= GM_PLAYER) && (ch->GetDungeon() || ch->GetWarMap()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open the safebox in dungeon or at war."));
		return;
	}

	ch->SetSafeboxOpenPosition();
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
}

ACMD(do_force_logout)
{
	LPDESC pDesc=DESC_MANAGER::instance().FindByCharacterName(ch->GetName());
	if (!pDesc)
		return;
	pDesc->DelayedDisconnect(0);
}
#endif

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_ride)
{
    dev_log(LOG_DEB0, "[DO_RIDE] start");
    if (ch->IsDead() || ch->IsStun())
	return;

    // ������
    {
	if (ch->IsHorseRiding())
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] stop riding");
	    ch->StopRiding();
	    return;
	}

	if (ch->GetMountVnum())
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] unmount");
	    do_unmount(ch, NULL, 0, 0);
	    return;
	}
    }

    // Ÿ��
    {
	if (ch->GetHorse() != NULL)
	{
	    dev_log(LOG_DEB0, "[DO_RIDE] start riding");
	    ch->StartRiding();
	    return;
	}

	for (BYTE i=0; i<INVENTORY_MAX_NUM; ++i)
	{
	    LPITEM item = ch->GetInventoryItem(i);
	    if (NULL == item)
		continue;

	    // ����ũ Ż�� ������
		if (item->IsRideItem())
		{
			if (
				NULL==ch->GetWear(WEAR_UNIQUE1)
				|| NULL==ch->GetWear(WEAR_UNIQUE2)
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				|| NULL==ch->GetWear(WEAR_COSTUME_MOUNT)
#endif
			)
			{
				dev_log(LOG_DEB0, "[DO_RIDE] USE UNIQUE ITEM");
				//ch->EquipItem(item);
				ch->UseItem(TItemPos (INVENTORY, i));
				return;
			}
		}

	    // �Ϲ� Ż�� ������
	    // TODO : Ż�Ϳ� SubType �߰�
	    switch (item->GetVnum())
	    {
		case 71114:	// �����̿��
		case 71116:	// ��߽��̿��
		case 71118:	// �������̿��
		case 71120:	// ���ڿ��̿��
		    dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
		    ch->UseItem(TItemPos (INVENTORY, i));
		    return;
	    }

		// GF mantis #113524, 52001~52090 �� Ż��
		if( (item->GetVnum() > 52000) && (item->GetVnum() < 52091) )	{
			dev_log(LOG_DEB0, "[DO_RIDE] USE QUEST ITEM");
			ch->UseItem(TItemPos (INVENTORY, i));
		    return;
		}
	}
    }


    // Ÿ�ų� ���� �� ������
    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("���� ���� ��ȯ���ּ���."));
}

#ifdef __AUCTION__
// temp_auction
ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_MAX_NUM; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "name : %s id : %d", item->GetProto()->szName, item->GetID());
	}
}

// temp_auction

ACMD(do_enroll_auction)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	char arg4[256];
	two_arguments (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	DWORD item_id = strtoul(arg1, NULL, 10);
	BYTE empire = strtoul(arg2, NULL, 10);
	int bidPrice = strtol(arg3, NULL, 10);
	int immidiatePurchasePrice = strtol(arg4, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_auction(ch, item, empire, bidPrice, immidiatePurchasePrice);
}

ACMD(do_enroll_wish)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	DWORD item_num = strtoul(arg1, NULL, 10);
	BYTE empire = strtoul(arg2, NULL, 10);
	int wishPrice = strtol(arg3, NULL, 10);

	AuctionManager::instance().enroll_wish(ch, item_num, empire, wishPrice);
}

ACMD(do_enroll_sale)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	one_argument (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	DWORD item_id = strtoul(arg1, NULL, 10);
	DWORD wisher_id = strtoul(arg2, NULL, 10);
	int salePrice = strtol(arg3, NULL, 10);

	LPITEM item = ITEM_MANAGER::instance().Find(item_id);
	if (item == NULL)
		return;

	AuctionManager::instance().enroll_sale(ch, item, wisher_id, salePrice);
}

// temp_auction
// packet���� ����ϰ� �ϰ�, �̰� �����ؾ��Ѵ�.
ACMD(do_get_auction_list)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	two_arguments (one_argument (argument, arg1, sizeof(arg1)), arg2, sizeof(arg2), arg3, sizeof(arg3));

	AuctionManager::instance().get_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10), strtoul(arg3, NULL, 10));
}
//
//ACMD(do_get_wish_list)
//{
//	char arg1[256];
//	char arg2[256];
//	char arg3[256];
//	two_arguments (one_argument (argument, arg1, sizeof(arg1)), arg2, sizeof(arg2), arg3, sizeof(arg3));
//
//	AuctionManager::instance().get_wish_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10), strtoul(arg3, NULL, 10));
//}
ACMD (do_get_my_auction_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_auction_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_get_my_purchase_list)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_my_purchase_list (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_bid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_auction_impur)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().immediate_purchase (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_get_auctioned_item)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_buy_sold_item)
{
	char arg1[256];
	char arg2[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().get_auctioned_item (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_cancel_auction)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_auction (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_wish)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_wish (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_cancel_sale)
{
	char arg1[256];
	one_argument (argument, arg1, sizeof(arg1));

	AuctionManager::instance().cancel_sale (ch, strtoul(arg1, NULL, 10));
}

ACMD (do_rebid)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().rebid (ch, strtoul(arg1, NULL, 10), strtoul(arg2, NULL, 10));
}

ACMD (do_bid_cancel)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	AuctionManager::instance().bid_cancel (ch, strtoul(arg1, NULL, 10));
}
#endif

#ifdef SHOP_SEARCH
#include "target.h"
class CFuncShopView
{
private:
	int dwViewRange;

public:
	LPENTITY m_me;
	DWORD dwVnum,dwSkill;
	long long llPrice;
	CFuncShopView(LPENTITY ent, DWORD vnum, long long price,DWORD skill) :
		dwViewRange(VIEW_RANGE + VIEW_BONUS_RANGE),
		m_me(ent), dwVnum(vnum),dwSkill(skill), llPrice(price)
	{
	}

	void operator () (LPENTITY ent)
	{
		/*if (!ent->IsType(ENTITY_OBJECT))
		if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > dwViewRange)
		return;*/
		if (!m_me->GetDesc())
			return;
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER chMe = (LPCHARACTER)m_me;
			LPCHARACTER chEnt = (LPCHARACTER)ent;
			if (chEnt->GetRaceNum() == 30000 && chEnt->GetMyShop())
			{
				if (llPrice == 0 && dwVnum == 0)
				{
					TargetInfo * pInfo_check = CTargetManager::instance().GetTargetInfo(chMe->GetPlayerID(), TARGET_TYPE_SHOP, chEnt->GetVID());
					if (pInfo_check)
						CTargetManager::Instance().DeleteTarget(chMe->GetPlayerID(), chEnt->IsPrivShop() ? chEnt->GetPrivShop() : chEnt->GetPlayerID(), "SHOP_SEARCH_TARGET");
					return;
				}
				LPSHOP shop = chEnt->GetMyShop();
				if (!shop)
					return;
				if (!shop->SearchItem(dwVnum, llPrice, dwSkill))
					return;

				if (!CTargetManager::instance().GetTargetInfo(chMe->GetPlayerID(), TARGET_TYPE_SHOP, chEnt->GetVID()))
					CTargetManager::Instance().CreateTarget(chMe->GetPlayerID(), chEnt->IsPrivShop() ? chEnt->GetPrivShop() : chEnt->GetPlayerID(), "SHOP_SEARCH_TARGET", TARGET_TYPE_SHOP, chEnt->GetVID(), 0, chEnt->GetMapIndex(), "shop");
				chMe->SetQuestFlag("shop.search", chMe->GetQuestFlag("shop.search") + 1);
			}
		}
	}
};

void ClearShopSearch(LPCHARACTER ch)
{
	CFuncShopView f(ch, 0, 0,0);
	ch->GetSectree()->ForEachAround(f);
}

ACMD(do_search_shop)
{
	if (get_global_time() < ch->GetQuestFlag("search.searchlast"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can not use too fast");
		return;
	}

	char arg1[256], arg2[256], arg3[256], arg4[256];
	DWORD vnum = 0;
	long long price = 0;
	DWORD book = 0;

	two_arguments (two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof (arg2)), arg3, sizeof (arg3), arg4, sizeof (arg4));

	if (!*arg1 || !*arg2 || !*arg3 || !isdigit (*arg1) || !isdigit (*arg2)|| !isdigit (*arg3))
		return;
	str_to_number(vnum,arg1);
	str_to_number(price,arg2);
	str_to_number(book,arg3);

	if(vnum == 0)
		ClearShopSearch(ch);
	else
	{
		ClearShopSearch(ch);
		TItemTable * pTable = ITEM_MANAGER::instance().GetTable(vnum);
		if (!pTable)
			return;
		CFuncShopView f(ch, vnum, price,book);
		ch->SetQuestFlag("shop.search", 0);
		ch->GetSectree()->ForEachAround(f);
		int c = ch->GetQuestFlag("shop.search");
		if (c>0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Found %s in %d shops"), pTable->szLocaleName, c);
			ch->SetQuestFlag("shop.search", 0);
		}
		ch->SetQuestFlag("shop.searchlast", get_global_time() + 2);
	}
}
#endif

#ifdef ENABLE_GUILD_WAR_EXTENSION
ACMD(do_guildwarextension)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];
	int score = 0;
	BYTE minlevel = 0;
	BYTE maxplayer = 0;
	BYTE horseriding = 0;
	two_arguments (two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (*arg1)
	str_to_number(score, arg1);

	if (*arg2)
	str_to_number(minlevel, arg2);

	if (*arg3)
	str_to_number(maxplayer, arg3);

	if (*arg4)
	str_to_number(horseriding, arg4);

	if (score < 1 || minlevel < 1 || maxplayer < 1)
		return;

	if (score > 500)
		score = 500;

	if (!ch->GetGuild())
	{
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (g->UnderAnyWar())
	{
		return;
	}

	if (gm->grade == GUILD_LEADER_GRADE)
	{
		quest::CQuestManager::instance().RequestSetEventFlagBR("horseriding", g->GetName(), horseriding);
		quest::CQuestManager::instance().RequestSetEventFlagBR("score", g->GetName(), score);
		quest::CQuestManager::instance().RequestSetEventFlagBR("maxplayer", g->GetName(), maxplayer);
		quest::CQuestManager::instance().RequestSetEventFlagBR("minlevel", g->GetName(), minlevel);
	}
	else
	{
	}
}
#endif

#ifdef ENABLE_ONLINE_COMMAND
ACMD(do_online)
{
	time_t currentTime;
	struct tm *localTime;
	
	time(&currentTime);
	localTime = localtime(&currentTime);

#ifdef ENABLE_OFFLINE_SHOP
	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM player.player_shop WHERE player_id and channel = %d", g_bChannel));
	MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

	int	mShopsOffline = 0;
	str_to_number(mShopsOffline, row[0]);
#endif


	int mResultHour = localTime->tm_hour, mResultMin = localTime->tm_min, mResultSec = localTime->tm_sec;	
	int mTotalOnline;
	int * mEmpire;
	int mLocal;

	DESC_MANAGER::instance().GetUserCount(mTotalOnline, &mEmpire, mLocal);
/*
	0 = 00:00	1 = 01:00	2 = 02:00	3 = 03:00	4 = 04:00	5 = 05:00
	6 = 06:00	7 = 07:00	8 = 08:00	9 = 09:00	10 = 10:00	11 = 11:00
	12 = 12:00	13 = 13:00	14 = 14:00	15 = 15:00	16 = 16:00	17 = 17:00
	18 = 18:00	19 = 19:00	20 = 14:00	21 = 21:00	22 = 22:00	23 = 23:00
*/
#ifdef ENABLE_FAKE_ONLINE_COUNT
	static int arrayDesc[23 + 1][2] =
	{
/* 
	First row -> Current hour | Second row -> Value added for players fake 
	Example: From 15:00 until 15:59:59 will be added on statics + 75 players online fake. Because -> { 15, 75 }
*/
		{ 0, 190  },	{ 1, 175  },	{ 2, 160  },	{ 3, 140  },	{ 4, 130  },	{ 5, 100  },
		{ 6, 80   },	{ 7, 75   },	{ 8, 60   },	{ 9, 55   },	{ 10, 40  },	{ 11, 30  },
		{ 12, 50  },	{ 13, 60  },	{ 14, 70  },	{ 15, 75  },	{ 16, 80  },	{ 17, 90  },
		{ 18, 105 },	{ 19, 120 },	{ 20, 135 },	{ 21, 155 },	{ 22, 175 },	{ 23, 180 }
	};

	for (int i=0; i<=24-1; i++)
	{
		if (mResultHour == arrayDesc[i][0])
		{
			for (int j=1; j<=3; j++)
				mEmpire[i] += arrayDesc[i][1] / 3;
			mTotalOnline += arrayDesc[i][1];
			break;
		}
	}
#endif

	if (ch->IsGM())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "[i] Current time on server: %d:%d:%d", mResultHour, mResultMin, mResultSec);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "[i] Current channel: [%d]", g_bChannel);
	ch->ChatPacket(CHAT_TYPE_INFO, "[i] Current players online on this channel (all empire): [%d]", mTotalOnline);
	ch->ChatPacket(CHAT_TYPE_INFO, "[i] Current players online on empire: Shinshoo - [%d] | Chunjo - [%d] | Jinno - [%d]", mEmpire[1], mEmpire[2], mEmpire[3]);
#ifdef ENABLE_OFFLINE_SHOP
	ch->ChatPacket(CHAT_TYPE_INFO, "[i] Current shops offline on this channel: [%d]", mShopsOffline);
#endif
}
#endif

#ifdef __CHANGE_LOOK_SYSTEM__
ACMD(do_transmutate)
{
	char arg1[256];
	char arg2[256];
	two_arguments (argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	BYTE slot0 = atoi(arg1);
	BYTE slot1 = atoi(arg2);

	LPITEM item0 = ch->GetInventoryItem(slot0);
	LPITEM item1 = ch->GetInventoryItem(slot1);

#ifdef ENABLE_SOULBIND_SYSTEM
	if (item0->IsBind() || item0->IsUntilBind())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't change the skin of you Item, because is binded!"));
		return;
	}
	if (item1->IsBind() || item1->IsUntilBind())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't change the skin of you Item, because is binded!"));
		return;
	}
#endif
	ch->ChatPacket(7, "ch->SameItem(item0, item1) = %d", ch->SameItem(item0, item1));
	if (!item0 || !item1 || !ch->SameItem(item0, item1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CHANGE_LOOK_NEED_ITEM."));
		return;
	}
	else
	{
		if ((item0->IsArmor() && item1->IsArmor()) || (item0->IsWeapon() && item1->IsWeapon()))
		{
			if (ch->GetGold() >= 50000000)
			{
				item0->SetItemLook(item1->GetVnum());
				ITEM_MANAGER::instance().RemoveItem(item1, "CHANGE_LOOK_SECOND_ITEM_DESTROY");
#ifdef FULL_YANG
				ch->ChangeGold(-50000000);
#else
				ch->PointChange(POINT_GOLD, -50000000);
#endif
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CHANGE_LOOK_ITEM_REMOVE"));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CHANGE_LOOK_GOLD_REMOVE"));
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CHANGE_LOOK_NOT_ENOUGH_MONEY"));
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CHANGE_LOOK_WRONG_TYPES"));
		}
	}
}
#endif

