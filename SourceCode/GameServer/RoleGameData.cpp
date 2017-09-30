#include "Stdafx.h"
#include "RoleGameData.h"
#include "RoleEx.h"
#include "FishServer.h"
RoleGameData::RoleGameData()
{
	m_IsLoadDB = false;
}
RoleGameData::~RoleGameData()
{
	//SaveRoleGameData();
}
bool RoleGameData::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	//�������ݿ�
	m_IsLoadDB = false;
	OnLoadRoleGameDataByDB();
	return true;
}
void RoleGameData::OnLoadRoleGameDataByDB()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	/*DBR_Cmd_LoadGameData msg;
	SetMsgInfo(msg, DBR_LoadGameData, sizeof(DBR_Cmd_LoadGameData));
	msg.dwUserID = m_pRole->GetUserID();
	g_FishServer.SendNetCmdToDB(&msg);*/
}
void RoleGameData::OnLoadRoleGameDataResult(DBO_Cmd_LoadGameData* pMsg)
{
	if (!m_pRole || !pMsg)
	{
		ASSERT(false);
		return;
	}
	//�����ݿ�ͽṹ������
	m_RoleGameData = pMsg->GameData;
	m_IsLoadDB = true;
	m_pRole->IsLoadFinish();
}
void RoleGameData::SendRoleGameDataToClient()
{
	//�����ݷ��͵��ͻ���ȥ
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_GendGameData msg;
	SetMsgInfo(msg, GetMsgType(Main_GameData, LC_GendGameData), sizeof(LC_Cmd_GendGameData));
	msg.GameData = m_RoleGameData;
	m_pRole->SendDataToClient(&msg);
}
//void RoleGameData::SaveRoleGameData()
//{
//	//�������ݿ����� �����ݱ�������
//	if (!m_IsNeedSave || !m_pRole)
//		return;
//	//����DBR���� ��������
//	DBR_Cmd_SaveGameData msg;
//	SetMsgInfo(msg, DBR_SaveGameData, sizeof(DBR_Cmd_SaveGameData));
//	msg.dwUserID = m_pRole->GetUserID();
//	msg.GameData = m_RoleGameData;
//	g_FishServer.SendNetCmdToDB(&msg);
//	m_IsNeedSave = false;
//}
void RoleGameData::OnHandleCatchFish(BYTE FishID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//��Ҳ���ָ��ID����.
	m_RoleGameData.RoleCatchFishTotalSum += 1;	
	if (FishID == 9)
	{
		m_RoleGameData.CatchFishSum_9 += 1;
	}
	else if (FishID == 18)
	{
		m_RoleGameData.CatchFishSum_18 += 1;
	}
	else if (FishID == 20)
	{
		m_RoleGameData.CatchFishSum_20 += 1;
	}
	else if (FishID == 1)
	{
		m_RoleGameData.CatchFishSum_1 += 1;
	}
	else if (FishID == 3)
	{
		m_RoleGameData.CatchFishSum_3 += 1;
	}
	else if (FishID == 19)
	{
		m_RoleGameData.CatchFishSum_19 += 1;
	}
	m_pRole->SetRoleIsNeedSave();
}
void RoleGameData::OnHandleRoleGetGlobel(int AddGlobel)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (!CheckChangeInt64Value(m_RoleGameData.RoleGetGlobelSum, AddGlobel))
		return ;

	if (AddGlobel > 0 && m_RoleGameData.RoleGetGlobelSum + AddGlobel >= g_FishServer.GetFishConfig().GetSystemConfig().MaxGobelSum)//��ҵ��������� �޷���ӽ��
		return ;
	m_pRole->SetRoleIsNeedSave();
}
void RoleGameData::OnHandleRoleMonthReward(int RewardIndex)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	m_RoleGameData.RoleMonthRewardSum += 1;
	if (RewardIndex == 1)
		m_RoleGameData.RoleMonthFirstSum += 1;
	else if (RewardIndex == 2)
		m_RoleGameData.RoleMonthSecondSum += 1;
	else if (RewardIndex == 3)
		m_RoleGameData.RoleMonthThreeSum += 1;
	m_pRole->SetRoleIsNeedSave();
}
void RoleGameData::OnHandleRoleJoinTable(bool IsMonth)
{
	m_IsInMonth = IsMonth;
	m_LogJoinTableTime = timeGetTime();
}
void RoleGameData::OnHandleRoleLeaveTable()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//���û��뿪���ӵ�ʱ�� 
	//1.�ж�����Ƿ��뿪���������� ���� �ж�����������ϵ���Ϸʱ�� ����ͳ������
	DWORD GameSec = (timeGetTime() - m_LogJoinTableTime) / 1000;
	m_RoleGameData.TotalGameSec += GameSec;
	if (!m_IsInMonth)
		m_RoleGameData.NonMonthGameSec += GameSec;
	m_pRole->SetRoleIsNeedSave();
}
void RoleGameData::OnHandleRoleSignUpMonth()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//��ұ����μӱ���
	m_RoleGameData.RoleMonthSigupSum += 1;
	m_pRole->SetRoleIsNeedSave();
}
void RoleGameData::OnHandleRoleComb()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
	if (!pRole)
	{
		return;
	}
	WORD ComnoSum = pRole->CombCount();
	if (m_RoleGameData.MaxCombSum >= ComnoSum)
		return;
	m_RoleGameData.MaxCombSum = ComnoSum;//��¼�������������
	m_pRole->SetRoleIsNeedSave();
}