#include "StdAfx.h"
#include "RoleCheck.h"
#include "FishServer.h"
#include "RoleEx.h"
RoleCheck::RoleCheck()
{
}
RoleCheck::~RoleCheck()
{

}
bool RoleCheck::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_Role = pRole;
	//return LoadRoleCheckInfo();
	return true;
}
//bool RoleCheck::LoadRoleCheckInfo()
//{
//	//�����ݿⷢ������  �������ȫ������ҵ�����
//	if (!m_Role)
//	{
//		ASSERT(false);
//		return false;
//	}
//	DBR_Cmd_LoadRoleCheckInfo msg;
//	SetMsgInfo(msg,DBR_LoadRoleCheckInfo, sizeof(DBR_Cmd_LoadRoleCheckInfo));
//	msg.dwUserID = m_Role->GetUserID();
//	g_FishServer.SendNetCmdToDB(&msg);
//	return true;
//}
//void RoleCheck::LoadRoleCheckInfoResult(DBO_Cmd_LoadRoleCheckInfo* pDB)
//{
//	if (!pDB || !m_Role)
//	{
//		ASSERT(false);
//		return;
//	}
//	m_RoleCheckInfo = pDB->CheckInfo;
//	HandleCheckCheckInfo();
//	m_IsLoadDB = true;
//	m_Role->IsLoadFinish();
//}
//bool RoleCheck::HandleCheckCheckInfo()
//{
//	//�жϽ��� �� ���ǩ������ �Ƿ�Ϊ���ڵ����� ������ͬһ�� 
//	time_t timeNow = time(NULL);
//	tm* pTm = localtime(&timeNow);
//	if (!pTm)
//	{
//		ASSERT(false);
//		return false;
//	}
//	pTm->tm_hour = 0;
//	pTm->tm_min = 0;
//	pTm->tm_sec = 0;
//	timeNow = mktime(pTm);//�����Data����
//	BYTE CheckMonth = pTm->tm_mon;
//
//	time_t timeCheck = time(NULL);
//	tm* pTmCheck = localtime(&timeCheck);
//	if (!pTmCheck)
//	{
//		ASSERT(false);
//		return false;
//	}
//	pTmCheck->tm_year = m_RoleCheckInfo.bLastLogYear + 100;
//	pTmCheck->tm_mon = m_RoleCheckInfo.bLastLogMonth - 1;
//	pTmCheck->tm_mday = m_RoleCheckInfo.bLastLogDay;
//	pTmCheck->tm_hour = 0;
//	pTmCheck->tm_min = 0;
//	pTmCheck->tm_sec = 0;
//	timeCheck = mktime(pTmCheck);
//
//	bool IsChange = false;
//	//if (timeNow - timeCheck < 0 || timeNow - timeCheck > 24 * 60 * 60)
//	//{
//	//	m_RoleCheckInfo.bCheckSum = 0;//����ǩ�������޸�Ϊ0
//	//	IsChange = true;
//	//}
//	if (CheckMonth != m_RoleCheckInfo.bLastLogMonth - 1)
//	{
//		//�·ݱ仯�� 
//		m_RoleCheckInfo.MonthCheckInfo = 0;//���·ݱ仯��ʱ�� ���
//		IsChange = true;
//	}
//	return IsChange;
//}
//bool RoleCheck::GetRoleCheckInfo()
//{
//	if (!m_Role)
//	{
//		ASSERT(false);
//		return false;
//	}
//	LC_Cmd_GetRoleCheckInfo msg;
//	SetMsgInfo(msg,GetMsgType(Main_Check, LC_GetRoleCheckInfo), sizeof(LC_Cmd_GetRoleCheckInfo));
//	msg.CheckInfo = m_RoleCheckInfo;
//	m_Role->SendDataToClient(&msg);
//	return true;
//}

//void RoleCheck::OnDayChange()
//{
	//if (!m_Role)
	//{
	//	ASSERT(false);
	//	return;
	//}
	//m_Role->ChangeRoleCheck(false);
	//m_Role->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Check, true);
	//if (HandleCheckCheckInfo())
	//{
	//	DBR_Cmd_ChangeRoleCheckInfo msgDB;
	//	SetMsgInfo(msgDB,DBR_ChangeRoleCheckInfo, sizeof(DBR_Cmd_ChangeRoleCheckInfo));
	//	msgDB.dwUserID = m_Role->GetUserID();
	//	msgDB.CheckInfo = m_RoleCheckInfo;
	//	g_FishServer.SendNetCmdToDB(&msgDB);
	//	//֪ͨ�ͻ���ȥ
	//	LC_Cmd_CheckChange msg;
	//	SetMsgInfo(msg,GetMsgType(Main_Check, LC_CheckChange), sizeof(LC_Cmd_CheckChange));
	//	msg.CheckInfo = m_RoleCheckInfo;
	//	m_Role->SendDataToClient(&msg);
	//}
//}

bool RoleCheck::IsCanCheckNowDay()
{
	return !(m_Role->GetRoleInfo().IsCheckToday);
	////�жϽ����Ƿ����ǩ��
	////SYSTEMTIME pNowTime;
	////GetLocalTime(&pNowTime);

	//time_t NowTime = time(null);
	//NowTime -= g_FishServer.GetFishConfig().GetWriteSec();
	//tm Now;
	//errno_t Err = localtime_s(&Now, &NowTime);
	//if (Err != 0)
	//{
	//	ASSERT(false);
	//	return false;
	//}
	//DWORD CheckData = m_Role->GetRoleInfo().CheckData;
	////DWORD MonthDay = Now.tm_mday;//1-31
	////DWORD  WeekCheck = 16;// GetWeek();  ��16λ��ʾ�����Ƿ�ǩ��
	//return ((CheckData& (1 << CheckSign)) == 0);
}


DWORD RoleCheck::GetWeekCheckNum()//����ǩ���ۼƴ���
{
	return m_Role->GetRoleInfo().CheckData;
	//CheckData = CheckSign&(~((1 << CheckSign)));
	//return CheckData;
	//time_t NowTime = time(null);
	//NowTime -= g_FishServer.GetFishConfig().GetWriteSec();
	//tm Now;
	//errno_t Err = localtime_s(&Now, &NowTime);
	//if (Err != 0)
	//{
	//	ASSERT(false);
	//	return false;
	//}
	////DWORD CheckData = m_Role->GetRoleInfo().CheckData;
	////DWORD MonthDay = Now.tm_mday;//1-31
	//DWORD  Week = Now.tm_wday == 0 ? 7 : Now.tm_wday;
	//return Week;
}
BYTE RoleCheck::GetMonthCheckSum()
{
	return m_Role->GetRoleInfo().AddupCheckNum;
	//BYTE MonthSum = 0;
	//DWORD CheckData = m_Role->GetRoleInfo().CheckData;
	//for (int i = 1; i <= 31; ++i)
	//{
	//	if ((CheckData & (1 << i)) != 0)
	//		++MonthSum;
	//}
	//return MonthSum;
}
bool RoleCheck::RoleChecking()
{
	if (!m_Role)
	{
		ASSERT(false);
		return false;
	}
	//time_t NowTime = time(null);
	//NowTime -= g_FishServer.GetFishConfig().GetWriteSec();
	//tm Now;
	//errno_t Err = localtime_s(&Now, &NowTime);
	//if (Err != 0)
	//{
	//	ASSERT(false);
	//	return false;
	//}
	if (!IsCanCheckNowDay())
	{
		//���첻����ǩ��
		LC_Cmd_CheckNowDay msg;
		SetMsgInfo(msg, GetMsgType(Main_Check, LC_CheckNowDay), sizeof(LC_Cmd_CheckNowDay));
		msg.Result = false;
		m_Role->SendDataToClient(&msg);
		return true;
	}
	////�������ǩ�� ���ǽ�������
	//DWORD CheckData = m_Role->GetRoleInfo().CheckData == 7 ? 1 : (m_Role->GetRoleInfo().CheckData + 1);//��ǩ����һ ������ѭ��

	//CheckData = m_Role->GetRoleInfo().CheckData  | (1 << CheckSign);//�Ѿ�ǩ��
	//m_Role->ChangeRoleCheckData(CheckData);//�޸�ǩ������

	m_Role->ChangeRoleCheck(true);

	//���Ǹ������� ���轱�� 
	LC_Cmd_CheckNowDay msgCheck;//ǩ���Ľ��
	SetMsgInfo(msgCheck, GetMsgType(Main_Check, LC_CheckNowDay), sizeof(LC_Cmd_CheckNowDay));
	msgCheck.Result = true;
	msgCheck.DayRewardID = 0;
	msgCheck.MonthRewardID = 0;
	//1.����Ľ��� 
	HashMap<DWORD, WORD>::iterator Iter = g_FishServer.GetFishConfig().GetCheckConfig().CheckDayReward.find(GetWeekCheckNum());
	if (Iter != g_FishServer.GetFishConfig().GetCheckConfig().CheckDayReward.end())
	{
		m_Role->OnAddRoleRewardByRewardID(Iter->second,TEXT("��ȡǩ�����콱��"));
		msgCheck.DayRewardID = Iter->second;
	}
	//2.����ǩ���Ľ���
	Iter = g_FishServer.GetFishConfig().GetCheckConfig().MonthCheckReward.find(GetMonthCheckSum());
	if (Iter != g_FishServer.GetFishConfig().GetCheckConfig().MonthCheckReward.end())
	{
		m_Role->OnAddRoleRewardByRewardID(Iter->second, TEXT("��ȡǩ��������������"));
		msgCheck.MonthRewardID = Iter->second;
	}
	m_Role->SendDataToClient(&msgCheck);

	//m_Role->ChangeRoleNobilityPoint(10);//�����ҹ������

	return true;
}
//bool RoleCheck::RoleCheckeOnther(BYTE DaySum)
//{
//	//��ҽ��е��µĲ�ǩ
//	//1.�ж�����ֱ���
//	if (!m_Role)
//	{
//		ASSERT(false);
//		return false;
//	}
//	//��ǩָ��������
//	time_t NowTime = time(null);
//	NowTime -= g_FishServer.GetFishConfig().GetWriteSec();
//	tm Now;
//	errno_t Err = localtime_s(&Now, &NowTime);
//	if (Err != 0)
//	{
//		ASSERT(false);
//		return false;
//	}
//	if (m_Role->GetRoleInfo().dwCurrencyNum < g_FishServer.GetFishConfig().GetCheckConfig().CheckOtherUser)
//	{
//		LC_Cmd_CheckOtherDay msg;
//		SetMsgInfo(msg, GetMsgType(Main_Check, LC_CheckOtherDay), sizeof(LC_Cmd_CheckOtherDay));
//		msg.Result = false;
//		m_Role->SendDataToClient(&msg);
//		return true;
//	}
//	BYTE MonthCheckSum = GetMonthCheckSum();//����ǩ���Ĵ���
//	if (IsCanCheckNowDay())
//		MonthCheckSum += 1;
//	if (Now.tm_mday == MonthCheckSum)
//	{
//		LC_Cmd_CheckOtherDay msg;
//		SetMsgInfo(msg, GetMsgType(Main_Check, LC_CheckOtherDay), sizeof(LC_Cmd_CheckOtherDay));
//		msg.Result = false;
//		m_Role->SendDataToClient(&msg);
//		return true;
//	}
//	else if (MonthCheckSum > Now.tm_mday)
//	{
//		ASSERT(false);
//		return false;
//	}
//	else if (DaySum >= Now.tm_mday)
//	{
//		ASSERT(false);
//		return false;
//	}
//	else
//	{
//		//�ж�ָ�������Ƿ���Բ�ǩ
//		DWORD CheckData = m_Role->GetRoleInfo().CheckData;
//		if (DaySum == Now.tm_mday || (CheckData & (1 << DaySum)) != 0)
//		{
//			LC_Cmd_CheckOtherDay msg;
//			SetMsgInfo(msg, GetMsgType(Main_Check, LC_CheckOtherDay), sizeof(LC_Cmd_CheckOtherDay));
//			msg.Result = false;
//			m_Role->SendDataToClient(&msg);
//			return true;
//		}
//		//���в�ǩ
//		//1.��Ǯ
//		m_Role->ChangeRoleCurrency(g_FishServer.GetFishConfig().GetCheckConfig().CheckOtherUser*-1,TEXT("���в�ǩ �۳��ֱ�"));
//
//		//��ǰûǩ�� 
//		CheckData |= (1 << DaySum);
//		m_Role->ChangeRoleCheckData(CheckData);//�޸�ǩ������
//		//���轱��
//		LC_Cmd_CheckOtherDay msg;
//		SetMsgInfo(msg, GetMsgType(Main_Check, LC_CheckOtherDay), sizeof(LC_Cmd_CheckOtherDay));
//		msg.Result = true;
//		msg.Day = DaySum;
//		msg.DayRewardID = 0;
//		msg.MonthRewardID = 0;
//
//		HashMap<DWORD, WORD>::iterator Iter = g_FishServer.GetFishConfig().GetCheckConfig().CheckDayReward.find(DaySum);
//		if (Iter != g_FishServer.GetFishConfig().GetCheckConfig().CheckDayReward.end())
//		{
//			m_Role->OnAddRoleRewardByRewardID(Iter->second, TEXT("��ȡ��ǩ���콱��"));
//			msg.DayRewardID = Iter->second;
//		}
//		Iter = g_FishServer.GetFishConfig().GetCheckConfig().MonthCheckReward.find(GetMonthCheckSum());
//		if (Iter != g_FishServer.GetFishConfig().GetCheckConfig().MonthCheckReward.end())
//		{
//			m_Role->OnAddRoleRewardByRewardID(Iter->second, TEXT("��ȡ��ǩ�·�������������"));
//			msg.MonthRewardID = Iter->second;
//		}
//		m_Role->SendDataToClient(&msg);
//		return true;
//	}
//}
bool RoleCheck::GetCheckMessageStates()
{
	return IsCanCheckNowDay();
}