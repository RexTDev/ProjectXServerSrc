#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"
#include "log.h"
#include "skill.h"
#include "unique_item.h"
#include "profiler.h"
#include "marriage.h"
#include "item_addon.h"
#include "dev_log.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include "../../common/VnumHelper.h"
#include "../../common/CommonDefines.h"

CItem::CItem(DWORD dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0),
#ifdef ENABLE_SOULBIND_SYSTEM
	m_lBind(0), m_pkBindingExpireEvent(NULL), 
#endif
	m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pkDestroyEvent(NULL), m_pkExpireEvent(NULL), m_pkUniqueExpireEvent(NULL),
	m_pkTimerBasedOnWearExpireEvent(NULL), m_pkRealTimeExpireEvent(NULL),
   	m_pkAccessorySocketExpireEvent(NULL), m_pkOwnershipEvent(NULL), m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum (0)
#ifdef __CHANGE_LOOK_SYSTEM__
	,m_dwLook(0)
#endif
{
	memset( &m_alSockets, 0, sizeof(m_alSockets) );
	memset( &m_aAttr, 0, sizeof(m_aAttr) );
}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = NULL;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
#ifdef ENABLE_SOULBIND_SYSTEM
	m_lBind = 0;
	m_pkBindingExpireEvent = NULL;
#endif
	m_pProto = NULL;
	m_bExchanging = false;
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));

	m_pkDestroyEvent = NULL;
	m_pkOwnershipEvent = NULL;
	m_dwOwnershipPID = 0;
	m_pkUniqueExpireEvent = NULL;
	m_pkTimerBasedOnWearExpireEvent = NULL;
	m_pkRealTimeExpireEvent = NULL;

	m_pkAccessorySocketExpireEvent = NULL;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;
#ifdef OFFLINE_SHOP
	m_dwRealID = 0;
#endif
#ifdef __CHANGE_LOOK_SYSTEM__
	m_dwLook = 0;
#endif
}

void CItem::Destroy()
{
	event_cancel(&m_pkDestroyEvent);
	event_cancel(&m_pkOwnershipEvent);
	event_cancel(&m_pkUniqueExpireEvent);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);
	event_cancel(&m_pkRealTimeExpireEvent);
	event_cancel(&m_pkAccessorySocketExpireEvent);

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "item_destroy_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetOwner())
		sys_err("item_destroy_event: Owner exist. (item %s owner %s)", pkItem->GetName(), pkItem->GetOwner()->GetName());

	pkItem->SetDestroyEvent(NULL);
	M2_DESTROY_ITEM(pkItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pkEvent)
{
	m_pkDestroyEvent = pkEvent;
}

void CItem::StartDestroyEvent(int iSec)
{
	if (m_pkDestroyEvent)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION & c_pos = GetXYZ();

	struct packet_item_ground_add pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_ADD;
	pack.x		= c_pos.x;
	pack.y		= c_pos.y;
	pack.z		= c_pos.z;
	pack.dwVnum		= GetVnum();
	pack.dwVID		= m_dwVID;
	//pack.count	= m_dwCount;

	d->Packet(&pack, sizeof(pack));

	if (m_pkOwnershipEvent != NULL)
	{
		item_event_info * info = dynamic_cast<item_event_info *>(m_pkOwnershipEvent->info);

		if ( info == NULL )
		{
			sys_err( "CItem::EncodeInsertPacket> <Factor> Null pointer" );
			return;
		}

		TPacketGCItemOwnership p;

		p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
		p.dwVID = m_dwVID;
		strlcpy(p.szName, info->szOwnerName, sizeof(p.szName));

		d->Packet(&p, sizeof(TPacketGCItemOwnership));
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	struct packet_item_ground_del pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_DEL;
	pack.dwVID		= m_dwVID;

	d->Packet(&pack, sizeof(pack));
	sys_log(2, "Item::EncodeRemovePacket %s to %s", GetName(), ((LPCHARACTER) ent)->GetName());
}

void CItem::SetProto(const TItemTable * table)
{
	assert(table != NULL);
	m_pProto = table;
	SetFlag(m_pProto->dwFlags);
}

void CItem::UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use *packet)
{
	if (!GetVnum())
		return;

	packet->header 	= HEADER_GC_ITEM_USE;
	packet->ch_vid 	= ch->GetVID();
	packet->victim_vid 	= victim->GetVID();
	packet->Cell = TItemPos(GetWindow(), m_wCell);
	packet->vnum	= GetVnum();
}

void CItem::RemoveFlag(long bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(long bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;

	TPacketGCItemUpdate pack;

	pack.header = HEADER_GC_ITEM_UPDATE;
	pack.Cell = TItemPos(GetWindow(), m_wCell);
#ifdef __CHANGE_LOOK_SYSTEM__
	pack.dwVnum = m_dwLook;
#endif
	pack.count	= m_dwCount;
#ifdef ENABLE_SOULBIND_SYSTEM
	pack.bind = m_lBind;
#endif

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = m_alSockets[i];

	thecore_memcpy(pack.aAttr, GetAttributes(), sizeof(pack.aAttr));

	sys_log(2, "UpdatePacket %s -> %s", GetName(), m_pOwner->GetName());
	m_pOwner->GetDesc()->Packet(&pack, sizeof(pack));
}

DWORD CItem::GetCount()
{
	if (GetType() == ITEM_ELK) return MIN(m_dwCount, INT_MAX);
#ifdef ENABLE_CHEQUE_SYSTEM
	else if (GetType() == ITEM_WON) return MIN(m_dwCount, INT_MAX);
#endif
	else
	{
		return MIN(m_dwCount, g_bItemCountLimit);
	}
}

bool CItem::SetCount(DWORD count)
{
	if (GetType() == ITEM_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
#ifdef ENABLE_CHEQUE_SYSTEM
	else if (GetType() == ITEM_WON)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
#endif
	else
	{
		m_dwCount = MIN(count, g_bItemCountLimit);
	}

	if (count == 0 && m_pOwner)
	{
		if (GetSubType() == USE_ABILITY_UP || GetSubType() == USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			WORD wCell = GetCell();

			RemoveFromCharacter();

			if (!IsDragonSoul())
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, 255);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		else
		{
			if (!IsDragonSoul())
			{
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, 255);
			}
			M2_DESTROY_ITEM(RemoveFromCharacter());
		}

		return false;
	}

	UpdatePacket();

	Save();
	return true;
}

#ifdef ENABLE_SOULBIND_SYSTEM
void CItem::Bind(long lTime)
{
	if (lTime >= 0)
	{
		m_lBind = lTime;
		UpdatePacket();
		Save();
	}
}

bool CItem::IsBind()
{
	if (GetBind() == 1)
		return true;
	
	return false;
}

bool CItem::IsUntilBind()
{
	if (GetBind() >= 2)
		return true;
	
	return false;
}

EVENTFUNC(unbinding_expire_event)
{
	const item_vid_event_info * pInfo = reinterpret_cast<item_vid_event_info*>(event->info);
	if (!pInfo)
		return 0;
	
	const LPITEM pItem = ITEM_MANAGER::instance().FindByVID(pInfo->item_vid);
	if (!pItem)
		return 0;
	
	int iNextExpire = pItem->GetBind() - processing_time / passes_per_sec;
	if (iNextExpire <= 2)
	{
		pItem->Bind(0);
		pItem->SetUnBindingExpireEvent(NULL);
		return 0;
	}
	
	pItem->Bind(iNextExpire);
	return PASSES_PER_SEC(MIN(60, iNextExpire));
}

void CItem::SetUnBindingExpireEvent(LPEVENT pkEvent)
{
	m_pkBindingExpireEvent = pkEvent;
}

void CItem::StartUnBindingExpireEvent()
{
	if (m_pkBindingExpireEvent)
		return;
	
	item_vid_event_info * pInfo = AllocEventInfo<item_vid_event_info>();
	pInfo->item_vid = GetVID();
	SetUnBindingExpireEvent(event_create(unbinding_expire_event, pInfo, PASSES_PER_SEC(60)));
}
#endif

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		sys_err("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (m_bEquipped)	// �����Ǿ��°�?
	{
		Unequip();
		//pOwner->UpdatePacket();

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX && GetWindow() != MALL)
		{
			if (IsDragonSoul())
			{
				if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
			else
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (false == cell.IsDefaultInventoryPosition() && false == cell.IsBeltInventoryPosition()) // �ƴϸ� ����ǰ��?
					sys_err("CItem::RemoveFromCharacter: Invalid Item Position");
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = NULL;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell)
{
	assert(GetSectree() == NULL);
	assert(m_pOwner == NULL);
	WORD pos = Cell.cell;
	BYTE window_type = Cell.window_type;

	if (INVENTORY == window_type)
	{
		if (m_wCell >= INVENTORY_MAX_NUM && BELT_INVENTORY_SLOT_START > m_wCell)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
#ifdef __HIGHLIGHT_SYSTEM__
	bool bWereMine = this->GetLastOwnerPID() == ch->GetPlayerID();
#endif
	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();

#ifdef ENABLE_SOULBIND_SYSTEM
	event_cancel(&m_pkBindingExpireEvent);
#endif
	event_cancel(&m_pkDestroyEvent);

#ifdef __HIGHLIGHT_SYSTEM__
	ch->SetItem(TItemPos(window_type, pos), this, bWereMine);
#else
	ch->SetItem(TItemPos(window_type, pos), this);
#endif
	m_pOwner = ch;

	Save();
	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(NULL);

		GetSectree()->RemoveEntity(this);

		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck)
{
	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	//tree->Touch();

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;

	int iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());

	if (iDist > 300)
		return false;

	return true;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
	// Anti flag check
	switch (ch->GetJob())
	{
		case JOB_WARRIOR:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;

#ifdef ENABLE_WOLFMAN_CHARACTER
		case JOB_WOLFMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break;
#endif

#ifdef ENABLE_KEYTO_CHARACTER
		case JOB_ELFE:
			if (GetAntiFlag() & ITEM_ANTIFLAG_ELFE)
				return false;
			break;
#endif
	}

	return true;
}

int CItem::FindEquipCell(LPCHARACTER ch, int iCandidateCell)
{
	// �ڽ��� ������(ITEM_COSTUME)�� WearFlag ��� ��. (sub type���� ������ġ ����. ������ �� wear flag �� �ʿ䰡 �ֳ�..)
	// ��ȥ��(ITEM_DS, ITEM_SPECIAL_DS)��  SUB_TYPE���� ����. �ű� ����, ��Ʈ�� ITEM_TYPE���� ���� -_-
	if ((0 == GetWearFlag() || ITEM_TOTEM == GetType()) && ITEM_COSTUME != GetType() && ITEM_DS != GetType() && ITEM_SPECIAL_DS != GetType() && ITEM_RING != GetType() && ITEM_BELT != GetType())
		return -1;

	// ��ȥ�� ������ WEAR�� ó���� ���� ���(WEAR�� �ִ� 32������ �����ѵ� ��ȥ���� �߰��ϸ� 32�� �Ѵ´�.)
	// �κ��丮�� Ư�� ��ġ((INVENTORY_MAX_NUM + WEAR_MAX_NUM)���� (INVENTORY_MAX_NUM + WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX - 1)����)��
	// ��ȥ�� �������� ����.
	// return �� ����, INVENTORY_MAX_NUM�� �� ������,
	// ���� WearCell�� INVENTORY_MAX_NUM�� ���� return �ϱ� ����.
	if (GetType() == ITEM_DS || GetType() == ITEM_SPECIAL_DS)
	{
		if (iCandidateCell < 0)
		{
			return WEAR_MAX_NUM + GetSubType();
		}
		else
		{
			for (int i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
			{
				if (WEAR_MAX_NUM + i * DS_SLOT_MAX + GetSubType() == iCandidateCell)
				{
					return iCandidateCell;
				}
			}
			return -1;
		}
	}
	else if (GetType() == ITEM_COSTUME)
	{
		if (GetSubType() == COSTUME_BODY)
			return WEAR_COSTUME_BODY;
		else if (GetSubType() == COSTUME_HAIR)
			return WEAR_COSTUME_HAIR;
#ifdef ENABLE_ITEM_EFFECT_SYSTEM
		else if (GetSubType() == USE_EFFECT)
		{
			if (GetWearFlag() & WEARABLE_EFFECT_ARMOR)
				return WEAR_EFFECT_ARMOR;
			else
				return WEAR_EFFECT_WEAPON;
		}
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_ACCE)
			return WEAR_COSTUME_ACCE;
#endif
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_MOUNT)
			return WEAR_COSTUME_MOUNT;
#endif
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
		else if (GetSubType() == COSTUME_WEAPON)
			return WEAR_COSTUME_WEAPON;
#endif
	}
//#if !defined(ENABLE_MOUNT_COSTUME_SYSTEM) && !defined(ENABLE_ACCE_COSTUME_SYSTEM)
	else if (GetType() == ITEM_RING)
	{
		if (ch->GetWear(WEAR_RING1))
			return WEAR_RING2;
		else
			return WEAR_RING1;
	}
//#endif
	else if (GetType() == ITEM_BELT)
		return WEAR_BELT;
	else if (GetWearFlag() & WEARABLE_BODY)
		return WEAR_BODY;
	else if (GetWearFlag() & WEARABLE_HEAD)
		return WEAR_HEAD;
	else if (GetWearFlag() & WEARABLE_FOOTS)
		return WEAR_FOOTS;
	else if (GetWearFlag() & WEARABLE_WRIST)
		return WEAR_WRIST;
	else if (GetWearFlag() & WEARABLE_WEAPON)
		return WEAR_WEAPON;
	else if (GetWearFlag() & WEARABLE_SHIELD)
		return WEAR_SHIELD;
	else if (GetWearFlag() & WEARABLE_NECK)
		return WEAR_NECK;
	else if (GetWearFlag() & WEARABLE_EAR)
		return WEAR_EAR;
	else if (GetWearFlag() & WEARABLE_ARROW)
		return WEAR_ARROW;
	else if (GetWearFlag() & WEARABLE_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE1))
			return WEAR_UNIQUE2;
		else
			return WEAR_UNIQUE1;
	}

	// ���� ����Ʈ�� ���� �������� �����°����� �ѹ� ������ ���� �E�� ����.
	else if (GetWearFlag() & WEARABLE_ABILITY)
	{
		if (!ch->GetWear(WEAR_ABILITY1))
		{
			return WEAR_ABILITY1;
		}
		else if (!ch->GetWear(WEAR_ABILITY2))
		{
			return WEAR_ABILITY2;
		}
		else if (!ch->GetWear(WEAR_ABILITY3))
		{
			return WEAR_ABILITY3;
		}
		else if (!ch->GetWear(WEAR_ABILITY4))
		{
			return WEAR_ABILITY4;
		}
		else if (!ch->GetWear(WEAR_ABILITY5))
		{
			return WEAR_ABILITY5;
		}
		else if (!ch->GetWear(WEAR_ABILITY6))
		{
			return WEAR_ABILITY6;
		}
		else if (!ch->GetWear(WEAR_ABILITY7))
		{
			return WEAR_ABILITY7;
		}
		else if (!ch->GetWear(WEAR_ABILITY8))
		{
			return WEAR_ABILITY8;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

void CItem::ModifyPoints(bool bAdd)
{
	int accessoryGrade;

	// ����� ���ʸ� ������ �����Ų��.
	if (false == IsAccessoryForSocket())
	{
		if (m_pProto->bType == ITEM_WEAPON || m_pProto->bType == ITEM_ARMOR)
		{
			// ������ �Ӽ���ȭ�� ���Ǵ� ��� �������� �ʴ´� (ARMOR_WRIST ARMOR_NECK ARMOR_EAR)
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				DWORD dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
				{
#ifdef ENABLE_QUIVER_SYSTEM
					if (m_pProto->bSubType != WEAPON_QUIVER)
						sys_err("cannot find table by vnum %u", dwVnum);
#else
					sys_err("cannot find table by vnum %u", dwVnum);
#endif
					continue;
				}

				if (ITEM_METIN == p->bType)
				{
					//m_pOwner->ApplyPoint(p->alValues[0], bAdd ? p->alValues[1] : -p->alValues[1]);
					for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
					{
						if (p->aApplies[i].bType == APPLY_NONE)
							continue;

						if (p->aApplies[i].bType == APPLY_SKILL)
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : p->aApplies[i].lValue ^ 0x00800000);
						else
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : -p->aApplies[i].lValue);
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGrade(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
	}

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == APPLY_NONE)
			continue;

		long value = m_pProto->aApplies[i].lValue;

		if (m_pProto->aApplies[i].bType == APPLY_SKILL)
		{
			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : value ^ 0x00800000);
		}
		else
		{
			if (0 != accessoryGrade)
				value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : -value);
		}
	}
	// �ʽ´��� ����, �ҷ��� ����, �ູ�� ����, ������ ����� ���Ʈ�� ���
	// ������ �ϵ� �ڵ����� ������ �Ӽ��� �ο�������,
	// �� �κ��� �����ϰ� special item group ���̺����� �Ӽ��� �ο��ϵ��� �����Ͽ���.
	// ������ �ϵ� �ڵ��Ǿ����� �� ������ �������� �������� ���� �־ Ư��ó�� �س��´�.
	// �� �����۵��� ���, �ؿ� ITEM_UNIQUE�� ���� ó���� �Ӽ��� �ο��Ǳ� ������,
	// �����ۿ� �����ִ� attribute�� �������� �ʰ� �Ѿ��.
	if (true == CItemVnumHelper::IsRamadanMoonRing(GetVnum()) || true == CItemVnumHelper::IsHalloweenCandy(GetVnum())
		|| true == CItemVnumHelper::IsHappinessRing(GetVnum()) || true == CItemVnumHelper::IsLovePendant(GetVnum()))
	{
		// Do not anything.
	}
	else
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (GetAttributeType(i))
			{
				const TPlayerItemAttribute& ia = GetAttribute(i);

				if (ia.bType == APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : ia.sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : -ia.sValue);
			}
		}
	}

	switch (m_pProto->bType)
	{
		case ITEM_PICK:
		case ITEM_ROD:
			{
				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_WEAPON:
			{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_WEAPON))
 					break;
#endif

#ifdef ENABLE_QUIVER_SYSTEM
				if (m_pProto->bSubType == WEAPON_ARROW || m_pProto->bSubType == WEAPON_QUIVER)
				{
					if (bAdd)
					{
						if (m_wCell == INVENTORY_MAX_NUM + WEAR_ARROW)
						{
							m_pOwner->SetPart(PART_ARROW_TYPE, m_pProto->bSubType);
							const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
							if (pWeapon != NULL && pWeapon->GetSubType() == WEAPON_BOW)
							{
#ifdef __WEAPON_COSTUME_SYSTEM__
								const CItem* pCostumeWeapon = m_pOwner->GetWear(WEAR_COSTUME_WEAPON);
								if (pCostumeWeapon != NULL)
								{
									m_pOwner->SetPart(PART_WEAPON, pCostumeWeapon->GetVnum());
								}
								else
								{
									m_pOwner->SetPart(PART_WEAPON, pWeapon->GetVnum());
								}
#else
								m_pOwner->SetPart(PART_WEAPON, pWeapon->GetVnum());
#endif
							}
						}
					}
					else
					{
						if (m_wCell == INVENTORY_MAX_NUM + WEAR_ARROW)
							m_pOwner->SetPart(PART_ARROW_TYPE, m_pOwner->GetOriginalPart(PART_ARROW_TYPE));
					}
					
					break;
				}
#endif

#ifdef ENABLE_QUIVER_SYSTEM
				const CItem* pArrow = m_pOwner->GetWear(WEAR_ARROW);
#endif

				if (bAdd)
				{
#ifdef ENABLE_QUIVER_SYSTEM
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						if (pArrow != NULL)
							m_pOwner->SetPart(PART_ARROW_TYPE, pArrow->GetSubType());
#endif

#ifdef __CHANGE_LOOK_SYSTEM__
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetItemLook() > 0 ? GetItemLook() : GetVnum());
#else
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
#endif
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, 0);
				}
			}
			break;

		case ITEM_ARMOR:
			{
				// �ڽ��� body�� �԰��ִٸ� armor�� ���� �Դ� ��� ���� ���־� ������ �ָ� �� ��.
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_BODY))
					break;

				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD)
				{
					if (bAdd)
					{
#ifdef __CHANGE_LOOK_SYSTEM__
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, GetItemLook() > 0 ? GetItemLook() : GetVnum());
#else
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, GetVnum());
#endif
					}
					else
					{
						if (GetProto()->bSubType == ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, m_pOwner->GetOriginalPart(PART_MAIN));
					}
				}
			}
			break;

		// �ڽ��� ������ �Ծ��� �� ĳ���� parts ���� ����. ���� ��Ÿ�ϴ�� �߰���..
		case ITEM_COSTUME:
			{
				DWORD toSetValue = this->GetVnum();
				EParts toSetPart = PART_MAX_NUM;

				// ���� �ڽ���
				if (GetSubType() == COSTUME_BODY)
				{
					toSetPart = PART_MAIN;

					if (false == bAdd)
					{
						// �ڽ��� ������ ������ �� ���� ������ �԰� �־��ٸ� �� �������� look ����, ���� �ʾҴٸ� default look
#ifdef __CHANGE_LOOK_SYSTEM__
						CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetItemLook() > 0 ? pArmor->GetItemLook() : pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);
#else
						const CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);
#endif
					}
				}

				// ��� �ڽ���
				else if (GetSubType() == COSTUME_HAIR)
				{
					toSetPart = PART_HAIR;

					// �ڽ��� ���� shape���� item proto�� value3�� �����ϵ��� ��. Ư���� ������ ���� ���� ����(ARMOR_BODY)�� shape���� �������� value3�� �־ �� ���� value3���� ��.
					// [NOTE] ������ ������ vnum�� ������ ���� shape(value3)���� ������ ������.. ���� �ý����� �׷��� �Ǿ�����...
					toSetValue = (true == bAdd) ? this->GetValue(3) : 0;
				}

#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_MOUNT)
				{
					// not need to do a thing in here
				}
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_ACCE)
				{
				
					toSetValue = (true == bAdd) ? this->GetVnum() : 0; // Scale
					
					toSetPart = PART_ACCE;
				}
#endif

#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
				else if (GetSubType() == COSTUME_WEAPON)
				{
					toSetPart = PART_WEAPON;
					if (false == bAdd)
					{
#ifdef __CHANGE_LOOK_SYSTEM__
						CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
						toSetValue = (NULL != pWeapon) ? pWeapon->GetItemLook() > 0 ? pWeapon->GetItemLook() : pWeapon->GetVnum() : m_pOwner->GetOriginalPart(PART_WEAPON);
#else
						const CItem* pWeapon = m_pOwner->GetWear(WEAR_WEAPON);
						toSetValue = (NULL != pWeapon) ? pWeapon->GetVnum() : m_pOwner->GetOriginalPart(PART_WEAPON);
#endif
					}
				}
#endif

				if (PART_MAX_NUM != toSetPart)
				{
					m_pOwner->SetPart((BYTE)toSetPart, toSetValue);
					m_pOwner->UpdatePacket();
				}

#ifdef ENABLE_ITEM_EFFECT_SYSTEM
				if (GetSubType() == USE_EFFECT)
				{
					if (GetWearFlag() & WEARABLE_EFFECT_ARMOR)
					{
						if (!bAdd)
							m_pOwner->SetPart((BYTE)BODY_EFFECT, 0);
						else
							m_pOwner->SetPart((BYTE)BODY_EFFECT, GetValue(0));
					}
					else
					{
						if (!bAdd)
						{
							m_pOwner->SetPart((BYTE)WEAPON_RIGHT_EFFECT, 0);
							m_pOwner->SetPart((BYTE)WEAPON_LEFT_EFFECT, 0);
						}
						else
						{
							const CItem* pWeaponEffectCheck = m_pOwner->GetWear(WEAR_WEAPON);
							if (pWeaponEffectCheck != NULL)
							{
								DWORD dwEffectRight = 0;
								DWORD dwEffectLeft = 0;
								if (pWeaponEffectCheck->GetSubType() == WEAPON_DAGGER)
								{
									dwEffectRight = GetValue(1);
									dwEffectLeft = GetValue(2);
								}
								else if (pWeaponEffectCheck->GetSubType() == WEAPON_BOW)
								{
									dwEffectRight = GetValue(3);
								}
								else if (pWeaponEffectCheck->GetSubType() == WEAPON_BELL)
								{
									dwEffectRight = GetValue(1);
								}
								else if (pWeaponEffectCheck->GetSubType() == WEAPON_FAN)
								{
									dwEffectRight = GetValue(4);
								}
#ifdef ENABLE_WOLFMAN_CHARACTER_ITEM_EFFECT
								else if (pWeaponEffectCheck->GetSubType() == WEAPON_CLAW)
								{
									dwEffectRight = GetValue(1);
									dwEffectLeft = GetValue(2);
								}
#endif
#ifdef ENABLE_KEYTO_CHARACTER_ITEM_EFFECT
								else if (pWeaponEffectCheck->GetSubType() == WEAPON_POLE)
								{
									dwEffectRight = GetValue(0);
								}
#endif

								else
								{
									dwEffectRight = GetValue(0);
								}
								
								m_pOwner->SetPart((BYTE)WEAPON_RIGHT_EFFECT, dwEffectRight);
								m_pOwner->SetPart((BYTE)WEAPON_LEFT_EFFECT, dwEffectLeft);
							}
						}
					}
					
					m_pOwner->UpdatePacket();
				}
#endif

			}
			break;
		case ITEM_UNIQUE:
			{
				if (0 != GetSIGVnum())
				{
					const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(GetSIGVnum());
					if (NULL == pItemGroup)
						break;
					DWORD dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
					const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(dwAttrVnum);
					if (NULL == pAttrGroup)
						break;
					for (itertype (pAttrGroup->m_vecAttrs) it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
					{
						m_pOwner->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -it->apply_value);
					}
				}
			}
			break;
	}
}

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
	case ITEM_COSTUME:
	case ITEM_ARMOR:
	case ITEM_WEAPON:
	case ITEM_ROD:
	case ITEM_PICK:
	case ITEM_UNIQUE:
	case ITEM_DS:
	case ITEM_SPECIAL_DS:
	case ITEM_RING:
	case ITEM_BELT:
		return true;
	}

	return false;
}

// return false on error state
bool CItem::EquipTo(LPCHARACTER ch, BYTE bWearCell)
{
	if (!ch)
	{
		sys_err("EquipTo: nil character");
		return false;
	}

	// ��ȥ�� ���� index�� WEAR_MAX_NUM ���� ŭ.
	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
		{
			sys_err("EquipTo: invalid dragon soul cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
	{
		if (bWearCell >= WEAR_MAX_NUM)
		{
			sys_err("EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
	}

	if (ch->GetWear(bWearCell))
	{
		sys_err("EquipTo: item already exist (this: #%d %s cell: %d %s)", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

	ch->SetWear(bWearCell, this); // ���⼭ ��Ŷ ����

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell	= INVENTORY_MAX_NUM + bWearCell;

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	if (IsDragonSoul())
	{
		DSManager::instance().ActivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(true);
		StartUniqueExpireEvent();
		if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
			StartTimerBasedOnWearExpireEvent();

		// ACCESSORY_REFINE
		StartAccessorySocketExpireEvent();
		// END_OF_ACCESSORY_REFINE
	}

	ch->BuffOnAttr_AddBuffsFromItem(this);

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	Save();

	return (true);
}

bool CItem::Unequip()
{
	if (!m_pOwner || GetCell() < INVENTORY_MAX_NUM)
	{
		// ITEM_OWNER_INVALID_PTR_BUG
		sys_err("%s %u m_pOwner %p, GetCell %d",
				GetName(), GetID(), get_pointer(m_pOwner), GetCell());
		// END_OF_ITEM_OWNER_INVALID_PTR_BUG
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - INVENTORY_MAX_NUM))
	{
		sys_err("m_pOwner->GetWear() != this");
		return false;
	}

	//�ű� �� ������ ���Ž� ó��
	if (IsRideItem())
		ClearMountAttributeAndAffect();

	if (IsDragonSoul())
	{
		DSManager::instance().DeactivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(false);
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		StopTimerBasedOnWearExpireEvent();

	// ACCESSORY_REFINE
	StopAccessorySocketExpireEvent();
	// END_OF_ACCESSORY_REFINE


	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - INVENTORY_MAX_NUM, NULL);

#ifndef ENABLE_IMMUNE_FIX
	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (m_pOwner->GetWear(i))
		{
			// m_pOwner->ChatPacket(CHAT_TYPE_INFO, "unequip immuneflag(%u)", m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag); // always 0
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);
		}
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);
#endif

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	m_pOwner = NULL;
	m_wCell = 0;
	m_bEquipped	= false;

	return true;
}

long CItem::GetValue(DWORD idx)
{
	assert(idx < ITEM_VALUES_MAX_NUM);
	return GetProto()->alValues[idx];
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::instance().DelayedSave(this);
}

bool CItem::CreateSocket(BYTE bSlot, BYTE bGold)
{
	assert(bSlot < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		sys_err("Item::CreateSocket : socket already exist %s %d", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const long * c_al)
{
	thecore_memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSocket(int i, long v, bool bLog)
{
	assert(i < ITEM_SOCKET_MAX_NUM);
	m_alSockets[i] = v;
	UpdatePacket();
	Save();
	if (bLog)
	{
#ifdef ENABLE_NEWSTUFF
		if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
		LogManager::instance().ItemLog(i, v, 0, GetID(), "SET_SOCKET", "", "", GetOriginalVnum());
	}
}

#ifdef FULL_YANG
long long CItem::GetGold()
#else
int CItem::GetGold()
#endif
{
	if (IS_SET(GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->dwGold == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->dwGold;
	}
	else
		return GetProto()->dwGold;
}

#ifdef FULL_YANG
long long CItem::GetShopBuyPrice()
#else
int CItem::GetShopBuyPrice()
#endif
{
	return GetProto()->dwShopBuyPrice;
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pkOwnershipEvent)
		return true;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ownership_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	pkItem->SetOwnershipEvent(NULL);

	TPacketGCItemOwnership p;

	p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID	= pkItem->GetVID();
	p.szName[0]	= '\0';

	pkItem->PacketAround(&p, sizeof(p));
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pkEvent)
{
	m_pkOwnershipEvent = pkEvent;
}

void CItem::SetOwnership(LPCHARACTER ch, int iSec)
{
	if (!ch)
	{
		if (m_pkOwnershipEvent)
		{
			event_cancel(&m_pkOwnershipEvent);
			m_dwOwnershipPID = 0;

			TPacketGCItemOwnership p;

			p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
			p.dwVID	= m_dwVID;
			p.szName[0]	= '\0';

			PacketAround(&p, sizeof(p));
		}
		return;
	}

	if (m_pkOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = m_dwVID;
	strlcpy(p.szName, ch->GetName(), sizeof(p.szName));

	PacketAround(&p, sizeof(p));
}

int CItem::GetSocketCount()
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM_SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int count = GetSocketCount();
	if (count == ITEM_SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int iSocketCount)
{
	if (iSocketCount >= ITEM_SOCKET_MAX_NUM)
	{
		sys_log(0, "Invalid Socket Count %d, set to maximum", ITEM_SOCKET_MAX_NUM);
		iSocketCount = ITEM_SOCKET_MAX_NUM;
	}

	for (int i = 0; i < iSocketCount; ++i)
		SetSocket(i, 1);
}

void CItem::AlterToMagicItem()
{
	int idx = GetAttributeSetIndex();

	if (idx < 0)
		return;

	//      Appeariance Second Third
	// Weapon 50        20     5
	// Armor  30        10     2
	// Acc    20        10     1

	int iSecondPct;
	int iThirdPct;

	switch (GetType())
	{
		case ITEM_WEAPON:
			iSecondPct = 20;
			iThirdPct = 5;
			break;

		case ITEM_ARMOR:
		case ITEM_COSTUME:
			if (GetSubType() == ARMOR_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			break;

		default:
			return;
	}

	// 100% Ȯ���� ���� �Ӽ� �ϳ�
	PutAttribute(aiItemMagicAttributePercentHigh);

	if (number(1, 100) <= iSecondPct)
		PutAttribute(aiItemMagicAttributePercentLow);

	if (number(1, 100) <= iThirdPct)
		PutAttribute(aiItemMagicAttributePercentLow);
}

DWORD CItem::GetRefineFromVnum()
{
	return ITEM_MANAGER::instance().GetRefineFromVnum(GetVnum());
}

int CItem::GetRefineLevel()
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int	rtn = 0;
	str_to_number(rtn, p+1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int	locale_rtn = 0;
		str_to_number(locale_rtn, p+1);
		if (locale_rtn != rtn)
		{
			sys_err("refine_level_based_on_NAME(%d) is not equal to refine_level_based_on_LOCALE_NAME(%d).", rtn, locale_rtn);
		}
	}

	return rtn;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "unique_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetValue(2) == 0)
	{
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			sys_log(0, "UNIQUE_ITEM: expire %s %u", pkItem->GetName(), pkItem->GetID());
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pkItem->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();

		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// ���� ���� �ð��� �����۵��� ���������ϰ� ������� �ʴ� ���װ� �־�
			// ����
			// by rtsummit
			if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

// �ð� �ĺ���
// timer�� ������ ���� �ð� �����ϴ� ���� �ƴ϶�,
// timer�� ��ȭ�� ���� timer�� ������ �ð� ��ŭ �ð� ������ �Ѵ�.
EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "expire_event <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) - processing_time/passes_per_sec;
	if (remain_time <= 0)
	{
		sys_log(0, "ITEM EXPIRED : expired %s %u", pkItem->GetName(), pkItem->GetID());
		pkItem->SetTimerBasedOnWearExpireEvent(NULL);
		pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, 0);

		// �ϴ� timer based on wear ��ȥ���� �ð� �� �Ǿ��ٰ� ������ �ʴ´�.
		if (pkItem->IsDragonSoul())
		{
			DSManager::instance().DeactivateDragonSoul(pkItem);
		}
		else
		{
			ITEM_MANAGER::instance().RemoveItem(pkItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		}
		return 0;
	}
	pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC (MIN (60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pkEvent)
{
	m_pkUniqueExpireEvent = pkEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent)
{
	m_pkTimerBasedOnWearExpireEvent = pkEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (NULL == info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID( info->item_vid );

	if (NULL == item)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		switch (item->GetVnum())
		{
			if(item->IsNewMountItem())
			{
				if (item->GetSocket(2) != 0)
					item->ClearMountAttributeAndAffect();
			}
			break;
		}

		ITEM_MANAGER::instance().RemoveItem(item, "REAL_TIME_EXPIRE");

		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pkRealTimeExpireEvent)
		return;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pkRealTimeExpireEvent = event_create( real_time_expire_event, info, PASSES_PER_SEC(1));

			sys_log(0, "REAL_TIME_EXPIRE: StartRealTimeExpireEvent");

			return;
		}
	}
}

bool CItem::IsRealTimeItem()
{
	if(!GetProto())
		return false;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return true;
	}
	return false;
}

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM_UNIQUE)
		return;

	if (m_pkUniqueExpireEvent)
		return;

	//�Ⱓ�� �������� ��� �ð��� �������� �������� �ʴ´�
	if (IsRealTimeItem())
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int iSec = GetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

// �ð� �ĺ���
// timer_based_on_wear_expire_event ���� ����
void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pkTimerBasedOnWearExpireEvent)
		return;

	//�Ⱓ�� �������� ��� �ð��� �������� �������� �ʴ´�
	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->cLimitTimerBasedOnWearIndex)
		return;

	int iSec = GetSocket(0);

	// ���� �ð��� �д����� ���� ����...
	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pkUniqueExpireEvent)
		return;

	if (GetValue(2) != 0) // ���ӽð��� �̿��� �������� UniqueExpireEvent�� �ߴ��� �� ����.
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, event_time(m_pkUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pkUniqueExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pkTimerBasedOnWearExpireEvent)
		return;

	int remain_time = GetSocket(ITEM_SOCKET_REMAIN_SEC) - event_processing_time(m_pkTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::ApplyAddon(int iAddonType)
{
	CItemAddonManager::instance().ApplyAddonTo(iAddonType, this);
}

int CItem::GetSpecialGroup() const
{
	return ITEM_MANAGER::instance().GetSpecialGroupFromItem(GetVnum());
}

//
// �Ǽ����� ���� ó��.
//
bool CItem::IsAccessoryForSocket()
{
	return (m_pProto->bType == ITEM_ARMOR && (m_pProto->bSubType == ARMOR_WRIST || m_pProto->bSubType == ARMOR_NECK || m_pProto->bSubType == ARMOR_EAR)) ||
		(m_pProto->bType == ITEM_BELT);				// 2013�� 2�� ���� �߰��� '��Ʈ' �������� ��� ��ȹ������ �Ǽ����� ���� �ý����� �״�� �̿����ڰ� ��.
}

void CItem::SetAccessorySocketGrade(int iGrade)
{
	SetSocket(0, MINMAX(0, iGrade, GetAccessorySocketMaxGrade()));

	int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

	//if (test_server)
	//	iDownTime /= 60;

	SetAccessorySocketDownGradeTime(iDownTime);
}

void CItem::SetAccessorySocketMaxGrade(int iMaxGrade)
{
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM));
}

void CItem::SetAccessorySocketDownGradeTime(DWORD time)
{
	SetSocket(2, time);

	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s���� ���� ���������� ���� �ð� %d"), GetName(), time);
}

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "accessory_socket_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);

	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
degrade:
		item->SetAccessorySocketExpireEvent(NULL);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pkAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade() == 0)
		return;

	int iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(NULL);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pkAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

	int new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pkAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pkAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}

bool CItem::IsRideItem()
{
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
	if (ITEM_COSTUME == GetType() && COSTUME_MOUNT == GetSubType())
		return true;
#endif
	return false;
}

bool CItem::IsRamadanRing()
{
	if (GetVnum() == UNIQUE_ITEM_RAMADAN_RING)
		return true;
	return false;
}

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();

	ch->RemoveAffect(AFFECT_MOUNT);
	ch->RemoveAffect(AFFECT_MOUNT_BONUS);

	ch->MountVnum(0);

	ch->PointChange(POINT_ST, 0);
	ch->PointChange(POINT_DX, 0);
	ch->PointChange(POINT_HT, 0);
	ch->PointChange(POINT_IQ, 0);
}

// fixme
// �̰� ������ �Ⱦ���... �ٵ� Ȥ�ó� �; ���ܵ�.
// by rtsummit
bool CItem::IsNewMountItem()
{
	switch(GetVnum())
	{
		case 76000: case 76001: case 76002: case 76003:
		case 76004: case 76005: case 76006: case 76007:
		case 76008: case 76009: case 76010: case 76011:
		case 76012: case 76013: case 76014:
			return true;
	}
	return false;
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pkEvent)
{
	m_pkAccessorySocketExpireEvent = pkEvent;
}

void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade() > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s�� �����ִ� ������ ������ϴ�."), GetName());
		}

		ModifyPoints(false);
		SetAccessorySocketGrade(GetAccessorySocketGrade()-1);
		ModifyPoints(true);

		int iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

		if (test_server)
			iDownTime /= 60;

		SetAccessorySocketDownGradeTime(iDownTime);

		if (iDownTime)
			StartAccessorySocketExpireEvent();
	}
}

// ring�� item�� ���� �� �ִ��� ���θ� üũ�ؼ� ����
static const bool CanPutIntoRing(LPITEM ring, LPITEM item)
{
	//const DWORD vnum = item->GetVnum();
	return false;
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM_BELT)
		return this->GetSubType() == USE_PUT_INTO_BELT_SOCKET;

	else if(item->GetType() == ITEM_RING)
		return CanPutIntoRing(item, this);

	else if (item->GetType() != ITEM_ARMOR)
		return false;

	DWORD vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
	};
	const static JewelAccessoryInfo infos[] = {
		{ 50634, 14420, 16220, 17220 },
		{ 50635, 14500, 16500, 17500 },
		{ 50636, 14520, 16520, 17520 },
		{ 50637, 14540, 16540, 17540 },
		{ 50638, 14560, 16560, 17560 },
		{ 50639, 14570, 16570, 17570 },
	};

	DWORD item_type = (item->GetVnum() / 10) * 10;
	for (size_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch(item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		}
	}
	if (item->GetSubType() == ARMOR_WRIST)
		vnum -= 14000;
	else if (item->GetSubType() == ARMOR_NECK)
		vnum -= 16000;
	else if (item->GetSubType() == ARMOR_EAR)
		vnum -= 17000;
	else
		return false;

	DWORD type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != GetVnum())
			return false;
		else
			return true;
	}

	return 50623 + type == GetVnum();
}

// PC_BANG_ITEM_ADD
bool CItem::IsPCBangItem()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (m_pProto->aLimits[i].bType == LIMIT_PCBANG)
			return true;
	}
	return false;
}
// END_PC_BANG_ITEM_ADD

bool CItem::CheckItemUseLevel(int nLevel)
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			if (this->m_pProto->aLimits[i].lValue > nLevel) return false;
			else return true;
		}
	}
	return true;
}

long CItem::FindApplyValue(BYTE bApplyType)
{
	if (m_pProto == NULL)
		return 0;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == bApplyType)
			return m_pProto->aApplies[i].lValue;
	}

	return 0;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}

int CItem::GetAccessorySocketGrade()
{
   	return MINMAX(0, GetSocket(0), GetAccessorySocketMaxGrade());
}

int CItem::GetAccessorySocketMaxGrade()
{
   	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}

int CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
}

void CItem::AttrLog()
{
	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (m_alSockets[i])
		{
#ifdef ENABLE_NEWSTUFF
			if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
			LogManager::instance().ItemLog(i, m_alSockets[i], 0, GetID(), "INFO_SOCKET", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}

	for (int i = 0; i<ITEM_ATTRIBUTE_MAX_NUM; ++i)
	{
		int	type	= m_aAttr[i].bType;
		int value	= m_aAttr[i].sValue;

		if (type)
		{
#ifdef ENABLE_NEWSTUFF
			if (g_iDbLogLevel>=LOG_LEVEL_MAX)
#endif
			LogManager::instance().ItemLog(i, type, value, GetID(), "INFO_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}
}

int CItem::GetLevelLimit()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == LIMIT_LEVEL)
		{
			return this->m_pProto->aLimits[i].lValue;
		}
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{
	// �������� �� ���̶� ����ߴٸ�, �� ���Ŀ� ��� ������ �ʾƵ� �ð��� �����Ǵ� ���
	if (-1 != this->GetProto()->cLimitRealTimeFirstUseIndex)
	{
		// Socket1�� �������� ��� Ƚ���� ��ϵǾ� ������, �� ���̶� ����� �������� Ÿ�̸Ӹ� �����Ѵ�.
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}

#ifdef ENABLE_SOULBIND_SYSTEM
	if (IsUntilBind())
		StartUnBindingExpireEvent();
#endif

	return true;
}


#ifdef __AUCTION__

// �����
// window�� ��������� �Ѵ�.

bool CItem::MoveToAuction()
{
	LPCHARACTER owner = GetOwner();
	if (owner == NULL)
	{
		sys_err ("Item those owner is not exist cannot regist in auction");
		return false;
	}

	if (GetWindow() == AUCTION)
	{
		sys_err ("Item is already in auction.");
	}

	SetWindow(AUCTION);
	owner->SetItem(m_bCell, NULL);
	Save();
	ITEM_MANAGER::instance().FlushDelayedSave(this);

	return true;
}

void CItem::CopyToRawData (TPlayerItem* new_item)
{
	if (new_item != NULL)
		return;

	new_item->id = m_dwID;
	new_item->window = m_bWindow;
	new_item->pos = m_bCell;
	new_item->count = m_dwCount;

	new_item->vnum = GetVnum();
	thecore_memcpy (new_item->alSockets, m_alSockets, sizeof (m_alSockets));
	thecore_memcpy (new_item->aAttr, m_aAttr, sizeof (m_aAttr));

	new_item->owner = m_pOwner->GetPlayerID();
}
#endif

bool CItem::IsDragonSoul()
{
	return GetType() == ITEM_DS;
}

int CItem::GiveMoreTime_Per(float fPercent)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		DWORD given_time = fPercent * duration / 100u;
		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}
	// �켱 ��ȥ���� ���ؼ��� �ϵ��� �Ѵ�.
	else
		return 0;
}

int CItem::GiveMoreTime_Fix(DWORD dwTime)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		DWORD remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}
	// �켱 ��ȥ���� ���ؼ��� �ϵ��� �Ѵ�.
	else
		return 0;
}


int	CItem::GetDuration()
{
	if(!GetProto())
		return -1;

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return GetProto()->aLimits[i].lValue;
	}

	if (GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		BYTE cLTBOWI = GetProto()->cLimitTimerBasedOnWearIndex;
		return GetProto()->aLimits[cLTBOWI].lValue;
	}

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	// ���� VNUM�� ���ٸ� ���� �׷��� ������ ����
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}

#ifdef __CHANGE_LOOK_SYSTEM__
void CItem::SetItemLook(DWORD look)
{
	m_dwLook = look;
	UpdatePacket();
	ITEM_MANAGER::instance().SaveSingleItem(this);
	Save();
}
#endif