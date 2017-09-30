#include "Stdafx.h"
#include "AnnouncementManager.h"
#include "FishServer.h"
AnnouncementManager::AnnouncementManager()
{
	m_AnnouncementList.clear();
	m_RoleVec.clear();
	m_IsFinish = false;
}
AnnouncementManager::~AnnouncementManager()
{
	m_AnnouncementList.clear();
	m_RoleVec.clear();
	m_IsFinish = false;
}
void AnnouncementManager::OnConnectionCenter()
{
	//����������������ӵ�ʱ��
	m_AnnouncementList.clear();
	//���������������� ��ȡȫ���Ĺ���
	GC_Cmd_GetAllAnnouncement msg;
	SetMsgInfo(msg, GetMsgType(Main_Announcement, GC_GetAllAnnouncement), sizeof(GC_Cmd_GetAllAnnouncement));
	g_FishServer.SendNetCmdToCenter(&msg);
	m_IsFinish = false;
}
void AnnouncementManager::OnLoadAllAnnouncementInfoByCenter(CG_Cmd_GetAllAnnouncement* pMsg)
{
	if (!pMsg)
	{
		ASSERT(false);
		return;
	}
	if ((pMsg->States & MsgBegin) != 0)
	{
		m_AnnouncementList.clear();
	}
	for (WORD i = 0; i < pMsg->Sum; ++i)
	{
		m_AnnouncementList.push_back(pMsg->Array[i]);//��ӵ�ͷȥ
	}
	if ((pMsg->States & MsgEnd) != 0)
	{
		m_IsFinish = true;
		while (m_AnnouncementList.size() > g_FishServer.GetFishConfig().GetSystemConfig().AnnouncementSum)
		{
			m_AnnouncementList.pop_back();//�Ƴ�������һ������
		}
		//�����淢�͸�ȫ�����
		if (!m_RoleVec.empty())
		{
			std::vector<DWORD>::iterator Iter = m_RoleVec.begin();
			for (; Iter != m_RoleVec.end(); ++Iter)
			{
				SendNewAnnouncementToClent(*Iter);
			}
			m_RoleVec.clear();
		}
	}
}
//void AnnouncementManager::OnLoadAllAnnouncementInfoFinish()
//{
//	
//}
void AnnouncementManager::OnAddNewAnnouncementOnceByCenter(AnnouncementOnce& pOnce)
{
	//�����������GameServer���һ���µĹ���
	m_AnnouncementList.push_front(pOnce);//���뵽ͷ
	while (m_AnnouncementList.size() > g_FishServer.GetFishConfig().GetSystemConfig().AnnouncementSum)
	{
		m_AnnouncementList.pop_back();//�Ƴ�������һ������
	}
	LC_Cmd_SendNewAnnouncementOnce msg;
	SetMsgInfo(msg, GetMsgType(Main_Announcement, LC_SendNewAnnouncementOnce), sizeof(LC_Cmd_SendNewAnnouncementOnce));
	msg.pOnce = pOnce;
	g_FishServer.SendNewCmdToAllClient(&msg);//�����µĹ����ȫ���Ŀͻ���
}
void AnnouncementManager::OnAddNewAnnouncementOnce(const TCHAR *pNickName, BYTE ShopID, BYTE ShopOnlyID)
{
	GC_Cmd_SendNewAnnouncementOnce msg;
	SetMsgInfo(msg, GetMsgType(Main_Announcement, GC_SendNewAnnouncementOnce), sizeof(GC_Cmd_SendNewAnnouncementOnce));
	TCHARCopy(msg.pOnce.NickName, CountArray(msg.pOnce.NickName), pNickName, _tcslen(pNickName));
	msg.pOnce.ShopID = ShopID;
	msg.pOnce.ShopOnlyID = ShopOnlyID;
	g_FishServer.SendNetCmdToCenter(&msg);
}
void AnnouncementManager::SendNewAnnouncementToClent(DWORD dwUserID)
{
	//�ͻ��˻�ȡ�������˵Ĺ���
	if (!m_IsFinish)
	{
		m_RoleVec.push_back(dwUserID);
		return;
	}
	if (!g_FishServer.GetRoleManager())
	{
		ASSERT(false);
		return;
	}
	CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	DWORD ArraySum = min(g_FishServer.GetFishConfig().GetSystemConfig().AnnouncementSum, m_AnnouncementList.size());
	DWORD PageSize = sizeof(LC_Cmd_GetAllAnnouncement)+(ArraySum - 1)*sizeof(AnnouncementOnce);
	LC_Cmd_GetAllAnnouncement * msg = (LC_Cmd_GetAllAnnouncement*)malloc(PageSize);
	msg->SetCmdType(GetMsgType(Main_Announcement, LC_GetAllAnnouncement));
	std::deque<AnnouncementOnce>::iterator Iter = m_AnnouncementList.begin();
	for (DWORD i = 0; i<g_FishServer.GetFishConfig().GetSystemConfig().AnnouncementSum&& Iter != m_AnnouncementList.end(); ++Iter, ++i)
	{
		msg->Array[i] = *Iter;
	}
	std::vector<LC_Cmd_GetAllAnnouncement*> pVec;
	SqlitMsg(msg, PageSize, ArraySum, true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetAllAnnouncement*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
}