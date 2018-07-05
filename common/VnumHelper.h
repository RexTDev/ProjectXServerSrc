#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__

#include "CommonDefines.h"

/**
	�̹� �����ϰų� ������ �߰��� ������, �� ���� �ҽ����� �ĺ��� �� ����� ���
	�ĺ���(����=VNum)�� �ϵ��ڵ��ϴ� ������� �Ǿ��־ �������� �ſ� �������µ�

	�����δ� �ҽ��� ���� � ������(Ȥ�� ��)���� �� �� �ְ� ���ڴ� ��ö���� �������� �߰�.

		* �� ������ ������ ���������� ����Ǵµ� PCH�� ������ �ٲ� ������ ��ü ������ �ؾ��ϴ�
		�ϴ��� �ʿ��� cpp���Ͽ��� include �ؼ� ������ ����.

		* cpp���� �����ϸ� ������ ~ ��ũ�ؾ��ϴ� �׳� common�� ����� �־���. (game, db������Ʈ �� �� ��� ����)

	@date	2011. 8. 29.
*/


class CItemVnumHelper
{
public:
	/// ���� DVD�� �һ��� ��ȯ��
	static	const bool	IsPhoenix(DWORD vnum)				{ return 53001 == vnum; }		// NOTE: �һ��� ��ȯ �������� 53001 ������ mob-vnum�� 34001 �Դϴ�.

	/// �󸶴� �̺�Ʈ �ʽ´��� ���� (������ �󸶴� �̺�Ʈ�� Ư�� �������̾����� ������ ���� �������� ��Ȱ���ؼ� ��� ���ٰ� ��)
	static	const bool	IsRamadanMoonRing(DWORD vnum)		{ return 71135 == vnum || 71235 == vnum; }

	/// �ҷ��� ���� (������ �ʽ´��� ������ ����)
	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }

	/// ũ�������� �ູ�� ����
	static	const bool	IsHappinessRing(DWORD vnum)		{ return 71143 == vnum; }

	/// �߷�Ÿ�� ����� �Ҵ�Ʈ
	static	const bool	IsLovePendant(DWORD vnum)		{ return 71145 == vnum; }

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	static	const bool	IsAcce_Grade1(DWORD vnum)		{return 85001 == vnum || 85005 == vnum || 85011 == vnum || 85015 == vnum;}
	static	const bool	IsAcce_Grade2(DWORD vnum)		{return 85002 == vnum || 85006 == vnum || 85012 == vnum || 85016 == vnum;}
	static	const bool	IsAcce_Grade3(DWORD vnum)		{return 85003 == vnum || 85007 == vnum || 85013 == vnum || 85017 == vnum;}
//	static	const bool	IsAcce_Grade4(DWORD vnum)		{return 85004 == vnum || 85104 == vnum || 85008 == vnum || 85108 == vnum || 85014 == vnum || 85114 == vnum || 85018 == vnum || 85118 == vnum;}
	static	const bool	IsAcce_Grade4(DWORD vnum)			{return 85004 == vnum || 85008 == vnum || 85014 == vnum || 85018 == vnum || 85104 == vnum || 85108 == vnum || 85114 == vnum || 85118 == vnum;}
#endif

#ifdef ENABLE_NEW_RING_EFFECT
	static	const bool	IsMagicCandy(DWORD vnum)		{ return 71188 == vnum; }
#endif
#ifdef ENABLE_NEW_WS_EFFECT
	static	const bool	Is_SPEED_BOOTS(DWORD vnum)		{ return 72701 == vnum; }
#endif
#ifdef ENABLE_NEW_HM_EFFECT
	static	const bool	Is_Heromedal(DWORD vnum)		{ return 71158 == vnum; }
#endif
#ifdef ENABLE_NEW_CHOC_EFFECT
	static	const bool	Is_Chocolate_Amulet(DWORD vnum)	{ return 71199 == vnum; }
#endif
#ifdef ENABLE_EMOTION_MASK_EFFECT
	static	const bool	Is_Emotion_Mask(DWORD vnum)		{ return 71011  == vnum; }
#endif
};

class CMobVnumHelper
{
public:
	/// ���� DVD�� �һ��� �� ��ȣ
	static	bool	IsPhoenix(DWORD vnum)				{ return 34001 == vnum; }
	static	bool	IsIcePhoenix(DWORD vnum)				{ return 34003 == vnum; }
	/// PetSystem�� �����ϴ� ���ΰ�?
	static	bool	IsPetUsingPetSystem(DWORD vnum)	{ return (IsPhoenix(vnum) || IsReindeerYoung(vnum)) || IsIcePhoenix(vnum); }

	/// 2011�� ũ�������� �̺�Ʈ�� �� (�Ʊ� ����)
	static	bool	IsReindeerYoung(DWORD vnum)	{ return 34002 == vnum; }

	/// �󸶴� �̺�Ʈ ����� �渶(20119) .. �ҷ��� �̺�Ʈ�� �󸶴� �渶 Ŭ��(������ ����, 20219)
	static	bool	IsRamadanBlackHorse(DWORD vnum)		{ return 20119 == vnum || 20219 == vnum || 22022 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__
