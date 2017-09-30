//��ǰ��Ϸ�ĺ����� 
//һ�����Ӵ��� ������ҵ�һ����Ϸ
//�����ڷ��俪ʼ��ʱ����Ѿ�������� �������ú����������
#pragma once
#include "Stdafx.h"
#include".\FishLogic\fishdesk.h"
#include "GameTable.h"
#include "RoleManager.h"
#include "RoleEx.h"
#include"FishLogic\ExtConfig.h"


struct  GoldPool
{
	GoldPool(int64 ngold)
	{
		gold = ngold;
		open = false;
		byGive = 0;
	}
	int64 gold;
	bool open;
	byte byGive;
};
class TableManager //�������� ��Ҫһ�����ӵ������ļ������� ������������������̳߳�ͻ
{
	//���ӵĹ�����
public:
	TableManager();
	virtual ~TableManager();

	void OnInit();//��ʼ��
	bool ReloadConfig();
	void Destroy();
	void OnStopService();
	void Update(DWORD dwTimeStep);
	bool OnHandleTableMsg(DWORD dwUserID,NetCmd* pData);
	void OnPlayerJoinTable(BYTE TableTypeID, CRoleEx* pRoleEx,BYTE MonthID = 0,bool IsSendToClient = true);
	bool OnPlayerJoinTable(WORD TableID, CRoleEx* pRoleEx, bool IsSendToClient = true);
	void OnPlayerLeaveTable(DWORD dwUserID);
	void ResetTableInfo(DWORD dwUserID);
	//����������Ϣ
	void	SendDataToTable(DWORD dwUserID, NetCmd* pData);
	

	CRole* SearchUser(DWORD dwUserid);
	CConfig *GetGameConfig();
	GameTable* GetTable(WORD TableID);
	DWORD GetTableSum(){ return m_TableVec.size(); }
	//TableConfig&	GetTableConfig(){ return m_Config; }

	void OnChangeTableGlobel(WORD TableID, int AddGlobel, USHORT uTableRate);
	void OnResetTableGlobel(WORD TableID,int64 nValue);
	int64  GetTableGlobel(WORD TableID);

	void OnChangeTableTypePlayerSum(BYTE TableTypeID,bool IsAddOrDel);
	bool QueryPool(WORD TableID, int64 & nPoolGold);
	void QueryPool(BYTE TableTypeID, bool &bopen, int64&nPoolGold);
	std::list<DWORD> GetBlackList();
	bool SetBlackList(DWORD *pid, BYTE byCount);
	bool SetBlackList(DWORD UserID);
	bool UnSetBlackList(DWORD UserID);
	bool Isabhor(DWORD dwUserid);

	//bool reloadTableConfig()
	//{
	//	if (!FishResManager::Inst() || !FishResManager::Inst()->Init(L"fishdata"))
	//	{
	//		//AfxMessageBox(TEXT("��ȡfishdataʧ�ܣ�����"));
	//		return false;
	//	}

	//	return true;
	//}

private:
	DWORD									m_LastUpdate;
	//TableConfig								m_Config;
	
	WORD						 			m_MaxTableID;
	//���Ӿ���Ķ�����
	HashMap<DWORD, WORD>					m_RoleTableMap;
	std::vector<GameTable*>					m_TableVec;

	CConfig                     *m_pGameConfig;

	//CFishTimer                  m_TimerRanomCatch;
	CFishTimer                  m_TimerRp;
	CFishTimer                  m_TimerGameTime;
	CFishTimer                  m_TimerRpAdd;

	bool						m_bUpdateTime;
	bool						m_bUpdateRp;
	bool						m_bUpdateRpEffect;

	HashMap<BYTE, GoldPool>		m_TableGlobeMap;//�������͵Ľ�ҳ�
	HashMap<BYTE, DWORD>        m_TableTypeSum;//��ǰ������ҵ�����
	std::list<DWORD>			m_blacklist;
};