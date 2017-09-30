#include "Stdafx.h"
#include "RoleRate.h"
#include "FishServer.h"
RoleRate::RoleRate()
{

}
RoleRate::~RoleRate()
{

}
bool RoleRate::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;

	//��ʼ����ҵı���
	DWORD InitRateValue = g_FishServer.GetFishConfig().GetSystemConfig().InitRateValue;
	bool IsChange = false;
	for (BYTE i = 0; i < 32; ++i)//��ҵ�Ĭ�ϱ��� ֻ������ǰ32��
	{
		if ((InitRateValue& (1 << i)) != 0 && !int256Handle::GetBitStates(m_pRole->GetRoleInfo().RateValue, i))
		{
			IsChange = true;
			int256Handle::SetBitStates(m_pRole->GetRoleInfo().RateValue, i, true);
		}
		else if ((InitRateValue& (1 << i)) == 0)
			break;
	}
	
	//���ʼ�����Ϻ� ���д���
	m_MinRateIndex = 0;
	m_MaxRateIndex = 0;
	if (m_pRole->IsRobot())
	{
		m_MaxRateIndex = RandRange(45, 50);
	}
	else
	{
		for (BYTE i = 0; i < g_FishServer.GetTableManager()->GetGameConfig()->RateCount(); ++i)
		{
			if (IsCanUseRateIndex(i))
			{
				if (i >= m_MaxRateIndex)
					m_MaxRateIndex = i;

				if (m_MinRateIndex >= i)
					m_MinRateIndex = i;
			}
			else
			{
				break;
			}
		}
	}
	GetInitRateReward();//�ж�����Ƿ���Ҫ��������
	//��int256 �������
	for (BYTE i = m_MaxRateIndex + 1; i < 255; ++i)//���������� ��ֹ�쳣����
	{
		if (IsCanUseRateIndex(i))
		{
			int256Handle::SetBitStates(m_pRole->GetRoleInfo().RateValue, i, false);
			IsChange = true;
		}
	}
	if (IsChange)
	{
		//������ҵı��ʵ����ݿ�ȥ
		DBR_Cmd_SaveRoleRateValue msgDB;
		SetMsgInfo(msgDB, DBR_SaveRoleRateValue, sizeof(DBR_Cmd_SaveRoleRateValue));
		msgDB.UserID = m_pRole->GetUserID();
		msgDB.RateValue = m_pRole->GetRoleInfo().RateValue;
		msgDB.MaxRateValue = m_MaxRateIndex;
		g_FishServer.SendNetCmdToSaveDB(&msgDB);
	}
	else
	{
		m_pRole->ChangeRoleMaxRate(m_MaxRateIndex,false);
		//����������������� ��ֹ�����ǰδ��½���� ����
		//DBR_Cmd_SaveRoleMaxRateValue msgDB;
		//SetMsgInfo(msgDB, DBR_SaveRoleMaxRateValue, sizeof(DBR_Cmd_SaveRoleMaxRateValue));
		//msgDB.dwUserID = m_pRole->GetUserID();
		//msgDB.MaxRate = m_MaxRateIndex;
		//g_FishServer.SendNetCmdToSaveDB(&msgDB);
	}
	return true;
}
void RoleRate::GetInitRateReward()
{
	time_t Nowtime = m_pRole->GetLastOnLineTime();
	time_t LogTime = g_FishServer.GetFishConfig().GetSystemConfig().RateInitRewardTime;
	if (Nowtime < LogTime)
	{
		tagRoleMail	MailInfo;
		MailInfo.bIsRead = false;
		//������������Ҫ����Ĵ��� ������Ҫһ�� �����ת���ַ��� �ͻ��� �� ������ͨ�õ�
		MailInfo.MailID = 0;
		MailInfo.SendTimeLog = time(NULL);
		MailInfo.SrcFaceID = 0;
		TCHARCopy(MailInfo.SrcNickName, CountArray(MailInfo.SrcNickName), TEXT(""), 0);
		MailInfo.SrcUserID = 0;//ϵͳ����
		MailInfo.bIsExistsReward = (MailInfo.RewardID != 0 && MailInfo.RewardSum != 0);
		DBR_Cmd_AddUserMail msg;
		SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
		msg.dwDestUserID = m_pRole->GetUserID();
		//�����֮ǰ��½�� ���д���
		for (BYTE i = m_MinRateIndex; i <= m_MaxRateIndex; ++i)
		{
			WORD RewardID = g_FishServer.GetTableManager()->GetGameConfig()->UnlockRateReward(i);
			if (RewardID == 0)
				continue;
			WORD RateSum = g_FishServer.GetTableManager()->GetGameConfig()->BulletMultiple(i);
			//�����ʼ�
			_sntprintf_s(MailInfo.Context, CountArray(MailInfo.Context), TEXT("����%u�����ʽ�������"), RateSum);
			MailInfo.RewardID = RewardID;
			MailInfo.RewardSum = 1;
			msg.MailInfo = MailInfo;
			g_FishServer.SendNetCmdToDB(&msg);
		}
	}
}
bool RoleRate::OnChangeRoleRate(BYTE AddRateIndex)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//�޸���ҵı���
	if (AddRateIndex <= m_MaxRateIndex)
	{
		ASSERT(false);
		return false;
	}
	//�ж��Ƿ����ʹ��
	if (IsCanUseRateIndex(AddRateIndex))
	{
		ASSERT(false);
		return false;
	}
	//�����µı���
	for (BYTE i = m_MaxRateIndex + 1; i <= AddRateIndex; ++i)
	{
		int256Handle::SetBitStates(m_pRole->GetRoleInfo().RateValue, i, 1);

		WORD RewardID = g_FishServer.GetTableManager()->GetGameConfig()->UnlockRateReward(i);

		if (RewardID != 0 )
		{
			m_pRole->OnAddRoleRewardByRewardID(RewardID, TEXT("�������ʻ�ý���"));
		}
	}

	if (AddRateIndex == g_FishServer.GetTableManager()->GetGameConfig()->GetRateQianPaoID())//�ﵽǧ��֪ͨ����
	{
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Forge, true, true);
	}

	m_MaxRateIndex = AddRateIndex;//����ʹ�õ���߱���
		
	LC_Cmd_ChangeRoleRateValue msg;
	SetMsgInfo(msg, GetMsgType(Main_Role, LC_ChangeRoleRateValue), sizeof(LC_Cmd_ChangeRoleRateValue));
	msg.RateValue = m_pRole->GetRoleInfo().RateValue;
	msg.OpenRateIndex = m_MaxRateIndex;
	m_pRole->SendDataToClient(&msg);
	//����������ݿ�ȥ
	DBR_Cmd_SaveRoleRateValue msgDB;
	SetMsgInfo(msgDB, DBR_SaveRoleRateValue, sizeof(DBR_Cmd_SaveRoleRateValue));
	msgDB.UserID = m_pRole->GetUserID();
	msgDB.RateValue = m_pRole->GetRoleInfo().RateValue;
	msgDB.MaxRateValue = m_MaxRateIndex;
	g_FishServer.SendNetCmdToSaveDB(&msgDB);

	return true;
}
bool RoleRate::IsCanUseRateIndex(BYTE RateIndex)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	if (m_pRole->IsRobot())
	{
		return true;
		//UINT maxRate = RandRange(5, 13);
		//return (RateIndex <= maxRate);
	}
	return int256Handle::GetBitStates(m_pRole->GetRoleInfo().RateValue, RateIndex);//�ж�ָ�������Ƿ����ʹ��
}
BYTE RoleRate::GetCanShowMaxRate()
{
	//��ȡ������ʾ����߱���
	BYTE MaxRate = g_FishServer.GetTableManager()->GetGameConfig()->RateCount() - 1;
	BYTE ShowRate = m_MaxRateIndex + 1;
	return min(MaxRate, ShowRate);
}
void RoleRate::ResetRateInfo()
{
	//���������Ѿ���������������
	m_MinRateIndex = 0;
	m_MaxRateIndex = 0;
	for (BYTE i = 0; i < g_FishServer.GetTableManager()->GetGameConfig()->RateCount(); ++i)
	{
		if (IsCanUseRateIndex(i))
		{
			if (i >= m_MaxRateIndex)
				m_MaxRateIndex = i;

			if (m_MinRateIndex >= i)
				m_MinRateIndex = i;
		}
		else
		{
			break;
		}
	}
}