#include "../../common/CommonDefines.h"
#ifdef NEW_PET_SYSTEM
#include "stdafx.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "New_PetSystem.h"
#include "../../common/VnumHelper.h"
#include "packet.h"
#include "item_manager.h"
#include "item.h"
#include "db.h"

//#define DISABLE_TRADE_UNSUMMON // this disable the unsummon of pet when a excange/trade/shop/myshop/safebox windows is open, MAKE SURE to have set the items with vnum 55401/55402/55403/55404 with antiflag ANTI_SAFEBOX | ANTI_PKDROP | ANTI_DROP | ANTI_SELL | ANTI_GIVE | ANTI_STACK | ANTI_MYSHOP
//USE AT OWN YOUR RISK

extern int passes_per_sec;
EVENTINFO(newpetsystem_event_info)
{
	CNewPetSystem* pPetSystem;
};

EVENTINFO(newpetsystem_event_infoe)
{
	CNewPetSystem* pPetSystem;
};

EVENTFUNC(newpetsystem_update_event)
{
	newpetsystem_event_info* info = dynamic_cast<newpetsystem_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	CNewPetSystem*	pPetSystem = info->pPetSystem;

	if (NULL == pPetSystem)
		return 0;

	pPetSystem->Update(0);
	return PASSES_PER_SEC(1) / 4;
}

EVENTFUNC(newpetsystem_expire_event)
{
	newpetsystem_event_infoe* info = dynamic_cast<newpetsystem_event_infoe*>(event->info);
	if (info == NULL)
	{
		sys_err("check_speedhack_event> <Factor> Null pointer");
		return 0;
	}

	CNewPetSystem*	pPetSystem = info->pPetSystem;

	if (NULL == pPetSystem)
		return 0;

	pPetSystem->UpdateTime();
	return PASSES_PER_SEC(1);
}

const float PET_COUNT_LIMIT = 3;

///////////////////////////////////////////////////////////////////////////////////////
//  CPetActor
///////////////////////////////////////////////////////////////////////////////////////
CNewPetActor::CNewPetActor(LPCHARACTER owner, DWORD vnum, DWORD options)
{
	m_dwVnum = vnum;
	m_dwVID = 0;
	m_dwlevel = 1;
	m_dwlevelstep = 0;
	m_dwExpFromMob = 0;
	m_dwExpFromItem = 0;
	m_dwexp = 0;
	m_dwexpitem = 0;
	m_dwOptions = options;
	m_dwLastActionTime = 0;
	m_pkChar = 0;
	m_pkOwner = owner;
	m_originalMoveSpeed = 0;
	m_dwSummonItemVID = 0;
	m_dwSummonItemID = 0;
	m_dwSummonItemVnum = 0;
	m_dwevolution = 0;
	m_dwduration = 0;
	m_dwtduration = 0;
	m_dwTimePet = 0;
	m_dwslotimm = 0;
	m_dwImmTime = 0;
	m_dwskill[0] = 0;
	m_dwskill[1] = 0;
	m_dwskill[2] = 0;

	for (int s = 0; s < 9; ++s)
	{
		m_dwpetslotitem[s] = -1;
	}

	//Riferimento allo slot -1 se non disp 0 disp non set > 0 setted
	m_dwskillslot[0] = -1;
	m_dwskillslot[0] = -1;
	m_dwskillslot[0] = -1;

	for (int x = 0; x < 3; ++x)				//Inizialisierung des Pet Bonuses
	{
		int btype[3] = { 1, 54, 2};
		m_dwbonuspet[x][0] = btype[x];
		m_dwbonuspet[x][1] = 0;
	}
}

CNewPetActor::~CNewPetActor()
{
	this->Unsummon();

	m_pkOwner = 0;
}

void CNewPetActor::SetName(const char* name)
{
//	std::string petName = m_pkOwner->GetName();
	std::string petName = "";

	if (0 != m_pkOwner && 
		0 == name && 
		0 != m_pkOwner->GetName())
	{
		petName += "'s Pet";
	}
	else
		petName += name;

	if (true == IsSummoned())
		m_pkChar->SetName(petName);

	m_name = petName;
}

void CNewPetActor::SetItemCube(int pos, int invpos)
{
	if (m_dwpetslotitem[pos] != -1)					//?erpr?t ob die Einstellung bereits festgelegt wurde
		return;

	if (pos > 180 || pos < 0)
		return;

	m_dwpetslotitem[pos] = invpos;
}

void CNewPetActor::ItemCubeFeed(int type)
{
	for (int i = 0; i < 9; ++i)
	{
		if (m_dwpetslotitem[i] != -1)
		{
			LPITEM itemxp = m_pkOwner->GetInventoryItem(m_dwpetslotitem[i]);
			if (!itemxp)
				return;

			if (itemxp->GetID() == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetID() || m_pkOwner->GetExchange() || m_pkOwner->GetMyShop() || m_pkOwner->GetShopOwner() || m_pkOwner->IsOpenSafebox() || m_pkOwner->IsCubeOpen())
				return;

#ifdef ENABLE_SOULBIND_SYSTEM
			if (itemxp->IsBind() || itemxp->IsUntilBind())
			{
				m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "You can't feed your Pet with a soulbinded item!");
				return;
			}
#endif

			if(type == 1)
			{
				if (itemxp->GetVnum() >= 55401 && itemxp->GetVnum() <= 55406 || itemxp->GetVnum() >= 55701 && itemxp->GetVnum() <= 55706 || itemxp->GetVnum() == 55001)
				{
					if(itemxp->GetVnum() == 55001)
					{
						int tmp_dur = m_dwtduration/2;
						if (m_dwduration+tmp_dur > m_dwtduration)
							m_dwduration = m_dwtduration;
						else
							m_dwduration += tmp_dur;
					}
					else
					{
						int tmp_dur = m_dwtduration * 3 / 100;
						if (m_dwduration+tmp_dur > m_dwtduration)
							m_dwduration = m_dwtduration;
						else
							m_dwduration += tmp_dur;
						m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetDuration %d %d", m_dwduration, m_dwtduration);
					}
					ITEM_MANAGER::instance().RemoveItem(itemxp);
				}
			}
			else if(type == 3)
			{
				if(itemxp->GetType() == 1 || itemxp->GetType() == 2)
				{
					SetExp(itemxp->GetShopBuyPrice() / 2, 1);
					ITEM_MANAGER::instance().RemoveItem(itemxp);
				}
			}
		}
	}

	for (int s = 0; s < 9; ++s)
	{
		m_dwpetslotitem[s] = -1;
	}
}

bool CNewPetActor::IncreasePetSkill(int skill)
{
	if (GetLevel() < 80 && m_dwevolution < 3)
		return false;
	for (int i = 0; i < 3; ++i)						//Itero gli slot per cercare la skill
	{
		if (m_dwskillslot[i] == skill)				//Se trova la skill o la aumenta oppure e' gi?max
		{
			if (m_dwskill[i] < 20)
			{
				m_dwskill[i] += 1;
				m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Die F?igkeit erreicht Stufe %d", m_dwskill[i]);
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", i, m_dwskillslot[i], m_dwskill[i]);
				return true;
			}
			else
			{
				m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Dein Pet hat diesen Skill bereits auf Stufe 20.");
				return false;
			}
		}
	}

	for (int i = 0; i < 3; ++i)
	{
		if (m_dwskillslot[i] == 0)					//Controllo se trovo uno slot vuoto abilitato 
		{
			m_dwskillslot[i] = skill;
			m_dwskill[i] = 1;
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Deine Pet hat eine neue Fahigkeit erlernt.");
			m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", i, m_dwskillslot[i], m_dwskill[i]);
			return true;
		}
	}

	/* Qualora il pet non soddisfi le condizioni precedenti
	   Allora tutti gli slot sono pieni e quind non pu?
	   imparare nuove skill 
	*/
	m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Das Pet kann keine weiteren Fahigkeiten erlernen!");
	return false;
}

bool CNewPetActor::IncreasePetEvolution()
{
	if (m_dwevolution < 3)
	{
		if (GetLevel() >= 40 && m_dwevolution < 1 || GetLevel() >= 60 && m_dwevolution < 2 || GetLevel() >= 80 && m_dwevolution < 3)
		{
			m_dwevolution += 1;
			m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 1, GetLevel(), 1);
			m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetEvolution %d", m_dwevolution);
			if (m_dwevolution == 3)
			{
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 0, m_dwskillslot[0], m_dwskill[0]);
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 1, m_dwskillslot[1], m_dwskill[1]);
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 2, m_dwskillslot[2], m_dwskill[2]);
			}
		}
		else
			return false;
	}
	else
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Dein Pet hat bereits die maximale Entwicklung erreicht!");
		return false;
	}
	return true;
}

void CNewPetActor:: IncreasePetBonus()
{
	int tmplevel = GetLevel();
	if (tmplevel % 5 == 0)
	{
		m_dwbonuspet[0][1] += number(1, 6);
	}

	if (tmplevel % 7 == 0)
	{
		m_dwbonuspet[1][1] += number(1, 6);
	}

	if (tmplevel % 4 == 0)
	{
		m_dwbonuspet[2][1] += number(1, 6);
	}

	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetBonus %d %d %d", m_dwbonuspet[0][1], m_dwbonuspet[1][1], m_dwbonuspet[2][1]);
	LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID());
	if (pSummonItem != NULL)
	{
		for (int b = 0; b < 3; b++)
		{
			pSummonItem->SetForceAttribute(b, 1, m_dwbonuspet[b][1]);
		}
	}
}

void CNewPetActor::SetNextExp(int nextExp)
{
	m_dwExpFromMob = (nextExp/10)* 9;
	m_dwExpFromItem = nextExp - m_dwExpFromMob;
}

void CNewPetActor::SetLevel(DWORD level)
{
	m_pkChar->SetLevel(static_cast<char>(level));
	m_dwlevel = level;
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetLevel %d", m_dwlevel);	
	SetNextExp(m_pkChar->PetGetNextExp());
}

void CNewPetActor::SetEvolution(int lv)
{
	if (lv == 40)
		m_dwevolution = 1;
	else if (lv == 60)
		m_dwevolution = 2;
	else if (lv == 80)
		m_dwevolution = 3;
}

void CNewPetActor::SetExp(DWORD exp, int mode)
{
	if (exp < 0)
		exp = MAX(m_dwexp - exp, 0);
	if(mode == 0)
	{
		if(GetExp() + exp >= GetNextExpFromMob() && GetExpI() >= GetNextExpFromItem())
		{
			if(GetEvolution() == 0 && GetLevel() == 40)
				return;
			else if(GetEvolution() == 1 && GetLevel() == 60)
				return;
			else if(GetEvolution() == 2 && GetLevel() == 80)
				return;
		}
	}
	else if(mode == 1)
	{
		if(GetExpI() + exp >= GetNextExpFromItem() && GetExp() >= GetNextExpFromMob())
		{
			if(GetEvolution() == 0 && GetLevel() == 40)
				return;
			else if(GetEvolution() == 1 && GetLevel() == 60)
				return;
			else if(GetEvolution() == 2 && GetLevel() == 80)
				return;
		}
	}

	if (mode == 0)
	{
		if (GetExp() + exp >= GetNextExpFromMob())
		{
			if (GetExpI() >= GetNextExpFromItem())
			{
				SetLevel(GetLevel() + 1);
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 1, GetLevel(), 1);
				IncreasePetBonus();
				m_dwlevelstep = 0;
				m_dwexp = 0;
				m_dwexpitem = 0;
				m_pkChar->SetExp(0);
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
//				SetEvolution(GetLevel());
				return;
			}
			else
			{
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 25, GetLevel(), 1);
				m_dwlevelstep = 4;
				exp = GetNextExpFromMob() - GetExp();
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
			}
		}
		m_dwexp += exp;
		m_pkChar->SetExp(m_dwexp);
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
		if (GetLevelStep() < 4)
		{
			if (GetExp() >= GetNextExpFromMob() / 4 * 3 && m_dwlevelstep != 3)
			{
				m_dwlevelstep = 3;
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 25, GetLevel(), 1);
			}
			else if (GetExp() >= GetNextExpFromMob() / 4 * 2 && m_dwlevelstep != 2)
			{
				m_dwlevelstep = 2;
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 25, GetLevel(), 1);
			}
			else if (GetExp() >= GetNextExpFromMob() / 4 && m_dwlevelstep != 1)
			{
				m_dwlevelstep = 1;
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 25, GetLevel(), 1);
			}
		}
	}
	else if(mode == 1)
	{
		if (GetExpI() + exp >= GetNextExpFromItem())
		{
			if (GetExp() >= GetNextExpFromMob())						//Set anche exp da mob e' piena livello e conservo exp restante
			{
				m_dwexpitem = GetExpI() + exp - GetNextExpFromItem();	//Nel caso in cui livelli setta exp in eccesso 0 se ==
				m_dwexp = 0;
				m_pkChar->SetExp(0);
				m_dwlevelstep = 0;
				SetLevel(GetLevel() + 1);
				m_pkChar->SendPetLevelUpEffect(m_pkChar->GetVID(), 1, GetLevel(), 1);
				IncreasePetBonus();
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
//				SetEvolution(GetLevel());
				return;
				if (GetExpI() > GetNextExpFromItem())					//Controllo che in caso exp avanzata sia superiore al livello successivo venga solo maxata ma non superi il limite
				{
					m_dwexpitem = GetNextExpFromItem();					//setto exp come massima ma non >
					m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
				}
			}
			else
			{
				exp = GetNextExpFromItem() - GetExpI();
				m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
			}
		}
		m_dwexpitem += exp;
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
	}
}

bool CNewPetActor::Mount()
{
	if (0 == m_pkOwner)
		return false;

	if (true == HasOption(EPetOption_Mountable))
		m_pkOwner->MountVnum(m_dwVnum);

	return m_pkOwner->GetMountVnum() == m_dwVnum;;
}

void CNewPetActor::UpdateTime()
{
	m_dwTimePet += 1;
	if (m_dwTimePet >= 60)
	{
		m_dwduration -= 1;
		m_dwTimePet = 0;
		LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID());
		if (pSummonItem != NULL)
		{
			pSummonItem->SetForceAttribute(3, 1, m_dwduration);
			pSummonItem->SetForceAttribute(4, 1, m_dwtduration);
		}
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetDuration %d %d", m_dwduration, m_dwtduration);		
	}
}

void CNewPetActor::Unmount()
{
	if (0 == m_pkOwner)
		return;

	if (m_pkOwner->IsHorseRiding())
		m_pkOwner->StopRiding();
}

void CNewPetActor::Unsummon()
{
	if (true == this->IsSummoned())
	{
//		sys_err("Salvo nel db level = %d, evolution=%d, exp=%d, expi=%d WHERE id = %lu ", this->GetLevel(), 0, this->GetExp(), this->GetExpI(), ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetID());
		DBManager::instance().DirectQuery("UPDATE new_petsystem SET level = %d, evolution=%d, exp=%d, expi=%d, bonus0=%d, bonus1=%d, bonus2=%d, skill0=%d, skill0lv= %d, skill1=%d, skill1lv= %d, skill2=%d, skill2lv= %d, duration=%d, tduration=%d WHERE id = %lu ", this->GetLevel(), this->m_dwevolution, this->GetExp(), this->GetExpI(), this->m_dwbonuspet[0][1], this->m_dwbonuspet[1][1], this->m_dwbonuspet[2][1], this->m_dwskillslot[0], this->m_dwskill[0], this->m_dwskillslot[1], this->m_dwskill[1], this->m_dwskillslot[2], this->m_dwskill[2], this->m_dwduration, this->m_dwtduration, ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetID());
		this->ClearBuff();

		LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID());
		if (pSummonItem != NULL)
		{
			for (int b = 0; b < 3; b++)
			{
				pSummonItem->SetForceAttribute(b, 1, m_dwbonuspet[b][1]);
			}
			pSummonItem->SetForceAttribute(3, 1, m_dwduration);
			pSummonItem->SetForceAttribute(4, 1, m_dwtduration);
			pSummonItem->SetSocket(1,m_dwlevel);
			pSummonItem->SetSocket(0, false);
			pSummonItem->Lock(false);
		}
		this->SetSummonItem(NULL);
		if (NULL != m_pkOwner)
			m_pkOwner->ComputePoints();

		if (NULL != m_pkChar)
			M2_DESTROY_CHARACTER(m_pkChar);

		m_pkChar = 0;
		m_dwVID = 0;
		m_dwlevel = 1;
		m_dwlevelstep = 0;
		m_dwExpFromMob = 0;
		m_dwExpFromItem = 0;
		m_dwexp = 0;
		m_dwexpitem = 0;
		m_dwTimePet = 0;
		m_dwImmTime = 0;
		m_dwslotimm = 0;

		for (int s = 0; s < 9; ++s)
		{
			m_dwpetslotitem[s] = -1;
		}
		ClearBuff();
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetUnsummon");
	}
}

DWORD CNewPetActor::Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar)
{
	long x = m_pkOwner->GetX();
	long y = m_pkOwner->GetY();
	long z = m_pkOwner->GetZ();

	if (true == bSpawnFar)
	{
		x += (number(0, 1) * 2 - 1) * number(2000, 2500);
		y += (number(0, 1) * 2 - 1) * number(2000, 2500);
	}
	else
	{
		x += number(-100, 100);
		y += number(-100, 100);
	}

	if (0 != m_pkChar)
	{
		m_pkChar->Show (m_pkOwner->GetMapIndex(), x, y);
		m_dwVID = m_pkChar->GetVID();

		return m_dwVID;
	}
	int evolution = 0,ivnum,ilevel;
	char szQuery2[1024];
	snprintf(szQuery2, sizeof(szQuery2), "SELECT level,evolution FROM new_petsystem WHERE id = %lu ", pSummonItem->GetID());
	std::auto_ptr<SQLMsg> pmsg3(DBManager::instance().DirectQuery(szQuery2));
	if (pmsg3->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row1 = mysql_fetch_row(pmsg3->Get()->pSQLResult);	
		ilevel = atoi(row1[0]);
		evolution = atoi(row1[1]);
	}

	if(evolution == 3 && ilevel >= 80)
	{
		ivnum = pSummonItem->GetVnum();
		if(ivnum == 55701)
			m_dwVnum = 34042;
		else if(ivnum == 55702)
			m_dwVnum = 34046;
		else if(ivnum == 55703)
			m_dwVnum = 34050;
		else if(ivnum == 55704)
			m_dwVnum = 34054;
		else if(ivnum == 55705)
			m_dwVnum = 34037;
		else if(ivnum == 55706)
			m_dwVnum = 34065;
	};

	if (evolution <= 2)
	{
		ivnum = pSummonItem->GetVnum();
		if (ivnum == 55701)
			m_dwVnum = 34041;
		else if (ivnum == 55702)
			m_dwVnum = 34045;
		else if (ivnum == 55703)
			m_dwVnum = 34049;
		else if (ivnum == 55704)
			m_dwVnum = 34053;
		else if (ivnum == 55705)
			m_dwVnum = 34036;
		else if(ivnum == 55706)
			m_dwVnum = 34064;
	}
	m_pkChar = CHARACTER_MANAGER::instance().SpawnMob
	(
		m_dwVnum, 
		m_pkOwner->GetMapIndex(), 
		x, y, z,
		false, (int)(m_pkOwner->GetRotation()+180), false
	);

	if (0 == m_pkChar)
	{
		sys_err("[CPetSystem::Summon] Fehler beim rufen des Pets. (vnum: %d)", m_dwVnum);
		return 0;
	}

	m_pkChar->SetNewPet();

//	m_pkOwner->DetailLog();
//	m_pkChar->DetailLog();

	m_pkChar->SetEmpire(m_pkOwner->GetEmpire());

	m_dwVID = m_pkChar->GetVID();

	char szQuery1[1024];
	snprintf(szQuery1, sizeof(szQuery1), "SELECT name,level,exp,expi,bonus0,bonus1,bonus2,skill0,skill0lv,skill1,skill1lv,skill2,skill2lv,duration,tduration,evolution FROM new_petsystem WHERE id = %lu ", pSummonItem->GetID());
	std::auto_ptr<SQLMsg> pmsg2(DBManager::instance().DirectQuery(szQuery1));
	if (pmsg2->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg2->Get()->pSQLResult);
		this->SetName(row[0]);
		this->SetLevel(atoi(row[1]));
		this->SetExp(atoi(row[2]), 0);
		this->SetExp(atoi(row[3]), 1);
		this->m_dwbonuspet[0][1] = atoi(row[4]);
		this->m_dwbonuspet[1][1] = atoi(row[5]);
		this->m_dwbonuspet[2][1] = atoi(row[6]);
		this->m_dwskillslot[0] = atoi(row[7]);
		this->m_dwskill[0] = atoi(row[8]);
		this->m_dwskillslot[1] = atoi(row[9]);
		this->m_dwskill[1] = atoi(row[10]);
		this->m_dwskillslot[2] = atoi(row[11]);
		this->m_dwskill[2] = atoi(row[12]);
		this->m_dwduration = atoi(row[13]);
		this->m_dwtduration = atoi(row[14]);
		this->m_dwevolution = atoi(row[15]);
	}
	else
		this->SetName(petName);

	this->SetSummonItem(pSummonItem);

//	this->SetNextExp(m_pkChar->PetGetNextExp());
	m_pkOwner->ComputePoints();
	m_pkChar->Show(m_pkOwner->GetMapIndex(), x, y, z);
	
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetIcon %d", m_dwSummonItemVnum);
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetEvolution %d", m_dwevolution);
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetName %s", m_name.c_str());
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetLevel %d", m_dwlevel);
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetDuration %d %d", m_dwduration, m_dwtduration);
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetBonus %d %d %d", m_dwbonuspet[0][1], m_dwbonuspet[1][1], m_dwbonuspet[2][1]);
	if (GetLevel() >= 80 && m_dwevolution == 3 )
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 0, m_dwskillslot[0], m_dwskill[0]);
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 1, m_dwskillslot[1], m_dwskill[1]);
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 2, m_dwskillslot[2], m_dwskill[2]);
	}
	else
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 0, -1, m_dwskill[0]);
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 1, -1, m_dwskill[1]);
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetSkill %d %d %d", 2, -1, m_dwskill[2]);
	}
	m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "PetExp %d %d %d", m_dwexp, m_dwexpitem, m_pkChar->PetGetNextExp());
	this->GiveBuff();

	for (int b = 0; b < 3; b++)
	{
		pSummonItem->SetForceAttribute(b, 1, m_dwbonuspet[b][1]);
	}
	pSummonItem->SetForceAttribute(3, 1, m_dwduration);
	pSummonItem->SetForceAttribute(4, 1, m_dwtduration);
	pSummonItem->SetSocket(1,m_dwlevel);
//	m_pkOwner->AddAffect( AFFECT_AUTO_SP_RECOVERY, POINT_NONE, 4, pSummonItem->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);
	pSummonItem->SetSocket(0,true);
	pSummonItem->Lock(true);
	return m_dwVID;
}

bool CNewPetActor::_UpdatAloneActionAI(float fMinDist, float fMaxDist)
{
	float fDist = number(fMinDist, fMaxDist);
	float r = (float)number (0, 359);
	float dest_x = GetOwner()->GetX() + fDist * cos(r);
	float dest_y = GetOwner()->GetY() + fDist * sin(r);

//	m_pkChar->SetRotation(number(0, 359));

//	GetDeltaByDegree(m_pkChar->GetRotation(), fDist, &fx, &fy);

/*	if (!(SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy) 
		&& SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx/2, m_pkChar->GetY() + (int) fy/2)))
		return true;*/

	m_pkChar->SetNowWalking(true);

//	if (m_pkChar->Goto(m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy))
//		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	if (!m_pkChar->IsStateMove() && m_pkChar->Goto(dest_x, dest_y))
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	m_dwLastActionTime = get_dword_time();

	return true;
}

// char_state.cpp StateHorse
bool CNewPetActor::_UpdateFollowAI()
{
	if (0 == m_pkChar->m_pkMobData)
	{
//		sys_err("[CPetActor::_UpdateFollowAI] m_pkChar->m_pkMobData is NULL");
		return false;
	}

	if (0 == m_originalMoveSpeed)
	{
		const CMob* mobData = CMobManager::Instance().Get(m_dwVnum);

		if (0 != mobData)
			m_originalMoveSpeed = mobData->m_table.sMovingSpeed;
	}

	float	START_FOLLOW_DISTANCE = 300.0f;
	float	START_RUN_DISTANCE = 900.0f;

	float	RESPAWN_DISTANCE = 4500.f;
	int		APPROACH = 290;

	bool bDoMoveAlone = true;
	bool bRun = false;

	DWORD currentTime = get_dword_time();

	long ownerX = m_pkOwner->GetX();		long ownerY = m_pkOwner->GetY();
	long charX = m_pkChar->GetX();			long charY = m_pkChar->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pkOwner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_pkChar->Show(m_pkOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
		{
			return true;
		}
	}

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if( fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}

		m_pkChar->SetNowWalking(!bRun);
		
		Follow(APPROACH);

		m_pkChar->SetLastAttacked(currentTime);
		m_dwLastActionTime = currentTime;
	}

/*	else
	{
		if (fabs(m_pkChar->GetRotation() - GetDegreeFromPositionXY(charX, charY, ownerX, ownerX)) > 10.f || fabs(m_pkChar->GetRotation() - GetDegreeFromPositionXY(charX, charY, ownerX, ownerX)) < 350.f)
		{
			m_pkChar->Follow(m_pkOwner, APPROACH);
			m_pkChar->SetLastAttacked(currentTime);
			m_dwLastActionTime = currentTime;
		}
	}*/

	else 
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
//	else if (currentTime - m_dwLastActionTime > number(5000, 12000))
//	{
//		this->_UpdatAloneActionAI(START_FOLLOW_DISTANCE / 2, START_FOLLOW_DISTANCE);
//	}

	return true;
}

bool CNewPetActor::Update(DWORD deltaTime)
{
	bool bResult = true;

	if (IsSummoned())
	{
		if (m_pkOwner->IsImmortal() && Pet_Skill_Table[16][2 + m_dwskill[m_dwslotimm]] <= (get_global_time() - m_dwImmTime)*10)
		{
//			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "%d - %d  diff %d  Skilltable %d", get_global_time(), m_dwImmTime, (get_global_time() - m_dwImmTime) * 10, Pet_Skill_Table[16][2 + m_dwskill[m_dwslotimm]]);
			m_dwImmTime = 0;
			m_pkOwner->SetImmortal(0);
		}
	}

#ifdef DISABLE_TRADE_UNSUMMON
	if (m_pkOwner->IsDead() || (IsSummoned() && m_pkChar->IsDead()) || (IsSummoned() && m_dwduration <= 0)
		|| NULL == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())
		|| ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner()
		)
#else
	if (m_pkOwner->IsDead() || (IsSummoned() && m_pkChar->IsDead()) || (IsSummoned() && (m_pkOwner->GetExchange() || m_pkOwner->GetMyShop() || m_pkOwner->GetShopOwner() || m_pkOwner->IsOpenSafebox() || m_pkOwner->IsCubeOpen() || m_dwduration <= 0))
		|| NULL == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())
		|| ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner()
		)
#endif
	{
		this->Unsummon();
		return true;
	}

	if (this->IsSummoned() && HasOption(EPetOption_Followable))
		bResult = bResult && this->_UpdateFollowAI();

	return bResult;
}

bool CNewPetActor::Follow(float fMinDistance)
{
	if( !m_pkOwner || !m_pkChar) 
		return false;

	float fOwnerX = m_pkOwner->GetX();
	float fOwnerY = m_pkOwner->GetY();

	float fPetX = m_pkChar->GetX();
	float fPetY = m_pkChar->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fPetX, fOwnerY - fPetY);
	if (fDist <= fMinDistance)
		return false;

	m_pkChar->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pkChar->GetRotation(), fDistToGo, &fx, &fy);
	
	if (!m_pkChar->Goto((int)(fPetX+fx+0.5f), (int)(fPetY+fy+0.5f)))
		return false;

	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

void CNewPetActor::SetSummonItem (LPITEM pItem)
{
	if (NULL == pItem)
	{
		m_dwSummonItemVID = 0;
		m_dwSummonItemID = 0;
		m_dwSummonItemVnum = 0;
		return;
	}

	m_dwSummonItemVID = pItem->GetVID();
	m_dwSummonItemID = pItem->GetID();
	m_dwSummonItemVnum = pItem->GetVnum();
}

void CNewPetActor::GiveBuff()
{
	// Laden des Standard-Bonis = TP, DEF, MP	[Affect = 559]
	int cbonus[3] =
	{
		m_pkOwner->GetMaxHP(),
		m_pkOwner->GetPoint(POINT_DEF_GRADE),
#ifndef ENABLE_BAUM_CHANGES
		m_pkOwner->GetMaxSP()	//  <- Original
#else
		m_pkOwner->GetPoint(POINT_ATTBONUS_MONSTER),
#endif
	};

	for (int i = 0; i < 3; ++i)
	{
		m_pkOwner->AddAffect(AFFECT_NEW_PET, aApplyInfo[m_dwbonuspet[i][0]].bPointType, (int)(cbonus[i]*m_dwbonuspet[i][1]/1000), 0,  60 * 60 * 24 * 365, 0, false);
	}

	// Laden des Skills [No 10-17-18 No 0 no -1]
	// Lvl 81 Evo 3 = Passive Skills
	if (GetLevel() >= 80 && m_dwevolution == 3)
	{
		for (int s = 0; s < 3; s++)
		{
			switch (m_dwskillslot[s])
			{

			/*
				Pet_Skill_Table[m_dwskillslot[s] - 1][0];					// Mi ritorna il type della skill
				Pet_Skill_Table[m_dwskillslot[s] - 1][1];					// Mi ritorna attiva/passiva della skill 
				Pet_Skill_Table[m_dwskillslot[s] - 1][2];					// Mi ritorna il cd della skill
				Pet_Skill_Table[m_dwskillslot[s] - 1][2 + m_dwskill[s]];	// Mi ritorna l'apply della skill
			*/
			case 1:		// Widerstand (Krieger)		78 Punti
			case 2:		// Widerstand (Sura)		80
			case 3:		// Widerstand (Ninja)		79
			case 4:		// Widerstand (Schamane)	81
			case 5:		// Widerstand (Lykaner)		93
			case 6:		// Berserker				53 Punti
			case 7:		// Magiebruch				97
			case 8:		// Beschleunigung			%
			case 9:		// Drill					16 Punti
			case 11:	// Vampir					Punti
			case 12:	// Geister					Punti
			case 13:	// Hindernis				27 Punti
			case 14:	// Spiegeln					28 Punti
			case 15:	// Yang Drop				44 Punti
			case 16:	// Reichweite				172 Punti
				m_pkOwner->AddAffect(AFFECT_NEW_PET, aApplyInfo[Pet_Skill_Table[m_dwskillslot[s] - 1][0]].bPointType, Pet_Skill_Table[m_dwskillslot[s] - 1][2 + m_dwskill[s]], 0, 60 * 60 * 24 * 365, 0, false);
				break;
			default:
				return;
			}
		}
	}
}

void CNewPetActor::ClearBuff()
{
	m_pkOwner->RemoveAffect(AFFECT_NEW_PET);
	return ;
}

void CNewPetActor::DoPetSkill(int skillslot)
{
	if (GetLevel() < 80 || m_dwevolution < 3)
		return;
	switch (m_dwskillslot[skillslot]) {
	case 10:
	{
		if (get_global_time() - m_pkOwner->GetNewPetSkillCD(0) <= 480)
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Du kannst diese Fertigkeit erst in %d Sekunde(n) wieder benutzen.", 480 - (get_global_time() - m_pkOwner->GetNewPetSkillCD(0)));
			return;
		}
		if (m_pkOwner->GetHPPct() > 20)
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Du kannst diese Fahigkeit nur mit PV nutzen <= 20%!");
			return;
		}
		m_pkOwner->SetNewPetSkillCD(0, get_global_time());
		int riphp = MIN(m_pkOwner->GetHP() + (int)Pet_Skill_Table[9][2 + m_dwskill[skillslot]], m_pkOwner->GetMaxHP());
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Hai ripristinato gli HP!");
		m_pkOwner->PointChange(POINT_HP, riphp);
		m_pkOwner->EffectPacket(SE_HPUP_RED);
	}
	break;

	case 17:
	{
		if (get_global_time() - m_pkOwner->GetNewPetSkillCD(1) <= 600)
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Du kannst diese Fertigkeit erst in!%ds wieder benutzen.", 600 - (get_global_time() - m_pkOwner->GetNewPetSkillCD(1)));
			return;
		}
		m_pkOwner->SetNewPetSkillCD(1, get_global_time());
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Du wirst unsterblich!");
		m_pkOwner->SetImmortal(1);
		m_dwslotimm = skillslot;
		m_dwImmTime = get_global_time();
	}
	break;
	case 18:
	{
		if (get_global_time() - m_pkOwner->GetNewPetSkillCD(2) <= 480)
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Du kannst diese Fertigkeit erst in!%ds wieder benutzen.", 480 -(get_global_time() - m_pkOwner->GetNewPetSkillCD(2)));
			return;
		}
		m_pkOwner->SetNewPetSkillCD(2, get_global_time());
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, "Hai rimosso gli effetti negativi!");
		m_pkOwner->RemoveBadAffect();
	}
	break;

	default:
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////////////
//  CPetSystem
///////////////////////////////////////////////////////////////////////////////////////
CNewPetSystem::CNewPetSystem(LPCHARACTER owner)
{
//	assert(0 != owner && "[CPetSystem::CPetSystem] Invalid owner");

	m_pkOwner = owner;
	m_dwUpdatePeriod = 400;

	m_dwLastUpdateTime = 0;
}

CNewPetSystem::~CNewPetSystem()
{
	Destroy();
}

bool  CNewPetSystem::IncreasePetSkill(int skill)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->IncreasePetSkill(skill);
			}
		}
	}
	return false;
}

bool  CNewPetSystem::IncreasePetEvolution()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->IncreasePetEvolution();
			}
		}
	}
	return false;
}

void CNewPetSystem::Destroy()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			delete petActor;
		}
	}
	event_cancel(&m_pkNewPetSystemUpdateEvent);
	event_cancel(&m_pkNewPetSystemExpireEvent);
	m_petActorMap.clear();
}

void CNewPetSystem::UpdateTime()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;

		if (0 != petActor && petActor->IsSummoned())
		{
			petActor->UpdateTime();
		}
	}
}

bool CNewPetSystem::Update(DWORD deltaTime)
{
	bool bResult = true;

	DWORD currentTime = get_dword_time();

	if (m_dwUpdatePeriod > currentTime - m_dwLastUpdateTime)
		return true;

	std::vector <CNewPetActor*> v_garbageActor;

	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;

		if (0 != petActor && petActor->IsSummoned())
		{
			LPCHARACTER pPet = petActor->GetCharacter();

			if (NULL == CHARACTER_MANAGER::instance().Find(pPet->GetVID()))
			{
				v_garbageActor.push_back(petActor);
			}
			else
			{
				bResult = bResult && petActor->Update(deltaTime);
			}
		}
	}
	for (std::vector<CNewPetActor*>::iterator it = v_garbageActor.begin(); it != v_garbageActor.end(); it++)
		DeletePet(*it);

	m_dwLastUpdateTime = currentTime;
	return bResult;
}

void CNewPetSystem::DeletePet(DWORD mobVnum)
{
	TNewPetActorMap::iterator iter = m_petActorMap.find(mobVnum);

	if (m_petActorMap.end() == iter)
	{
		sys_err("[CPetSystem::DeletePet] Can't find pet on my list (VNUM: %d)", mobVnum);
		return;
	}

	CNewPetActor* petActor = iter->second;

	if (0 == petActor)
		sys_err("[CPetSystem::DeletePet] Null Pointer (petActor)");
	else
		delete petActor;

	m_petActorMap.erase(iter);
}

void CNewPetSystem::DeletePet(CNewPetActor* petActor)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		if (iter->second == petActor)
		{
			delete petActor;
			m_petActorMap.erase(iter);
			return;
		}
	}

	sys_err("[CPetSystem::DeletePet] Can't find petActor(0x%x) on my list(size: %d) ", petActor, m_petActorMap.size());
}

void CNewPetSystem::Unsummon(DWORD vnum, bool bDeleteFromList)
{
	CNewPetActor* actor = this->GetByVnum(vnum);

	if (0 == actor)
	{
		sys_err("[CPetSystem::GetByVnum(%d)] Null Pointer (petActor)", vnum);
		return;
	}
	actor->Unsummon();

	if (true == bDeleteFromList)
		this->DeletePet(actor);

	bool bActive = false;
	for (TNewPetActorMap::iterator it = m_petActorMap.begin(); it != m_petActorMap.end(); it++)
	{
		bActive |= it->second->IsSummoned();
	}
	if (false == bActive)
	{
		event_cancel(&m_pkNewPetSystemUpdateEvent);
		event_cancel(&m_pkNewPetSystemExpireEvent);
		m_pkNewPetSystemUpdateEvent = NULL;
		m_pkNewPetSystemExpireEvent = NULL;
	}
}

DWORD CNewPetSystem::GetNewPetITemID()
{
	DWORD itemid = 0;
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				itemid = petActor->GetSummonItemID();
				break;
			}
		}
	}
	return itemid;
}

bool CNewPetSystem::IsActivePet()
{
	bool state = false;
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				state = true;
				break;
			}
		}
	}
	return state;
}

int CNewPetSystem::GetLevelStep()
{
	int step = 4;
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				step = petActor->GetLevelStep();
				break;
			}
		}
	}
	return step;
}

void CNewPetSystem::SetExp(int iExp, int mode)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				petActor->SetExp(iExp, mode);
				break;
			}
		}
	}
}

int CNewPetSystem::GetEvolution()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->GetEvolution();
			}
		}
	}
	return -1;
}

int CNewPetSystem::GetLevel()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->GetLevel();
			}
		}
	}
	return -1;
}

int CNewPetSystem::GetExp()
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->GetExp();
			}
		}
	}
	return 0;
}

void CNewPetSystem::SetItemCube(int pos, int invpos)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->SetItemCube(pos, invpos);
			}
		}
	}
}

void CNewPetSystem::ItemCubeFeed(int type)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->ItemCubeFeed(type);
			}
		}
	}
}

void CNewPetSystem::DoPetSkill(int skillslot)
{
	for (TNewPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;
		if (petActor != 0)
		{
			if (petActor->IsSummoned())
			{
				return petActor->DoPetSkill(skillslot);
			}
		}
	}
}

CNewPetActor* CNewPetSystem::Summon(DWORD mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, DWORD options)
{
	CNewPetActor* petActor = this->GetByVnum(mobVnum);

	if (0 == petActor)
	{
		petActor = M2_NEW CNewPetActor(m_pkOwner, mobVnum, options);
		m_petActorMap.insert(std::make_pair(mobVnum, petActor));
	}

	DWORD petVID = petActor->Summon(petName, pSummonItem, bSpawnFar);

	if (NULL == m_pkNewPetSystemUpdateEvent)
	{
		newpetsystem_event_info* info = AllocEventInfo<newpetsystem_event_info>();

		info->pPetSystem = this;

		m_pkNewPetSystemUpdateEvent = event_create(newpetsystem_update_event, info, PASSES_PER_SEC(1) / 4);
	}

	if (NULL == m_pkNewPetSystemExpireEvent)
	{
		newpetsystem_event_infoe* infoe = AllocEventInfo<newpetsystem_event_infoe>();

		infoe->pPetSystem = this;

		m_pkNewPetSystemExpireEvent = event_create(newpetsystem_expire_event, infoe, PASSES_PER_SEC(1) );	// 1 volata per sec
	}

	return petActor;
}

CNewPetActor* CNewPetSystem::GetByVID(DWORD vid) const
{
	CNewPetActor* petActor = 0;

	bool bFound = false;

	for (TNewPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		petActor = iter->second;

		if (0 == petActor)
		{
			sys_err("[CPetSystem::GetByVID(%d)] Null Pointer (petActor)", vid);
			continue;
		}

		bFound = petActor->GetVID() == vid;

		if (true == bFound)
			break;
	}

	return bFound ? petActor : 0;
}

CNewPetActor* CNewPetSystem::GetByVnum(DWORD vnum) const
{
	CNewPetActor* petActor = 0;

	TNewPetActorMap::const_iterator iter = m_petActorMap.find(vnum);

	if (m_petActorMap.end() != iter)
		petActor = iter->second;

	return petActor;
}

size_t CNewPetSystem::CountSummoned() const
{
	size_t count = 0;

	for (TNewPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
				++count;
		}
	}

	return count;
}

void CNewPetSystem::RefreshBuff()
{
	for (TNewPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CNewPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
			{
				petActor->ClearBuff();
				petActor->GiveBuff();
			}
		}
	}
}
#endif
