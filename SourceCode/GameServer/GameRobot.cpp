#include "stdafx.h"
#include "GameRobot.h"
#include "FishServer.h"
#include "FishLogic\NetCmd.h"
GameRobotManager::GameRobotManager()
{
	m_RobotMap.clear();
	m_FreeRobot.clear();
	m_MonthSignMap.clear();
	m_WriteList.clear();
	m_IsLoadFinish = false;
	m_RobotClose = false;
}
GameRobotManager::~GameRobotManager()
{
	HashMap<DWORD, GameRobot*>::iterator IterMap = m_RobotMap.begin();
	for (; IterMap != m_RobotMap.end(); ++IterMap)
	{
		delete IterMap->second;
	}
	m_RobotMap.clear();
	vector<GameRobot*>::iterator IterVec = m_FreeRobot.begin();
	for (; IterVec != m_FreeRobot.end(); ++IterVec)
	{
		delete *IterVec;
	}
	m_FreeRobot.clear();
	m_MonthSignMap.clear();
	m_WriteList.clear();
}
void GameRobotManager::OnLoadAllGameRobot(DWORD BeginUserID, DWORD EndUserID)
{
	m_FreeRobot.clear();
	m_RobotMap.clear();
	if (BeginUserID == 0 || EndUserID == 0)
		return;//���������
	//�����˵Ĺ����� ����ǰGameServer��ȫ���Ļ����� 
	//��ʼ��  ��Ҫ֪�������˵����� ���Ǵ��������Ļ����� ���д��� �����ݿ��ȡ����  
	DBR_Cmd_GetAccountInfoByUserID msg;
	SetMsgInfo(msg, DBR_GetAccountInfoByUserID, sizeof(DBR_Cmd_GetAccountInfoByUserID));
	msg.ClientID = 0;
	msg.LogonByGameServer = false;
	for (size_t i = BeginUserID; i <= EndUserID; ++i)
	{
		//�������������
		msg.dwUserID = i;
		g_FishServer.SendNetCmdToDB(&msg);//��������
	}
	m_RobotConfigSum = EndUserID - BeginUserID + 1;
}
void GameRobotManager::OnAddRobot(DWORD dwUserID)
{
	//�����˼������
	CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	GameRobot* pRobot = new GameRobot(this,pRole);
	if (!pRobot)
	{
		ASSERT(false);
		return;
	}
	m_FreeRobot.push_back(pRobot);
	if (m_FreeRobot.size() >= m_RobotConfigSum*0.8 && !m_IsLoadFinish)
	{
		//����GameServer���������
		//g_FishServer.ShowInfoToWin("�����˼��ش������");
		m_IsLoadFinish = true;
		OnHandleAllMonthStates();
		g_FishServer.OnLoadFinish();
	}
	return;
}
GameRobot* GameRobotManager::GetFreeRobot(DWORD RobotID)
{
	//��ȡһ�����еĻ�����
	if (m_FreeRobot.empty())
		return NULL;
	size_t Index = RandUInt() % m_FreeRobot.size();
	GameRobot* pRobot = m_FreeRobot[Index];
	if (!pRobot)
		return NULL;

	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return NULL;
	{
		//����������޸�
		if (pRobot->GetRoleInfo()->GetRoleInfo().dwGlobeNum > Iter->second.InitGlobelSum)
		{
			DWORD MaxGlobel = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.GlobelRateF * 2)) - Iter->second.GlobelRateF) / 100.0) * Iter->second.InitGlobelSum);
			pRobot->GetRoleInfo()->ChangeRoleGlobe(MaxGlobel - pRobot->GetRoleInfo()->GetRoleInfo().dwGlobeNum);
		}
	}
	{

		if (pRobot->GetRoleInfo()->GetRoleInfo().dwCurrencyNum >  Iter->second.InitCurrceySum)
		{
			DWORD MaxCurrcey = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.CurrceyRateF * 2)) - Iter->second.CurrceyRateF) / 100.0) * Iter->second.InitCurrceySum);
			pRobot->GetRoleInfo()->ChangeRoleCurrency(MaxCurrcey - pRobot->GetRoleInfo()->GetRoleInfo().dwCurrencyNum, TEXT("�����˲�����ʯ"));
		}
	}

	{
		if (pRobot->GetRoleInfo()->GetRoleInfo().dwGlobeNum < Iter->second.AddGlobelSum)
		{
			DWORD AddGlobel = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.GlobelRateF * 2)) - Iter->second.GlobelRateF) / 100.0) * Iter->second.AddGlobelSum);
			pRobot->GetRoleInfo()->ChangeRoleGlobe(AddGlobel);
		}
	}

	{
		if (pRobot->GetRoleInfo()->GetRoleInfo().dwCurrencyNum < Iter->second.AddCurrceySum)
		{
			DWORD AddCurrcey = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.CurrceyRateF * 2)) - Iter->second.CurrceyRateF) / 100.0) * Iter->second.AddCurrceySum);
			pRobot->GetRoleInfo()->ChangeRoleCurrency(AddCurrcey, TEXT("�����˲�����ʯ"));
		}
	}


	m_FreeRobot[Index] = m_FreeRobot[m_FreeRobot.size() - 1];
	m_FreeRobot[m_FreeRobot.size() - 1] = pRobot;
	m_FreeRobot.pop_back();//�Ƴ�
	pRobot->SetRobotUse(RobotID);//���û������Ѿ�ʹ����
	m_RobotMap.insert(HashMap<DWORD, GameRobot*>::value_type(pRobot->GetRobotUserID(), pRobot));
	//g_FishServer.ShowInfoToWin("�����»����� ʣ�����������:%d", m_FreeRobot.size());
	return pRobot;
}
void GameRobotManager::ResetGameRobot(DWORD dwUserID)
{
	HashMap<DWORD, GameRobot*>::iterator Iter = m_RobotMap.find(dwUserID);
	if (Iter == m_RobotMap.end())
	{
		ASSERT(false);
		return;
	}
	GameRobot* pRobot = Iter->second;
	m_RobotMap.erase(Iter);
	pRobot->ResetRobot();
	m_FreeRobot.push_back(pRobot);
	//g_FishServer.ShowInfoToWin("�黹������ ʣ�����������:%d", m_FreeRobot.size());
	//g_FishServer.ShowInfoToWin("�黹������");

	return;
}
void GameRobotManager::ResetGameRobot(GameRobot* pRobot)
{
	if (!pRobot)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, GameRobot*>::iterator Iter = m_RobotMap.find(pRobot->GetRobotUserID());
	if (Iter == m_RobotMap.end())
	{
		ASSERT(false);
		return;
	}
	m_RobotMap.erase(Iter);
	pRobot->ResetRobot();
	m_FreeRobot.push_back(pRobot);
	//g_FishServer.ShowInfoToWin("�黹������ ʣ�����������:%d", m_FreeRobot.size());
	//g_FishServer.ShowInfoToWin("�黹������");
	return;
}
bool GameRobotManager::GameRobotIsCanJoinTable(GameTable* pTable)
{
	//�жϻ������Ƿ���Լ��뵱ǰ����
	if (!pTable)
	{
		ASSERT(false);
		return false;
	}
	if (pTable->GetTableMonthID() == 0)
	{
		//�Ǳ�������
		//����ͨ��ҽ���һ���Ѿ����ڵķ����ʱ�� ���������� С�ڵ���2�� ���� ȫ��Ϊ��ͨ��ҵ�ʱ�� ���һ�������˽���
		/*if (pTable->GetRoleManager().GetRoleSum() > 2)
			return false;
		for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
		{
			CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
			if (!pRole || !pRole->IsActionUser())
				continue;
			if (pRole->GetRoleExInfo()->IsRobot())
			{
				return false;
			}
		}*/

		//�޸Ĺ��� ���һ��λ�� �����˲����Խ���
		if (pTable->GetRoleManager().GetRoleSum() >= 3)
			return false;

		return true;
	}
	else
	{
		//�������� һ���������1��������
		BYTE RobotSum = 0;
		for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
		{
			CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
			if (!pRole || !pRole->IsActionUser())
				continue;
			if (pRole->GetRoleExInfo()->IsRobot())
				++RobotSum;
		}
		if (RobotSum >= 1)
			return false;
		else
			return true;
	}
}
void GameRobotManager::OnRoleCreateNormalRoom(GameTable* pTable)
{
	if (!pTable || pTable->GetTableMonthID() != 0)
		return;
	
	//1��Ϊ0  ��������ǰ��ȥ
	if (pTable->GetRoleManager().GetRoleSum() != 0)
		return;

	//�л����˾Ͳ���ȥ
	for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
	{
		CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
		if (!pRole || !pRole->IsActionUser())
			continue;
		if (pRole->GetRoleExInfo()->IsRobot())
			return;
	}
	//����һ�������� ���뵽��������ȥ
	/*DWORD AddSec = RandUInt() % 10000 + 5000;

	AdddWriteRobot(pTable->GetTableID(), timeGetTime() + AddSec);*/

	//����ID �� ����ID �������ɻ����˵�����ID
	DWORD Key = (pTable->GetTableTypeID() << 16) + pTable->GetTableMonthID();
	multimap<DWORD, DWORD>::iterator Iter= g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.find(Key);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.end())
		return;
	
	DWORD RobotNum = RandRange(0,2);//(RandUInt() % 99999) % 3;
	if (RobotNum == 0) RandRange(0, 2);

	for (DWORD i = 0; i < RobotNum; ++i)
	{
		GameRobot* pRobot = GetFreeRobot(Iter->second);//��ȡһ�����еĻ�����
		if (!pRobot)
			return;
		CRoleEx* pRole = pRobot->GetRoleInfo();
		if (!pRole)
			return;
		if (!g_FishServer.GetTableManager()->OnPlayerJoinTable(pTable->GetTableID(), pRole))
			AddResetRobot(pRole->GetUserID());
	}
}
void GameRobotManager::OnRoleLeaveNormalRoom(GameTable* pTable)
{
	//����ͨ����뿪һ�������ʱ�� �ж����������û����ͨ����� ȫ���Ļ������뿪
	if (!pTable || pTable->GetTableMonthID() != 0)
		return;
	BYTE RoleSum = 0;
	for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
	{
		CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
		if (!pRole || !pRole->IsActionUser())
			continue;
		if (!pRole->GetRoleExInfo()->IsRobot())
			++RoleSum;
	}
	if (RoleSum == 0)
	{
		for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
		{
			CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
			if (!pRole || !pRole->IsActionUser())
				continue;
			if (pRole->GetRoleExInfo()->IsRobot())
			{
				//�黹������
				DWORD dwUserID = pRole->GetID();
				g_FishServer.GetTableManager()->OnPlayerLeaveTable(dwUserID);
				ResetGameRobot(dwUserID);//�黹������
			}
		}
	}
}
void GameRobotManager::OnRoleJoinNormalRoom(GameTable* pTable)
{
	//����ͨ��ҽ���һ���Ѿ����ڵķ����ʱ�� ���������� С�ڵ���2�� ���� ȫ��Ϊ��ͨ��ҵ�ʱ�� ���һ�������˽���
	if (!pTable || pTable->GetTableMonthID() != 0)
		return;
	if (pTable->GetRoleManager().GetRoleSum() >= 2)
		return;
	BYTE RobotSum = 0;
	for (BYTE i = 0; i < pTable->GetRoleManager().GetMaxPlayerSum(); ++i)
	{
		CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
		if (!pRole || !pRole->IsActionUser())
			continue;
		if (pRole->GetRoleExInfo()->IsRobot())
			++RobotSum;
	}
	if (RobotSum > 0)
		return;

	DWORD Key = (pTable->GetTableTypeID() << 16) + pTable->GetTableMonthID();
	multimap<DWORD, DWORD>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.find(Key);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.end())
		return;
	DWORD RobotID = Iter->second;

	DWORD RobotNum = RandRange(0, 2);//(RandUInt() % 99999) % 3;
	if (RobotNum == 0) RandRange(0, 2);

	for (DWORD i = 0; i < RobotNum; ++i)
	{
		GameRobot* pRobot = GetFreeRobot(RobotID);//��ȡһ�����еĻ�����
		if (!pRobot)
			return;
		CRoleEx* pRole = pRobot->GetRoleInfo();
		if (!pRole)
			return;
		if (!g_FishServer.GetTableManager()->OnPlayerJoinTable(pTable->GetTableID(), pRole))
			AddResetRobot(pRole->GetUserID());
	}
}
void GameRobotManager::OnMonthBeginSign(BYTE MonthID)
{
	//g_FishServer.ShowInfoToWin("%d ���� ��ʼ����", MonthID);
	//��������ʼ������ʱ��
	//���ǻ�ȡָ�������Ļ����� ȥ���� 
	HashMap<BYTE, tagMonthConfig>::iterator Iter = g_FishServer.GetFishConfig().GetMonthConfig().m_MonthMap.find(MonthID);
	if (Iter == g_FishServer.GetFishConfig().GetMonthConfig().m_MonthMap.end())
		return;
	BYTE TableID = Iter->second.TableTypeID;
	DWORD Key = (TableID << 16) + MonthID;

	multimap<DWORD, DWORD>::_Pairii pair = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.equal_range(Key);
	multimap<DWORD, DWORD>::iterator IterFind = pair.first;
	for (; IterFind != pair.second; ++IterFind)
	{
		DWORD RobotID = IterFind->second;

		HashMap<DWORD, tagGameRobotConfig>::iterator IterRobot = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(RobotID);
		if (IterRobot == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;
		vector<GameRobot*> pVec;
		for (DWORD i = 0; i < IterRobot->second.MonthRobotSum; ++i)
		{
			GameRobot* pRobot = GetFreeRobot(RobotID);
			if (!pRobot)
			{
				continue;
			}
			//��������
			GC_Cmd_SignUpMonth msg;
			SetMsgInfo(msg, GetMsgType(Main_Month, GC_SignUpMonth), sizeof(GC_Cmd_SignUpMonth));
			msg.dwUserID = pRobot->GetRobotUserID();
			msg.MonthID = MonthID;
			pRobot->GetRoleInfo()->SendDataToCenter(&msg);
			//�������˴洢����������ȥ
			pVec.push_back(pRobot);
		}
		//������ �� MonthID ������
		m_MonthSignMap.insert(HashMap<BYTE, vector<GameRobot*>>::value_type(MonthID, pVec));
	}
}
void GameRobotManager::OnMonthEndSign(BYTE MonthID)
{
	//����������
	//g_FishServer.ShowInfoToWin("%d ���� ��������", MonthID);
}
void GameRobotManager::OnMonthBeginStar(BYTE MonthID)
{
	//g_FishServer.ShowInfoToWin("%d ���� ��ʼ����", MonthID);

	//��ָ���Ļ�����ֱ�ӽ������ �����б�������
	//��ʼ���� ��������
	//��ȡָ�������Ļ�����ȥ�μӱ���
 	HashMap<BYTE, vector<GameRobot*>>::iterator Iter= m_MonthSignMap.find(MonthID);
	if (Iter == m_MonthSignMap.end())
		return;
	HashMap<BYTE, tagMonthConfig>::iterator IterMonth = g_FishServer.GetFishConfig().GetMonthConfig().m_MonthMap.find(MonthID);
	if (IterMonth == g_FishServer.GetFishConfig().GetMonthConfig().m_MonthMap.end())
		return;
	vector<GameRobot*>::iterator IterVec = Iter->second.begin();
	for (; IterVec != Iter->second.end(); ++IterVec)
	{
		CRoleEx* pRole = (*IterVec)->GetRoleInfo();
		if (!pRole)
			continue;

		GC_Cmd_JoinMonth msg;
		SetMsgInfo(msg, GetMsgType(Main_Month, GC_JoinMonth), sizeof(GC_Cmd_JoinMonth));
		msg.dwUserID = pRole->GetUserID();
		msg.TableID = 0xffff;//����ָ������
		msg.MonthID = MonthID;
		pRole->SendDataToCenter(&msg);
		//g_FishServer.GetTableManager()->OnPlayerJoinTable(IterMonth->second.TableTypeID, pRole, MonthID, true);
	}
}
void GameRobotManager::OnMonthEndStar(BYTE MonthID)
{	
	//���ڱ�������Ļ����� ȫ�����ջ���
	//g_FishServer.ShowInfoToWin("%d ���� ��������", MonthID);
	HashMap<BYTE, vector<GameRobot*>>::iterator Iter = m_MonthSignMap.find(MonthID);
	if (Iter == m_MonthSignMap.end())
		return;
	
	vector<GameRobot*>::iterator IterVec = Iter->second.begin();
	for (; IterVec != Iter->second.end(); ++IterVec)
	{
		g_FishServer.GetTableManager()->OnPlayerLeaveTable((*IterVec)->GetRobotUserID());//����뿪��������
		ResetGameRobot(*IterVec);//�黹������
	}
	m_MonthSignMap.erase(Iter);//�Ƴ�������
}
void GameRobotManager::Update()
{
	if (m_RobotClose)
	    return;
	UpdateResetVec();
	UpdateWriteList();
	HashMap<DWORD, GameRobot*>::iterator Iter = m_RobotMap.begin();
	for (; Iter != m_RobotMap.end(); ++Iter)
	{
		Iter->second->Update();
	}
}
void GameRobotManager::AdddWriteRobot(WORD TableID, DWORD WriteTime)
{
	tagRobotWrite pInfo;
	pInfo.TableID = TableID;
	pInfo.TimeLog = WriteTime;
	m_WriteList.push_back(pInfo);
}
void GameRobotManager::UpdateWriteList()
{
	if (m_WriteList.empty())
		return;
	DWORD Time = timeGetTime();
	list<tagRobotWrite>::iterator Iter = m_WriteList.begin();
	for (; Iter != m_WriteList.end();)
	{
		if (Time > Iter->TimeLog)
		{
			GameTable* pTable = g_FishServer.GetTableManager()->GetTable(Iter->TableID);
			if (!pTable)
				continue;
			BYTE TableTypeID = pTable->GetTableTypeID();
			BYTE MonthID = pTable->GetTableMonthID();
			
			DWORD Key = (TableTypeID << 16) + MonthID;
			multimap<DWORD, DWORD>::iterator IterFind = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.find(Key);
			if (IterFind == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotIndexMap.end())
				return;
			DWORD RobotID = IterFind->second;

			GameRobot* pRobot = GetFreeRobot(RobotID);
			if (!pRobot)
			{
				++Iter;
				continue;
			}
			if(!g_FishServer.GetTableManager()->OnPlayerJoinTable(Iter->TableID, pRobot->GetRoleInfo()))
				AddResetRobot(pRobot->GetRobotUserID());

			Iter = m_WriteList.erase(Iter);
		}
		else
		{
			return;
		}
	}
}
void GameRobotManager::AddResetRobot(DWORD dwUserID)
{
	m_ResetUserVec.push_back(dwUserID);
}
void GameRobotManager::UpdateResetVec()
{
	if (m_ResetUserVec.empty())
		return;
	vector<DWORD>::iterator Iter = m_ResetUserVec.begin();
	for (; Iter != m_ResetUserVec.end(); ++Iter)
	{
		ResetGameRobot(*Iter);
	}
	m_ResetUserVec.clear();
}
void GameRobotManager::OnHandleMonthStates(BYTE MonthID, BYTE MonthStates)
{
	if (!m_IsLoadFinish)
	{
		HashMap<BYTE, BYTE>::iterator Iter= m_MonthStatesMap.find(MonthID);
		if (Iter == m_MonthStatesMap.end())
		{
			m_MonthStatesMap.insert(HashMap<BYTE, BYTE>::value_type(MonthID,MonthStates));
		}
		else
		{
			Iter->second = MonthStates;
		}
		return;
	}
	else
	{
		if (MonthStates == 1)
		{
			OnMonthBeginSign(MonthID);
		}
		else if (MonthStates == 2)
		{
			OnMonthEndSign(MonthID);
		}
		else if (MonthStates == 4)
		{
			OnMonthBeginStar(MonthID);
		}
		else if (MonthStates == 8)
		{
			OnMonthEndStar(MonthID);
		}
		else
		{
			//���ϲ���
			//�Ѿ���������� ֱ�Ӵ���
			bool IsInSign = false;
			if ((MonthStates & 1) != 0)
				IsInSign = true;
			else  if ((MonthStates & 2) != 0)
				IsInSign = false;

			bool IsInStar = false;
			if ((MonthStates & 4) != 0)
				IsInStar = true;
			else  if ((MonthStates & 8) != 0)
				IsInStar = false;


			//��ʼ���� ��ʼ����
			//�������� ��������
			//�������� ��ʼ����
			//�������� ��ʼ���� 
			if (IsInStar == false && IsInSign == true)
			{
				if (IsInStar)
					OnMonthBeginStar(MonthID);
				else
					OnMonthEndStar(MonthID);

				if (IsInSign)
					OnMonthBeginSign(MonthID);
				else
					OnMonthEndSign(MonthID);
			}
			else
			{
				if (IsInSign)
					OnMonthBeginSign(MonthID);
				else
					OnMonthEndSign(MonthID);

				if (IsInStar)
					OnMonthBeginStar(MonthID);
				else
					OnMonthEndStar(MonthID);
			}
		}
	}
}
void GameRobotManager::OnHandleAllMonthStates()
{
	if (m_MonthStatesMap.empty())
		return;
	HashMap<BYTE, BYTE>::iterator Iter = m_MonthStatesMap.begin();
	for (; Iter != m_MonthStatesMap.end(); ++Iter)
	{
		OnHandleMonthStates(Iter->first, Iter->second);
	}
	m_MonthStatesMap.clear();
}
void GameRobotManager::OnChangeCharmValue(DWORD SrcUserID, DWORD DestUserID, BYTE CharmIndexID, DWORD CharmItemSum, int ChangeCharmValue)
{
	HashMap<DWORD, GameRobot*>::iterator Iter = m_RobotMap.find(DestUserID);
	if (Iter == m_RobotMap.end())
		return;
	Iter->second->OnChangeCharmValue(SrcUserID, CharmIndexID, CharmItemSum, ChangeCharmValue);
}
//���������˴���
GameRobot::GameRobot(GameRobotManager* pManager, CRoleEx* pRole)
{
	if (!pRole || !pManager)
	{
		ASSERT(false);
		return;
	}
	m_pManager = pManager;
	m_pRole = pRole;
	m_RobotID = 0;
	m_IsUse = false;
	/*m_TableID = 0;
	m_TableTypeID = 0;
	m_MonthID = 0;
	m_IsInTable = false;*/
	//
	m_LeaveTableTimeLog = 0;
	m_RoomTimeLog = 0;
	//
	m_NowRate = 0xff;
	m_ChangeRateTimeLog = 0;
	m_RateTimeLog = 0;
	//
	m_UseSkillTimeLog = 0;
	m_SkillTimeLog = 0;
	//
	m_ChangeLauncherTimeLog = 0;
	m_LauncherTimeLog = 0;
	//
	m_OpenFireTimeLog = 0;
	//
	m_StopOpenFireTimeLog = 0;
	//
	m_RobotInfoTimeLog = 0;
	//
	m_IsOpenFire = false;
	m_RobotOpenFireTimeLog = 0;
	//
	m_RobotSendCharmItemTimeLog = 0;
	m_dwRobotSendCharmTimeLog = 0;
	m_vecSendCharmArray.clear();
	//
	m_dwSendCharmTineLog = 0;
	//
	m_RobotCharmEvent.Clear();
}
GameRobot::~GameRobot()
{

}
void GameRobot::SetRobotUse(DWORD RobotID)
{
	if (m_IsUse)
	{
		ASSERT(false);
		return;
	}
	m_RobotID = RobotID;
	m_IsUse = true;
	/*m_TableTypeID = static_cast<BYTE>(RobotID >> 16);
	m_MonthID = (BYTE)RobotID;
	m_TableID = 0;*/
}
void GameRobot::ResetRobot()
{
	if (!m_IsUse)
	{
		ASSERT(false);
		return;
	}
	m_RobotID = 0;
	m_IsUse = false;
	//
	m_LeaveTableTimeLog = 0;
	m_RoomTimeLog = 0;
	//
	m_NowRate = 0xff;
	m_ChangeRateTimeLog = 0;
	m_RateTimeLog = 0;
	//
	m_UseSkillTimeLog = 0;
	m_SkillTimeLog = 0;
	//
	m_ChangeLauncherTimeLog = 0;
	m_LauncherTimeLog = 0;
	//
	m_OpenFireTimeLog = 0;
	//
	m_StopOpenFireTimeLog = 0;
	//
	m_RobotInfoTimeLog = 0;
	//
	m_IsOpenFire = false;
	m_RobotOpenFireTimeLog = 0;

	//
	m_RobotSendCharmItemTimeLog = 0;
	m_dwRobotSendCharmTimeLog = 0;
	m_vecSendCharmArray.clear();
	//
	m_dwSendCharmTineLog = 0;
	//
	m_RobotCharmEvent.Clear();
}
DWORD GameRobot::GetRobotUserID()
{
	if (!m_pRole)
		return 0;
	return m_pRole->GetUserID();
}
CRoleEx* GameRobot::GetRoleInfo()
{
	return m_pRole;
}
void GameRobot::Update()
{
	if (m_pRole == nullptr)
	{
		return;
	}

	UpdateRobotInfo();
	UpdateRobotRoom();
	UpdateRobotRate();
	UpdateRobotSkill();
	UpdateRobotLauncher();
	UpdateRobotOpenFire();
	UpdateRobotIsCanOpenFire();
	UpdateRobotCharm();
	UpdateRobotSendCharm();
}
void GameRobot::UpdateRobotRoom()
{
	if (m_RoomTimeLog != 0 && timeGetTime() - m_RoomTimeLog < 500)
		return;
	m_RoomTimeLog = timeGetTime();
	//���»����˽�������
	if (!m_pRole)
	{
		//ASSERT(false);
		return;
	}
	if (m_pRole->GetRoleMonth().IsInMonthTable())//���������������
	{
		//�����Ҵ����Ʋ�״̬ ���� �޷�ʹ�ü����뿪
		HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
		if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;
		if (Iter->second.SkillInfo.IsOpen)
			return;
		if (m_pRole->GetRoleMonth().GetMonthInfo().dwMonthGlobel == 0 && !m_pRole->GetRoleMonth().IsCanAddMonthGlobel())
		{
			g_FishServer.GetTableManager()->OnPlayerLeaveTable(GetRobotUserID());//�û������뿪����
			m_pManager->AddResetRobot(GetRobotUserID());
			return;
		}
		return;
	}
	//m_IsInTable = false;
	CRole* pRole= g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
	if (!pRole)//��Ҳ���������
		return;
	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	if (!Iter->second.RoomInfo.IsOpen)//Ϊ�����Ļ� ��ʾ���һֱ����������
		return;
	if (m_LeaveTableTimeLog == 0)
	{
		DWORD JoinSec = RandUInt() % (Iter->second.RoomInfo.RoomInfo.OpenMaxSec - Iter->second.RoomInfo.RoomInfo.OpenMinSec) + Iter->second.RoomInfo.RoomInfo.OpenMinSec;//�ڷ����������
		m_LeaveTableTimeLog = timeGetTime() + JoinSec * 1000;
		//m_TableID = pRole->GetTableID();
		//m_IsInTable = true;
	}
	if (timeGetTime() > m_LeaveTableTimeLog)
	{
		//��������Ҫ�뿪������
		g_FishServer.GetTableManager()->OnPlayerLeaveTable(GetRobotUserID());//�û������뿪����
		//���û���
		DWORD LeaveSec = RandUInt() % (Iter->second.RoomInfo.RoomInfo.StopMaxSec - Iter->second.RoomInfo.RoomInfo.StopMinSec) + Iter->second.RoomInfo.RoomInfo.StopMinSec;
		m_pManager->AdddWriteRobot(pRole->GetTableID(), timeGetTime() + LeaveSec * 1000);
		//���ջ�����
		//m_pManager->ResetGameRobot(this);//������ֱ���Ƴ� ��Ҫ��Update������Ƴ� ����
		m_pManager->AddResetRobot(GetRobotUserID());
		//
		//m_IsInTable = false;
	}
}

void GameRobot::UpdateRobotRate()
{
	if (m_RateTimeLog != 0 && timeGetTime() - m_RateTimeLog < 300)
		return;
	m_RateTimeLog = timeGetTime();
	//���»������л�����
	CRole* pUser = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
	if (!pUser /*|| m_pRole->GetRoleMonth().IsInMonthTable()*/)
		return;
	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pUser->GetTableID());
	if (!pTable)
		return;
	if (m_NowRate == 0xff)
	{
		if (m_ChangeRateTimeLog == 0)
		{
			HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
			if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
				return;

			DWORD RateF = RandUInt() % Iter->second.RateInfo.MaxRateF;
			vector<tagRobotRateType>::iterator IterVec = Iter->second.RateInfo.RateVec.begin();
			for (; IterVec != Iter->second.RateInfo.RateVec.end(); ++IterVec)
			{
				if (RateF < IterVec->RateF)
				{
					//�л�����ǰ����
					//������һ���л���ָ������ ���� һ��һ������ �м���
					m_NowRate = IterVec->RateType;
					break;
				}
			}
			HashMap<BYTE, TableInfo>::iterator IterTable = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(pTable->GetTableTypeID());
			if (IterTable == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
				return;
			if (m_NowRate < IterTable->second.MinRate)
				m_NowRate = IterTable->second.MinRate;
			if (m_NowRate > IterTable->second.MaxRate)
				m_NowRate = IterTable->second.MaxRate;

			if (m_NowRate > pUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate())
			{
				m_pRole->ChangeRoleMaxRate(m_NowRate);
			}
			//LogInfoToFile("WmRate.txt", "update 1 userid=%d, maxrate=%d  m_NowRate=%d", m_pRole->GetUserID(), m_pRole->GetRoleRate().GetCanUseMaxRate(), m_NowRate);

			NetCmdChangeRateType msg;
			SetMsgInfo(msg, CMD_CHANGE_RATE_TYPE, sizeof(msg));
			msg.Seat = pUser->GetSeatID();
			msg.RateIndex = m_NowRate;
			g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msg);
			m_NowRate = 0xff;
			DWORD JoinSec = RandUInt() % (Iter->second.RateInfo.RateInfo.OpenMaxSec - Iter->second.RateInfo.RateInfo.OpenMinSec) + Iter->second.RateInfo.RateInfo.OpenMinSec;//�л����ʵ�CD
			m_ChangeRateTimeLog = timeGetTime() + JoinSec*1000;
		}
		else if (timeGetTime() >= m_ChangeRateTimeLog)
		{
			HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
			if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
				return;

			if (!Iter->second.RateInfo.IsOpen)
				return;

			DWORD RateF = RandUInt() % Iter->second.RateInfo.MaxRateF;
			vector<tagRobotRateType>::iterator IterVec = Iter->second.RateInfo.RateVec.begin();
			for (; IterVec != Iter->second.RateInfo.RateVec.end(); ++IterVec)
			{
				if (RateF < IterVec->RateF)
				{
					//�л�����ǰ����
					//������һ���л���ָ������ ���� һ��һ������ �м���
					m_NowRate = IterVec->RateType;
					break;
				}
			}

			//�޸ĵ�ǰ����
			HashMap<BYTE, TableInfo>::iterator IterTable = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(pTable->GetTableTypeID());
			if (IterTable == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
				return;
			if (m_NowRate < IterTable->second.MinRate)
				m_NowRate = IterTable->second.MinRate;
			if (m_NowRate > IterTable->second.MaxRate)
				m_NowRate = IterTable->second.MaxRate;

			if (m_NowRate > pUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate())
			{
				m_pRole->ChangeRoleMaxRate(m_NowRate);
			}
			//LogInfoToFile("WmRate.txt", "update 2 userid=%d, maxrate=%d  m_NowRate=%d", m_pRole->GetUserID(), m_pRole->GetRoleRate().GetCanUseMaxRate(), m_NowRate);
			NetCmdChangeRateType msg;
			SetMsgInfo(msg, CMD_CHANGE_RATE_TYPE, sizeof(msg));
			msg.Seat = pUser->GetSeatID();
			msg.RateIndex = m_NowRate;
			g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msg);
			m_NowRate = 0xff;

			DWORD JoinSec = RandUInt() % (Iter->second.RateInfo.RateInfo.OpenMaxSec - Iter->second.RateInfo.RateInfo.OpenMinSec) + Iter->second.RateInfo.RateInfo.OpenMinSec;//�л����ʵ�CD
			m_ChangeRateTimeLog = timeGetTime() + JoinSec * 1000;
		}
	}
	else
	{
		if (pUser->GetRateIndex() == m_NowRate)
		{
			m_NowRate = 0xff;
			return;
		}
		//����ҵ�ǰ���� �� ָ�����ʽ����л�
		BYTE RateIndex = (m_NowRate < pUser->GetRateIndex() ? pUser->GetRateIndex() - 1 : pUser->GetRateIndex() + 1);
		NetCmdChangeRateType msg;
		SetMsgInfo(msg, CMD_CHANGE_RATE_TYPE, sizeof(msg));
		msg.Seat = pUser->GetSeatID();
		msg.RateIndex = RateIndex;
		g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msg);

		if (m_NowRate > pUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate())
		{
			m_pRole->ChangeRoleMaxRate(m_NowRate);
		}
		//LogInfoToFile("WmRate.txt", "update 1 userid=%d, maxrate=%d  RateIndex=%d", m_pRole->GetUserID(), m_pRole->GetRoleRate().GetCanUseMaxRate(), RateIndex);
		return;
	}
}
void GameRobot::UpdateRobotSkill()
{
	if (m_SkillTimeLog != 0 && timeGetTime() - m_SkillTimeLog < 1000)
		return;
	m_SkillTimeLog = timeGetTime();
	//������ʹ�ü��� ��ͨ��
	CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
	if (!pRole)
		return;
	if (m_UseSkillTimeLog == 0)
	{
		HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
		if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;
		DWORD JoinSec = RandUInt() % (Iter->second.SkillInfo.SkillInfo.OpenMaxSec - Iter->second.SkillInfo.SkillInfo.OpenMinSec) + Iter->second.SkillInfo.SkillInfo.OpenMinSec;//�л������ܹ���CD
		m_UseSkillTimeLog = timeGetTime() + JoinSec * 1000;//�´�ʹ�ü��ܵ�CD
	}
	else if (timeGetTime() >= m_UseSkillTimeLog)
	{
		GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pRole->GetTableID());
		if (!pTable)
			return;
		if (!m_pRole->GetRoleMonth().IsInMonthTable() && pTable->GetFishDesk()->GetFishSum() < 80)
			return;
		if (m_pRole->GetRoleMonth().IsInMonthTable() && pTable->GetFishDesk()->GetFishSum() < 20)
			return;

		HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
		if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;

		if (!Iter->second.SkillInfo.IsOpen)
			return;

		//ʹ�ü���
		BYTE SkillID = g_FishServer.GetTableManager()->GetGameConfig()->CannonOwnSkill(pRole->GetLauncherType());

		NetCmdUseSkill msg;
		SetMsgInfo(msg, CMD_USE_SKILL, sizeof(msg));
		msg.SkillID = SkillID;
		g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msg);

		

		DWORD JoinSec = RandUInt() % (Iter->second.SkillInfo.SkillInfo.OpenMaxSec - Iter->second.SkillInfo.SkillInfo.OpenMinSec) + Iter->second.SkillInfo.SkillInfo.OpenMinSec;//�л������ܹ���CD
		m_UseSkillTimeLog = timeGetTime() + JoinSec * 1000;//�´�ʹ�ü��ܵ�CD

		m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog,timeGetTime() + 3000);
	}
}
void GameRobot::UpdateRobotLauncher()
{
	if (m_LauncherTimeLog != 0 && timeGetTime() - m_LauncherTimeLog < 1000)
		return;
	m_LauncherTimeLog = timeGetTime();
	CRole* pUser = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
	if (!pUser)
		return;

	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pUser->GetTableID());
	if (!pTable)
		return;
	if (pTable->GetTableMonthID() != 0 )//�������䲻����
	{
		return;
	}

	//�л��ڵ�CD
	if (m_ChangeLauncherTimeLog == 0)
	{
		HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
		if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;
		DWORD LauncherCDSec = RandUInt() % (Iter->second.LauncherInfo.LauncherInfo.OpenMaxSec - Iter->second.LauncherInfo.LauncherInfo.OpenMinSec) + Iter->second.LauncherInfo.LauncherInfo.OpenMinSec;
		m_ChangeLauncherTimeLog = timeGetTime() + LauncherCDSec * 1000;
		//��ҽ�����������һ�ֿ���ʹ�õ���
		DWORD RateF = RandUInt() % Iter->second.LauncherInfo.MaxRateF;
		vector<tagRobotLauncherType>::iterator IterVec = Iter->second.LauncherInfo.LauncherVec.begin();
		for (; IterVec != Iter->second.LauncherInfo.LauncherVec.end(); ++IterVec)
		{
			if (RateF < IterVec->RateF)
			{
				BYTE LauncherType = IterVec->LauncherType;
				while (LauncherType >0 && !m_pRole->GetRoleLauncherManager().IsCanUserLauncherByID(LauncherType))
				{
					--LauncherType;
				}

				//�л�����ǰ��̨
				NetCmdChangeLauncherType change;//����ǰ2s �����Է����ӵ�
				SetMsgInfo(change, CMD_CHANGE_LAUNCHER_TYPE, sizeof(change));
				change.Seat = pUser->GetSeatID();
				change.LauncherType = LauncherType;
				g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &change);

				return;
			}
		}
	}
	else if (timeGetTime() >= m_ChangeLauncherTimeLog)
	{
		HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
		if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
			return;
		if (!Iter->second.LauncherInfo.IsOpen)
			return;
		//�����л���̨��
		DWORD LauncherCDSec = RandUInt() % (Iter->second.LauncherInfo.LauncherInfo.OpenMaxSec - Iter->second.LauncherInfo.LauncherInfo.OpenMinSec) + Iter->second.LauncherInfo.LauncherInfo.OpenMinSec;
		m_ChangeLauncherTimeLog = timeGetTime() + LauncherCDSec * 1000;

		DWORD RateF = RandUInt() % Iter->second.LauncherInfo.MaxRateF;
		vector<tagRobotLauncherType>::iterator IterVec = Iter->second.LauncherInfo.LauncherVec.begin();
		for (; IterVec != Iter->second.LauncherInfo.LauncherVec.end(); ++IterVec)
		{
			if (RateF < IterVec->RateF)
			{
				BYTE LauncherType = IterVec->LauncherType;
				while (LauncherType >0 && !m_pRole->GetRoleLauncherManager().IsCanUserLauncherByID(LauncherType))
				{
					--LauncherType;
				}

				//�л�����ǰ��̨
				NetCmdChangeLauncherType change;//����ǰ2s �����Է����ӵ�
				SetMsgInfo(change, CMD_CHANGE_LAUNCHER_TYPE, sizeof(change));
				change.Seat = pUser->GetSeatID();
				change.LauncherType = LauncherType;
				g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &change);

				m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 1000);
				return;
			}
		}
	}
}
void GameRobot::UpdateRobotOpenFire()
{
	if (m_OpenFireTimeLog == 0 || timeGetTime() - m_OpenFireTimeLog > 300)
	{
		m_OpenFireTimeLog = timeGetTime();

		CRole* pUser = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
		if (!pUser)
			return;
		
		//�ж�����Ƿ���Կ���
		if (m_NowRate != 0xff)
			return;
		if (!m_IsOpenFire)
			return;
		if (m_OpenFireTimeLog < m_StopOpenFireTimeLog)
			return;
		GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pUser->GetTableID());
		if (!pTable)
			return; 
		m_Angle = pTable->GetFishDesk()->GetAngleByFish(m_LockFishID, pUser->GetSeatID());
		if (m_LockFishID == 0 && m_Angle == 0xffff)
			return;
		//��ҿ���
		if (!pUser->IsFullEnergy())//�����ӵ�
		{
			BYTE tabletype;
			if (pUser->CheckFire(tabletype, pUser->GetLauncherType()))
			{
				//1.�����ӵ�
				NetCmdBullet msg;
				SetMsgInfo(msg, CMD_BULLET, sizeof(NetCmdBullet));
				msg.BulletID = 0;
				msg.Angle = m_Angle % 65535;
				msg.LockFishID = 0;
				msg.LauncherType = pUser->GetLauncherType();
				msg.Energy = (uint)pUser->GetEnergy();
				msg.ReboundCount = 0; //pUser->GetRoleExInfo()->GetRoleVip().GetLauncherReBoundNum();
				//if (msg.LauncherType == 4)
				//	msg.ReboundCount = 0;
				//���������õ�GameServerȥ
				g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msg);
			}
			else
			{
				//�������ڱ���ģʽ
				if (m_NowRate == 0xff && m_pRole->GetRoleMonth().IsInMonthTable() && m_pRole->GetRoleMonth().GetMonthInfo().dwMonthGlobel > 0 && !m_pRole->GetRoleMonth().IsCanAddMonthGlobel())
				{
					//������ҵı��� �� 1����  ��0��ʼ
					m_NowRate = 0;
					HashMap<BYTE, TableInfo>::iterator IterTable = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(pTable->GetTableTypeID());
					if (IterTable == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
						return;
					if (m_NowRate < IterTable->second.MinRate)
						m_NowRate = IterTable->second.MinRate;
					if (m_NowRate > IterTable->second.MaxRate)
						m_NowRate = IterTable->second.MaxRate;

					HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
					if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
						return;
					DWORD JoinSec = RandUInt() % (Iter->second.RateInfo.RateInfo.OpenMaxSec - Iter->second.RateInfo.RateInfo.OpenMinSec) + Iter->second.RateInfo.RateInfo.OpenMinSec;//�л����ʵ�CD
					m_ChangeRateTimeLog = timeGetTime() + JoinSec * 1000;
				}
			}
		}
		else//�������
		{
			NetCmdSkillLaser msgSkill;
			SetMsgInfo(msgSkill, CMD_SKILL_LASER_REQUEST, sizeof(msgSkill));
			msgSkill.Degree = m_Angle % 65535;
			g_FishServer.GetTableManager()->OnHandleTableMsg(m_pRole->GetUserID(), &msgSkill);

			m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 2000);
		}
	}
}
void GameRobot::UpdateRobotInfo()
{
	if (m_RobotInfoTimeLog != 0 && timeGetTime() - m_RobotInfoTimeLog < 1000)
		return;
	m_RobotInfoTimeLog = timeGetTime();
	if (m_pRole == nullptr)
	{
		return;
	}
	CRole* pUser = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
	if (!pUser)
		return;

	//������ҵĽ�� ��ʯ  ���� �������� �Ȳ���
	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	
	{
		if (m_pRole->GetRoleInfo().dwGlobeNum < Iter->second.AddGlobelSum)
		{
			DWORD AddGlobel = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.GlobelRateF * 2)) - Iter->second.GlobelRateF) / 100.0) * Iter->second.AddGlobelSum);
			m_pRole->ChangeRoleGlobe(AddGlobel);
			m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 2000);
		}
	}

	{
		if (m_pRole->GetRoleInfo().dwCurrencyNum < Iter->second.AddCurrceySum)
		{
			DWORD AddCurrcey = static_cast<DWORD>(((100 + (RandUInt() % (Iter->second.CurrceyRateF * 2)) - Iter->second.CurrceyRateF) / 100.0) * Iter->second.AddCurrceySum);
			m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 2000);
			m_pRole->ChangeRoleCurrency(AddCurrcey, TEXT("�����˲�����ʯ"));
		}
	}

	if (m_pRole->GetRoleInfo().wLevel > Iter->second.MaxLevel)
	{
		m_pRole->ChangeRoleLevel(static_cast<short>(Iter->second.MaxLevel - m_pRole->GetRoleInfo().wLevel));
		m_pRole->ChangeRoleExp(-1 * m_pRole->GetRoleInfo().dwExp,false);
	}
	if (m_pRole->GetRoleInfo().wLevel < Iter->second.MinLevel)
	{
		m_pRole->ChangeRoleLevel(static_cast<short>(m_pRole->GetRoleInfo().wLevel - Iter->second.MinLevel));
		m_pRole->ChangeRoleExp(-1 * m_pRole->GetRoleInfo().dwExp, false);
	}

	if (m_pRole->GetRoleMonth().IsInMonthTable())
	{
		if (m_pRole->GetRoleMonth().GetMonthInfo().dwMonthGlobel <= 100)
		{
			//��ҽ�������
			if (m_pRole->GetRoleMonth().IsCanAddMonthGlobel())
			{
				m_pRole->GetRoleMonth().OnRoleAddMonthGlobel();
				m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 1000);
			}
		}
	}
}
void GameRobot::UpdateRobotIsCanOpenFire()
{
	//���»������Ƿ���Կ���
	//�������������ں� ��Ҫֹͣһ��ʱ��
	if (timeGetTime() <= m_RobotOpenFireTimeLog)
		return;
	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	if (!Iter->second.OpenFireInfo.IsOpen)
	{
		m_IsOpenFire = true;
		return;
	}
	if (m_IsOpenFire)
	{
		m_IsOpenFire = false;
		//����ͣ�� �ȴ��´ο�ʼ
		DWORD StarSec = RandUInt() % (Iter->second.OpenFireInfo.FireInfo.StopMaxSec - Iter->second.OpenFireInfo.FireInfo.StopMinSec) + Iter->second.OpenFireInfo.FireInfo.StopMinSec;
		m_RobotOpenFireTimeLog = timeGetTime() + StarSec * 1000;
	}
	else
	{
		m_IsOpenFire = true;
		DWORD StarSec = RandUInt() % (Iter->second.OpenFireInfo.FireInfo.OpenMaxSec - Iter->second.OpenFireInfo.FireInfo.OpenMinSec) + Iter->second.OpenFireInfo.FireInfo.OpenMinSec;
		m_RobotOpenFireTimeLog = timeGetTime() + StarSec * 1000;
	}
}
void GameRobot::UpdateRobotCharm()
{
	//��ϵ������ʹ��������Ʒ
	if (m_RobotSendCharmItemTimeLog != 0 && timeGetTime() - m_RobotSendCharmItemTimeLog < 2000)
		return;
	m_RobotSendCharmItemTimeLog = timeGetTime();

	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	if (!Iter->second.CharmInfo.IsOpen)
		return;
	if (m_dwRobotSendCharmTimeLog == 0)
	{
		m_dwRobotSendCharmTimeLog = timeGetTime() + (RandUInt() % (Iter->second.CharmInfo.SendCharmMaxCDSec - Iter->second.CharmInfo.SendCharmMinCDSec) + Iter->second.CharmInfo.SendCharmMinCDSec) * 1000;
		return;
	}
	if (timeGetTime() < m_dwRobotSendCharmTimeLog)
		return;

	BYTE RandF = static_cast<BYTE>(RandUInt() % 100);
	if (RandF < Iter->second.CharmInfo.NonRateF)
	{
		m_dwRobotSendCharmTimeLog = timeGetTime() + (RandUInt() % (Iter->second.CharmInfo.SendCharmMaxCDSec - Iter->second.CharmInfo.SendCharmMinCDSec) + Iter->second.CharmInfo.SendCharmMinCDSec) * 1000;
		return;
	}
	//ÿ2�����һ��
	//1.���� ���� 
	CRole* pUser = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());//��ҽ������� �첽��
	if (!pUser /*|| m_pRole->GetRoleMonth().IsInMonthTable()*/)
		return;
	GameTable* pTable = g_FishServer.GetTableManager()->GetTable(pUser->GetTableID());
	if (!pTable)
		return;
	if (pTable->GetRoleManager().GetRoleSum() <= 1)
		return;
	BYTE SeatIndex = RandUInt() % (pTable->GetRoleManager().GetRoleSum() -1);
	DWORD dwUserID = 0;
	BYTE NowIndex = 0;
	for (BYTE i = 0; i <= SeatIndex;)
	{
		CRole* pRole = pTable->GetRoleManager().GetRoleBySeatID(i);
		if (!pRole || !pRole->IsActionUser())
		{
			++i;
			continue;
		}
		else if (pRole->GetID() == GetRobotUserID())
		{
			++i;
			++NowIndex;
			continue;
		}
		else
		{
			if (NowIndex == SeatIndex)
			{
				dwUserID = pRole->GetID();
				break;
			}
			++i;
			++NowIndex;
		}
	}
	if (dwUserID == 0)
		return;
	//�������������ѡһλ��� �������һ������ ��Ʒ �������������Ʒ������ ���ǽ��м�¼����������ȥ	
	DWORD RateF = RandUInt() % Iter->second.CharmInfo.SendInfo.dwSendCharmArrayMaxRateF;
	std::vector<tagRobotSendCharmInfo>::iterator IterVec = Iter->second.CharmInfo.SendInfo.vecSendCharmArray.begin();
	for (; IterVec != Iter->second.CharmInfo.SendInfo.vecSendCharmArray.end(); ++IterVec)
	{
		if (RateF < IterVec->RateF)
		{
			BYTE ItemID = IterVec->CharmIndexID;
			WORD Sum = RandUInt() % (IterVec->MaxItemSum - IterVec->MinItemSum) + IterVec->MinItemSum;
			if (Sum > 0)
			{
				tagGameRobotCharm pEvent;
				pEvent.dwUserID = dwUserID;
				pEvent.CharmIndexID = ItemID;
				pEvent.CharmItemSum = Sum;
				pEvent.BeginTimeLog = timeGetTime() + 2000;
				m_vecSendCharmArray.push_back(pEvent);
			}
			//���뵽���ͼ�������ȥ
			m_dwRobotSendCharmTimeLog = timeGetTime() + (RandUInt() % (Iter->second.CharmInfo.SendCharmMaxCDSec - Iter->second.CharmInfo.SendCharmMinCDSec) + Iter->second.CharmInfo.SendCharmMinCDSec) * 1000;
			break;
		}
	}
}
void GameRobot::UpdateRobotSendCharm()
{
	//1s����һ��
	if (m_dwSendCharmTineLog != 0 && timeGetTime() - m_dwSendCharmTineLog < 500)
		return;
	m_dwSendCharmTineLog = timeGetTime();
	if (m_vecSendCharmArray.empty())
		return;

	HashMap<DWORD, tagGameRobotConfig>::iterator IterRobot = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (IterRobot == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	if (!IterRobot->second.CharmInfo.IsOpen)
		return;

	list<tagGameRobotCharm>::iterator Iter = m_vecSendCharmArray.begin();
	for (; Iter != m_vecSendCharmArray.end();)
	{
		//��ָ����ҷ���������Ʒ
		CRoleEx* pDestRole = g_FishServer.GetRoleManager()->QueryUser(Iter->dwUserID);
		if (!pDestRole)
		{
			Iter = m_vecSendCharmArray.erase(Iter);
			continue;
		}
		if (Iter->BeginTimeLog > m_dwSendCharmTineLog)
			return;
		if (Iter->CharmItemSum == 0)
		{
			Iter = m_vecSendCharmArray.erase(Iter);
			continue;
		}

		//�۳���ҽ�� ���� ��ʯ
		HashMap<BYTE, tagCharmOnce>::iterator IterCharm = g_FishServer.GetFishConfig().GetCharmConfig().CharmIndexMap.find(Iter->CharmIndexID);
		if (IterCharm == g_FishServer.GetFishConfig().GetCharmConfig().CharmIndexMap.end())
		{
			Iter = m_vecSendCharmArray.erase(Iter);
			continue;
		}
		if (!m_pRole->LostUserMoney(IterCharm->second.UserGlobel, IterCharm->second.UserMedal, IterCharm->second.UserCurrcey , TEXT("����������������Ʒ �۳�����")))
		{
			Iter = m_vecSendCharmArray.erase(Iter);
			continue;
		}
		pDestRole->ChangeRoleCharmValue(Iter->CharmIndexID,1);
		LC_Cmd_ChangeRoleCharmResult pResult;
		SetMsgInfo(pResult, GetMsgType(Main_Charm, LC_ChangeRoleCharmResult), sizeof(LC_Cmd_ChangeRoleCharmResult));
		pResult.Result = true;
		pResult.dwDestUserID = pDestRole->GetUserID();
		pResult.dwDestUserCharmValue = g_FishServer.GetFishConfig().GetCharmValue(pDestRole->GetRoleInfo().CharmArray);
		m_pRole->SendDataToClient(&pResult);
		LC_Cmd_TableRoleCharmInfo msgTable;
		SetMsgInfo(msgTable, GetMsgType(Main_Charm, LC_TableRoleCharmInfo), sizeof(LC_Cmd_TableRoleCharmInfo));
		msgTable.SrcUserID = m_pRole->GetUserID();
		msgTable.DestUserID = Iter->dwUserID;
		msgTable.CharmIndex = Iter->CharmIndexID;
		msgTable.ItemSum =1;
		m_pRole->SendDataToClient(&msgTable);
		m_pRole->SendDataToTable(&msgTable);


		Iter->CharmItemSum -= 1;
		if (Iter->CharmItemSum == 0)
			m_vecSendCharmArray.erase(Iter);

		m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 3000);
		return;
	}
}
void GameRobot::OnChangeCharmValue(DWORD SrcUserID, BYTE CharmIndexID, DWORD CharmItemSum, int ChangeCharmValue)
{
	HashMap<DWORD, tagGameRobotConfig>::iterator Iter = g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.find(m_RobotID);
	if (Iter == g_FishServer.GetFishConfig().GetFishGameRobotConfig().RobotMap.end())
		return;
	//1.��������Ч��
	HashMap<BYTE, tagRobotBeSendCharm>::iterator IterFind = Iter->second.CharmInfo.BeSendInfoMap.find(CharmIndexID);
	if (IterFind != Iter->second.CharmInfo.BeSendInfoMap.end())
	{
		BYTE NonRandF = static_cast<BYTE>(RandUInt() % 100);
		if (NonRandF >= IterFind->second.NonRateF)
		{
			DWORD RateF = RandUInt() % IterFind->second.SendInfo.dwSendCharmArrayMaxRateF;
			std::vector<tagRobotSendCharmInfo>::iterator IterVec = IterFind->second.SendInfo.vecSendCharmArray.begin();
			for (; IterVec != IterFind->second.SendInfo.vecSendCharmArray.end(); ++IterVec)
			{
				if (RateF < IterVec->RateF)
				{
					BYTE ItemID = IterVec->CharmIndexID;
					WORD Sum = RandUInt() % (IterVec->MaxItemSum - IterVec->MinItemSum) + IterVec->MinItemSum;
					//���뵽���ͼ�������ȥ
					if (Sum > 0)
					{
						tagGameRobotCharm pEvent;
						pEvent.dwUserID = SrcUserID;
						pEvent.CharmIndexID = ItemID;
						pEvent.CharmItemSum = Sum;
						pEvent.BeginTimeLog = timeGetTime() + 2000;
						m_vecSendCharmArray.push_back(pEvent);
						m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 3000);
					}
					break;
				}
			}
		}
	}
	//2.��ʱ����뵽��������ȥ �ж��Ƿ���Ҫ�����¼�
	int CharmValue = m_RobotCharmEvent.AddCharmEvent(SrcUserID, ChangeCharmValue, Iter->second.CharmInfo.BeChangeCharmSec * 1000);
	if (static_cast<DWORD>(abs(CharmValue)) >= Iter->second.CharmInfo.BeChangeCharmValue)
	{
		//�����¼�
		m_RobotCharmEvent.Clear(SrcUserID);

		BYTE NonRandF = static_cast<BYTE>(RandUInt() % 100);
		if (NonRandF >= Iter->second.CharmInfo.CharmChangeEvent.NonRateF)
		{
			DWORD RateF = RandUInt() % 100;
			if (RateF < Iter->second.CharmInfo.CharmChangeEvent.LeaveRoomRateF && !m_pRole->GetRoleMonth().IsInMonthTable())
			{
				CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
				if (!pRole)//��Ҳ���������
					return;
				DWORD LeaveSec = RandUInt() % (Iter->second.RoomInfo.RoomInfo.StopMaxSec - Iter->second.RoomInfo.RoomInfo.StopMinSec) + Iter->second.RoomInfo.RoomInfo.StopMinSec;
				m_pManager->AdddWriteRobot(pRole->GetTableID(), timeGetTime() + LeaveSec * 1000);
				//��������Ҫ�뿪������
				g_FishServer.GetTableManager()->OnPlayerLeaveTable(GetRobotUserID());//�û������뿪����
				//���ջ�����
				m_pManager->AddResetRobot(GetRobotUserID());
				return;
			}
			else
			{
				if (CharmValue >=0)
				{
					//��������
					DWORD RateF = RandUInt() % Iter->second.CharmInfo.CharmChangeEvent.AddSendInfo.dwSendCharmArrayMaxRateF;
					std::vector<tagRobotSendCharmInfo>::iterator IterVec = Iter->second.CharmInfo.CharmChangeEvent.AddSendInfo.vecSendCharmArray.begin();
					for (; IterVec != Iter->second.CharmInfo.CharmChangeEvent.AddSendInfo.vecSendCharmArray.end(); ++IterVec)
					{
						if (RateF < IterVec->RateF)
						{
							BYTE ItemID = IterVec->CharmIndexID;
							WORD Sum = RandUInt() % (IterVec->MaxItemSum - IterVec->MinItemSum) + IterVec->MinItemSum;
							//���뵽���ͼ�������ȥ
							if (Sum > 0)
							{
								tagGameRobotCharm pEvent;
								pEvent.dwUserID = SrcUserID;
								pEvent.CharmIndexID = ItemID;
								pEvent.CharmItemSum = Sum;
								pEvent.BeginTimeLog = timeGetTime() + 2000;
								m_vecSendCharmArray.push_back(pEvent);
								m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 3000);
							}
							break;
						}
					}
				}
				else
				{
					//��������
					DWORD RateF = RandUInt() % Iter->second.CharmInfo.CharmChangeEvent.DelSendInfo.dwSendCharmArrayMaxRateF;
					std::vector<tagRobotSendCharmInfo>::iterator IterVec = Iter->second.CharmInfo.CharmChangeEvent.DelSendInfo.vecSendCharmArray.begin();
					for (; IterVec != Iter->second.CharmInfo.CharmChangeEvent.DelSendInfo.vecSendCharmArray.end(); ++IterVec)
					{
						if (RateF < IterVec->RateF)
						{
							BYTE ItemID = IterVec->CharmIndexID;
							WORD Sum = RandUInt() % (IterVec->MaxItemSum - IterVec->MinItemSum) + IterVec->MinItemSum;
							//���뵽���ͼ�������ȥ
							if (Sum > 0)
							{
								tagGameRobotCharm pEvent;
								pEvent.dwUserID = SrcUserID;
								pEvent.CharmIndexID = ItemID;
								pEvent.CharmItemSum = Sum;
								pEvent.BeginTimeLog = timeGetTime() + 2000;
								m_vecSendCharmArray.push_back(pEvent);
								m_StopOpenFireTimeLog = max(m_StopOpenFireTimeLog, timeGetTime() + 3000);
							}
							break;
						}
					}
				}
			}
		}
	}
}

