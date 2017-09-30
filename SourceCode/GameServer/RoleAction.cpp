#include "Stdafx.h"
#include "RoleAction.h"
#include "RoleEx.h"
#include "FishServer.h"
RoleActionManager::RoleActionManager()
{
	m_IsLoadDB = false;
	m_IsSendClient = false;
	m_pRole = NULL;
	m_ActionVec.clear();
	int256Handle::Clear(m_JoinActionLog);
}
RoleActionManager::~RoleActionManager()
{
	//SaveAction();//�Ƚ���ǰ���ڽ��е�����ȫ������һ��
	OnDestroy();//ɾ������
}
//void RoleActionManager::OnSaveByUpdate()
//{
//	//��������û15���ӱ���һ��
//	SaveAction();
//}
RoleActionBase* RoleActionManager::QueryAction(BYTE ActionID)
{
	//����
	if (m_ActionVec.empty())
		return NULL;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); ++Iter)
	{
		if (!(*Iter))
		{
			ASSERT(false);
			continue;
		}
		if ((*Iter)->GetActionID() == ActionID)
		{
			return *Iter;
		}
	}
	return NULL;
}
void RoleActionManager::UpdateByHour()//ÿСʱ 0���ӿ�ʼ��׼����
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//��Сʱ�Ե�ǰ�Ļ�ý��и���
	if (m_ActionVec.size() == 0)
		return;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end();)
	{
		//�жϻ�Ƿ���ʱ���� ���ڵĻ� �����ɾ����
		if (!(*Iter))
		{
			ASSERT(false);
			++Iter;
			continue;
		}
		if ((*Iter)->IsInTime())
		{
			++Iter;
			continue;
		}
		DBR_Cmd_DelRoleAction msg;
		SetMsgInfo(msg, DBR_DelRoleAction, sizeof(DBR_Cmd_DelRoleAction));
		msg.dwUserID = m_pRole->GetUserID();
		msg.bActionID = (*Iter)->GetActionID();
		g_FishServer.SendNetCmdToSaveDB(&msg);
		//4.������ɾ����
		int256Handle::SetBitStates(m_JoinActionLog, (*Iter)->GetActionID(), false);

		if (m_IsSendClient)
		{
			LC_Cmd_DelAction msgDel;
			SetMsgInfo(msgDel, GetMsgType(Main_Action, LC_DelAction), sizeof(LC_Cmd_DelAction));
			msgDel.ActionID = (*Iter)->GetActionID();
			m_pRole->SendDataToClient(&msgDel);
		}
		delete *Iter;
		Iter = m_ActionVec.erase(Iter);
	}
	//������Ϻ� ������Ҫ��������½�ȡ�
	OnJoinActionByConfig(false);
}

void RoleActionManager::OnDayChange()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//��Сʱ�Ե�ǰ�Ļ�ý��и���
	if (m_ActionVec.size() == 0)
		return;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end();)
	{
		//�жϻ�Ƿ���ʱ���� ���ڵĻ� �����ɾ����
		if (!(*Iter))
		{
			ASSERT(false);
			++Iter;
			continue;
		}
		//if ((*Iter)->IsInTime())
		//{
		//	++Iter;
		//	continue;
		//}
		HashMap<BYTE, tagActionConfig>::iterator IterCfg = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find((*Iter)->GetActionID());
		if (IterCfg->second.Sign != DAY_ACTION)//�����ճ��
		{
			++Iter;
			continue;
		}

		DBR_Cmd_DelRoleAction msg;
		SetMsgInfo(msg, DBR_DelRoleAction, sizeof(DBR_Cmd_DelRoleAction));
		msg.dwUserID = m_pRole->GetUserID();
		msg.bActionID = (*Iter)->GetActionID();
		g_FishServer.SendNetCmdToSaveDB(&msg);
		//4.������ɾ����
		int256Handle::SetBitStates(m_JoinActionLog, (*Iter)->GetActionID(), false);

		m_pRole->ChangeRoleActionStates((*Iter)->GetActionID(), false);//ָ����Ѿ������ ����ȡ����
		if (m_IsSendClient)
		{
			LC_Cmd_DelAction msgDel;
			SetMsgInfo(msgDel, GetMsgType(Main_Action, LC_DelAction), sizeof(LC_Cmd_DelAction));
			msgDel.ActionID = (*Iter)->GetActionID();
			m_pRole->SendDataToClient(&msgDel);
		}
		delete *Iter;
		Iter = m_ActionVec.erase(Iter);
	}
}
bool RoleActionManager::IsCanJoinAction(BYTE ActionID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//�Ƿ���Խ�ȡָ��������
	//0.����������Ҫ��
	if (m_ActionVec.size() >= g_FishServer.GetFishConfig().GetActionConfig().m_MaxJoinActionSum)
	{
		return false;
	}
	//�ж� �����Ƿ��Ѿ���ȡ��  ���ǵ�ǰVector�����Ƿ��е�ǰ��ID�Ĵ���
	if (int256Handle::GetBitStates(m_JoinActionLog, ActionID))
	{
		return false;
	}
	//1.�ж������Ƿ��Ѿ������
	if (int256Handle::GetBitStates(m_pRole->GetRoleInfo().ActionStates, ActionID))
	{
		return false;//�����Ѿ������
	}
	//2.�ȼ�����Ҫ��
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(ActionID);
	if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (!Iter->second.IsInTime())
		return false;
	return true;
}
void RoleActionManager::OnDestroy()
{
	if (m_ActionVec.size() == 0)
		return;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); ++Iter)
	{
		delete (*Iter);
	}
	m_ActionVec.clear();
	int256Handle::Clear(m_JoinActionLog);
}
//void RoleActionManager::SaveAction()
//{
//	//���ֵ�ǰȫ��������
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return;
//	}
//	if (m_ActionVec.size() == 0)
//		return;
//	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
//	for (; Iter != m_ActionVec.end(); ++Iter)
//	{
//		if (!(*Iter)->IsNeedSave())
//			continue;
//		DBR_Cmd_SaveRoleAction msg;
//		SetMsgInfo(msg, DBR_SaveRoleAction, sizeof(DBR_Cmd_SaveRoleAction));
//		msg.dwUserID = m_pRole->GetUserID();
//		msg.ActionInfo = (*Iter)->GetActionInfo();
//		g_FishServer.SendNetCmdToDB(&msg);
//	}
//}
void RoleActionManager::GetAllNeedSaveAction(vector<tagRoleActionInfo>& pVec)
{
	pVec.clear();
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (m_ActionVec.size() == 0)
		return;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		if (!(*Iter)->IsNeedSave())
			continue;
		pVec.push_back((*Iter)->GetActionInfo());
		(*Iter)->OnSave();
	}
}
bool RoleActionManager::GetActionMessageStates()
{
	

	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); /*++Iter*/)
	{
		if (!(*Iter))
		{
			continue;
			++Iter;
		}

		if ((*Iter)->IsExistsFinishEvent() && (*Iter)->GetActionID() != 101)
		{
			
			//if ((*Iter)->GetActionID() == 101)
			//{
				//���������� m_pRole->GetRoleActionManager().OnFinishAction(101, 1);//�׳��Զ���ȡ
			//	Iter = m_ActionVec.begin();
			//	continue;
			//}
			return true;
		}
		++Iter;
	}
	return false;
}
void RoleActionManager::OnResetJoinAllAction()
{
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		(*Iter)->OnJoinAction();
	}
}
bool RoleActionManager::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	SendLoadAllActionInfoToDB();
	return true;
}
void RoleActionManager::SendLoadAllActionInfoToDB()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//DBR_Cmd_LoadRoleAction msg;
	//SetMsgInfo(msg, DBR_LoadRoleAction, sizeof(DBR_Cmd_LoadRoleAction));
	//msg.dwUserID = m_pRole->GetUserID();
	//g_FishServer.SendNetCmdToDB(&msg);
}
void RoleActionManager::OnLoadAllActionInfoByDB(DBO_Cmd_LoadRoleAction* pDB)
{
	if (!pDB || !m_pRole)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_ActionVec.clear();
	}
	for (WORD i = 0; i < pDB->Sum; ++i)
	{
		//UserID ActionID Param ��������ݿ�� 3�������Ϳ�����
		HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(pDB->Array[i].ActionID);
		if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end() || !IsCanJoinAction(pDB->Array[i].ActionID))
		{
			//������ɾ����
			DBR_Cmd_DelRoleAction msg;
			SetMsgInfo(msg, DBR_DelRoleAction, sizeof(DBR_Cmd_DelRoleAction));
			msg.dwUserID = m_pRole->GetUserID();
			msg.bActionID = pDB->Array[i].ActionID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			continue;
		}
		else
		{
			//����һ������
			RoleActionBase* pBase = CreateActionByEventID(Iter->second.EventID);//�����¼�ID ����һ���������
			if (!pBase || !pBase->OnInit(m_pRole, this, &pDB->Array[i]))
			{
				ASSERT(false);
				continue;
			}
			//���뼯������ȥ
			m_ActionVec.push_back(pBase);
			int256Handle::SetBitStates(m_JoinActionLog, pDB->Array[i].ActionID, true);
			continue;
		}
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		if (!m_pRole->IsOnceDayOnline())
		{
			OnDayChange();
		}
		OnJoinActionByConfig(false);
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Action);
		m_IsLoadDB = true;
		m_pRole->IsLoadFinish();
	}
}
void RoleActionManager::SendAllActionToClient()
{
	//����ǰ���ڽ��е������͵��ͻ���ȥ
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//�ͻ������������ѯ��ȫ����������Ϣ 
	DWORD PageSize = sizeof(LC_Cmd_GetRoleActionInfo)+(m_ActionVec.size() - 1)*sizeof(tagRoleActionInfo);
	LC_Cmd_GetRoleActionInfo * msg = (LC_Cmd_GetRoleActionInfo*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	msg->SetCmdType(GetMsgType(Main_Action, LC_GetRoleActionInfo));
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (WORD i = 0; Iter != m_ActionVec.end(); ++Iter, ++i)
	{
		if (!(*Iter))
			continue;
		msg->Array[i] = (*Iter)->GetActionInfo();
	}
	std::vector<LC_Cmd_GetRoleActionInfo*> pVec;
	SqlitMsg(msg, PageSize, m_ActionVec.size(),true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetRoleActionInfo*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_IsSendClient = true;
}
void RoleActionManager::OnRoleLevelChange()
{
	OnJoinActionByConfig(false);
}
void RoleActionManager::OnJoinActionByConfig(bool IsNeedSave)
{
	if (m_pRole->IsRobot())
	{
		return;
	}

	if (m_ActionVec.size() >= g_FishServer.GetFishConfig().GetActionConfig().m_MaxJoinActionSum)//���������޷���ȡ
		return;
	//���Խ�ȡ����
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end(); ++Iter)
	{
		if (m_ActionVec.size() < g_FishServer.GetFishConfig().GetActionConfig().m_MaxJoinActionSum)
			OnJoinAction(Iter->second.ActionID, IsNeedSave);
		else
			break;
	}
	if (!IsNeedSave)
	{
		//��ȡȫ�������������� ����ֱ�ӽ��б�������
		if (!m_pRole)
		{
			ASSERT(false);
			return;
		}
		vector<tagRoleActionInfo> pVec;
		GetAllNeedSaveAction(pVec);
		if (pVec.empty())
			return;
		//����һ�����ȫ�������񱣴�����
		DWORD PageSize = sizeof(DBR_Cmd_SaveAllAction)+(pVec.size() - 1)*sizeof(tagRoleActionInfo);
		DBR_Cmd_SaveAllAction* msg = (DBR_Cmd_SaveAllAction*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return;
		}
		msg->SetCmdType(DBR_SaveAllAction);
		msg->dwUserID = m_pRole->GetUserID();
		for (DWORD i = 0; i < pVec.size(); ++i)
		{
			msg->Array[i] = pVec[i];
		}

		std::vector<DBR_Cmd_SaveAllAction*> pVec2;
		SqlitMsg(msg, PageSize, pVec.size(),false, pVec2);
		free(msg);
		if (!pVec2.empty())
		{
			std::vector<DBR_Cmd_SaveAllAction*>::iterator Iter = pVec2.begin();
			for (; Iter != pVec2.end(); ++Iter)
			{
				g_FishServer.SendNetCmdToSaveDB(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
}
bool RoleActionManager::OnFinishAction(BYTE ActionID, DWORD ActionOnceID)//��ȡ��������ʱ��
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//��һ��������ɵ�ʱ�� ���ǽ��д���
	//1.�ж������Ƿ����
	if (!int256Handle::GetBitStates(m_JoinActionLog, ActionID))
	{
		return false;
	}
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(ActionID);
	if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		return false;
	}
	std::vector<RoleActionBase*>::iterator IterAction = m_ActionVec.begin();
	for (; IterAction != m_ActionVec.end(); ++IterAction)
	{
		if (!(*IterAction))
			continue;
		if ((*IterAction)->GetActionID() == ActionID)
		{
			//��ȡȫ���ĵ�ǰ������ȡ�Ļ�Ľ���
			HashMap<DWORD, tagActionEventConfig>::iterator IterReward = Iter->second.EventMap.find(ActionOnceID);
			if (IterReward == Iter->second.EventMap.end())
				return false;
			if ((*IterAction)->GetActionInfo().ActionValue < IterReward->second.FinishValue || ((*IterAction)->GetActionInfo().ActionStates & IterReward->first) != 0)
				return false;
			m_pRole->OnAddRoleRewardByRewardID(IterReward->second.RewardID,TEXT("��ɻ�׶ν���"));//��һ�ý���
			(*IterAction)->GetActionInfo().ActionStates |= IterReward->first;//��������Ѿ���ȡ�˵�ǰ�Ľ���
			if (Iter->second.FinishValue != (*IterAction)->GetActionInfo().ActionStates)
			{
				//����Ҫɾ��� 
				//���� �� ��ǰ���� �׶��Եı��� �����״̬
				DBR_Cmd_SaveRoleAction msg;
				SetMsgInfo(msg, DBR_SaveRoleAction, sizeof(DBR_Cmd_SaveRoleAction));
				msg.dwUserID = m_pRole->GetUserID();
				msg.ActionInfo = (*IterAction)->GetActionInfo();
				g_FishServer.SendNetCmdToSaveDB(&msg);

				if (m_IsSendClient)
				{
					//���߿ͻ��� ָ���Ļ����̬�����仯�� ��ȡ���ֽ�����
					LC_Cmd_GetOnceActionInfo msg;
					SetMsgInfo(msg, GetMsgType(Main_Action, LC_GetOnceActionInfo), sizeof(LC_Cmd_GetOnceActionInfo));
					msg.ActionInfo = (*IterAction)->GetActionInfo();
					m_pRole->SendDataToClient(&msg);
				}
			}
			else
			{
				int256Handle::SetBitStates(m_JoinActionLog, ActionID, false);//
				m_pRole->ChangeRoleActionStates(ActionID, true);//ָ����Ѿ������ ����ȡ����
				if (m_IsSendClient)
				{
					LC_Cmd_GetActionReward msg;//���߿ͻ���ָ����Ѿ������
					SetMsgInfo(msg, GetMsgType(Main_Action, LC_GetActionReward), sizeof(LC_Cmd_GetActionReward));
					msg.ActionID = ActionID;
					m_pRole->SendDataToClient(&msg);
				}

				DBR_Cmd_DelRoleAction msgDel;
				SetMsgInfo(msgDel, DBR_DelRoleAction, sizeof(DBR_Cmd_DelRoleAction));
				msgDel.dwUserID = m_pRole->GetUserID();
				msgDel.bActionID = ActionID;
				g_FishServer.SendNetCmdToSaveDB(&msgDel);

				delete *IterAction;
				m_ActionVec.erase(IterAction);

				//7.��Ϊ��������һ������ 
				OnJoinActionByConfig(false);//�����Ҫ���½�ȡһ������ ����ȱʧ
				
			}
			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Action);
			return true;
		}
	}
	return false;
}
bool RoleActionManager::OnJoinAction(BYTE ActionID, bool IsNeedSave)//��ȡһ�������ʱ��
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	if (m_pRole->IsRobot())
	{
		return false;
	}
	//����ҽ�ȡһ���µ������ʱ�� ���ǽ��д���
	//1.�ж������Ƿ����
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(ActionID);
	if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		return false;
	}
	if (!IsCanJoinAction(ActionID))
		return false;
	RoleActionBase* pAction = CreateActionByEventID(Iter->second.EventID);
	if (!pAction)
	{
		ASSERT(false);
		return false;
	}
	m_ActionVec.push_back(pAction);
	pAction->OnInit(m_pRole, this, ActionID, IsNeedSave);//��ʼ������
	int256Handle::SetBitStates(m_JoinActionLog, ActionID, true);
	//4.��������ͻ���ȥ
	if (m_IsSendClient)
	{
		LC_Cmd_JoinAction msg;
		SetMsgInfo(msg, GetMsgType(Main_Action, LC_JoinAction), sizeof(LC_Cmd_JoinAction));
		msg.ActionID = ActionID;
		m_pRole->SendDataToClient(&msg);
	}
	if (IsNeedSave)
	{
		DBR_Cmd_SaveRoleAction msg;
		SetMsgInfo(msg, DBR_SaveRoleAction, sizeof(DBR_Cmd_SaveRoleAction));
		msg.dwUserID = m_pRole->GetUserID();
		msg.ActionInfo = pAction->GetActionInfo();
		g_FishServer.SendNetCmdToSaveDB(&msg);
	}
	return true;
}
RoleActionBase* RoleActionManager::CreateActionByEventID(BYTE EventID)
{
	switch (EventID)
	{
	case ET_GetGlobel:
		return new GetGlobelRoleAction();
	case ET_GetMadel:
		return new GetMadelRoleAction();
	case ET_GetCurren:
		return new GetCurrenRoleAction();
	case ET_UpperLevel:
		return new UpperLevelRoleAction();
	case ET_CatchFish:
		return new CatchFishRoleAction();
	case ET_SendGiff:
		return new SendGiffRoleAction();
	case ET_UseSkill:
		return new UseSkillRoleAction();
	case ET_LauncherGlobel:
		return new LauncherGlobelRoleAction();
	case ET_MaxGlobelSum:
		return new MaxGlobelSumRoleAction();
	case ET_ToLevel:
		return new ToLevelRoleAction();
	case ET_MaxCurren:
		return new MaxCurrenRoleAction();
	case ET_Recharge:
		return new RechargeRoleAction();
	case ET_Recharge_One:
		return new RechargeOneRoleAction();
	case ET_Recharge_First:
		return new RechargeFirstRoleAction();
	default:
		return NULL;
	}
}
void RoleActionManager::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (m_ActionVec.empty())
		return;
	std::vector<RoleActionBase*>::iterator Iter = m_ActionVec.begin();
	for (; Iter != m_ActionVec.end(); ++Iter)
	{
		(*Iter)->OnHandleEvent(EventID, BindParam, Param);
	}
}

//�������
RoleActionBase::RoleActionBase()
{
	m_pActionManager = NULL;
	m_IsNeedSave = false;
	//m_ActionConfig = NULL;
	m_ActionInfo.ActionID = 0;
	m_ActionInfo.ActionValue = 0;
	m_ActionInfo.ActionStates = 0;
}
RoleActionBase::~RoleActionBase()
{
}
bool RoleActionBase::OnInit(CRoleEx* pRole, RoleActionManager* pManager, tagRoleActionInfo* pInfo)
{
	if (!pInfo || !pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pActionManager = pManager;
	m_ActionInfo.ActionID = pInfo->ActionID;
	m_ActionInfo.ActionValue = pInfo->ActionValue;
	m_ActionInfo.ActionStates = pInfo->ActionStates;//���û��״̬��BUG
	m_IsNeedSave = false;
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_ActionConfig = Iter->second;
	return true;
}
bool RoleActionBase::OnInit(CRoleEx* pRole, RoleActionManager* pManager, BYTE ActionID, bool IsNeedSave)
{
	if (!pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pActionManager = pManager;
	m_ActionInfo.ActionID = ActionID;
	m_ActionInfo.ActionValue = 0;
	m_ActionInfo.ActionStates = 0;
	m_IsNeedSave = !IsNeedSave;
	HashMap<BYTE, tagActionConfig>::iterator Iter = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (Iter == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_ActionConfig = Iter->second;
	OnJoinAction();
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Action);
	return true;
}
bool RoleActionBase::IsInTime()
{
	/*if (!m_ActionConfig)
		return false;*/
	HashMap<BYTE, tagActionConfig>::iterator IterGroup = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterGroup == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return false;
	}
	return IterGroup->second.IsInTime();
}
void RoleActionBase::OnJoinAction()
{

}
bool RoleActionBase::IsExistsFinishEvent()
{
	//�Ƿ�����Ѿ���ɵĽ׶� ����δ��ȡ��
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return false;
	}

	HashMap<DWORD, tagActionEventConfig>::iterator Iter = IterConfig->second.EventMap.begin();
	for (; Iter != IterConfig->second.EventMap.end(); ++Iter)
	{
		if (m_ActionInfo.ActionValue >= Iter->second.FinishValue && (m_ActionInfo.ActionStates & Iter->first) == 0)
			return true;
	}
	return false;
}
void RoleActionBase::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	//�����߼�
	if (/*!m_ActionConfig || */!m_pRole)
	{
		ASSERT(false);
		return;
	}
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != EventID)
	{
		return;
	}
	m_ActionInfo.ActionValue += Param;
	m_IsNeedSave = true;
	m_pRole->SetRoleIsNeedSave();
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Action);
}


//�������Ķ���������
void GetGlobelRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetGlobel || Param == 0)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetMadelRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetMadel || Param == 0)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetCurrenRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetCurren || Param == 0)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void UpperLevelRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_UpperLevel || Param == 0)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void CatchFishRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_CatchFish || Param == 0 || (BindParam != IterConfig->second.BindParam  && IterConfig->second.BindParam != 0xff))
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void SendGiffRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_SendGiff || Param == 0)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void UseSkillRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_UseSkill || Param == 0 || (BindParam != IterConfig->second.BindParam  && IterConfig->second.BindParam != 0xff))
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void LauncherGlobelRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_LauncherGlobel || Param == 0 || BindParam < IterConfig->second.BindParam)
		return;
	//�����ݽ���
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxGlobelSumRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxGlobelSum || Param == 0 || Param <= m_ActionInfo.ActionValue)
		return;
	//�����ݽ���
	m_ActionInfo.ActionValue = 0;//����� ������
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void ToLevelRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_ToLevel || Param == 0 || Param <= m_ActionInfo.ActionValue)
		return;
	//�����ݽ���
	m_ActionInfo.ActionValue = 0;//����� ������
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxCurrenRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxCurren || Param == 0 || Param <= m_ActionInfo.ActionValue)
		return;
	//�����ݽ���
	m_ActionInfo.ActionValue = 0;//����� ������
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}
void RechargeRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge || Param == 0 || Param <= m_ActionInfo.ActionValue)
		return;
	m_ActionInfo.ActionValue = 0;//����� ������
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}

void RechargeOneRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge_One || Param == 0 /*|| m_ActionInfo.ActionValue != 0*/)
		return;

	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != EventID)
	{
		return;
	}
	if (IterConfig->second.FinishValue != Param)
	{
		return;
	}
	if (IterConfig->second.EventMap.size() != 1)
	{
		ASSERT(false);
		return;
	}
	//��ȡȫ���ĵ�ǰ������ȡ�Ļ�Ľ���
	HashMap<DWORD, tagActionEventConfig>::iterator IterReward = IterConfig->second.EventMap.begin();
	if (IterReward == IterConfig->second.EventMap.end())
		return ;

	if (IterReward->second.FinishValue != Param)
	{
		return;
	}
	//m_ActionInfo.ActionValue = 0;
	m_ActionInfo.ActionValue = Param;
	m_IsNeedSave = true;
	m_pRole->SetRoleIsNeedSave();
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Action);
	//RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}

void RechargeFirstRoleAction::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge_First || Param == 0 /*|| m_ActionInfo.ActionValue != 0*/)
		return;
	m_ActionInfo.ActionValue = 0;
	RoleActionBase::OnHandleEvent(EventID, BindParam, Param);
}


void MaxGlobelSumRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_MaxGlobelSum)
		return;
	OnHandleEvent(ET_MaxGlobelSum, 0, m_pRole->GetRoleInfo().dwGlobeNum);
}
void ToLevelRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_ToLevel)
		return;
	OnHandleEvent(ET_ToLevel, 0,m_pRole->GetRoleInfo().wLevel);
}
void MaxCurrenRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_MaxCurren)
		return;
	OnHandleEvent(ET_MaxCurren, 0,m_pRole->GetRoleInfo().dwCurrencyNum);
}
void RechargeRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_Recharge)
		return;
	//OnHandleEvent(ET_Recharge, 0, m_pRole->GetRoleInfo().TotalRechargeSum);
}

void RechargeOneRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_Recharge_One)
		return;
}

void RechargeFirstRoleAction::OnJoinAction()
{
	//��������
	HashMap<BYTE, tagActionConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.find(m_ActionInfo.ActionID);
	if (IterConfig == g_FishServer.GetFishConfig().GetActionConfig().m_ActionMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.EventID != ET_Recharge_First)
		return;
}