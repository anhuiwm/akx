#include "Stdafx.h"
#include "RoleRelation.h"
#include "RoleEx.h"
#include "RoleManager.h"
#include "FishServer.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
RoleRelationManager::RoleRelationManager()
{
	m_pUser = NULL;
	m_RoleManager = NULL;
	m_BeRelationMap.clear();
	m_RelationMap.clear();
	m_IsLoadToClient = false;
	m_IsLoadDB = false;
}
RoleRelationManager::~RoleRelationManager()
{

}
bool RoleRelationManager::OnInit(CRoleEx* pUser, RoleManager* pManager)
{
	if (!pUser ||  !pManager)
	{
		ASSERT(false);
		return false;
	}
	m_RoleManager = pManager;
	m_pUser = pUser;
	return OnLoadUserRelation();
}
bool RoleRelationManager::OnLoadUserRelation()
{
	//����DBR�������ȫ��������
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	m_BeRelationMap.clear();
	m_RelationMap.clear();
	/*DBR_Cmd_LoadUserRelation msg;
	SetMsgInfo(msg,DBR_LoadUserRelation, sizeof(DBR_Cmd_LoadUserRelation));
	msg.dwUserID = m_pUser->GetUserID();
	g_FishServer.SendNetCmdToDB(&msg);*/
	return true;
}
void RoleRelationManager::OnLoadBeUserRelationResult(DBO_Cmd_LoadBeUserRelation* pDB)//��ӱ�����ϵ
{
	if (!pDB || !m_pUser)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_BeRelationMap.clear();
	}
	for (size_t i = 0; i < pDB->Sum; ++i)
	{
		tagBeRoleRelation pType;
		pType.dwUserID = pDB->Array[i].dwUserID;
		pType.bRelationType = (RelationType)pDB->Array[i].bRelationType;
		m_BeRelationMap.insert(HashMap<DWORD, tagBeRoleRelation>::value_type(pDB->Array[i].dwUserID, pType));
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		m_IsLoadDB = true;
		m_pUser->IsLoadFinish();
	}
}
void RoleRelationManager::OnLoadUserRelationResult(DBO_Cmd_LoadUserRelation* pDB)
{
	//�����صĹ�ϵȫ����������
	if (!pDB)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_RelationMap.clear();
	}
	for (size_t i = 0; i < pDB->Sum; ++i)
	{
		//����ϵ���뵽��������ȥ
		m_RelationMap.insert(HashMap<DWORD, tagRoleRelation>::value_type(pDB->Array[i].dwDestRoleID, pDB->Array[i]));
	}
}
//void RoleRelationManager::OnLoadBeUserRelationFinish()
//{
//	if (!m_pUser)
//	{
//		ASSERT(false);
//		return;
//	}
//
//}
bool RoleRelationManager::OnAddUserRelation(CL_Cmd_AddUserRelation* pMsg)
{
	//������һ����ϵ
	if (!pMsg || !m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if (pMsg->dwDestUserID == m_pUser->GetUserID())//����������Լ�Ϊ��ϵ
	{
		return false;
	}
	//�п������޸� ���������

	if (m_RelationMap.size() >= g_FishServer.GetFishConfig().GetRelation().MaxRelationSum && m_RelationMap.count(pMsg->dwDestUserID) == 0)
	{
		//ASSERT(false);//�������� �޷�������ӹ�ϵ���� ��ֹ��ҵĹ�ϵ���ݹ���
		return false;
	}

	HashMap<DWORD, tagRoleRelation>::iterator Iter= m_RelationMap.find(pMsg->dwDestUserID);
	if (Iter != m_RelationMap.end() && Iter->second.bRelationType == pMsg->bRelationType)
	{
		ASSERT(false);
		LC_Cmd_AddUserRelation msg;
		SetMsgInfo(msg,GetMsgType(Main_Relation, LC_AddUserRelation), sizeof(LC_Cmd_AddUserRelation));
		msg.RelationInfo.dwDestRoleID = pMsg->dwDestUserID;
		msg.RelationInfo.bRelationType = pMsg->bRelationType;
		msg.Result = false;
		m_pUser->SendDataToClient(&msg);
		return false;
	}
	//ִ�д洢����
	DBR_Cmd_AddUserRelation msg;
	SetMsgInfo(msg,DBR_AddUserRelation, sizeof(DBR_Cmd_AddUserRelation));
	msg.dwSrcUserID = m_pUser->GetUserID();
	msg.dwDestUserID = pMsg->dwDestUserID;
	msg.bRelationType = pMsg->bRelationType;
	g_FishServer.SendNetCmdToDB(&msg);
	return true;
}
void RoleRelationManager::OnAddUserRelationResult(DBO_Cmd_AddUserRelation* pDB)
{
	if (!pDB || !m_pUser || !m_RoleManager)
	{
		ASSERT(false);
		return;
	}
	//�п������޸� �п��������
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(pDB->RelationInfo.dwDestRoleID);
	if (Iter == m_RelationMap.end())
	{
		//���
		LC_Cmd_AddUserRelation msg;
		SetMsgInfo(msg, GetMsgType(Main_Relation, LC_AddUserRelation), sizeof(LC_Cmd_AddUserRelation));
		msg.Result = pDB->Result;
		if (msg.Result)
		{
			m_RelationMap.insert(HashMap<DWORD, tagRoleRelation>::value_type(pDB->RelationInfo.dwDestRoleID, pDB->RelationInfo));

			msg.RelationInfo.dwDestRoleID = pDB->RelationInfo.dwDestRoleID;
			msg.RelationInfo.bGender = pDB->RelationInfo.bGender;
			msg.RelationInfo.bRelationType = pDB->RelationInfo.bRelationType;
			msg.RelationInfo.dwFaceID = pDB->RelationInfo.dwFaceID;
			msg.RelationInfo.wLevel = pDB->RelationInfo.wLevel;
			msg.RelationInfo.TitleID = pDB->RelationInfo.TitleID;
			msg.RelationInfo.dwAchievementPoint = pDB->RelationInfo.dwAchievementPoint;
			msg.RelationInfo.VipLevel = pDB->RelationInfo.VipLevel;
			msg.RelationInfo.IsInMonthCard = pDB->RelationInfo.IsInMonthCard;
			msg.RelationInfo.GameID = pDB->RelationInfo.GameID;
			msg.RelationInfo.WeekGoldNum = pDB->RelationInfo.WeekGoldNum;
			if (pDB->RelationInfo.IsShowIpAddress)
				g_FishServer.GetAddressByIP(pDB->RelationInfo.ClientIP, msg.RelationInfo.IPAddress, CountArray(msg.RelationInfo.IPAddress));
			else
				TCHARCopy(msg.RelationInfo.IPAddress, CountArray(msg.RelationInfo.IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));
			for (int i = 0; i < MAX_CHARM_ITEMSUM; ++i)
				msg.RelationInfo.CharmArray[i] = pDB->RelationInfo.CharmArray[i];

			//��������ϴε�½��ʱ��
			if (pDB->RelationInfo.IsOnline)
				msg.RelationInfo.bDiffDay = 0xff;//���������ߵ���� ��ǰ���� Ϊ-1
			else
			{
				time_t Now = time(NULL);
				__int64 diffDay = GetDiffDay(pDB->RelationInfo.LastLogonTime,g_FishServer.GetFishConfig().GetWriteSec());
				if (diffDay > 250)
					diffDay = 250;
				msg.RelationInfo.bDiffDay = (BYTE)diffDay;
			}

			TCHARCopy(msg.RelationInfo.DestNickName, CountArray(msg.RelationInfo.DestNickName), pDB->RelationInfo.DestNickName, _tcslen(pDB->RelationInfo.DestNickName));
			if (m_IsLoadToClient)
				m_pUser->SendDataToClient(&msg);
		}
		else
		{
			m_pUser->SendDataToClient(&msg);
		}
		//��ӹ�ϵ�ɹ� ������Ҫ�ñ���ӵ���ҽ������
		if (pDB->Result)
		{
			//����ǰ��ӳɹ������ݷ��͵�Centerȥ
			CC_Cmd_AddUserRelation msg;
			SetMsgInfo(msg, GetMsgType(Main_Relation, CC_AddUserRelation), sizeof(CC_Cmd_AddUserRelation));
			msg.dwUserID = m_pUser->GetUserID();
			msg.RelationInfo = pDB->RelationInfo;
			m_pUser->SendDataToCenter(&msg);//�����ݷ��͵����������ȥ
		}
		m_pUser->GetRoleRelationRequest().OnAddRelationResult(m_pUser->GetUserID(), pDB->RelationInfo.dwDestRoleID, pDB->RelationInfo.bRelationType, msg.Result);
	}
	else
	{
		//�޸�
		if (pDB->Result)
		{
			Iter->second.bRelationType = pDB->RelationInfo.bRelationType;
			//���͵����������ȥ
			CC_Cmd_ChangeUserRelation msgCenter;
			SetMsgInfo(msgCenter, GetMsgType(Main_Relation, CC_ChangeUserRelation), sizeof(CC_Cmd_ChangeUserRelation));
			msgCenter.dwUserID = m_pUser->GetUserID();
			msgCenter.dwDestUserID = pDB->RelationInfo.dwDestRoleID;
			msgCenter.RelationType = pDB->RelationInfo.bRelationType;
			m_pUser->SendDataToCenter(&msgCenter);
			//���͵��ͻ���ȥ
			LC_Cmd_ChangeUserRelation msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Relation, LC_ChangeUserRelation), sizeof(LC_Cmd_ChangeUserRelation));
			msgClient.dwDestUserID = pDB->RelationInfo.dwDestRoleID;
			msgClient.RelationType = pDB->RelationInfo.bRelationType;
			msgClient.Result = true;
			m_pUser->SendDataToClient(&msgClient);
		}
		else
		{
			LC_Cmd_ChangeUserRelation msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Relation, LC_ChangeUserRelation), sizeof(LC_Cmd_ChangeUserRelation));
			msgClient.dwDestUserID = pDB->RelationInfo.dwDestRoleID;
			msgClient.RelationType = pDB->RelationInfo.bRelationType;
			msgClient.Result = false;
			m_pUser->SendDataToClient(&msgClient);
		}
	}
}
bool RoleRelationManager::OnDelUserRelation(CL_Cmd_DelUserRelation* pDB)
{
	//���ɾ��һ����ϵ
	if (!pDB || !m_pUser || !m_IsLoadToClient)
	{
		ASSERT(false);
		return false;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(pDB->dwDestUserID);
	if (Iter != m_RelationMap.end())
	{
		//����DBR����
		DBR_Cmd_DelUserRelation msg;
		SetMsgInfo(msg,DBR_DelUserRelation, sizeof(DBR_Cmd_DelUserRelation));
		msg.dwSrcUserID = m_pUser->GetUserID();
		msg.dwDestUserID = pDB->dwDestUserID;
		g_FishServer.SendNetCmdToSaveDB(&msg);
		//���͵����������ȥ
		CC_Cmd_DelUserRelation msgCenter;
		SetMsgInfo(msgCenter,GetMsgType(Main_Relation, CC_DelUserRelation), sizeof(CC_Cmd_DelUserRelation));
		msgCenter.dwUserID = m_pUser->GetUserID();
		msgCenter.dwDestUserID = pDB->dwDestUserID;
		m_pUser->SendDataToCenter(&msgCenter);
		//���͵��ͻ���ȥ
		LC_Cmd_DelUserRelation msgClient;
		SetMsgInfo(msgClient,GetMsgType(Main_Relation, LC_DelUserRelation), sizeof(LC_Cmd_DelUserRelation));
		msgClient.dwDestUserID = pDB->dwDestUserID;
		msgClient.Result = true;
		m_pUser->SendDataToClient(&msgClient);


		BYTE RelationType = Iter->second.bRelationType;
		m_RelationMap.erase(Iter);

		
		m_pUser->GetRoleCharManager().OnDelRelation(pDB->dwDestUserID);

		m_pUser->GetRoleRelationRequest().OnDelRelation(pDB->dwDestUserID, RelationType);
		return true;
	}
	LC_Cmd_DelUserRelation msg;
	SetMsgInfo(msg,GetMsgType(Main_Relation, LC_DelUserRelation), sizeof(LC_Cmd_DelUserRelation));
	msg.dwDestUserID = pDB->dwDestUserID;
	msg.Result = false;
	m_pUser->SendDataToClient(&msg);
	return true;
}
//bool RoleRelationManager::OnChangeUserRelation(CL_Cmd_ChangeUserRelation* pMsg)
//{
//	//�޸�һ���Ѿ����ڵĹ�ϵ
//	if (!pMsg || !m_pUser)
//	{
//		ASSERT(false);
//		return false;
//	}
//	if (!m_IsLoadToClient)
//	{
//		ASSERT(false);
//		return false;
//	}
//	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(pMsg->dwDestUserID);
//	if (Iter != m_RelationMap.end())
//	{
//		
//		return true;
//	}
//	
//	return true;
//}
void RoleRelationManager::OnAddBeUserRelation(tagBeRoleRelation* pInfo)
{
	//���һ��������ϵ
	if (!pInfo)
	{
		ASSERT(false);
		return;
	}
	tagBeRoleRelation pType;
	pType.dwUserID = pInfo->dwUserID;
	pType.bRelationType = (RelationType)pInfo->bRelationType;
	m_BeRelationMap.insert(HashMap<DWORD, tagBeRoleRelation>::value_type(pInfo->dwUserID, pType));
}
void RoleRelationManager::OnDelBeUserRelation(DWORD dwDestUserID)
{
	m_BeRelationMap.erase(dwDestUserID);
}
void RoleRelationManager::OnChagneBeUserRelation(DWORD dwDestUserID, BYTE bRelationType)
{
	HashMap<DWORD, tagBeRoleRelation>::iterator Iter = m_BeRelationMap.find(dwDestUserID);
	if (Iter != m_BeRelationMap.end())
	{
		Iter->second.bRelationType = (RelationType)bRelationType;
	}
}
bool RoleRelationManager::OnGetUserRelation()
{
	//�ͻ��˻�ȡȫ����ҵĹ�ϵ���� �洢����
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	DWORD PageSize = sizeof(LC_Cmd_GetUserRelation)+sizeof(tagRoleRelationClient)*(m_RelationMap.size() - 1);
	LC_Cmd_GetUserRelation * msg = (LC_Cmd_GetUserRelation*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return false;
	}
	msg->SetCmdType(GetMsgType(Main_Relation, LC_GetUserRelation));
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.begin();
	for (int i = 0; Iter != m_RelationMap.end(); ++Iter, ++i)
	{
		msg->Array[i].dwDestRoleID = Iter->second.dwDestRoleID;
		msg->Array[i].bGender = Iter->second.bGender;
		msg->Array[i].bRelationType = Iter->second.bRelationType;
		msg->Array[i].dwFaceID = Iter->second.dwFaceID;
		msg->Array[i].wLevel = Iter->second.wLevel;
		msg->Array[i].TitleID = Iter->second.TitleID;
		msg->Array[i].dwAchievementPoint = Iter->second.dwAchievementPoint;
		msg->Array[i].VipLevel = Iter->second.VipLevel;
		msg->Array[i].IsInMonthCard = Iter->second.IsInMonthCard;
		msg->Array[i].GameID = Iter->second.GameID;
		msg->Array[i].WeekGoldNum = Iter->second.WeekGoldNum;
		msg->Array[i].byUsingLauncher = Iter->second.byUsingLauncher;
		//msg->Array[i].RateIndex = Iter->second.RateIndex;
		if (Iter->second.IsShowIpAddress)
			g_FishServer.GetAddressByIP(Iter->second.ClientIP, msg->Array[i].IPAddress, CountArray(msg->Array[i].IPAddress));
		else
			TCHARCopy(msg->Array[i].IPAddress, CountArray(msg->Array[i].IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));
		for (int j = 0; j < MAX_CHARM_ITEMSUM; ++j)
			msg->Array[i].CharmArray[j] = Iter->second.CharmArray[j];
		TCHARCopy(msg->Array[i].DestNickName, CountArray(msg->Array[i].DestNickName), Iter->second.DestNickName, _tcslen(Iter->second.DestNickName));
		if (Iter->second.IsOnline)
			msg->Array[i].bDiffDay = 0xff;//���������ߵ���� ��ǰ���� Ϊ-1
		else
		{
			time_t Now = time(NULL);
			__int64 diffDay = GetDiffDay(Iter->second.LastLogonTime, g_FishServer.GetFishConfig().GetWriteSec());
			if (diffDay > 250)
				diffDay = 250;
			msg->Array[i].bDiffDay = (BYTE)diffDay;
		}
	}
	std::vector<LC_Cmd_GetUserRelation*> pVec;
	SqlitMsg(msg, PageSize, m_RelationMap.size(), true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetUserRelation*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pUser->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	//�����������
	m_IsLoadToClient = true;
	return true;
}
void RoleRelationManager::SendRoleRelationToCenter()
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	//����ҵĹ�ϵ���ݷ��͵�Centerȥ 
	{
		DWORD PageSize = sizeof(CC_Cmd_GetUserRelation)+sizeof(tagRoleRelation)*(m_RelationMap.size() - 1);
		CC_Cmd_GetUserRelation * msg = (CC_Cmd_GetUserRelation*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return;
		}
		msg->SetCmdType(GetMsgType(Main_Relation, CC_LoadUserRelation));
		msg->dwUserID = m_pUser->GetUserID();// ����
		HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.begin();
		for (int i = 0; Iter != m_RelationMap.end(); ++Iter, ++i)
		{
			msg->Array[i] = Iter->second;
		}
		std::vector<CC_Cmd_GetUserRelation*> pVec;
		SqlitMsg(msg, PageSize, m_RelationMap.size(), false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<CC_Cmd_GetUserRelation*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				m_pUser->SendDataToCenter(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
	//������ϵ 
	{
		DWORD PageSize = sizeof(CC_Cmd_LoadBeUserRelation)+sizeof(tagBeRoleRelation)*(m_BeRelationMap.size() - 1);
		CC_Cmd_LoadBeUserRelation * msg = (CC_Cmd_LoadBeUserRelation*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return;
		}
		msg->SetCmdType(GetMsgType(Main_Relation, CC_LoadBeUserRelation));
		msg->dwUserID = m_pUser->GetUserID();// ����
		HashMap<DWORD, tagBeRoleRelation>::iterator Iter = m_BeRelationMap.begin();
		for (int i = 0; Iter != m_BeRelationMap.end(); ++Iter, ++i)
		{
			msg->Array[i] = Iter->second;
		}
		std::vector<CC_Cmd_LoadBeUserRelation*> pVec;
		SqlitMsg(msg, PageSize, m_BeRelationMap.size(), false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<CC_Cmd_LoadBeUserRelation*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				m_pUser->SendDataToCenter(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
	//�����������
	/*CC_Cmd_GetUserRelationFinish msgFinish;
	SetMsgInfo(msgFinish,GetMsgType(Main_Relation, CC_LoadUserRelationFinish), sizeof(CC_Cmd_GetUserRelationFinish));
	msgFinish.dwUserID = m_pUser->GetUserID();
	m_pUser->SendDataToCenter(&msgFinish);*/
}
void RoleRelationManager::OnChangeUserOnline(DWORD dwUserID, bool IsOnline)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		//ASSERT(false);
		return;
	}
	else
	{
		Iter->second.IsOnline = IsOnline;
		if (IsOnline)
		{
			Iter->second.LastLogonTime = time(NULL);
		}
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeUserOline msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_RoleChangeOnline), sizeof(LC_Cmd_ChangeUserOline));
			//msg.dwSrcUserID = m_pUser->GetUserID();
			msg.dwDestUserID = dwUserID;
			msg.IsOnline = IsOnline;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationLevel(DWORD dwUserID, WORD wLevel)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (Iter->second.wLevel == wLevel)
			return;
		Iter->second.wLevel = wLevel;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationLevel msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationLevel), sizeof(LC_Cmd_ChangeRelationLevel));
			msg.dwDestUserID = dwUserID;
			msg.wLevel = wLevel;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationFaceID(DWORD dwUserID, DWORD FaceID)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (Iter->second.dwFaceID == FaceID)
			return;
		Iter->second.dwFaceID = FaceID;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationFaceID msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationFaceID), sizeof(LC_Cmd_ChangeRelationFaceID));
			msg.dwDestUserID = dwUserID;
			msg.dwFaceID = FaceID;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationGender(DWORD dwUserID, bool bGender)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (Iter->second.bGender == bGender)
			return;
		Iter->second.bGender = bGender;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationGender msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationGender), sizeof(LC_Cmd_ChangeRelationGender));
			msg.dwDestUserID = dwUserID;
			msg.bGender = bGender;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationNickName(DWORD dwUserID, LPTSTR pNickName)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (_tcscmp(pNickName, Iter->second.DestNickName) == 0)
			return;
		TCHARCopy(Iter->second.DestNickName, CountArray(Iter->second.DestNickName), pNickName, _tcslen(pNickName));
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationNickName msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationNickName), sizeof(LC_Cmd_ChangeRelationNickName));
			msg.dwDestUserID = dwUserID;
			TCHARCopy(msg.cNickName, CountArray(msg.cNickName), pNickName, _tcslen(pNickName));
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationTitle(DWORD dwUserID, BYTE TitleID)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (Iter->second.TitleID == TitleID)
			return;
		Iter->second.TitleID = TitleID;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationTitle msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationTitle), sizeof(LC_Cmd_ChangeRelationTitle));
			msg.dwDestUserID = dwUserID;
			msg.TitleID = TitleID;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationAchievementPoint(DWORD dwUserID, DWORD dwAchievementPoint)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	//�޸���ҵĳɾ͵���
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		if (Iter->second.dwAchievementPoint == dwAchievementPoint)
			return;
		Iter->second.dwAchievementPoint = dwAchievementPoint;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationAchievementPoint msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationAchievementPoint), sizeof(LC_Cmd_ChangeRelationAchievementPoint));
			msg.dwDestUserID = dwUserID;
			msg.dwAchievementPoint = dwAchievementPoint;
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationCharmValue(DWORD dwUserID, DWORD pCharm[MAX_CHARM_ITEMSUM])
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		for (int i = 0; i < MAX_CHARM_ITEMSUM;++i)
			Iter->second.CharmArray[i] = pCharm[i];
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationCharmValue msg;
			SetMsgInfo(msg,GetMsgType(Main_Relation, LC_ChangeRelationCharmValue), sizeof(LC_Cmd_ChangeRelationCharmValue));
			msg.dwDestUserID = dwUserID;
			for (int i = 0; i < MAX_CHARM_ITEMSUM; ++i)
				msg.CharmInfo[i] = pCharm[i];
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationCLientIP(DWORD dwUserID, DWORD ClientIP)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		Iter->second.ClientIP = ClientIP;
		//��������ͻ���ȥ
		if (m_IsLoadToClient)
		{
			LC_Cmd_ChangeRelationIP msg;
			SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationIP), sizeof(LC_Cmd_ChangeRelationIP));
			msg.dwDestUserID = dwUserID;
			g_FishServer.GetAddressByIP(ClientIP, msg.IPAddress, CountArray(msg.IPAddress));
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationIsShowIpAddress(DWORD dwUserID, bool States)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		Iter->second.IsShowIpAddress = States;
		if (States)
		{
			//�޸ĵ�ַ
			LC_Cmd_ChangeRelationIP msg;
			SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationIP), sizeof(LC_Cmd_ChangeRelationIP));
			msg.dwDestUserID = dwUserID;
			g_FishServer.GetAddressByIP(Iter->second.ClientIP, msg.IPAddress, CountArray(msg.IPAddress));
			m_pUser->SendDataToClient(&msg);
		}
		else
		{
			LC_Cmd_ChangeRelationIP msg;
			SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationIP), sizeof(LC_Cmd_ChangeRelationIP));
			msg.dwDestUserID = dwUserID;
			TCHARCopy(msg.IPAddress, CountArray(msg.IPAddress), Defalue_Ip_Address, _tcslen(Defalue_Ip_Address));//Ŀ��ĵ�ַΪ����
			m_pUser->SendDataToClient(&msg);
		}
	}
}
void RoleRelationManager::OnChangeRelationVipLevel(DWORD dwUserID, BYTE VipLevel)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		Iter->second.VipLevel = VipLevel;
		LC_Cmd_ChangeRelationVipLevel msg;
		SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationVipLevel), sizeof(LC_Cmd_ChangeRelationVipLevel));
		msg.dwDestUserID = dwUserID;
		msg.VipLevel = VipLevel;
		m_pUser->SendDataToClient(&msg);
	}
}

void RoleRelationManager::OnChangeRelationWeekGlobal(DWORD dwUserID, __int64 WeekGoldNum)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		Iter->second.WeekGoldNum = WeekGoldNum;
		LC_Cmd_ChangeRelationWeekGlobal msg;
		SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationWeekGlobal), sizeof(LC_Cmd_ChangeRelationWeekGlobal));
		msg.dwDestUserID = dwUserID;
		msg.WeekGoldNum = WeekGoldNum;
		m_pUser->SendDataToClient(&msg);
	}
}

void RoleRelationManager::OnChangeRelationIsInMonthCard(DWORD dwUserID, bool IsInMonthCard)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return;
	}
	else
	{
		Iter->second.IsInMonthCard = IsInMonthCard;
		LC_Cmd_ChangeRelationIsInMonthCard msg;
		SetMsgInfo(msg, GetMsgType(Main_Relation, LC_ChangeRelationIsInMonthCard), sizeof(LC_Cmd_ChangeRelationIsInMonthCard));
		msg.dwDestUserID = dwUserID;
		msg.IsInMonthCard = IsInMonthCard;
		m_pUser->SendDataToClient(&msg);
	}
}
tagRoleRelation* RoleRelationManager::QueryRelationInfo(DWORD dwDestUserID)
{
	HashMap<DWORD, tagRoleRelation>::iterator Iter = m_RelationMap.find(dwDestUserID);
	if (Iter == m_RelationMap.end())
	{
		ASSERT(false);
		return null;
	}
	else
		return &Iter->second;
}