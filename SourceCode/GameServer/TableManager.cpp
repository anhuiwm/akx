#include "StdAfx.h"
#include "Role.h"
#include "TableManager.h"
#include "FishLogic\FishScene\FishResManager.h"
#include "FishServer.h"
#include<atlconv.h>
#include <mmsystem.h>

#include "..\CommonFile\FishServerConfig.h"
#include<algorithm>

#pragma comment(lib, "winmm.lib")
#include "..\CommonFile\DBLogManager.h"

extern void SendLogDB(NetCmd* pCmd);

static volatile long	g_UpdateCount = 0;
static volatile bool	g_bRun = true;
struct TableUpdateParam
{
	GameTable* pTable;
	bool m_bUpdateTime;
	bool m_bUpdateRp;
	bool m_bUpdateRpEffect;
};
SafeList<TableUpdateParam*> g_SafeList[THREAD_NUM];
UINT WINAPI TableThreadUpdate(void *p)
{
	int idx = (int)p;
	SafeList<TableUpdateParam*> &safeList = g_SafeList[idx];
	while (g_bRun)
	{
		TableUpdateParam* pDesk = safeList.GetItem();
		while (pDesk)
		{
			//���ݲ�����������
			if (!pDesk)
				continue;
			if (!pDesk->pTable)
			{
				free(pDesk);
				continue;
			}
			pDesk->pTable->Update(pDesk->m_bUpdateTime, pDesk->m_bUpdateRp, pDesk->m_bUpdateRpEffect);
			::InterlockedIncrement(&g_UpdateCount);
			free(pDesk);

			pDesk = safeList.GetItem();
		}
		Sleep(1);
	}
	return 0;
}



//ȫ�����ӹ�����
TableManager::TableManager() //:m_GameConfig(this)
{
	m_pGameConfig = new CConfig(this);
	m_MaxTableID = 0;

	m_TableTypeSum.clear();
	m_TableGlobeMap.clear();
}
TableManager::~TableManager()
{
	Destroy();
}
void TableManager::OnInit()
{
	//��ȡȫ�ֵ������ļ�
	FishCallback::SetFishSetting(m_pGameConfig);
	if (!FishResManager::Inst() || !FishResManager::Inst()->Init(L"fishdata"))
	{
		//AfxMessageBox(TEXT("��ȡfishdataʧ�ܣ�����"));
		return;
	}

	{
		//USES_CONVERSION;
		//if (!m_GameConfig.LoadConfig(W2A(L"fishdata")))
		//{
		//	//AfxMessageBox(TEXT("��ȡgame configʧ��"));
		//	return;
		//}
		//m_TimerRanomCatch.StartTimer(m_pGameConfig->RandomCatchCycle()*MINUTE2SECOND, REPEATTIMER);//?
		//m_TimerRanomCatch.StartTimer(m_pGameConfig->RandomCatchCycle(), REPEATTIMER);//?
		m_TimerRp.StartTimer(m_pGameConfig->RpCycle(), REPEATTIMER);
		m_TimerGameTime.StartTimer(GAME_TIME_SPACE, REPEATTIMER);
	}
	
	for (int i = 0; i < THREAD_NUM; ++i)//�����߳�
	{
		::_beginthreadex(0, 0, (TableThreadUpdate), (void*)i, 0, 0);
	}
	m_LastUpdate = timeGetTime();
}

bool TableManager::ReloadConfig()
{
	if (!FishResManager::Inst() || !FishResManager::Inst()->Init(L"fishdata"))
	{
		//AfxMessageBox(TEXT("��ȡfishdataʧ�ܣ�����"));
		return false;
	}
	return true;
}

void TableManager::Destroy()
{
	//�����ٵ�ʱ�� 
	//1.���������Ӷ���ȫ�����ٵ� 
	g_bRun = false;
	if (!m_TableVec.empty())
	{
		std::vector<GameTable*>::iterator Iter = m_TableVec.begin();
		for (; Iter != m_TableVec.end(); ++Iter)
		{
			if (!(*Iter))
				continue;
			//CTraceService::TraceString(TEXT("һ�����Ӷ��� �Ѿ�������"), TraceLevel_Normal);
			delete *Iter;
		}
		m_TableVec.clear();
	}
	//2.����ȫ���������ļ�
	if (FishResManager::Inst())
		FishResManager::Inst()->Shutdown();
}
void TableManager::OnStopService()
{
	//��������ֹͣ��ʱ��
	Destroy();//���ӹ�����ֹͣ��������ʱ�� ���ǹرս���
}
void TableManager::Update(DWORD dwTimeStep)
{
	if (m_TableVec.empty())
		return;
	float TimeStep = (dwTimeStep - m_LastUpdate) * 0.001f;
	m_LastUpdate = dwTimeStep;

	//if (m_TimerRanomCatch.Update(TimeStep))
	//{

	//	//m_pGameConfig->RandomdFishByTime();
	//}

	bool m_bUpdateTime = m_TimerGameTime.Update(TimeStep);
	bool m_bUpdateRp = m_TimerRp.Update(TimeStep);
	bool m_bUpdateRpEffect = m_TimerRpAdd.Update(TimeStep);// zhi hou
	if (m_bUpdateRp)
	{
		m_TimerRpAdd.StartTimer(m_pGameConfig->RpEffectCycle(), 1);
	}
	//������������и���
	g_UpdateCount = 0;
	int UpdateSum = 0;
	for (UINT i = 0; i < m_TableVec.size(); ++i)
	{
		if (!m_TableVec[i])
			continue;
		if (m_TableVec[i]->GetTablePlayerSum() == 0)
			continue;
		int idx = i % THREAD_NUM;
		TableUpdateParam* pParam = (TableUpdateParam*)malloc(sizeof(TableUpdateParam));
		if (!pParam)
		{
			ASSERT(false);
			return;
		}
		pParam->m_bUpdateRp = m_bUpdateRp;
		pParam->m_bUpdateTime = m_bUpdateTime;
		pParam->m_bUpdateRpEffect = m_bUpdateRpEffect;
		pParam->pTable = m_TableVec[i];
		g_SafeList[idx].AddItem(pParam);
		++UpdateSum;
	}
	while (g_UpdateCount != UpdateSum)
		Sleep(1);
}
GameTable* TableManager::GetTable(WORD TableID)
{
	{
	if (TableID == 0xffff || TableID >= m_TableVec.size())
		//ASSERT(false);
		return NULL;
	}
	GameTable* pTable = m_TableVec[TableID];
	return pTable;
}
bool TableManager::OnPlayerJoinTable(WORD TableID, CRoleEx* pRoleEx, bool IsSendToClient)
{
	//��ҽ���һ������ 
	//�ж�����Ƿ���Լ��뷿��
	//���ȷ�������Ƿ���Խ��뷿������� ����ɹ��� �ڷ���������ҵ�����
	if (!pRoleEx)
	{
		ASSERT(false);
		return false;
	}
	GameTable* pTable = GetTable(TableID);
	if (!pTable)
	{
		ASSERT(false);
		return false;
	}
	BYTE TableTypeID = pTable->GetTableTypeID();
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(pRoleEx->GetUserID());
	if (Iter != m_RoleTableMap.end())
	{
		//����Ѿ����������޷�������������
		LC_JoinTableResult msg;
		SetMsgInfo(msg, GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		ASSERT(false);
		return false;
	}
	HashMap<BYTE, TableInfo>::iterator IterConfig = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(TableTypeID);
	if (IterConfig == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		LC_JoinTableResult msg;
		SetMsgInfo(msg, GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		ASSERT(false);
		return false;
	}

	if (pRoleEx->IsRobot() && !pTable->IsCanAddRobot())
	{
		return false;
	}
	if (pTable->OnRoleJoinTable(pRoleEx, pTable->GetTableMonthID(), IsSendToClient))
	{
		ServerClientData* pData = g_FishServer.GetUserClientDataByIndex(pRoleEx->GetGameSocketID());
		if (pData)
		{
			pData->IsInScene = true;
		}

		if (pTable->GetTableMonthID() == 0)
			OnChangeTableTypePlayerSum(TableTypeID, true);

		

		DBR_Cmd_TableChange msgDB;//��¼��ҽ���
		SetMsgInfo(msgDB, DBR_TableChange, sizeof(DBR_Cmd_TableChange));
		msgDB.dwUserID = pRoleEx->GetUserID();
		msgDB.CurrceySum = pRoleEx->GetRoleInfo().dwCurrencyNum;
		msgDB.GlobelSum = pRoleEx->GetRoleInfo().dwGlobeNum;
		msgDB.MedalSum = pRoleEx->GetRoleInfo().dwMedalNum;
		msgDB.JoinOrLeave = true;
		msgDB.LogTime = time(null);
		msgDB.TableTypeID = TableTypeID;
		msgDB.TableMonthID = pTable->GetTableMonthID();
		g_FishServer.SendNetCmdToSaveDB(&msgDB);
		g_DBLogManager.LogRoleJoinOrLeaveTableInfo(msgDB.dwUserID, msgDB.GlobelSum, msgDB.CurrceySum, msgDB.MedalSum, msgDB.TableTypeID, msgDB.TableMonthID, true, SendLogDB);

		//�û��ɹ��������� ������Ҫ��������ͻ���ȥ ���߿ͻ�����ҽ�����Ϸ�ɹ�
		m_RoleTableMap.insert(HashMap<DWORD, WORD>::value_type(pRoleEx->GetUserID(), pTable->GetTableID()));

		//if (pTable->GetTableMonthID() == 0 && !pRoleEx->IsRobot())
		//	g_FishServer.GetRobotManager().OnRoleJoinNormalRoom(pTable);
		return true;
	}
	else
	{
		//����������� �����Խ������� ������������ж�
		//��������ͻ��� ������ҽ�������ʧ����
		LC_JoinTableResult msg;
		SetMsgInfo(msg, GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		return false;
	}
}
void TableManager::OnPlayerJoinTable(BYTE TableTypeID, CRoleEx* pRoleEx, BYTE MonthID, bool IsSendToClient)
{
	//��ҽ���һ������ 
	//�ж�����Ƿ���Լ��뷿��
	//���ȷ�������Ƿ���Խ��뷿������� ����ɹ��� �ڷ���������ҵ�����
	if (!pRoleEx)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(pRoleEx->GetUserID());
	if (Iter != m_RoleTableMap.end())
	{
		//����Ѿ����������޷�������������
		LC_JoinTableResult msg;
		SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	HashMap<BYTE, TableInfo>::iterator IterConfig =  g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(TableTypeID);
	if (IterConfig == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		LC_JoinTableResult msg;
		SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	std::vector<GameTable*>::iterator IterVec = m_TableVec.begin();
	for (WORD i = 0; IterVec != m_TableVec.end(); ++IterVec, ++i)
	{
		if (!(*IterVec))
			continue;
		if ((*IterVec)->GetTableTypeID() == TableTypeID && MonthID == (*IterVec)->GetTableMonthID() && !(*IterVec)->IsFull())
		{
			//�жϻ������Ƿ���Լ��뵱ǰ����
			if (pRoleEx->IsRobot() && !(*IterVec)->IsCanAddRobot())
			{
				continue;
			}
			//��ǰ���������
			if (MonthID == 0 && !pRoleEx->IsRobot())
				g_FishServer.GetRobotManager().OnRoleJoinNormalRoom(*IterVec);

			if ((*IterVec)->OnRoleJoinTable(pRoleEx,MonthID,IsSendToClient))
			{
				ServerClientData* pData = g_FishServer.GetUserClientDataByIndex(pRoleEx->GetGameSocketID());
				if (pData)
				{
					pData->IsInScene = true;
				}

				if (MonthID == 0)
					OnChangeTableTypePlayerSum(TableTypeID, true);

				

				DBR_Cmd_TableChange msgDB;//��¼��ҽ���
				SetMsgInfo(msgDB, DBR_TableChange, sizeof(DBR_Cmd_TableChange));
				msgDB.dwUserID = pRoleEx->GetUserID();
				msgDB.CurrceySum = pRoleEx->GetRoleInfo().dwCurrencyNum;
				msgDB.GlobelSum = pRoleEx->GetRoleInfo().dwGlobeNum;
				msgDB.MedalSum = pRoleEx->GetRoleInfo().dwMedalNum;
				msgDB.JoinOrLeave = true;
				msgDB.LogTime = time(null);
				msgDB.TableTypeID = TableTypeID;
				msgDB.TableMonthID = MonthID;
				g_FishServer.SendNetCmdToSaveDB(&msgDB);
				g_DBLogManager.LogRoleJoinOrLeaveTableInfo(msgDB.dwUserID, msgDB.GlobelSum, msgDB.CurrceySum, msgDB.MedalSum, msgDB.TableTypeID, msgDB.TableMonthID, true, SendLogDB);

				//�û��ɹ��������� ������Ҫ��������ͻ���ȥ ���߿ͻ�����ҽ�����Ϸ�ɹ�
				m_RoleTableMap.insert(HashMap<DWORD, WORD>::value_type(pRoleEx->GetUserID(), i));

				//if (MonthID == 0 && !pRoleEx->IsRobot())
				//	g_FishServer.GetRobotManager().OnRoleJoinNormalRoom(*IterVec);

				return;
			}
			else
			{
				//����������� �����Խ������� ������������ж�
				//��������ͻ��� ������ҽ�������ʧ����
				LC_JoinTableResult msg;
				SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
				msg.bTableTypeID = TableTypeID;
				msg.Result = false;
				pRoleEx->SendDataToClient(&msg);
				return;
			}
		}
	}
	GameTable* pTable = new GameTable();
	if (!pTable)
	{
		LC_JoinTableResult msg;
		SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		//ASSERT(false);
		return;
	}
	pTable->OnInit(m_MaxTableID, TableTypeID,MonthID);//�����ӳ�ʼ��
	m_MaxTableID++;
	m_TableVec.push_back(pTable);

	//��ǰ���������
	if (MonthID == 0 && !pRoleEx->IsRobot())
		g_FishServer.GetRobotManager().OnRoleCreateNormalRoom(pTable);
	//���Ӳ���� ���ǿ�ʼ����
	if (pTable->OnRoleJoinTable(pRoleEx,MonthID,IsSendToClient))
	{
		//��ҽ������ӳɹ���
		ServerClientData* pData = g_FishServer.GetUserClientDataByIndex(pRoleEx->GetGameSocketID());
		if (pData)
		{
			pData->IsInScene = true;
		}

		if (MonthID == 0)
			OnChangeTableTypePlayerSum(TableTypeID, true);

		DBR_Cmd_TableChange msgDB;//��¼��ҽ���
		SetMsgInfo(msgDB, DBR_TableChange, sizeof(DBR_Cmd_TableChange));
		msgDB.dwUserID = pRoleEx->GetUserID();
		msgDB.CurrceySum = pRoleEx->GetRoleInfo().dwCurrencyNum;
		msgDB.GlobelSum = pRoleEx->GetRoleInfo().dwGlobeNum;
		msgDB.MedalSum = pRoleEx->GetRoleInfo().dwMedalNum;
		msgDB.JoinOrLeave = true;
		msgDB.LogTime = time(null);
		msgDB.TableTypeID = TableTypeID;
		msgDB.TableMonthID = MonthID;
		g_FishServer.SendNetCmdToSaveDB(&msgDB);
		g_DBLogManager.LogRoleJoinOrLeaveTableInfo(msgDB.dwUserID, msgDB.GlobelSum, msgDB.CurrceySum, msgDB.MedalSum, msgDB.TableTypeID, msgDB.TableMonthID, true, SendLogDB);

		m_RoleTableMap.insert(HashMap<DWORD, WORD>::value_type(pRoleEx->GetUserID(), m_TableVec.size() - 1));

		//if (MonthID == 0 && !pRoleEx->IsRobot())
		//	g_FishServer.GetRobotManager().OnRoleCreateNormalRoom(pTable);

		return;
	}
	else
	{
		LC_JoinTableResult msg;
		SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
		msg.bTableTypeID = TableTypeID;
		msg.Result = false;
		pRoleEx->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
}
void TableManager::ResetTableInfo(DWORD dwUserID)
{
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(dwUserID);
	if (Iter == m_RoleTableMap.end())
	{
		//ASSERT(false);
		return;
	}
	WORD Index = Iter->second;
	if (m_TableVec.size() <= Index)
	{
		ASSERT(false);
	}
	GameTable* pTable = m_TableVec[Index];
	if (!pTable)
	{
		ASSERT(false);
	}
	pTable->SendTableRoleInfoToClient(dwUserID);
}
void TableManager::OnPlayerLeaveTable(DWORD dwUserID)
{
	//��������ҵ���ǰ������ 
	//���������ҵ���� Ȼ��������뿪����
	//CWHDataLocker lock(m_CriticalSection);
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(dwUserID);
	if (Iter == m_RoleTableMap.end())
	{
		//ASSERT(false);
		return;
	}
	WORD Index = Iter->second;
	if (m_TableVec.size() <= Index)
	{
		ASSERT(false);
	}
	GameTable* pTable = m_TableVec[Index];
	if (!pTable)
	{
		ASSERT(false);
	}

	if (pTable->GetTableMonthID() == 0)
		OnChangeTableTypePlayerSum(pTable->GetTableTypeID(), false);

	//������뿪���ӵ�ʱ�� �����ˢ���µ��ͻ���ȥ
	CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (pRole)
	{
		DBR_Cmd_TableChange msgDB;//��¼��ҽ���
		SetMsgInfo(msgDB, DBR_TableChange, sizeof(DBR_Cmd_TableChange));
		msgDB.dwUserID = pRole->GetUserID();
		msgDB.CurrceySum = pRole->GetRoleInfo().dwCurrencyNum;
		msgDB.GlobelSum = pRole->GetRoleInfo().dwGlobeNum;
		msgDB.MedalSum = pRole->GetRoleInfo().dwMedalNum;
		msgDB.JoinOrLeave = false;
		msgDB.LogTime = time(null);
		msgDB.TableTypeID = pTable->GetTableTypeID();
		msgDB.TableMonthID = pTable->GetTableMonthID();
		g_FishServer.SendNetCmdToSaveDB(&msgDB);
		g_DBLogManager.LogRoleJoinOrLeaveTableInfo(msgDB.dwUserID, msgDB.GlobelSum, msgDB.CurrceySum, msgDB.MedalSum, msgDB.TableTypeID, msgDB.TableMonthID, false, SendLogDB);

		if (pTable->GetTableMonthID() != 0)
		{
			pRole->GetRoleMonth().OnPlayerLeaveTable();//����뿪����
		}

		ServerClientData* pData = g_FishServer.GetUserClientDataByIndex(pRole->GetGameSocketID());
		if (pData)
		{
			pData->IsInScene = false;
		}

		LC_Cmd_ResetRoleGlobel msg;
		SetMsgInfo(msg, GetMsgType(Main_Role, LC_ResetRoleGlobel), sizeof(LC_Cmd_ResetRoleGlobel));
		msg.dwTotalGlobel = pRole->GetRoleInfo().dwGlobeNum;
		msg.dwLotteryScore = pRole->GetRoleInfo().LotteryScore;
		pRole->SendDataToClient(&msg);//
	}

	pTable->OnRoleLeaveTable(dwUserID);
	m_RoleTableMap.erase(Iter);//����ҵ�����ɾ����

	if (pTable->GetTableMonthID() == 0 && !pRole->IsRobot())
		g_FishServer.GetRobotManager().OnRoleLeaveNormalRoom(pTable);
}
bool TableManager::OnHandleTableMsg(DWORD dwUserID,NetCmd* pData)
{
	//��Game��Ϣ���ݵ��ڲ�ȥ
	//CWHDataLocker lock(m_CriticalSection);
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(dwUserID);
	if (Iter == m_RoleTableMap.end())
	{
		//ASSERT(false);
		return false;
	}
	WORD Index = Iter->second;
	if (m_TableVec.size() <= Index)
	{
		ASSERT(false);
		return false;
	}
	GameTable* pTable = m_TableVec[Index];
	if (!pTable)
	{
		ASSERT(false); 
	}
	pTable->OnHandleTableMsg(dwUserID,pData);
	return true;
}
void TableManager::SendDataToTable(DWORD dwUserID, NetCmd* pData)
{
	//������͵�ָ����ҵ���������ȥ
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(dwUserID);
	if (Iter == m_RoleTableMap.end())
	{
		return;
	}
	WORD Index = Iter->second;
	if (m_TableVec.size() <= Index)
	{
		ASSERT(false);
		return;
	}
	GameTable* pTable = m_TableVec[Index];
	if (pTable)
	{
		pTable->SendDataToTable(dwUserID, pData);
	}
	else
	{
		ASSERT(false);
	}
}
CRole* TableManager::SearchUser(DWORD dwUserid)
{
	HashMap<DWORD, WORD>::iterator Iter = m_RoleTableMap.find(dwUserid);
	if (Iter == m_RoleTableMap.end())
	{
		//ASSERT(false);
		return NULL;
	}
	WORD Index = Iter->second;
	if (m_TableVec.size() <= Index)
	{
		ASSERT(false);
		return NULL; 
	}
	GameTable* pTable = m_TableVec[Index];
	if (!pTable)
	{
		ASSERT(false);
		return NULL;
	}
	return pTable->SearchUser(dwUserid);
}
CConfig* TableManager::GetGameConfig()
{
	return m_pGameConfig;
}
void TableManager::OnChangeTableGlobel(WORD TableID, int AddGlobel, USHORT uTableRate)
{
	
	GameTable* pTable = GetTable(TableID);
	if (!pTable)
	{
		ASSERT(false);
		return;
	}
	int64 AllGlobelSum = 0;
	HashMap<BYTE, GoldPool>::iterator Iter = m_TableGlobeMap.find(pTable->GetTableTypeID());
	if (Iter == m_TableGlobeMap.end())
	{
		AllGlobelSum = AddGlobel;
		m_TableGlobeMap.insert(HashMap<BYTE, GoldPool>::value_type(pTable->GetTableTypeID(), GoldPool(AllGlobelSum)));
	}
	else
	{
		Iter->second.gold += AddGlobel;
		AllGlobelSum = Iter->second.gold;
	}	

	Iter = m_TableGlobeMap.find(pTable->GetTableTypeID());
	//{
	//	char szKey[10] = { 0 };
	//	char szValue[32] = { 0 };
	//	char szFile[256] = { 0 };
	//	sprintf_s(szKey, sizeof(szKey), "%d", pTable->GetTableTypeID());
	//	sprintf_s(szValue, sizeof(szValue), "%lld", AllGlobelSum);
	//	sprintf_s(szFile, sizeof(szFile), "%s\\%s", g_FishServerConfig.GetFTPServerConfig()->FTPFilePath, "fish.txt");
	//	WritePrivateProfileStringA("fish", szKey, szValue, szFile);
	//}	
	if (Iter->second.open)
	{
		if (AllGlobelSum <=0)
		{
			Iter->second.open = false;
			//Iter->second.gold = 0;			
		}
	}

	if(AllGlobelSum >= Con_LuckItem::s_threshold*uTableRate)
	{
		Iter->second.open = true;
		Iter->second.gold -= (int64)(Con_LuckItem::s_base*AllGlobelSum);//�ȿ�
		byte nGiveIndex=Iter->second.byGive++;			

		DWORD UseGlobelSum = (DWORD)AllGlobelSum;// 		
		DWORD PlayerSum = 0;
		HashMap<BYTE, DWORD>::iterator Iter = m_TableTypeSum.find(pTable->GetTableTypeID());
		if (Iter == m_TableTypeSum.end())
		{
			PlayerSum = 0;
		}
		else
		{
			PlayerSum = Iter->second;
		}
		if (PlayerSum != 0 )
		{					
			DWORD RoleAddGlobel = (DWORD)min(UseGlobelSum / PlayerSum, UseGlobelSum *Con_LuckItem::s_part);
			if (nGiveIndex%2 == 0)//�ȵ��ȵ�
			{
				RoleAddGlobel = (int64)(UseGlobelSum *Con_LuckItem::s_part);
			}
			DWORD AddLuckyValue = RoleAddGlobel ;			
			//��ȫ��������ֵ������ҵ�����
			std::vector<GameTable*>::iterator IterVec = m_TableVec.begin();
			for (; IterVec != m_TableVec.end(); ++IterVec)
			{
				if (!(*IterVec))
					continue;
				if ((*IterVec)->GetTableTypeID() == pTable->GetTableTypeID() && (*IterVec)->GetTableMonthID() == 0)
				{
					BYTE MaxSum = (*IterVec)->GetRoleManager().GetMaxPlayerSum();
					for (BYTE i = 0; i < MaxSum; ++i)
					{
						CRole* pRole = (*IterVec)->GetRoleManager().GetRoleBySeatID(i);
						if (pRole && pRole->IsActionUser())
						{
							pRole->SetRoleLuckyValue(AddLuckyValue);
						}
					}
				}
			}
		}
	}
}
void TableManager::OnResetTableGlobel(WORD TableID, int64 nValue)
{
	GameTable* pTable = GetTable(TableID);
	if (!pTable)
	{
		ASSERT(false);
		return;
	}
	HashMap<BYTE, GoldPool>::iterator Iter = m_TableGlobeMap.find(pTable->GetTableTypeID());
	if (Iter != m_TableGlobeMap.end())
	{
		Iter->second = nValue;
	}
}
int64 TableManager::GetTableGlobel(WORD TableID)
{
	GameTable* pTable = GetTable(TableID);
	if (!pTable)
	{
		ASSERT(false);
		return 0;
	}
	HashMap<BYTE, GoldPool>::iterator Iter = m_TableGlobeMap.find(pTable->GetTableTypeID());
	if (Iter != m_TableGlobeMap.end())
		return Iter->second.gold;
	else
		return 0;
}
void TableManager::OnChangeTableTypePlayerSum(BYTE TableTypeID, bool IsAddOrDel)
{
	HashMap<BYTE, DWORD>::iterator Iter = m_TableTypeSum.find(TableTypeID);
	if (Iter == m_TableTypeSum.end())
	{
		if (IsAddOrDel)
		{
			m_TableTypeSum.insert(HashMap<BYTE, DWORD>::value_type(TableTypeID, 1));
			return;
		}
		else
		{
			ASSERT(false);
			return;
		}
	}
	else
	{
		if (IsAddOrDel)
		{
			Iter->second += 1;
			return;
		}
		else
		{
			if (Iter->second == 0)
			{
				ASSERT(false);
				return;
			}
			Iter->second -= 1;
			return;
		}
	}
}

bool TableManager::QueryPool(WORD TableID,int64 & nPoolGold)
{
	GameTable* pTable = GetTable(TableID);
	if (!pTable)
	{
		ASSERT(false);
		return false;
	}
	HashMap<BYTE, GoldPool>::iterator Iter = m_TableGlobeMap.find(pTable->GetTableTypeID());
	if (Iter == m_TableGlobeMap.end())
	{
		return false;
	}
	nPoolGold = Iter->second.gold;
	return 	Iter->second.open;
}

void TableManager::QueryPool(BYTE TableTypeID, bool & bopen, int64 & nPoolGold)
{
	bopen = false;
	nPoolGold = 0;

	HashMap<BYTE, GoldPool>::iterator Iter = m_TableGlobeMap.find(TableTypeID);
	if (Iter == m_TableGlobeMap.end())
	{
		return;
	}
	bopen = Iter->second.open;
	nPoolGold = Iter->second.gold;
}

std::list<DWORD> TableManager::GetBlackList()
{
	return m_blacklist;
}

//bool TableManager::SetBlackList(DWORD *pid, BYTE byCount)
//{
//	if (pid == NULL || byCount == 0)
//	{
//		return false;
//	}
//	m_blacklist.clear();
//	for (int i = 0; i < byCount; i++)
//	{
//		m_blacklist.push_back(pid[i]);
//	}	
//	return true;
//}

bool TableManager::SetBlackList(DWORD UserID)
{
	if (!Isabhor(UserID))
	{
		m_blacklist.push_back(UserID);
	}
	std::cout << "start black list:" << endl;
	for (auto id : m_blacklist)
	{
		std::cout << id << endl;
	}
	std::cout << "end" << endl;

	return true;
}

bool TableManager::UnSetBlackList(DWORD UserID)
{
	auto it = std::find(m_blacklist.begin(), m_blacklist.end(), UserID);
	if (it != m_blacklist.end())
	{
		m_blacklist.erase(it);
	}
	std::cout << "start" << endl;
	for (auto id : m_blacklist)
	{
		std::cout << id << endl;
	}
	std::cout << "end" << endl;
	return true;
}

bool TableManager::Isabhor(DWORD dwUserid)
{
	return std::find(m_blacklist.begin(), m_blacklist.end(), dwUserid) != m_blacklist.end();
}