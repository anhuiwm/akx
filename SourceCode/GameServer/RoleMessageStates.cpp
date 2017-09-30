#include "StdAfx.h"
#include "RoleMessageStates.h"
#include "RoleEx.h"
RoleMessageStates::RoleMessageStates()
{
	m_IsInit = false;
}
RoleMessageStates::~RoleMessageStates()
{

}
void RoleMessageStates::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	m_pRole = pRole;
	m_StatesValue = 0;
	m_IsInit = true;
	//���ݸ������������������յ�ֵ
	OnChangeRoleMessageStates(RMT_Mail, false);
	OnChangeRoleMessageStates(RMT_WeekRank, false);
	OnChangeRoleMessageStates(RMT_Giff, false);
	OnChangeRoleMessageStates(RMT_Task, false);
	OnChangeRoleMessageStates(RMT_Achievement, false);
	OnChangeRoleMessageStates(RMT_Action, false);
	OnChangeRoleMessageStates(RMT_Check, false);
	OnChangeRoleMessageStates(RMT_Char, false);
	OnChangeRoleMessageStates(RMT_Relation, false);
	OnChangeRoleMessageStates(RMT_WeekTask, false);
	OnChangeRoleMessageStates(RMT_Forge, false);
	//�������Զ����͵��ͻ���ȥ
	LC_Cmd_RoleMessageChange msg;
	SetMsgInfo(msg, GetMsgType(Main_RoleMessage, LC_RoleMessageChange), sizeof(LC_Cmd_RoleMessageChange));
	msg.RoleMessageData = m_StatesValue;
	m_pRole->SendDataToClient(&msg);
}
void RoleMessageStates::OnChangeRoleMessageStates(RoleMessageType Type, bool IsSendToClient, bool Once)
{
	if (!m_pRole)
	{
		//ASSERT(false);
		return;
	}
	if (!m_IsInit)
		return;//δ��ʼ�� 
	DWORD OldStates = m_StatesValue;
	//��ָ�����ͱ仯��ʱ�� �������»�ȡ���͵�������
	switch (Type)
	{
	case RMT_Mail:
		{
			bool States = m_pRole->GetMailManager().GetMailMessageStates();
			if ((m_StatesValue & RMT_Mail) !=0)
				m_StatesValue ^= RMT_Mail;
			if (States)
				m_StatesValue |= RMT_Mail;
		}
		break;
	case RMT_WeekRank:
		{
			//bool States = m_pRole->GetRoleRank().GetRankMessageStates();
			//if ((m_StatesValue & RMT_WeekRank) != 0)
			//	m_StatesValue ^= RMT_WeekRank;
			//if (States)
			//	m_StatesValue |= RMT_WeekRank;
		}
		break;
	case RMT_Giff:
		{
			bool States = m_pRole->GetRoleGiffManager().GetGiffMessageStates();
			if ((m_StatesValue & RMT_Giff) != 0)
				m_StatesValue ^= RMT_Giff;
			if (States)
				m_StatesValue |= RMT_Giff;
		}
		break;
	case RMT_Task:
		{
			bool States = m_pRole->GetRoleTaskManager().GetTaskMessageStates();
			if ((m_StatesValue & RMT_Task) != 0)
				m_StatesValue ^= RMT_Task;
			if (States)
				m_StatesValue |= RMT_Task;
		}
		break;
	case RMT_Achievement:
		{
			bool States = m_pRole->GetRoleAchievementManager().GetAchievementMessageStates();
			if ((m_StatesValue & RMT_Achievement) != 0)
				m_StatesValue ^= RMT_Achievement;
			if (States)
				m_StatesValue |= RMT_Achievement;
		}
		break;
	case RMT_Action:
		{
			bool States = m_pRole->GetRoleActionManager().GetActionMessageStates();
			if ((m_StatesValue & RMT_Action) != 0)
				m_StatesValue ^= RMT_Action;
			if (States)
				m_StatesValue |= RMT_Action;
		}
		break;
	case RMT_Check:
		{
			bool States = m_pRole->GetRoleCheckManager().GetCheckMessageStates();
			if ((m_StatesValue & RMT_Check) != 0)
				m_StatesValue ^= RMT_Check;
			if (States)
				m_StatesValue |= RMT_Check;
		}
		break;
	case RMT_Char:
		{
			bool States = m_pRole->GetRoleCharManager().GetCharMessageStates();
			if ((m_StatesValue & RMT_Char) != 0)
				m_StatesValue ^= RMT_Char;
			if (States)
				m_StatesValue |= RMT_Char;
		}
		break;
	case RMT_Relation:
		{
			bool States = m_pRole->GetRoleRelationRequest().GetRelationRequestMessageStates();
			if ((m_StatesValue & RMT_Relation) != 0)
				m_StatesValue ^= RMT_Relation;
			if (States)
				m_StatesValue |= RMT_Relation;
		}
	case RMT_Online:
	   {
		    bool States = m_pRole->GetOnlineRewardMessageStates();
		    if ((m_StatesValue & RMT_Online) != 0)
		    	m_StatesValue ^= RMT_Online;
		    if (States)
		    	m_StatesValue |= RMT_Online;
	   }
	   break;
	case RMT_WeekTask:
	{
		bool States = m_pRole->GetRoleTaskManager().GetWeekTaskMessageStates();
		if ((m_StatesValue & RMT_WeekTask) != 0)
			m_StatesValue ^= RMT_WeekTask;
		if (States)
			m_StatesValue |= RMT_WeekTask;
	}
	break;
	case RMT_Forge:
	{
		bool States = Once;
		if ((m_StatesValue & RMT_Forge) != 0)
			m_StatesValue ^= RMT_Forge;
		if (States)
			m_StatesValue |= RMT_Forge;
	}
	break;
	default: 
	{
		return;
	}
	}
	//���ֵ�仯�� ��������ͻ���ȥ 
	if (OldStates == m_StatesValue)
		return;

	LogInfoToFile("WmRmt.txt", "m_StatesValue=%x"	, m_StatesValue);
	//�����͵��ͻ���ȥ
	if (IsSendToClient)
	{
		LC_Cmd_RoleMessageChange msg;
		SetMsgInfo(msg, GetMsgType(Main_RoleMessage, LC_RoleMessageChange), sizeof(LC_Cmd_RoleMessageChange));
		msg.RoleMessageData = m_StatesValue;
		m_pRole->SendDataToClient(&msg);
	}

	if (Once)//һ��������0
	{
		if ((m_StatesValue & Type) != 0)
			m_StatesValue ^= Type;
	}
}