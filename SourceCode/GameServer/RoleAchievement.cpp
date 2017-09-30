#include "Stdafx.h"
#include "RoleAchievement.h"
#include "RoleEx.h"
#include "FishServer.h"
RoleAchievementManager::RoleAchievementManager()
{
	m_IsLoadDB = false;
	m_IsSendClient = false;
	m_pRole = NULL;
	m_AchievementVec.clear();
	int256Handle::Clear(m_JoinAchievementLog);
}
RoleAchievementManager::~RoleAchievementManager()
{
	//SaveAchievement();//�Ƚ���ǰ���ڽ��е�����ȫ������һ��
	OnDestroy();//ɾ������
}
//void RoleAchievementManager::OnSaveByUpdate()
//{
//	//��������û15���ӱ���һ��
//	SaveAchievement();
//}
RoleAchievementBase* RoleAchievementManager::QueryAchievement(BYTE AchievementID)
{
	//����
	if (m_AchievementVec.empty())
		return NULL;
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		if (!(*Iter))
		{
			ASSERT(false);
			continue;
		}
		if ((*Iter)->GetAchievementID() == AchievementID)
		{
			return *Iter;
		}
	}
	return NULL;
}
bool RoleAchievementManager::IsCanJoinAchievement(BYTE AchievementID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//�Ƿ���Խ�ȡָ��������
	//0.����������Ҫ��
	if (m_AchievementVec.size() >= g_FishServer.GetFishConfig().GetAchievementConfig().m_MaxJoinAchievementSum)
	{
		return false;
	}
	//�ж� �����Ƿ��Ѿ���ȡ��  ���ǵ�ǰVector�����Ƿ��е�ǰ��ID�Ĵ���
	if (int256Handle::GetBitStates(m_JoinAchievementLog, AchievementID))
	{
		return false;
	}
	//1.�ж������Ƿ��Ѿ������
	if (int256Handle::GetBitStates(m_pRole->GetRoleInfo().AchievementStates, AchievementID))
	{
		return false;//�����Ѿ������
	}
	//2.�ȼ�����Ҫ��
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(AchievementID);
	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (m_pRole->GetRoleInfo().wLevel < Iter->second.JoinLevel)
	{
		return false;
	}
	//3.ǰ�������Ƿ��Ѿ������
	if (Iter->second.LowerAchievementVec.empty())
		return true;
	else
	{
		vector<BYTE>::iterator IterLower = Iter->second.LowerAchievementVec.begin();
		for (; IterLower != Iter->second.LowerAchievementVec.end(); ++IterLower)
		{
			if (!int256Handle::GetBitStates(m_pRole->GetRoleInfo().AchievementStates, *IterLower))
				return false;
		}
		return true;
	}
}
void RoleAchievementManager::OnDestroy()
{
	if (m_AchievementVec.size() == 0)
		return;
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		delete (*Iter);
	}
	m_AchievementVec.clear();
	int256Handle::Clear(m_JoinAchievementLog);
}
void RoleAchievementManager::GetAllNeedSaveAchievement(vector<tagRoleAchievementInfo>& pVec)
{
	pVec.clear();
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//���ֵ�ǰȫ��������
	if (m_AchievementVec.size() == 0)
		return;
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		if (!(*Iter))
		{
			ASSERT(false);
			continue;
		}
		if (!(*Iter)->IsNeedSave())
			continue;
		pVec.push_back((*Iter)->GetAchievementInfo());
		(*Iter)->OnSave();
	}
}
bool RoleAchievementManager::GetAchievementMessageStates()
{
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		if ((*Iter)->IsEventFinish())
			return true;
	}
	return false;
}
void RoleAchievementManager::OnResetJoinAllAchievement()
{
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		if (!(*Iter))
			continue;
		(*Iter)->OnJoinAchievement();
	}
}
//void RoleAchievementManager::SaveAchievement()
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return; 
//	}
//	//���ֵ�ǰȫ��������
//	if (m_AchievementVec.size() == 0)
//		return;
//	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
//	for (; Iter != m_AchievementVec.end(); ++Iter)
//	{
//		if (!(*Iter))
//		{
//			ASSERT(false);
//			continue;
//		}
//		if (!(*Iter)->IsNeedSave())
//			continue;
//		DBR_Cmd_SaveRoleAchievement msg;
//		SetMsgInfo(msg,DBR_SaveRoleAchievement, sizeof(DBR_Cmd_SaveRoleAchievement));
//		msg.dwUserID = m_pRole->GetUserID();
//		msg.AchievementInfo = (*Iter)->GetAchievementInfo();
//		g_FishServer.SendNetCmdToDB(&msg);
//	}
//}
bool RoleAchievementManager::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	SendLoadAllAchievementInfoToDB();
	return true;
}
void RoleAchievementManager::SendLoadAllAchievementInfoToDB()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//DBR_Cmd_LoadRoleAchievement msg;
	//SetMsgInfo(msg,DBR_LoadRoleAchievement, sizeof(DBR_Cmd_LoadRoleAchievement));
	//msg.dwUserID = m_pRole->GetUserID();
	//g_FishServer.SendNetCmdToDB(&msg);
}
void RoleAchievementManager::OnLoadAllAchievementInfoByDB(DBO_Cmd_LoadRoleAchievement* pDB)
{
	if (!pDB || !m_pRole)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_AchievementVec.clear();
	}
	for (WORD i = 0; i < pDB->Sum; ++i)
	{
		//UserID AchievementID Param ��������ݿ�� 3�������Ϳ�����
		HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(pDB->Array[i].AchievementID);
		if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end() || !IsCanJoinAchievement(pDB->Array[i].AchievementID))
		{
			//������ɾ����
			DBR_Cmd_DelRoleAchievement msg;
			SetMsgInfo(msg,DBR_DelRoleAchievement, sizeof(DBR_Cmd_DelRoleAchievement));
			msg.dwUserID = m_pRole->GetUserID();
			msg.bAchievementID = pDB->Array[i].AchievementID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			continue;
		}
		else
		{
			//����һ������
			RoleAchievementBase* pBase = CreateAchievementByEventID(Iter->second.AchievementEventInfo.EventID);//�����¼�ID ����һ���������
			if (!pBase || !pBase->OnInit(m_pRole, this, &pDB->Array[i]))
			{
				ASSERT(false);
				continue;
			}
			//���뼯������ȥ
			m_AchievementVec.push_back(pBase);
			int256Handle::SetBitStates(m_JoinAchievementLog, pDB->Array[i].AchievementID, true);
			continue;
		}
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		OnJoinAchievementByConfig(false);
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Achievement);
		m_IsLoadDB = true;
		m_pRole->IsLoadFinish();
	}
}
void RoleAchievementManager::SendAllAchievementToClient()
{
	//����ǰ���ڽ��е������͵��ͻ���ȥ
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//�ͻ������������ѯ��ȫ����������Ϣ 
	DWORD PageSize = sizeof(LC_Cmd_GetRoleAchievementInfo)+(m_AchievementVec.size() - 1)*sizeof(tagRoleAchievementInfo);
	LC_Cmd_GetRoleAchievementInfo * msg = (LC_Cmd_GetRoleAchievementInfo*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	msg->SetCmdType(GetMsgType(Main_Achievement, LC_GetRoleAchievementInfo));
	if (!m_AchievementVec.empty())
	{
		std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
		for (WORD i = 0; Iter != m_AchievementVec.end(); ++Iter, ++i)
		{
			msg->Array[i] = (*Iter)->GetAchievementInfo();
		}
	}
	std::vector<LC_Cmd_GetRoleAchievementInfo*> pVec;
	SqlitMsg(msg, PageSize, m_AchievementVec.size(),true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetRoleAchievementInfo*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_IsSendClient = true;
}
void RoleAchievementManager::OnRoleLevelChange()
{
	OnJoinAchievementByConfig(false);
}
void RoleAchievementManager::OnJoinAchievementByConfig(bool IsNeedSave)
{
	if (m_pRole->IsRobot())
	{
		return;
	}

	if (m_AchievementVec.size() >= g_FishServer.GetFishConfig().GetAchievementConfig().m_MaxJoinAchievementSum)//���������޷���ȡ
		return;
	//���Խ�ȡ����
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end(); ++Iter)
	{
		if (m_AchievementVec.size() < g_FishServer.GetFishConfig().GetAchievementConfig().m_MaxJoinAchievementSum)
			OnJoinAchievement(Iter->second.AchievementID,IsNeedSave);
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
		vector<tagRoleAchievementInfo> pVec;
		GetAllNeedSaveAchievement(pVec);
		if (pVec.empty())
			return;
		//����һ�����ȫ�������񱣴�����
		DWORD PageSize = sizeof(DBR_Cmd_SaveAllAchievement)+(pVec.size() - 1)*sizeof(tagRoleAchievementInfo);
		DBR_Cmd_SaveAllAchievement* msg = (DBR_Cmd_SaveAllAchievement*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return;
		}
		msg->SetCmdType(DBR_SaveAllAchievement);
		msg->dwUserID = m_pRole->GetUserID();
		for (BYTE i = 0; i < pVec.size(); ++i)
		{
			msg->Array[i] = pVec[i];
		}

		std::vector<DBR_Cmd_SaveAllAchievement*> pVec2;
		SqlitMsg(msg, PageSize, pVec.size(),false, pVec2);
		free(msg);
		if (!pVec2.empty())
		{
			std::vector<DBR_Cmd_SaveAllAchievement*>::iterator Iter = pVec2.begin();
			for (; Iter != pVec2.end(); ++Iter)
			{
				g_FishServer.SendNetCmdToSaveDB(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
}
bool RoleAchievementManager::OnFinishAchievement(BYTE AchievementID)//��ȡ��������ʱ��
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//��һ��������ɵ�ʱ�� ���ǽ��д���
	//1.�ж������Ƿ����
	if (!int256Handle::GetBitStates(m_JoinAchievementLog, AchievementID))
	{
		return false;
	}
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(AchievementID);
	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		return false;
	}
	std::vector<RoleAchievementBase*>::iterator IterAchievement = m_AchievementVec.begin();
	for (; IterAchievement != m_AchievementVec.end(); ++IterAchievement)
	{
		if (!(*IterAchievement))
		{
			ASSERT(false);
			continue;
		}
		if ((*IterAchievement)->GetAchievementID() == AchievementID)
		{
			if (!(*IterAchievement)->IsEventFinish())
			{
				ASSERT(false);
				if (m_IsSendClient)
				{
					LC_Cmd_GetOnceAchievementInfo msg;
					SetMsgInfo(msg,GetMsgType(Main_Achievement, LC_GetOnceAchievementInfo), sizeof(LC_Cmd_GetOnceAchievementInfo));
					msg.AchievementInfo = (*IterAchievement)->GetAchievementInfo();
					m_pRole->SendDataToClient(&msg);
				}
				return false;
			}
			int256Handle::SetBitStates(m_JoinAchievementLog, AchievementID, false);
			//3.��������Ľ���
			m_pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID,TEXT("��ɳɾͽ���"));
			m_pRole->ChangeRoleAchievementStates(AchievementID, true);
			if (m_IsSendClient)
			{
				LC_Cmd_GetAchievementReward msg;
				SetMsgInfo(msg,GetMsgType(Main_Achievement, LC_GetAchievementReward), sizeof(LC_Cmd_GetAchievementReward));
				msg.AchievementID = AchievementID;
				m_pRole->SendDataToClient(&msg);
			}

			DBR_Cmd_DelRoleAchievement msgDel;
			SetMsgInfo(msgDel,DBR_DelRoleAchievement, sizeof(DBR_Cmd_DelRoleAchievement));
			msgDel.dwUserID = m_pRole->GetUserID();
			msgDel.bAchievementID = AchievementID;
			g_FishServer.SendNetCmdToSaveDB(&msgDel);

			delete *IterAchievement;
			m_AchievementVec.erase(IterAchievement);
			//6.��������ͻ���ȥ ���ָ�������Ѿ���ȡ������ ������ɾ����
			/*LC_Cmd_DelAchievement msg;
			msg.AchievementID = AchievementID;
			m_pRole->SendDataToClient(Main_Achievement, LC_DelAchievement, &msg, sizeof(msg));*/
			//7.��Ϊ��������һ������ 
			if (Iter->second.UpperAchievementID == 0)
				OnJoinAchievementByConfig(false);//�����Ҫ���½�ȡһ������ ����ȱʧ
			else
			{
				if (!OnJoinAchievement(Iter->second.UpperAchievementID,true))
					OnJoinAchievementByConfig(false);//�����Ҫ���½�ȡһ������ ����ȱʧ
			}
			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Achievement);
			return true;
		}
	}
	return false;
}
bool RoleAchievementManager::OnJoinAchievement(BYTE AchievementID, bool IsNeedSave)//��ȡһ�������ʱ��
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
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(AchievementID);
	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		return false;
	}
	if (!IsCanJoinAchievement(AchievementID))
		return false;
	//3.�����Ƿ���Խ�ȡ ǰ�ú����Ѿ��жϹ��� ��������ж� �����������
	RoleAchievementBase* pAchievement = CreateAchievementByEventID(Iter->second.AchievementEventInfo.EventID);
	if (!pAchievement)
	{
		ASSERT(false);
		return false;
	}
	m_AchievementVec.push_back(pAchievement);//�ȷ��뵽��������ȥ
	pAchievement->OnInit(m_pRole, this, AchievementID, IsNeedSave);//��ʼ������
	int256Handle::SetBitStates(m_JoinAchievementLog, AchievementID, true);
	//4.��������ͻ���ȥ
	if (m_IsSendClient)
	{
		LC_Cmd_JoinAchievement msg;
		SetMsgInfo(msg,GetMsgType(Main_Achievement, LC_JoinAchievement), sizeof(LC_Cmd_JoinAchievement));
		msg.AchievementID = AchievementID;

		m_pRole->SendDataToClient(&msg);
	}
	if (IsNeedSave)
	{
		DBR_Cmd_SaveRoleAchievement msg;
		SetMsgInfo(msg, DBR_SaveRoleAchievement, sizeof(DBR_Cmd_SaveRoleAchievement));
		msg.dwUserID = m_pRole->GetUserID();
		msg.AchievementInfo = pAchievement->GetAchievementInfo();
		g_FishServer.SendNetCmdToSaveDB(&msg);
	}
	return true;
}
//bool RoleAchievementManager::OnDelAchievement(BYTE AchievementID)
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return false;
//	}
//	//ɾ��һ������
//	//1.���������Ƿ����
//	HashMap<BYTE, tagAchievementConfig*>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(AchievementID);
//	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end() || !Iter->second)
//	{
//		return false;
//	}
//	//2.�����Ƿ��Ѿ���ȡ��
//	if (!int256Handle::GetBitStates(m_JoinAchievementLog, AchievementID))
//	{
//		return false;
//	}
//	//3.������ɾ���� 
//	DBR_Cmd_DelRoleAchievement msg;
//	SetMsgInfo(msg,DBR_DelRoleAchievement, sizeof(DBR_Cmd_DelRoleAchievement));
//	msg.dwUserID = m_pRole->GetUserID();
//	msg.bAchievementID = AchievementID;
//	g_FishServer.SendNetCmdToDB(&msg);
//	//4.������ɾ����
//	int256Handle::SetBitStates(m_JoinAchievementLog, AchievementID, false);
//	std::vector<RoleAchievementBase*>::iterator IterAchievement = m_AchievementVec.begin();
//	for (; IterAchievement != m_AchievementVec.end(); ++IterAchievement)
//	{
//		if ((*IterAchievement)->GetAchievementID() == AchievementID)
//		{
//			delete *IterAchievement;
//			m_AchievementVec.erase(IterAchievement);
//			break;
//		}
//	}
//	//5.֪ͨ�ͻ��� ��ҷ�������
//	if (m_IsSendClient)
//	{
//		LC_Cmd_DelAchievement msgDel;
//		SetMsgInfo(msgDel,GetMsgType(Main_Achievement, LC_DelAchievement), sizeof(LC_Cmd_DelAchievement));
//		msgDel.AchievementID = AchievementID;
//		m_pRole->SendDataToClient(&msgDel);
//	}
//	return true;
//}
RoleAchievementBase* RoleAchievementManager::CreateAchievementByEventID(BYTE EventID)
{
	switch (EventID)
	{
	case ET_GetGlobel:
		return new GetGlobelRoleAchievement();
	case ET_GetMadel:
		return new GetMadelRoleAchievement();
	case ET_GetCurren:
		return new GetCurrenRoleAchievement();
	case ET_UpperLevel:
		return new UpperLevelRoleAchievement();
	case ET_CatchFish:
		return new CatchFishRoleAchievement();
	case ET_SendGiff:
		return new SendGiffRoleAchievement();
	case ET_UseSkill:
		return new UseSkillRoleAchievement();
	case ET_LauncherGlobel:
		return new LauncherGlobelRoleAchievement();
	case ET_MaxGlobelSum:
		return new MaxGlobelSumRoleAchievement();
	case ET_ToLevel:
		return new ToLevelRoleAchievement();
	case ET_MaxCurren:
		return new MaxCurrenRoleAchievement();
	case ET_Recharge:
		return new RechargeRoleAchievement();
	default:
		return NULL;
	}
}
void RoleAchievementManager::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (m_AchievementVec.empty())
		return;
	std::vector<RoleAchievementBase*>::iterator Iter = m_AchievementVec.begin();
	for (; Iter != m_AchievementVec.end(); ++Iter)
	{
		if (!(*Iter))
		{
			ASSERT(false);
			continue;
		}
		(*Iter)->OnHandleEvent(EventID, BindParam, Param);
	}
}

//�������
RoleAchievementBase::RoleAchievementBase()
{
	m_pAchievementManager = NULL;
	m_EventIsFinish = false;
	m_IsNeedSave = false;
	//m_AchievementConfig = NULL;
	m_AchievementInfo.AchievementID = 0;
	m_AchievementInfo.AchievementValue = 0;
}
RoleAchievementBase::~RoleAchievementBase()
{
}
bool RoleAchievementBase::OnInit(CRoleEx* pRole, RoleAchievementManager* pManager, tagRoleAchievementInfo* pInfo)
{
	if (!pInfo || !pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pAchievementManager = pManager;
	m_AchievementInfo.AchievementID = pInfo->AchievementID;
	m_AchievementInfo.AchievementValue = pInfo->AchievementValue;
	m_IsNeedSave = false;
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_AchievementConfig = Iter->second;
	m_EventIsFinish = (Iter->second.AchievementEventInfo.FinishValue <= m_AchievementInfo.AchievementValue);
	return true;
}
bool RoleAchievementBase::OnInit(CRoleEx* pRole, RoleAchievementManager* pManager, BYTE AchievementID, bool IsNeedSave)
{
	if (!pManager || !pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	m_pAchievementManager = pManager;
	m_AchievementInfo.AchievementID = AchievementID;
	m_AchievementInfo.AchievementValue = 0;
	m_IsNeedSave = !IsNeedSave;
	HashMap<BYTE, tagAchievementConfig>::iterator Iter = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (Iter == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return false;
	}
	//m_AchievementConfig = Iter->second;
	m_EventIsFinish = false;
	OnJoinAchievement();
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Achievement);
	return true;
}
void RoleAchievementBase::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//�����߼�
	if (m_EventIsFinish)
		return;
	/*if (!m_AchievementConfig)
	{
		ASSERT(false);
		return;
	}*/
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end() )
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.AchievementEventInfo.EventID != EventID)
	{
		return;
	}
	m_AchievementInfo.AchievementValue += Param;
	m_IsNeedSave = true;
	m_pRole->SetRoleIsNeedSave();
	if (m_AchievementInfo.AchievementValue >= IterConfig->second.AchievementEventInfo.FinishValue)
	{
		//��������� 
		m_EventIsFinish = true;
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Achievement);

		LC_Cmd_AchievementAllEventFinish msg;
		SetMsgInfo(msg,GetMsgType(Main_Achievement, LC_AchievementAllEventFinish), sizeof(LC_Cmd_AchievementAllEventFinish));
		msg.AchievementID = m_AchievementInfo.AchievementID;
		m_pRole->SendDataToClient(&msg);
	}
}
void RoleAchievementBase::OnJoinAchievement()
{

}

//�������Ķ���������
void GetGlobelRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetGlobel || Param == 0)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetMadelRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetMadel || Param == 0)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void GetCurrenRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_GetCurren || Param == 0)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void UpperLevelRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_UpperLevel || Param == 0)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void CatchFishRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_CatchFish || Param == 0 || (BindParam != IterConfig->second.AchievementEventInfo.BindParam && IterConfig->second.AchievementEventInfo.BindParam != 0xff))
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void SendGiffRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_SendGiff || Param == 0)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void UseSkillRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_UseSkill || Param == 0 || (BindParam != IterConfig->second.AchievementEventInfo.BindParam && IterConfig->second.AchievementEventInfo.BindParam != 0xff))
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void LauncherGlobelRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (EventID != ET_LauncherGlobel || Param == 0 || BindParam < IterConfig->second.AchievementEventInfo.BindParam)
		return;
	//�����ݽ���
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxGlobelSumRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	//���������� BindParam  
	if (EventID != ET_MaxGlobelSum || Param == 0 || Param <= m_AchievementInfo.AchievementValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_AchievementInfo.AchievementValue = 0;//����� ������
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param); 
}
void ToLevelRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_ToLevel || Param == 0 || Param <= m_AchievementInfo.AchievementValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_AchievementInfo.AchievementValue = 0;//����� ������
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void MaxCurrenRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_MaxCurren || Param == 0 || Param <= m_AchievementInfo.AchievementValue)
		return;
	//�����ݽ���
	if (!m_EventIsFinish)
		m_AchievementInfo.AchievementValue = 0;//����� ������
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}
void RechargeRoleAchievement::OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param)
{
	if (EventID != ET_Recharge || Param == 0 || Param <= m_AchievementInfo.AchievementValue)
		return;
	if (!m_EventIsFinish)
		m_AchievementInfo.AchievementValue = 0;//����� ������
	RoleAchievementBase::OnHandleEvent(EventID, BindParam, Param);
}

void MaxGlobelSumRoleAchievement::OnJoinAchievement()
{
	//��������
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.AchievementEventInfo.EventID != ET_MaxGlobelSum)
		return;
	OnHandleEvent(ET_MaxGlobelSum, 0, m_pRole->GetRoleInfo().dwGlobeNum);
}
void ToLevelRoleAchievement::OnJoinAchievement()
{
	//��������
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.AchievementEventInfo.EventID != ET_ToLevel)
		return;
	OnHandleEvent(ET_ToLevel, 0, m_pRole->GetRoleInfo().wLevel);
}
void MaxCurrenRoleAchievement::OnJoinAchievement()
{
	//��������
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.AchievementEventInfo.EventID != ET_MaxCurren)
		return;
	OnHandleEvent(ET_MaxCurren, 0, m_pRole->GetRoleInfo().dwCurrencyNum);
}
void RechargeRoleAchievement::OnJoinAchievement()
{
	//��������
	HashMap<BYTE, tagAchievementConfig>::iterator IterConfig = g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.find(m_AchievementInfo.AchievementID);
	if (IterConfig == g_FishServer.GetFishConfig().GetAchievementConfig().m_AchievementMap.end())
	{
		ASSERT(false);
		return;
	}
	if (IterConfig->second.AchievementEventInfo.EventID != ET_Recharge)
		return;
	//OnHandleEvent(ET_Recharge, 0, m_pRole->GetRoleInfo().TotalRechargeSum);
}