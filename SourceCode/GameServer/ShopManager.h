//�̵������
#pragma once
#include "Stdafx.h"
class CRoleEx;
class ShopManager
{
public:
	ShopManager();
	virtual ~ShopManager();
	void OnInit();
	void Destroy();
	void OnShellShopItem(CRoleEx* pRole, BYTE ShopID, BYTE ItemIndex, DWORD ItemSum);//�����̵���Զ���Ʒ

	void UpdateByMin();
private:
	bool ShopItemIsInTime(tagShopItemConfig* pItemConfig);
	void HandleShopItem();
private:
	HashMap<WORD, bool>		m_ShopItemStates;
};