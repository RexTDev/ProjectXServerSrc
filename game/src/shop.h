#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;
class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;
#ifdef FULL_YANG
			long long	price;
#else
			long	price;
#endif
#ifdef ENABLE_CHEQUE_SYSTEM
			long	price_cheque;
#endif
			BYTE	count;
			LPITEM	pkItem;
			int		itemid;

			shop_item()
			{
				vnum = 0;
				price = 0;
#ifdef ENABLE_CHEQUE_SYSTEM
				price_cheque = 0;
#endif
				count = 0;
				itemid = 0;
				pkItem = NULL;
			}
		} SHOP_ITEM;

		CShop();
		virtual ~CShop(); // @fixme139 (+virtual)

		bool	Create(DWORD dwVnum, DWORD dwNPCVnum, TShopItemTable * pItemTable);
		void	SetShopItems(TShopItemTable * pItemTable, BYTE bItemCount);

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pkPC ? true : false; }

		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void	RemoveGuest(LPCHARACTER ch);

		void	BroadcastUpdateItem(BYTE pos);

		int		GetNumberByVnum(DWORD dwVnum);

		virtual bool	IsSellingItem(DWORD itemID);
#ifdef FULL_YANG
		virtual long long	Buy(LPCHARACTER ch, BYTE pos);
#else
		virtual int Buy(LPCHARACTER ch, BYTE pos);
#endif

#if defined(SHOP_SEARCH) && defined(OFFLINE_SHOP)
  		bool SearchItem(DWORD vnum, long long price, DWORD book);
#endif

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

#ifdef OFFLINE_SHOP
public:
	int		GetItemCount();
	bool	GetItems();
	void	SetLocked(bool val) { m_bLocked = val; }
	bool	IsLocked() { return m_bLocked; }
	void	RemoveGuests(LPCHARACTER except);
	void	ClearItems();
	void	SetPrivShopItems(std::vector<TShopItemTable *> map_shop);
	LPCHARACTER GetPC() { return m_pkPC; }
protected:
	bool					m_bLocked;
#endif

	protected:
		void	Broadcast(const void * data, int bytes);

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;

		CGrid *				m_pGrid;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;

		LPCHARACTER			m_pkPC;
};
#endif
