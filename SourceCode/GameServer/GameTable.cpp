#include "StdAfx.h"
#include "GameTable.h"
#include "FishLogic\FishScene\FishResManager.h"
#include "RoleEx.h"
#include "FishServer.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")


GameTable::GameTable()
{
	//m_TableConfig = NULL;
}
GameTable::~GameTable()
{
	OnDestroy();
}
bool GameTable::OnInit(WORD TableID, BYTE TableTypeID, BYTE MonthID)
{
	HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return false;
	}
	m_TableID = TableID;//���ú����ӵ�ID 
	m_MonthID = MonthID;
	m_TableTypeID = TableTypeID;
	m_RoleManager.OnInit(TableID, Iter->second.wMaxPlayerSum);//��ʼ����ҹ�����
	m_fishdesk.Init(m_TableID, this, Iter->second.cMapName, &m_RoleManager);
	m_LastUpdateTime = timeGetTime();
	return true;
}
bool GameTable::IsFull()
{
	return m_RoleManager.IsFull();
}
bool GameTable::IsCanAddRobot()
{
	return g_FishServer.GetRobotManager().GameRobotIsCanJoinTable(this);

	//�жϵ�ǰ�����Ƿ���Լ��������
	//1.������ӵĿ�λ��ֻ��һ���� �������޷�����
	/*HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(m_TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return false;
	}
	if (m_RoleManager.GetRoleSum() >= (Iter->second.wMaxPlayerSum - 1))
		return false;
	return true;*/
}
WORD GameTable::GetTablePlayerSum()
{
	return ConvertIntToWORD(m_RoleManager.GetRoleSum());
}
CRole* GameTable::SearchUser(DWORD dwUserID)
{
	return m_RoleManager.GetRoleByUserID(dwUserID);
}
void GameTable::OnDestroy()
{
	m_TableTypeID = 0;
	m_fishdesk.Shutdown();
	m_RoleManager.Destroy();
}
bool GameTable::IsCanJoinTable(CRoleEx* pRoleEx, BYTE MonthID)
{
	//�ж�����Ƿ���Խ��뵱ǰ���͵�����
	if (!pRoleEx)
	{
		ASSERT(false);
		return false;
	}
	CRole* pRole = g_FishServer.GetTableManager()->SearchUser(pRoleEx->GetUserID());
	if (pRole)
	{
		ASSERT(false);
		return false;
	}
	//�ж�����Ƿ���Խ���
	if (!IsCanJoinRoom(pRoleEx))//�ж�����Ƿ���Խ������� 
	{
		return false;
	}
	if (m_RoleManager.IsFull())
	{
		return false;
	}
	return true;
}
bool GameTable::OnRoleJoinTable(CRoleEx* pRoleEx, BYTE MonthID, bool IsSendToClient)
{
	if (!pRoleEx)
	{
		ASSERT(false);
		return false;
	}
	HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(m_TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return false;
	}
	//�ж�����Ƿ���Խ���
	if (!IsCanJoinRoom(pRoleEx))//�ж�����Ƿ���Խ������� 
	{
		ASSERT(false);
		return false;
	}
	if (!m_RoleManager.OnInitRole(pRoleEx))
	{
		ASSERT(false);
		return false;
	}
	CRole* pRole = m_RoleManager.GetRoleByUserID(pRoleEx->GetUserID());
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}

	pRole->SetRoleIsCanSendTableMsg(IsSendToClient);//��������Ƿ���Է�������ͻ���ȥ
	//������ҳɹ��������ӵ�����
	pRole->SetBulletRate(Iter->second.TableRate, Iter->second.MinRate, Iter->second.MaxRate);
	//pRole->SetRoomLauncher();//������ҵ���̨

	LC_JoinTableResult msgJoin;
	SetMsgInfo(msgJoin, GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
	msgJoin.bTableTypeID = GetTableTypeID();
	msgJoin.Result = true;
	msgJoin.BackgroundImage = m_fishdesk.GetSceneBackground();

	if (pRoleEx->GetRoleLauncherManager().IsCanUserLauncherByID(pRole->GetLauncherType()))
		msgJoin.LauncherType = pRole->GetLauncherType() | 128;
	else
		msgJoin.LauncherType = pRole->GetLauncherType();

	msgJoin.SeatID = pRole->GetSeatID();
	msgJoin.RateIndex = pRole->GetRateIndex();
	msgJoin.Energy = pRole->GetEnergy();
	pRoleEx->SendDataToClient(&msgJoin);
	
	//�����Լ�����λ��
	LC_Cmd_MeSeatID msg;
	SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_MeSeatID), sizeof(LC_Cmd_MeSeatID));
	msg.SeatID = pRole->GetSeatID();
	pRole->GetRoleExInfo()->SendDataToClient(&msg);
	//�������ݷ��͸��Լ�
	if (MonthID != 0)
	{
		if (pRole->GetLauncherType() != 0 )
		{
			pRole->SetTempLauncherType(pRole->GetLauncherType());
			pRole->SetLauncherType(0);

			LC_ChangeLauncherType msg;
			SetMsgInfo(msg, GetMsgType(Main_Month, CL_ChangeLauncherType), sizeof(LC_ChangeLauncherType));
			msg.LauncherType = pRole->GetLauncherType();
			pRole->GetRoleExInfo()->SendDataToClient(&msg);
		}

		LC_Cmd_JoinMonthSucess msg;//�Լ������ݷ��͸�ֱ��
		SetMsgInfo(msg,GetMsgType(Main_Month, LC_JoinMonthSucess), sizeof(LC_Cmd_JoinMonthSucess));
		msg.MonthInfo = pRole->GetRoleExInfo()->GetRoleMonth().GetMonthInfo();
		pRole->GetRoleExInfo()->SendDataToClient(&msg);
	}

	pRole->GetRoleExInfo()->GetRoleGameData().OnHandleRoleJoinTable(MonthID != 0);

	if (m_RoleManager.GetRoleSum() == 1)
	{
		OnGameStart();
	}
	else
	{
		//��������1  ������Ҫ������ҽ����ͬ���������� 
		SendRoleJoinInfo(pRoleEx->GetUserID());
		////ͬ����������
		if (MonthID != 0)
		{
			SendRoleMonthInfo(pRoleEx->GetUserID());
		}
	}

	//��Ϸ�ڲ���ͬ�������
	byte SeatID = 0;
	m_fishdesk.PlayerJoin(pRoleEx->GetUserID(), SeatID);//��Ҽ���

	return true;
}
void GameTable::SendRoleMonthInfo(DWORD dwUserID)
{
	HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(m_TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return;
	}
	if (GetTableMonthID() ==0)
	{
		ASSERT(false);
		return;
	}
	CRole* pUser = m_RoleManager.GetRoleByUserID(dwUserID);
	if (!pUser || !pUser->IsActionUser())
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_JoinMonthSucess msg;
	SetMsgInfo(msg,GetMsgType(Main_Month, LC_JoinMonthSucess), sizeof(LC_Cmd_JoinMonthSucess));
	msg.MonthInfo = pUser->GetRoleExInfo()->GetRoleMonth().GetMonthInfo();

	for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
	{
		CRole* pOtherUser = m_RoleManager.GetRoleBySeatID(i);
		if (!pOtherUser || !pOtherUser->IsActionUser())
			continue;
		if (pOtherUser->GetID() != pUser->GetID())
		{
			LC_Cmd_JoinMonthSucess otherMsg;
			SetMsgInfo(otherMsg, GetMsgType(Main_Month, LC_JoinMonthSucess), sizeof(LC_Cmd_JoinMonthSucess));
			otherMsg.MonthInfo = pOtherUser->GetRoleExInfo()->GetRoleMonth().GetMonthInfo();

			pUser->GetRoleExInfo()->SendDataToClient(&otherMsg);

			pOtherUser->GetRoleExInfo()->SendDataToClient(&msg);
		}
	}
}
void GameTable::SendTableRoleInfoToClient(DWORD dwUserID)
{
	CRole* pUser = m_RoleManager.GetRoleByUserID(dwUserID);
	if (!pUser || !pUser->GetRoleExInfo())
	{
		ASSERT(false);
		return;
	}
	//ֱ�ӵ�һЩ��̨������ ���͵��ͻ���ȥ
	LC_JoinTableResult msgJoin;
	SetMsgInfo(msgJoin, GetMsgType(Main_Table, LC_Sub_JoinTable), sizeof(LC_JoinTableResult));
	msgJoin.bTableTypeID = GetTableTypeID();
	msgJoin.Result = true;
	msgJoin.BackgroundImage = m_fishdesk.GetSceneBackground();

	if (pUser->GetRoleExInfo()->GetRoleLauncherManager().IsCanUserLauncherByID(pUser->GetLauncherType()))
		msgJoin.LauncherType = pUser->GetLauncherType() | 128;
	else
		msgJoin.LauncherType = pUser->GetLauncherType();

	msgJoin.SeatID = pUser->GetSeatID();
	msgJoin.RateIndex = pUser->GetRateIndex();
	msgJoin.Energy = pUser->GetEnergy();
	pUser->GetRoleExInfo()->SendDataToClient(&msgJoin);

	//�����Լ�����λ��
	LC_Cmd_MeSeatID msg;
	SetMsgInfo(msg, GetMsgType(Main_Table, LC_Sub_MeSeatID), sizeof(LC_Cmd_MeSeatID));
	msg.SeatID = pUser->GetSeatID();
	pUser->GetRoleExInfo()->SendDataToClient(&msg);

	if (m_MonthID != 0)
	{
		//����������� ���һ��ڱ����� �����������ݽ���ˢ�� �����Է��ı�������
		LC_Cmd_ResetRoleMonthInfo msg;
		SetMsgInfo(msg, GetMsgType(Main_Month, LC_ResetRoleMonthInfo), sizeof(LC_Cmd_ResetRoleMonthInfo));
		msg.MonthInfo = pUser->GetRoleExInfo()->GetRoleMonth().GetMonthInfo();
		pUser->GetRoleExInfo()->SendDataToClient(&msg);
	}

	time_t pNow = time(null);

	for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
	{
		CRole* pOtherUser = m_RoleManager.GetRoleBySeatID(i);
		if (!pOtherUser || !pOtherUser->IsActionUser())
			continue;
		if (pOtherUser->GetID() != pUser->GetID())
		{
			LC_Cmd_ResetOtherUserInfo msg;
			SetMsgInfo(msg, GetMsgType(Main_Table, LC_ResetOtherUserInfo), sizeof(LC_Cmd_ResetOtherUserInfo));
			msg.UserInfo.dwUserID = pOtherUser->GetID();
			msg.UserInfo.dwFaceID = pOtherUser->GetFaceID();
			msg.UserInfo.bGender = pOtherUser->GetRoleExInfo()->GetRoleInfo().bGender;
			//msg.UserInfo.dwExp = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwExp;
			msg.UserInfo.dwCurrencyNum = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwCurrencyNum;
			msg.UserInfo.dwGlobeNum = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwGlobeNum;
			msg.UserInfo.dwAchievementPoint = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwAchievementPoint;
			msg.UserInfo.TitleID = pOtherUser->GetRoleExInfo()->GetRoleInfo().TitleID;
			msg.UserInfo.GameID = pOtherUser->GetRoleExInfo()->GetRoleInfo().GameID;
			msg.UserInfo.dwCashpoint = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwCashpoint;       //��ȯ
			msg.UserInfo.byUsingLauncher = pOtherUser->GetRoleExInfo()->GetRoleInfo().byUsingLauncher;//��ǰʹ�õ���̨
			msg.UserInfo.MaxRateIndex = pOtherUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate();//����

			if (pOtherUser->GetRoleExInfo()->GetRoleInfo().IsShowIPAddress)
			{
				//TCHARCopy(msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress), pOtherUser->GetRoleExInfo()->GetRoleInfo().IPAddress, _tcslen(pOtherUser->GetRoleExInfo()->GetRoleInfo().IPAddress));
				g_FishServer.GetAddressByIP(pOtherUser->GetRoleExInfo()->GetRoleInfo().ClientIP, msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress));
			}
			else
			{
				TCHARCopy(msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));
			}

			for (int j = 0; j < MAX_CHARM_ITEMSUM; ++j)
				msg.UserInfo.CharmArray[j] = pOtherUser->GetRoleExInfo()->GetRoleInfo().CharmArray[j];
			TCHARCopy(msg.UserInfo.NickName, CountArray(msg.UserInfo.NickName), pOtherUser->GetNickName(), _tcslen(pOtherUser->GetNickName()));
			msg.UserInfo.VipLevel = pOtherUser->GetRoleExInfo()->GetRoleInfo().VipLevel;
			msg.UserInfo.IsInMonthCard = (pOtherUser->GetRoleExInfo()->GetRoleInfo().MonthCardID != 0 && pOtherUser->GetRoleExInfo()->GetRoleInfo().MonthCardEndTime >= pNow);
			msg.UserInfo.SeatID = pOtherUser->GetSeatID();
			msg.UserInfo.wLevel = pOtherUser->GetRoleExInfo()->GetRoleInfo().wLevel;

			pUser->GetRoleExInfo()->SendDataToClient(&msg);
		}
	}
	//����ڱ��� ����Ҫˢ�±��������� 
	if (m_MonthID != 0)
	{
		for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
		{
			CRole* pOtherUser = m_RoleManager.GetRoleBySeatID(i);
			if (!pOtherUser || !pOtherUser->IsActionUser())
				continue;
			if (pOtherUser->GetID() != pUser->GetID())
			{
				LC_Cmd_ResetRoleMonthInfo otherMsg;
				SetMsgInfo(otherMsg, GetMsgType( Main_Month, LC_ResetRoleMonthInfo), sizeof(LC_Cmd_ResetRoleMonthInfo));
				otherMsg.MonthInfo = pOtherUser->GetRoleExInfo()->GetRoleMonth().GetMonthInfo();
				pUser->GetRoleExInfo()->SendDataToClient(&otherMsg);
			}
		}
	}
	m_fishdesk.AsyncPlayerJoin(pUser, true);//��Ҽ���
}
void GameTable::SendRoleJoinInfo(DWORD dwUserID)
{
	
	//���¼����������ݽ���ͬ������
	CRole* pUser = m_RoleManager.GetRoleByUserID(dwUserID);
	if (!pUser)
	{
		ASSERT(false);
		return;
	}

	time_t pNow = time(null);

	LC_OtherUserInfo msgUser;
	SetMsgInfo(msgUser,GetMsgType(Main_Table, LC_Sub_OtherUserInfo), sizeof(LC_OtherUserInfo));
	msgUser.UserInfo.dwUserID = pUser->GetID();
	msgUser.UserInfo.dwFaceID = pUser->GetFaceID();
	msgUser.UserInfo.bGender = pUser->GetRoleExInfo()->GetRoleInfo().bGender;
	//msgUser.UserInfo.dwExp = pUser->GetRoleExInfo()->GetRoleInfo().dwExp;
	msgUser.UserInfo.dwCurrencyNum = pUser->GetRoleExInfo()->GetRoleInfo().dwCurrencyNum;
	msgUser.UserInfo.dwGlobeNum = pUser->GetRoleExInfo()->GetRoleInfo().dwGlobeNum;
	msgUser.UserInfo.dwAchievementPoint = pUser->GetRoleExInfo()->GetRoleInfo().dwAchievementPoint;
	msgUser.UserInfo.TitleID = pUser->GetRoleExInfo()->GetRoleInfo().TitleID;
	msgUser.UserInfo.VipLevel = pUser->GetRoleExInfo()->GetRoleInfo().VipLevel;
	msgUser.UserInfo.IsInMonthCard = (pUser->GetRoleExInfo()->GetRoleInfo().MonthCardID != 0 && pUser->GetRoleExInfo()->GetRoleInfo().MonthCardEndTime >= pNow);
	msgUser.UserInfo.GameID = pUser->GetRoleExInfo()->GetRoleInfo().GameID;
	msgUser.UserInfo.dwCashpoint = pUser->GetRoleExInfo()->GetRoleInfo().dwCashpoint;       //��ȯ
	msgUser.UserInfo.byUsingLauncher = pUser->GetRoleExInfo()->GetRoleInfo().byUsingLauncher;//��ǰʹ�õ���̨
	msgUser.UserInfo.MaxRateIndex = pUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate();//����
	//msgUser.UserInfo.ClientIP = pUser->GetRoleExInfo()->GetRoleInfo().ClientIP;

	
	if (pUser->GetRoleExInfo()->GetRoleInfo().IsShowIPAddress)
	{
		g_FishServer.GetAddressByIP(pUser->GetRoleExInfo()->GetRoleInfo().ClientIP, msgUser.UserInfo.IPAddress, CountArray(msgUser.UserInfo.IPAddress));
		//չʾ�Լ�������
		//TCHARCopy(msgUser.UserInfo.IPAddress, CountArray(msgUser.UserInfo.IPAddress), pUser->GetRoleExInfo()->GetRoleInfo().IPAddress, _tcslen(pUser->GetRoleExInfo()->GetRoleInfo().IPAddress));
	}
	else
	{
		TCHARCopy(msgUser.UserInfo.IPAddress, CountArray(msgUser.UserInfo.IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));
	}



	for (int i = 0; i < MAX_CHARM_ITEMSUM;++i)
		msgUser.UserInfo.CharmArray[i] = pUser->GetRoleExInfo()->GetRoleInfo().CharmArray[i];

	msgUser.UserInfo.SeatID = pUser->GetSeatID();
	//������ҵ�����
	TCHARCopy(msgUser.UserInfo.NickName, CountArray(msgUser.UserInfo.NickName), pUser->GetNickName(), _tcslen(pUser->GetNickName()));

	msgUser.UserInfo.wLevel = pUser->GetRoleExInfo()->GetRoleInfo().wLevel;
	for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
	{
		CRole* pOtherUser = m_RoleManager.GetRoleBySeatID(i);
		if (!pOtherUser || !pOtherUser->IsActionUser())
			continue;
		if (pOtherUser->GetID() != pUser->GetID())
		{
			LC_OtherUserInfo msg;
			SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_OtherUserInfo), sizeof(LC_OtherUserInfo));
			msg.UserInfo.dwUserID = pOtherUser->GetID();
			msg.UserInfo.dwFaceID = pOtherUser->GetFaceID();
			msg.UserInfo.bGender = pOtherUser->GetRoleExInfo()->GetRoleInfo().bGender;
		//	msg.UserInfo.dwExp = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwExp;
			msg.UserInfo.dwCurrencyNum = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwCurrencyNum;
			msg.UserInfo.dwGlobeNum = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwGlobeNum;
			msg.UserInfo.dwAchievementPoint = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwAchievementPoint;
			msg.UserInfo.TitleID = pOtherUser->GetRoleExInfo()->GetRoleInfo().TitleID;
			msg.UserInfo.GameID = pOtherUser->GetRoleExInfo()->GetRoleInfo().GameID;

			for (int j = 0;j < MAX_CHARM_ITEMSUM; ++j)
				msg.UserInfo.CharmArray[j] = pOtherUser->GetRoleExInfo()->GetRoleInfo().CharmArray[j];
			TCHARCopy(msg.UserInfo.NickName, CountArray(msg.UserInfo.NickName), pOtherUser->GetNickName(), _tcslen(pOtherUser->GetNickName()));

			if (pOtherUser->GetRoleExInfo()->GetRoleInfo().IsShowIPAddress)
			{
				g_FishServer.GetAddressByIP(pOtherUser->GetRoleExInfo()->GetRoleInfo().ClientIP, msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress));
				//TCHARCopy(msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress), pOtherUser->GetRoleExInfo()->GetRoleInfo().IPAddress, _tcslen(pOtherUser->GetRoleExInfo()->GetRoleInfo().IPAddress));
			}
			else
			{
				TCHARCopy(msg.UserInfo.IPAddress, CountArray(msg.UserInfo.IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));
			}

			msg.UserInfo.SeatID = pOtherUser->GetSeatID();
			msg.UserInfo.wLevel = pOtherUser->GetRoleExInfo()->GetRoleInfo().wLevel;

			msg.UserInfo.VipLevel = pOtherUser->GetRoleExInfo()->GetRoleInfo().VipLevel;
			msg.UserInfo.IsInMonthCard = (pOtherUser->GetRoleExInfo()->GetRoleInfo().MonthCardID != 0 && pOtherUser->GetRoleExInfo()->GetRoleInfo().MonthCardEndTime >= pNow);
			msg.UserInfo.dwCashpoint = pOtherUser->GetRoleExInfo()->GetRoleInfo().dwCashpoint;       //��ȯ
			msg.UserInfo.byUsingLauncher = pOtherUser->GetRoleExInfo()->GetRoleInfo().byUsingLauncher;//��ǰʹ�õ���̨
			msg.UserInfo.MaxRateIndex = pOtherUser->GetRoleExInfo()->GetRoleRate().GetCanUseMaxRate();//����
																									  
			//LogInfoToFile("WmRate.txt", "userid=%d, maxrate=%d", msg.UserInfo.dwUserID, msg.UserInfo.MaxRateIndex);

			pUser->GetRoleExInfo()->SendDataToClient(&msg);

			pOtherUser->GetRoleExInfo()->SendDataToClient(&msgUser);
		}
	}
}
bool GameTable::OnRoleLeaveTable(DWORD dwUserID)
{
	//����뿪����
	CRole* pRole = m_RoleManager.GetRoleByUserID(dwUserID);
	if (m_MonthID != 0)
	{
		if (!pRole->GetRoleExInfo())
		{
			ASSERT(false);
			return false;
		}
		if (pRole->GetRoleExInfo()->GetRoleMonth().GetMonthID() == m_MonthID)
			pRole->GetRoleExInfo()->GetRoleMonth().ClearMonthInfo();
		/*else
		{
			ASSERT(false);
		}*/

		//�������������
		{
			if (pRole->GetTempLauncherType() != 0)
			{
				pRole->SetLauncherType(pRole->GetTempLauncherType());
				LC_ChangeLauncherType msg;
				SetMsgInfo(msg, GetMsgType(Main_Month, CL_ChangeLauncherType), sizeof(LC_ChangeLauncherType));
				msg.LauncherType = pRole->GetLauncherType();
				pRole->GetRoleExInfo()->SendDataToClient(&msg);
			}
		}
	}
	pRole->GetRoleExInfo()->GetRoleGameData().OnHandleRoleLeaveTable();
	if (!pRole || !m_fishdesk.PlayerLeave(dwUserID))
	{
		ASSERT(false);
		return false;
	}
	if (m_RoleManager.GetRoleSum() == 0)
		OnGameStop();
	else
	{
		//��������1  ������Ҫ��������뿪���������
		//���������ɾ����ǰ���
		LC_OtherUserLeave msg;
		SetMsgInfo(msg,GetMsgType(Main_Table, LC_Sub_UserLeave), sizeof(LC_OtherUserLeave));
		msg.dwUserID = dwUserID;
		for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
		{
			CRole* pOtherUser = m_RoleManager.GetRoleBySeatID(i);
			if (!pOtherUser || !pOtherUser->IsActionUser())
				continue;
			if (pOtherUser->GetID() != dwUserID && pOtherUser->GetRoleExInfo())
			{
				pOtherUser->GetRoleExInfo()->SendDataToClient(&msg);
			}
		}
	}
	return true;
}
bool GameTable::OnHandleTableMsg(DWORD dwUserID, NetCmd* pCmd)
{
	if (!pCmd)
	{
		ASSERT(false);
		return false;
	}
	return m_fishdesk.HandleNetCmd(dwUserID, pCmd);
}
void GameTable::Update(bool bUpdateTime, bool bUpdateRp, bool bUpdateRpEffect)
{
	DWORD dwCurrent = timeGetTime();
	m_fishdesk.Update((dwCurrent - m_LastUpdateTime)*0.001f);
	m_LastUpdateTime = dwCurrent;

	if (bUpdateTime)
	{
		for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); i++)
		{
			CRole *pRole = m_RoleManager.GetRoleBySeatID(i);
			if (pRole&&pRole->IsActionUser())
			{
				pRole->AddGameTime(GAME_TIME_SPACE);
			}
		}
	}

	if (bUpdateRp)
	{
		for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); i++)
		{
			CRole *pRole = m_RoleManager.GetRoleBySeatID(i);
			if (pRole&&pRole->IsActionUser())
			{
				pRole->SetRpEffect(true);
			}
		}
	}

	if (bUpdateRpEffect)
	{
		for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); i++)
		{
			CRole *pRole = m_RoleManager.GetRoleBySeatID(i);
			if (pRole&&pRole->IsActionUser())
			{
				pRole->SetRpEffect(false);
			}
		}
	}
	DWORD dwTimer = timeGetTime();
	m_RoleManager.OnUpdate(dwTimer);
}
void GameTable::OnGameStart()
{
	m_fishdesk.OnGameStar();
	m_LastUpdateTime = timeGetTime();
	//CTraceService::TraceString(TEXT("һ�����ӿ�ʼ��Ϸ"), TraceLevel_Normal);
}
void GameTable::OnGameStop()
{
	m_fishdesk.OnGameEnd();
	//CTraceService::TraceString(TEXT("һ�����ӽ�����Ϸ"), TraceLevel_Normal);
}
void GameTable::Send(PlayerID RoleID, NetCmd*pCmd)
{
	CRole* pRole = m_RoleManager.GetRoleByUserID(RoleID);
	if (pRole != NULL && pRole->IsActionUser())
		pRole->GetRoleExInfo()->SendDataToClient(pCmd);
	/*else
		free(pCmd);*/
}
void GameTable::SendBySeat(byte seatID, NetCmd*pCmd)
{
	CRole* pRole = m_RoleManager.GetRoleBySeatID(seatID);
	if (pRole != NULL && pRole->IsActionUser())
		pRole->GetRoleExInfo()->SendDataToClient(pCmd);
	/*else
		free(pCmd);*/
}
bool GameTable::IsUseSeat()const
{
	return true;
}
void GameTable::SendDataToTable(DWORD dwUserID, NetCmd* pData)
{
	if (!pData)
	{
		ASSERT(false);
		return;
	}
	if (m_RoleManager.GetRoleSum() == 0)
	{
		return;
	}
	for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
	{
		CRole* pRole = m_RoleManager.GetRoleBySeatID(i);
		if (pRole->IsActionUser() && pRole->GetID() != dwUserID)
		{
			pRole->GetRoleExInfo()->SendDataToClient(pData);
		}
	}
}
void GameTable::SendDataToTableAllUser(NetCmd* pData)
{
	if (!pData)
	{
		ASSERT(false);
		return;
	}
	if (m_RoleManager.GetRoleSum() == 0)
	{
		return;
	}
	for (BYTE i = 0; i < m_RoleManager.GetMaxPlayerSum(); ++i)
	{
		CRole* pRole = m_RoleManager.GetRoleBySeatID(i);
		if (pRole->IsActionUser())
		{
			pRole->GetRoleExInfo()->SendDataToClient(pData);
		}
	}
}
bool GameTable::ChangeRoleRate(BYTE Rate)
{
	//��ǰ�����Ƿ���ȷ
	HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(m_TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return false;
	}
	if (Rate >= Iter->second.MinRate && Rate <= Iter->second.MaxRate)
		return true;
	else
		return false;
}
bool GameTable::IsCanJoinRoom(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	if (pRole->IsRobot())
	{
		return IsCanAddRobot();
	}
	HashMap<BYTE, TableInfo>::iterator Iter = g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.find(m_TableTypeID);
	if (Iter == g_FishServer.GetFishConfig().GetTableConfig().m_TableConfig.end())
	{
		ASSERT(false);
		return false;
	}

	if (Iter->second.MinLevel != 0xffffffff && pRole->GetRoleInfo().wLevel <Iter->second.MinLevel)
	{
		ASSERT(false);
		return false;
	}
	if (Iter->second.MaxLevel != 0xffffffff && pRole->GetRoleInfo().wLevel > Iter->second.MaxLevel)
	{
		ASSERT(false);
		return false;
	}

	if (Iter->second.MinCurreySum != 0xffffffff && pRole->GetRoleInfo().dwCurrencyNum <Iter->second.MinCurreySum)
	{
		ASSERT(false);
		return false;
	}
	if (Iter->second.MaxCurreySum != 0xffffffff && pRole->GetRoleInfo().dwCurrencyNum > Iter->second.MaxCurreySum)
	{
		ASSERT(false);
		return false;
	}
	
	if (Iter->second.MinGlobelSum != 0xffffffff && pRole->GetRoleInfo().dwGlobeNum < Iter->second.MinGlobelSum)
	{
		ASSERT(false);
		return false;
	}
	if (Iter->second.MaxGlobelSum != 0xffffffff && pRole->GetRoleInfo().dwGlobeNum > Iter->second.MaxGlobelSum)
	{
		ASSERT(false);
		return false;
	}
	//�ж�����Ƿ����ʹ����ͱ���
	if (!pRole->GetRoleRate().IsCanUseRateIndex(Iter->second.MinRate))//����޷�ʹ����ͱ��� �޷����뷿�� ���ʲ�����
	{
		ASSERT(false);
		return false;
	}
	//�ж���Ʒ�Ƿ��㹻
	if (!Iter->second.JoinItemVec.empty())
	{
		vector<tagTableJoinItem>::iterator IterItem = Iter->second.JoinItemVec.begin();
		for (; IterItem != Iter->second.JoinItemVec.end(); ++IterItem)
		{
			if (pRole->GetItemManager().QueryItemCount(IterItem->ItemID) < IterItem->ItemSum)
			{
				ASSERT(false);
				return false;
			}
		}
	}
	return true;
}