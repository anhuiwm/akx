//�����Ʒ������
#pragma once
#include "Stdafx.h"
class CRoleEx;
class RoleManager;
class RoleItemManger;
struct tagItemType //���һ�����͵���Ʒ�ļ�������
{
	//DWORD							AllItemSum;
	tagItemInfo						NonTimeItem;//����ʱ��Ʒ
	//HashMap<DWORD,tagItemInfo>		TimeItem; //��ʱ��Ʒ�ĳ�ʱ�����.��ʱ�����ṹ
	tagItemInfo						TimeItem;

	CRoleEx*						m_pRole;
	RoleItemManger*					m_pItemManager;
	tagItemType()
	{
		//AllItemSum = 0;
		ZeroMemory(&NonTimeItem, sizeof(NonTimeItem));;
		//TimeItem.clear();
		ZeroMemory(&TimeItem, sizeof(TimeItem));
	}
	~tagItemType()
	{
		Destroy();
	}
	DWORD AllItemSum();
	void OnInit(CRoleEx* pRole, RoleItemManger* pManager);
	void LoadItem(tagItemInfo& pInfo);
	void Destroy();
	bool AddItem(tagItemOnce& pItem);
	void OnAddItemResult(DBO_Cmd_AddUserItem* pMsg);
	bool DelItem(DWORD ItemID, DWORD ItemSum);
	//bool DelItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum);
	void OnUpdateByMin();
};
class RoleItemManger//�����Logon�ϵ���Ʒ��
{
public:
	RoleItemManger();
	virtual ~RoleItemManger();
	bool OnInit(CRoleEx* pUser, RoleManager* pManager);
	bool OnLoadUserItem();
	void OnLoadUserItemResult(DBO_Cmd_LoadUserItem* pDB);
	//void OnLoadUserItemFinish();
	bool OnGetUserItem();
	bool OnAddUserItem(tagItemOnce& pItem);
	void OnAddUserItemResult(DBO_Cmd_AddUserItem* pDB);
	bool OnDelUserItem(DWORD ItemID, DWORD ItemSum);
	bool OnDelUserItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum);
	bool OnQueryDelUserItemList(vector<tagItemOnce>& pVec, DWORD Currey, bool bQuery = true);//ɾ��������Ʒ��ʱ��  һ���Խ����ж� 
	void OnUpdateByMin(bool IsHourChange, bool IsDayChange, bool IsMonthChange, bool IsYearChange);
	void LogItemToDB(DWORD dwUserID, int ItemID, int ItemSum, int EndItemSum, const TCHAR *pcStr);
	DWORD QueryItemCount(DWORD ItemID);
	DWORD QueryItemAllTimeCount(DWORD ItemID);
	bool IsLoadDB(){ return m_IsLoadDB; }
	bool IsSendClient(){ return m_IsLoadToClient; }

	void ResetClientInfo(){ m_IsLoadToClient = false; }

	bool OnTryUseItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum);
	bool OnTryAcceptItemToFriend(DWORD DestUserID,DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum);

	bool GetItemIsAllExists(DWORD ItemID, DWORD ItemSum);
	DWORD GetGoldBulletID() { return 910; }
	DWORD GetSilverBulletID() { return 909; }
	DWORD GetBronzeBulletID() { return 908; }
private:
	bool IsExistsItem(DWORD ItemID);
	bool IsCanUseItem(DWORD ItemID);
	bool IsCanAcceptItem(DWORD ItemID, DWORD ItemSum);
	bool OnUseItem(DWORD ItemOnlyID,DWORD ItemID, DWORD ItemSum);
private:
	bool							m_IsLoadToClient;
	//ָ��
	RoleManager*					m_RoleManager;
	CRoleEx*						m_pUser;
	bool							m_IsLoadDB;
	//��������
	HashMap<DWORD, tagItemType*>	m_ItemMap;
};