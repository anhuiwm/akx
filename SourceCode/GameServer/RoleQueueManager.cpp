#include "Stdafx.h"
#include "RoleQueueManager.h"
#include "FishServer.h"
#include "RoleEx.h"
RoleQueueManager::RoleQueueManager()
{
	m_QueueList.clear();
	m_UpdateTimeLog = 0;
	m_QueueMsgUpate = 0;
}
RoleQueueManager::~RoleQueueManager()
{

}
bool RoleQueueManager::OnAddRoleToQueue(DWORD dwUserID, DWORD ClientID, bool LogonByGameServer)
{
	CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (pRole)
	{
		ASSERT(false);
		return false;
	}
	ServerClientData* pClient = g_FishServer.GetUserClientDataByIndex(ClientID);
	if (!pClient)
	{
		ASSERT(false);
		return false;
	}
	tagRoleQueueOnce pOnce;
	pOnce.ClientID = ClientID;
	pOnce.dwUserID = dwUserID;
	pOnce.LogonByGameServer = LogonByGameServer;
	m_QueueList.push_back(pOnce);
	//����ͻ��� ��Ҫ�Ŷӵ�ʱ�� ÿ10�����10�� 
	DWORD HandleSum = g_FishServer.GetFishConfig().GetSystemConfig().UserQueueHandleSumBySec;
	DWORD WriteSec = ((m_QueueList.size() % HandleSum == 0) ? (m_QueueList.size() / HandleSum) : (m_QueueList.size() / HandleSum + 1)) * 1;//��Ҫ�ȴ�������
	//���͵��ͻ���ȥ
	LC_Cmd_LogonQueueWrite msg;
	SetMsgInfo(msg, GetMsgType(Main_Logon, LC_LogonQueueWrite), sizeof(LC_Cmd_LogonQueueWrite));
	msg.WriteIndex = m_QueueList.size();
	msg.WriteSec = WriteSec;
	g_FishServer.SendNewCmdToClient(pClient, &msg);
	return true;
}
void RoleQueueManager::OnUpdateQueue(DWORD dwTimer)
{
	if (m_UpdateTimeLog == 0 || dwTimer - m_UpdateTimeLog >= 1000)
	{
		m_UpdateTimeLog = dwTimer;
		//��������
		if (m_QueueList.empty())
			return;

		DWORD HandleSum = g_FishServer.GetFishConfig().GetSystemConfig().UserQueueHandleSumBySec;

		std::list<tagRoleQueueOnce>::iterator Iter = m_QueueList.begin();
		for (size_t i = 0; Iter != m_QueueList.end() && i< HandleSum;)
		{
			ServerClientData* pClient = g_FishServer.GetUserClientDataByIndex(Iter->ClientID);
			if (!pClient)
			{
				//��Ҳ��Ŷ��� ��һ��
				Iter = m_QueueList.erase(Iter);
				continue;
			}
			//һ����� 10�����
			DWORD dwUserID = Iter->dwUserID;
			//����ҽ��е�½ �����ǽ������ݿ�ĵ�½
			CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
			if (pRole)
			{
				ASSERT(false);
				if (pRole->IsRobot())
				{
					ASSERT(false);
				}
				else
				{
					if (!pRole->IsAFK())
					{
						NetCmd msg;
						SetMsgInfo(msg, GetMsgType(Main_Logon, LC_ServerChangeSocket), sizeof(NetCmd));
						pRole->SendDataToClient(&msg);

						DelClient pDel;
						pDel.LogTime = timeGetTime();
						pDel.SocketID = pRole->GetGameSocketID();
						g_FishServer.AddDelRoleSocket(pDel);

						//���Socket�����л�
						DWORD SocketID = pRole->GetGameSocketID();
						pRole->ChangeRoleSocketID(Iter->ClientID);

						//���߿ͻ�����ҵ�½�ɹ���
						pRole->OnUserLoadFinish(Iter->LogonByGameServer);//��ҵ�½ ������������
					}
					else
					{
						g_FishServer.GetTableManager()->OnPlayerLeaveTable(pRole->GetUserID());//���뿪����
						g_FishServer.GetRoleCache().OnDleRoleCache(pRole->GetUserID());
						pRole->ChangeRoleSocketID(Iter->ClientID);//�޸Ļ�����ҵ�Socket
						pRole->OnUserLoadFinish(Iter->LogonByGameServer);//���� ���� �������������� ���� ��������������߲���
					}
					if (pRole->IsExit())
					{
						pRole->SetIsExit(false);//���������������
						//g_FishServer.ShowInfoToWin("��ҽ������߱��� �ڻط�ȷ������֮ǰ ���µ�½��  �ط�ȷ������ ʧЧ ");
					}
					
				}
			}
			else
			{
				//����ҽ������ݿ��ѯ ��½
				DBR_Cmd_GetAccountInfoByUserID msg;
				SetMsgInfo(msg, DBR_GetAccountInfoByUserID, sizeof(DBR_Cmd_GetAccountInfoByUserID));
				msg.dwUserID = dwUserID;
				msg.ClientID = Iter->ClientID;
				msg.LogonByGameServer = true;
				g_FishServer.SendNetCmdToDB(&msg);

				++i;
			}
			Iter = m_QueueList.erase(Iter);
		}
	}
}