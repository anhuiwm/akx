#include "Stdafx.h"
#include "RoleEx.h"
#include "RoleGiff.h"
#include "FishServer.h"
RoleGiffManager::RoleGiffManager()
{
	m_IsLoadDB = false;
	m_IsSendClient = false;
	m_GiffVec.clear();
}
RoleGiffManager::~RoleGiffManager()
{

}
bool RoleGiffManager::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	return OnLoadRoleGiff();
}
bool RoleGiffManager::OnLoadRoleGiff()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	//����DBR����
	/*DBR_Cmd_LoadRoleGiff msg;
	SetMsgInfo(msg,DBR_LoadRoleGiff, sizeof(DBR_Cmd_LoadRoleGiff));
	msg.dwUserID = m_pRole->GetUserID();
	msg.LimitDay = g_FishServer.GetFishConfig().GetGiffConfig().LimitDay;
	g_FishServer.SendNetCmdToDB(&msg);*/
	return true;
}
void RoleGiffManager::OnLoadRoleGiffResult(DBO_Cmd_LoadRoleGiff* pDB)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (!pDB || pDB->dwUserID != m_pRole->GetUserID())
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_GiffVec.clear();
	}
	DBR_Cmd_DelRoleGiff msgDB;
	SetMsgInfo(msgDB, DBR_DelRoleGiff, sizeof(DBR_Cmd_DelRoleGiff));
	for (WORD i = 0; i < pDB->Sum; ++i)
	{
		//�ж������Ƿ������
		DWORD DiffDay = GetDiffDay(pDB->Array[i].SendTime, g_FishServer.GetFishConfig().GetWriteSec());
		if (DiffDay > g_FishServer.GetFishConfig().GetGiffConfig().LimitDay)
		{
			msgDB.GiffID = pDB->Array[i].OnlyID;
			g_FishServer.SendNetCmdToSaveDB(&msgDB);//�Ƴ�������
			continue;
		}
		m_GiffVec.push_back(pDB->Array[i]);
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Giff);

		m_IsLoadDB = true;
		m_pRole->IsLoadFinish();
	}
}
void RoleGiffManager::OnLoadRoleSendGiffInfo(DBO_Cmd_GetNowDayGiff* pDB)
{
	if (!pDB || !m_pRole || pDB->dwUserID != m_pRole->GetUserID())
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_SendInfo.clear();
	}
	for (size_t i = 0; i < pDB->Sum; ++i)
	{
		m_SendInfo.insert(HashMap<DWORD, tagNowDaySendGiffInfo>::value_type(pDB->Array[i].dwUserID, pDB->Array[i]));
	}
}
void RoleGiffManager::GetRoleGiffToClient()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//�����͵����ݷ��͵��ͻ���ȥ
	DWORD PageSize = sizeof(LC_Cmd_GetRoleGiff)+sizeof(tagGiffClientInfo)*(m_GiffVec.size() - 1);
	LC_Cmd_GetRoleGiff * msg = (LC_Cmd_GetRoleGiff*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	msg->SetCmdType(GetMsgType(Main_Giff, LC_GetRoleGiff));
	std::vector<tagGiffInfo>::iterator Iter = m_GiffVec.begin();
	for (WORD i = 0; Iter != m_GiffVec.end(); ++Iter, ++i)
	{
		msg->Array[i].dwUserID = Iter->dwUserID;
		msg->Array[i].dwFaceID = Iter->dwFaceID;
		msg->Array[i].OnlyID = Iter->OnlyID;
		__int64 diffSec = time(NULL) - Iter->SendTime;
		__int64 diffDay = diffSec / (24 * 60 * 60);
		if (diffDay > 250)
			diffDay = 250;
		msg->Array[i].DiffTime = (BYTE)diffDay;
		TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), Iter->NickName, _tcslen(Iter->NickName));
	}
	std::vector<LC_Cmd_GetRoleGiff*> pVec;
	SqlitMsg(msg, PageSize, m_GiffVec.size(), true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetRoleGiff*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	//�����������
	m_IsSendClient = true;//�Ѿ����͵��ͻ���ȥ�� 
}
void RoleGiffManager::SendNowDaySendGiffToClient()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	DWORD PageSize = sizeof(LC_Cmd_GetRoleNowDaySendGiff)+sizeof(tagNowDaySendGiffInfo)*(m_SendInfo.size());
	LC_Cmd_GetRoleNowDaySendGiff * msg = (LC_Cmd_GetRoleNowDaySendGiff*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	msg->SetCmdType(GetMsgType(Main_Giff, LC_GetRoleNowDaySendGiff));
	HashMap<DWORD, tagNowDaySendGiffInfo>::iterator Iter = m_SendInfo.begin();
	for (WORD i = 0; Iter != m_SendInfo.end(); ++Iter,++i)
	{
		msg->Array[i] = Iter->second;
	}
	std::vector<LC_Cmd_GetRoleNowDaySendGiff*> pVec;
	SqlitMsg(msg, PageSize, m_SendInfo.size(),true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetRoleNowDaySendGiff*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
}
void RoleGiffManager::SendGiffToOtherRole(DWORD dwUserID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	//�ж��Ƿ���Է���
	if (dwUserID == m_pRole->GetUserID())
		return;
	if (g_FishServer.GetFishConfig().GetGiffConfig().SendSumLimitByDay != 0 && m_pRole->GetRoleInfo().SendGiffSum >= g_FishServer.GetFishConfig().GetGiffConfig().SendSumLimitByDay)
		return;
	HashMap<DWORD, tagNowDaySendGiffInfo>::iterator Iter = m_SendInfo.find(dwUserID);
	if (Iter != m_SendInfo.end() && Iter->second.SendSum >= g_FishServer.GetFishConfig().GetGiffConfig().SendOnceUserLimitByDay)
		return;
	//��Ŀ����ҷ������� ��Ҫ��ͨ����ҽ��д����ж� �Ƿ���Է��� Ȼ�󷵻����� ���д���
	CRoleEx* pDestRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (pDestRole)
	{
		//ֱ�ӷ��͵�����������
		pDestRole->GetRoleGiffManager().AddBeGiff(m_pRole->GetUserID());//��ָ����ҷ�������
	}
	else
	{
		GC_Cmd_AddRoleGiff msg;
		SetMsgInfo(msg, GetMsgType(Main_Giff, GC_AddRoleGiff), sizeof(GC_Cmd_AddRoleGiff));
		msg.dwDestUserID = dwUserID;
		msg.dwSrcUserID = m_pRole->GetUserID();
		m_pRole->SendDataToCenter(&msg);
	}
}
void RoleGiffManager::SendGiffToOtherRoleResult(DWORD dwDestUserID)
{
	//������͵ķ��ؽ��
	if (dwDestUserID == m_pRole->GetUserID())
	{
		ASSERT(false);
		return;
	}
	if (!m_pRole->ChangeRoleSendGiffSum(1))
	{
		ASSERT(false);
		return;
	}

	//�����¼�
	m_pRole->OnHandleEvent(true, true, true, ET_SendGiff, 0, 1);

	//��¼����
	HashMap<DWORD, tagNowDaySendGiffInfo>::iterator Iter = m_SendInfo.find(dwDestUserID);
	if (Iter == m_SendInfo.end())
	{
		tagNowDaySendGiffInfo p;
		p.dwUserID = dwDestUserID;
		p.SendSum = 1;
		m_SendInfo.insert(HashMap<DWORD, tagNowDaySendGiffInfo>::value_type(dwDestUserID, p));
	}
	else
	{
		Iter->second.SendSum += 1;
	}
	return;
}
void RoleGiffManager::AcceptRoleGiff(DWORD GiffID)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay != 0 && m_pRole->GetRoleInfo().AcceptGiffSum >= g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay)
	{
		return;
	}
	//����һ������
	std::vector<tagGiffInfo>::iterator Iter = m_GiffVec.begin();
	for (; Iter != m_GiffVec.end(); ++Iter)
	{
		if (Iter->OnlyID == GiffID)
		{
			if (!m_pRole->ChangeRoleAcceptGiffSum(1))
			{
				ASSERT(false);
				return;
			}
			m_pRole->OnAddRoleRewardByRewardID(g_FishServer.GetFishConfig().GetGiffConfig().RewardID,TEXT("��ȡ������ͽ���"));

			DBR_Cmd_DelRoleGiff msg;
			SetMsgInfo(msg,DBR_DelRoleGiff, sizeof(DBR_Cmd_DelRoleGiff));
			msg.GiffID = GiffID;
			g_FishServer.SendNetCmdToSaveDB(&msg);

			LC_Cmd_GetRoleGiffReward msgClient;
			SetMsgInfo(msgClient,GetMsgType(Main_Giff, LC_GetRoleGiffReward), sizeof(LC_Cmd_GetRoleGiffReward));
			msgClient.GiffID = GiffID;
			m_pRole->SendDataToClient(&msgClient);

			m_GiffVec.erase(Iter);

			m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Giff);
			return;
		}
	}
}
void RoleGiffManager::AcceptAllGiff()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay != 0 && m_pRole->GetRoleInfo().AcceptGiffSum >= g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay)
	{
		return;
	}
	BYTE AcceptGiffSum = min((g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay - m_pRole->GetRoleInfo().AcceptGiffSum),(BYTE) m_GiffVec.size());
	if (AcceptGiffSum < 0)
		return;
	std::vector<tagGiffInfo>::iterator Iter = m_GiffVec.begin();
	DWORD PageSize = sizeof(LC_Cmd_GetAllRoleGiffReward)+(AcceptGiffSum - 1)*sizeof(DWORD);
	LC_Cmd_GetAllRoleGiffReward* pMsg = (LC_Cmd_GetAllRoleGiffReward*)malloc(PageSize);
	if (!pMsg)
	{
		ASSERT(false);
		return;
	}
	WORD UseGiffSum = 0;
	pMsg->SetCmdType(GetMsgType(Main_Giff, LC_GetAllRoleGiffReward));
	for (; Iter != m_GiffVec.end() && UseGiffSum< AcceptGiffSum; ++UseGiffSum)
	{
		if (!m_pRole->ChangeRoleAcceptGiffSum(1))
		{
			free(pMsg);
			ASSERT(false);
			return;
		}
		m_pRole->OnAddRoleRewardByRewardID(g_FishServer.GetFishConfig().GetGiffConfig().RewardID, TEXT("��ȡ������ͽ���"));
		DBR_Cmd_DelRoleGiff msg;
		SetMsgInfo(msg,DBR_DelRoleGiff, sizeof(DBR_Cmd_DelRoleGiff));
		msg.GiffID = Iter->OnlyID;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		pMsg->Array[UseGiffSum] = Iter->OnlyID;

		Iter = m_GiffVec.erase(Iter);
	}
	PageSize = sizeof(LC_Cmd_GetAllRoleGiffReward)+(UseGiffSum - 1)*sizeof(DWORD);
	std::vector<LC_Cmd_GetAllRoleGiffReward*> pVec;
	SqlitMsg(pMsg, PageSize, UseGiffSum,true, pVec);
	free(pMsg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetAllRoleGiffReward*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Giff);
}
void RoleGiffManager::OnDayChange()
{
	//ÿ��4����и��� ��ȫ�����ڵ�����ɾ����
	time_t Time = time(NULL);

	DWORD PageSize = sizeof(LC_Cmd_DelRoleGiff)+(m_GiffVec.size() - 1)*sizeof(DWORD);
	LC_Cmd_DelRoleGiff * msg = (LC_Cmd_DelRoleGiff*)malloc(PageSize);
	msg->SetCmdType(GetMsgType(Main_Giff, LC_DelRoleGiff));
	
	DWORD ArraySum = 0;

	std::vector<tagGiffInfo>::iterator Iter = m_GiffVec.begin();
	for (; Iter != m_GiffVec.end();)
	{
		DWORD DiffDay = GetDiffDay(Iter->SendTime, g_FishServer.GetFishConfig().GetWriteSec());
		if (DiffDay > g_FishServer.GetFishConfig().GetGiffConfig().LimitDay)
		{
			msg->Array[ArraySum] = Iter->OnlyID;
			//����ɾ������
			DBR_Cmd_DelRoleGiff msgDB;
			SetMsgInfo(msgDB, DBR_DelRoleGiff, sizeof(DBR_Cmd_DelRoleGiff));
			msgDB.GiffID = Iter->OnlyID;
			g_FishServer.SendNetCmdToSaveDB(&msgDB);

			Iter = m_GiffVec.erase(Iter);
			++ArraySum;
		}
		else
			++Iter;
	}
	PageSize = sizeof(LC_Cmd_DelRoleGiff)+(ArraySum-1)*sizeof(DWORD);

	std::vector<LC_Cmd_DelRoleGiff*> pVec;
	SqlitMsg(msg, PageSize, ArraySum,true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_DelRoleGiff*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			if (m_IsSendClient)
				m_pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_SendInfo.clear();//���췢�͵����

	DBR_Cmd_ClearNowDayGiff msgDBSave;
	SetMsgInfo(msgDBSave, DBR_ClearNowDayGiff, sizeof(DBR_Cmd_ClearNowDayGiff));
	msgDBSave.dwUserID = m_pRole->GetUserID();
	g_FishServer.SendNetCmdToSaveDB(&msgDBSave);

	m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Giff);
}
void RoleGiffManager::AddBeGiff(DWORD SrcUserID)
{
	//�ж��Ƿ�����������
	if (m_GiffVec.size() >= g_FishServer.GetFishConfig().GetGiffConfig().MaxUserGiffSum)
	{
		CRoleEx* pSrcRole = g_FishServer.GetRoleManager()->QueryUser(SrcUserID);
		if (pSrcRole)
		{
			LC_Cmd_AddRoleGiffResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Giff, LC_AddRoleGiffResult), sizeof(LC_Cmd_AddRoleGiffResult));
			msg.dwDestUserID = m_pRole->GetUserID();
			msg.Result = false;
			pSrcRole->SendDataToClient(&msg);
		}
		else
		{
			GC_Cmd_AddRoleGiffResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Giff, GC_AddRoleGiffResult), sizeof(GC_Cmd_AddRoleGiffResult));
			msg.dwDestUserID = m_pRole->GetUserID();
			msg.Result = false;
			msg.dwSrcUserID = SrcUserID;
			m_pRole->SendDataToCenter(&msg);
		}
		return;
	}
	//�������� ���͵����ݿ�ȥ
	DBR_Cmd_AddRoleGiff msg;
	SetMsgInfo(msg, DBR_AddRoleGiff, sizeof(DBR_Cmd_AddRoleGiff));
	msg.SrcUserID = SrcUserID;
	msg.DestUserID = m_pRole->GetUserID();
	g_FishServer.SendNetCmdToDB(&msg);
}
void RoleGiffManager::AddBeGiffResult(DBO_Cmd_AddRoleGiff* pMsg)
{
	if (!m_pRole || !pMsg)
	{
		ASSERT(false);
		return;
	}
	if (pMsg->Result)
	{
		//���ͳɹ���
		m_GiffVec.push_back(pMsg->GiffInfo);
		m_pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Giff);
		if (m_IsSendClient)
		{
			//��������ͻ���ȥ
			LC_Cmd_AddBeRoleGiff msg;
			SetMsgInfo(msg, GetMsgType(Main_Giff, LC_AddBeRoleGiff), sizeof(LC_Cmd_AddBeRoleGiff));
			msg.GiffInfo.dwUserID = pMsg->GiffInfo.dwUserID;
			msg.GiffInfo.dwFaceID = pMsg->GiffInfo.dwFaceID;
			msg.GiffInfo.OnlyID = pMsg->GiffInfo.OnlyID;
			__int64 diffSec = time(NULL) - pMsg->GiffInfo.SendTime;
			__int64 diffDay = diffSec / (24 * 60 * 60);
			if (diffDay > 250)
				diffDay = 250;
			msg.GiffInfo.DiffTime = (BYTE)diffDay;
			TCHARCopy(msg.GiffInfo.NickName, CountArray(msg.GiffInfo.NickName), pMsg->GiffInfo.NickName, _tcslen(pMsg->GiffInfo.NickName));
			m_pRole->SendDataToClient(&msg);
		}
	}
	//���������Ƿ�ɹ� ����Ҫ���ͽ��
	CRoleEx* pSrcRole = g_FishServer.GetRoleManager()->QueryUser(pMsg->GiffInfo.dwUserID);
	if (pSrcRole)
	{
		if (pMsg->Result)
			pSrcRole->GetRoleGiffManager().SendGiffToOtherRoleResult(pMsg->dwDestUserID);//���ͳɹ���ʱ�� 
		LC_Cmd_AddRoleGiffResult msg;
		SetMsgInfo(msg, GetMsgType(Main_Giff, LC_AddRoleGiffResult), sizeof(LC_Cmd_AddRoleGiffResult));
		msg.dwDestUserID = m_pRole->GetUserID();
		msg.Result = pMsg->Result;
		pSrcRole->SendDataToClient(&msg);
	}
	else
	{
		GC_Cmd_AddRoleGiffResult msg;
		SetMsgInfo(msg, GetMsgType(Main_Giff, GC_AddRoleGiffResult), sizeof(GC_Cmd_AddRoleGiffResult));
		msg.dwDestUserID = m_pRole->GetUserID();
		msg.dwSrcUserID = pMsg->GiffInfo.dwUserID;
		msg.Result = pMsg->Result;
		m_pRole->SendDataToCenter(&msg);
	}
}
bool RoleGiffManager::GetGiffMessageStates()
{
	if (m_GiffVec.empty())
		return false;
	if (g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay != 0 && m_pRole->GetRoleInfo().AcceptGiffSum >= g_FishServer.GetFishConfig().GetGiffConfig().AcceptSubLimitByDay)
		return false;
	return true;
}