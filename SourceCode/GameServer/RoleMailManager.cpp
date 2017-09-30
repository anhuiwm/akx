#include "Stdafx.h"
#include "RoleMailManager.h"
#include "RoleEx.h"
#include "RoleManager.h"
#include "FishServer.h"
RoleMailManager::RoleMailManager()
{
	m_IsLoadToClient = false;
	m_IsLoadDB = false;
	m_RoleManager = NULL;
	m_pUser = NULL;
	m_ClientMailSum = 0;
}
RoleMailManager::~RoleMailManager()
{
	if (!m_RoleMailVec.empty())
	{
		std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
		for (; Iter != m_RoleMailVec.end(); ++Iter)
		{
			delete *Iter;
		}
		m_RoleMailVec.clear();
	}

	//if (!m_RoleMailRecordVec.empty())
	//{
	//	std::list<tagRoleMailRecord*>::iterator IterRc = m_RoleMailRecordVec.begin();
	//	for (; IterRc != m_RoleMailRecordVec.end(); ++IterRc)
	//	{
	//		delete *IterRc;
	//	}
	//	m_RoleMailRecordVec.clear();
	//}
}
bool RoleMailManager::OnInit(CRoleEx* pUser, RoleManager* pManager)
{
	//��ȡ�ʼ�����
	if (!pUser ||  !pManager)
	{
		ASSERT(false);
		return false;
	}
	m_RoleManager = pManager;
	m_pUser = pUser;
	m_RoleMailVec.clear();
	//m_RoleMailRecordVec.clear();
	return OnLoadUserMailByPage();//���ص�0ҳ���ʼ�
}
bool RoleMailManager::OnLoadUserMailByPage()
{
	//������ȫ�����������
	//����ָ��ҳ����ʼ����� ����DBR����
	if ( !m_pUser)
	{
		ASSERT(false);
		return false;
	} 
	m_IsLoadDB = false;
	m_RoleMailVec.clear();
	//DBR_Cmd_LoadUserMail msg;
	//SetMsgInfo(msg,DBR_LoadUserMail, sizeof(DBR_Cmd_LoadUserMail));
	//msg.dwUserID = m_pUser->GetUserID();
	//g_FishServer.SendNetCmdToDB(&msg);
	return true;
}
bool RoleMailManager::OnLoadUserMailByPageResult(DBO_Cmd_LoadUserMail* pDB)
{
	//�����ʼ�������
	if (!pDB || !m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if ((pDB->States & MsgBegin) != 0)
	{
		m_RoleMailVec.clear();
	}
	DBR_Cmd_DelUserMail msg;
	SetMsgInfo(msg, DBR_DelUserMail, sizeof(DBR_Cmd_DelUserMail));
	for (size_t i = 0; i < pDB->Sum; ++i)
	{
		//1.�ж��ʼ��Ƿ������ �����˵Ļ� ����ɾ����
		int DiffDay = GetDiffDay(pDB->Array[i].SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());//�����ʼ�������������� 
		if (DiffDay > g_FishServer.GetFishConfig().GetFishMailConfig().MailLimitDay && pDB->Array[i].SrcUserID != 0) //����ʼ��Ž���ɾ���ж� ϵͳ�ʼ������й����ж�
		{
			msg.dwMailID = pDB->Array[i].MailID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			continue;
		}
		tagRoleMail* pMail = new tagRoleMail;
		memcpy_s(pMail, sizeof(tagRoleMail), &pDB->Array[i], sizeof(tagRoleMail));
		m_RoleMailVec.push_back(pMail);
	}
	if ((pDB->States & MsgEnd) != 0)
	{
		m_IsLoadDB = true;
		m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
		m_pUser->IsLoadFinish();//�ж�����Ƿ��������
	}
	return true;
}


bool RoleMailManager::OnLoadUserMailRecordResult(DBO_Cmd_LoadUserMailRecord* pDB)
{
	////�����ʼ�������
	//if (!pDB || !m_pUser)
	//{
	//	ASSERT(false);
	//	return false;
	//}
	//if ((pDB->States & MsgBegin) != 0)
	//{
	//	m_RoleMailVec.clear();
	//}
	//DBR_Cmd_DelUserMail msg;
	//SetMsgInfo(msg, DBR_DelUserMail, sizeof(DBR_Cmd_DelUserMail));
	//for (size_t i = 0; i < pDB->Sum; ++i)
	//{
	//	//1.�ж��ʼ��Ƿ������ �����˵Ļ� ����ɾ����
	//	int DiffDay = GetDiffDay(pDB->Array[i].SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());//�����ʼ�������������� 
	//	if (DiffDay > g_FishServer.GetFishConfig().GetFishMailConfig().MailLimitDay && pDB->Array[i].SrcUserID != 0) //����ʼ��Ž���ɾ���ж� ϵͳ�ʼ������й����ж�
	//	{
	//		msg.dwMailID = pDB->Array[i].MailID;
	//		g_FishServer.SendNetCmdToSaveDB(&msg);
	//		continue;
	//	}
	//	tagRoleMail* pMail = new tagRoleMail;
	//	memcpy_s(pMail, sizeof(tagRoleMail), &pDB->Array[i], sizeof(tagRoleMail));
	//	m_RoleMailVec.push_back(pMail);
	//}
	//if ((pDB->States & MsgEnd) != 0)
	//{
	//	m_IsLoadDB = true;
	//	m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
	//	m_pUser->IsLoadFinish();//�ж�����Ƿ��������
	//}
	return true;
}

//void RoleMailManager::OnLoadUserMailFinish()
//{
//	if (!m_pUser)
//	{
//		ASSERT(false);
//		return;
//	}
//	
//}
bool RoleMailManager::OnGetAllUserMail()
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//��һ�ȡ�ʼ���Ϣ
	//��ͻ��˷������� �ѵ�ǰ�ʼ���ǰ���ٷ��͵��ͻ���ȥ
	//���� �ȷ�����ͨ�ʼ� �����ϵͳ�ʼ� ϵͳ�ʼ�������� �ڿͻ��˱�������ʼ��Ѿ��������
	m_ClientMailSum = 0;
	//1.�ȷ���ͨ���ʼ�
	DWORD MailSum = g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum;

	DWORD NormalPageSize = sizeof(LC_Cmd_NormalMail)+sizeof(tagNormalMail)*(MailSum - 1);
	LC_Cmd_NormalMail * msgNormal = (LC_Cmd_NormalMail*)malloc(NormalPageSize);
	if (!msgNormal)
	{
		ASSERT(false);
		return false;
	}
	msgNormal->SetCmdType(GetMsgType(Main_Mail, LC_GetUserNormalMail));
	msgNormal->Sum = 0;

	DWORD SystemPageSize = sizeof(LC_Cmd_SystemMail)+sizeof(tagSystemMail)*(MailSum - 1);
	LC_Cmd_SystemMail * msgSystem = (LC_Cmd_SystemMail*)malloc(SystemPageSize);
	if (!msgSystem)
	{
		ASSERT(false);
		return false;
	}
	msgSystem->SetCmdType(GetMsgType(Main_Mail, LC_GetUserSystemMail));
	msgSystem->Sum = 0;

	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	for (BYTE i = 0, j = 0, k = 0; Iter != m_RoleMailVec.end() && i < g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum; ++Iter, ++i)
	{
		if ((*Iter)->SrcUserID == 0)//ϵͳ�ʼ�
		{
			//DWORD		MailID;
			//TCHAR		Context[MAX_MAIL_CONTEXT + 1];//�ʼ�����
			//WORD		RewardID;
			//DWORD		RewardSum;//����������
			//BYTE		bDiffTime;//����������
			//bool		bIsRead;//�Ƿ��Ѿ��Ķ�
			//bool		bIsExistsReward;//�Ƿ���ڽ���


			msgSystem->Array[k].MailID = (*Iter)->MailID;
			msgSystem->Array[k].bIsRead = (*Iter)->bIsRead;
			__int64 diffDay = GetDiffDay((*Iter)->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
			if (diffDay > 250)
				diffDay = 250;
			msgSystem->Array[k].bDiffTime = (BYTE)diffDay;
			TCHARCopy(msgSystem->Array[k].Context, CountArray(msgSystem->Array[k].Context), (*Iter)->Context, _tcslen((*Iter)->Context));
			msgSystem->Array[k].RewardID = (*Iter)->RewardID;
			msgSystem->Array[k].RewardSum = (*Iter)->RewardSum;
			msgSystem->Array[k].bIsExistsReward = (*Iter)->bIsExistsReward;
			//if (((*Iter)->RewardID != 0 && (*Iter)->RewardSum != 0) && (*Iter)->bIsExistsReward)
			//	msgSystem->Array[k].bIsExistsItem = true;
			//else
			//	msgSystem->Array[k].bIsExistsItem = false;
			++k;
			msgSystem->Sum = k;
		}
		else
		{
			//����ʼ�
			msgNormal->Array[j].MailID = (*Iter)->MailID;
			msgNormal->Array[j].bIsRead = (*Iter)->bIsRead;
			msgNormal->Array[j].SrcFaceID = (*Iter)->SrcFaceID;
			msgNormal->Array[j].SrcUserID = (*Iter)->SrcUserID;
			TCHARCopy(msgNormal->Array[j].SrcNickName, CountArray(msgNormal->Array[j].SrcNickName), (*Iter)->SrcNickName, _tcslen((*Iter)->SrcNickName));
			msgNormal->Array[j].RewardID = (*Iter)->RewardID;
			msgNormal->Array[j].RewardSum = (*Iter)->RewardSum;
			TCHARCopy(msgNormal->Array[j].Context, CountArray(msgNormal->Array[j].Context), (*Iter)->Context, _tcslen((*Iter)->Context));
			//if (((*Iter)->RewardID != 0 && (*Iter)->RewardSum != 0) && (*Iter)->bIsExistsReward)
			//	msgNormal->Array[j].bIsExistsItem = true;
			//else
			//	msgNormal->Array[j].bIsExistsItem = false;
			__int64 diffDay = GetDiffDay((*Iter)->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
			if (diffDay > 250)
				diffDay = 250;
			msgNormal->Array[j].bDiffTime = (BYTE)diffDay;
			++j;
			msgNormal->Sum = j;
		}
		++m_ClientMailSum;//�ͻ����ʼ�����
	}
	//�����Ѿ�׼����Ϻ�
	{
		std::vector<LC_Cmd_NormalMail*> pVec;
		SqlitMsg(msgNormal, sizeof(LC_Cmd_NormalMail)+sizeof(tagNormalMail)*(msgNormal->Sum - 1), msgNormal->Sum, true, pVec);
		free(msgNormal);
		if (!pVec.empty())
		{
			std::vector<LC_Cmd_NormalMail*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				m_pUser->SendDataToClient(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
	{
		std::vector<LC_Cmd_SystemMail*> pVec;
		SqlitMsg(msgSystem, sizeof(LC_Cmd_SystemMail)+sizeof(tagSystemMail)*(msgSystem->Sum - 1), msgSystem->Sum, true, pVec);
		free(msgSystem);
		if (!pVec.empty())
		{
			std::vector<LC_Cmd_SystemMail*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				m_pUser->SendDataToClient(*Iter);
				free(*Iter);
			}
			pVec.clear();
		}
	}
	/*LC_Cmd_GetMailFinish msgFinish;
	SetMsgInfo(msgFinish,GetMsgType(Main_Mail, LC_GetUserMailFinish), sizeof(LC_Cmd_GetMailFinish));
	m_pUser->SendDataToClient(&msgFinish);*/
	m_IsLoadToClient = true;
	return true;
}
//bool RoleMailManager::OnGetUserMailContext(DWORD MailID)
//{
//	if ( !m_pUser)
//	{
//		ASSERT(false);
//		return false;
//	}
//	//��һ���ʼ�����ϸ��Ϣ���͵��ͻ���ȥ 
//	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
//	for (; Iter != m_RoleMailVec.end(); ++Iter)
//	{
//		if ((*Iter)->MailID == MailID)
//		{
//			//����DBR����
//			if ((*Iter)->bIsRead == false)
//			{
//				(*Iter)->bIsRead = true;
//
//				m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
//
//				DBR_Cmd_ChangeUserMailIsRead msg;
//				SetMsgInfo(msg,DBR_ChangeUserMailIsRead, sizeof(DBR_Cmd_ChangeUserMailIsRead));
//				msg.dwMailID = (*Iter)->MailID;
//				msg.bIsRead = true;
//				g_FishServer.SendNetCmdToSaveDB(&msg);
//			}
//			//��ȡ���ʼ���  ���ǽ������� 
//			if ((*Iter)->SrcUserID == 0)
//			{
//				//ϵͳ�ʼ� 
//				LC_Cmd_GetSystemMail msg;
//				SetMsgInfo(msg,GetMsgType(Main_Mail, LC_ReadSystemMail), sizeof(LC_Cmd_GetSystemMail));
//				//msg.dwUserID = m_pUser->GetUserID();
//				msg.MailInfo.MailID = (*Iter)->MailID;
//				msg.MailInfo.bIsRead = (*Iter)->bIsRead;
//				__int64 diffDay = GetDiffDay((*Iter)->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
//				if (diffDay > 250)
//					diffDay = 250;
//				msg.MailInfo.bDiffTime = (BYTE)diffDay;
//				TCHARCopy(msg.MailInfo.Context, CountArray(msg.MailInfo.Context), (*Iter)->Context, _tcslen((*Iter)->Context));
//				msg.MailInfo.RewardID = (*Iter)->RewardID;
//				msg.MailInfo.RewardSum = (*Iter)->RewardSum;
//				msg.MailInfo.bIsExistsReward = (*Iter)->bIsExistsReward;
//				m_pUser->SendDataToClient(&msg);
//				return true;
//			}
//			else
//			{
//				//��ͨ�ʼ�
//				LC_Cmd_GetNormalMail msg;
//				SetMsgInfo(msg,GetMsgType(Main_Mail,LC_ReadNormalMail), sizeof(LC_Cmd_GetNormalMail));
//				msg.MailInfo.MailID = (*Iter)->MailID;
//				msg.MailInfo.bIsRead = (*Iter)->bIsRead;
//				msg.MailInfo.SrcFaceID = (*Iter)->SrcFaceID;
//				msg.MailInfo.SrcUserID = (*Iter)->SrcUserID;
//				msg.MailInfo.RewardID = (*Iter)->RewardID;
//				msg.MailInfo.RewardSum = (*Iter)->RewardSum;
//				TCHARCopy(msg.MailInfo.SrcNickName, CountArray(msg.MailInfo.SrcNickName), (*Iter)->SrcNickName, _tcslen((*Iter)->SrcNickName));
//				TCHARCopy(msg.MailInfo.Context, CountArray(msg.MailInfo.Context), (*Iter)->Context, _tcslen((*Iter)->Context));
//				__int64 diffDay = GetDiffDay((*Iter)->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
//				if (diffDay > 250)
//					diffDay = 250;
//				msg.MailInfo.bDiffTime = (BYTE)diffDay;
//				m_pUser->SendDataToClient(&msg);
//				return true;
//			}
//		}
//	}
//	//�Ҳ����ʼ� ����Ҳ��Ҫ��Ӧ��ҵĲ���
//	LC_Cmd_GetMailError msg;
//	SetMsgInfo(msg,GetMsgType(Main_Mail, LC_ReadMailError), sizeof(LC_Cmd_GetMailError));
//	//msg.dwUserID = m_pUser->GetUserID();
//	msg.MailID = MailID;
//	m_pUser->SendDataToClient(&msg);
//	return true;
//}

bool RoleMailManager::OnGetUserAllMailItem(BYTE type)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//���ʼ����ȡ��Ʒ
	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	std::map<WORD, DWORD> mapReward;
	std::map<WORD, DWORD> mapItem;
	for (; Iter != m_RoleMailVec.end(); ++Iter)
	{
		if (type == 0)//ϵͳ�ʼ�
		{
			if ((*Iter)->SrcUserID != 0)
			{
				continue;
			}
		}
		else
		{
			if ((*Iter)->SrcUserID == 0)
			{
				continue;
			}
		}

		DWORD MailID = (*Iter)->MailID;
		{
			if ((*Iter)->SrcUserID != 0)//�����ʼ���ʾItemID
			{
				auto itm = mapItem.find((*Iter)->RewardID);
				if (itm == mapItem.end())
				{
					mapItem.insert(make_pair((*Iter)->RewardID, (*Iter)->RewardSum));
				}
				else
				{
					itm->second += (*Iter)->RewardSum;
				}

				//tagItemOnce pOnce;
				//pOnce.ItemID = (*Iter)->RewardID;
				//pOnce.ItemSum = (*Iter)->RewardSum;
				//pOnce.LastMin = 0;
				//m_pUser->GetItemManager().OnAddUserItem(pOnce);
			}
			else//ϵͳ�ʼ���ʾRewardID
			{
				auto itm = mapReward.find((*Iter)->RewardID);
				if (itm == mapReward.end())
				{
					mapReward.insert(make_pair((*Iter)->RewardID, (*Iter)->RewardSum));
				}
				else
				{
					itm->second += (*Iter)->RewardSum;
				}

				//m_pUser->OnAddRoleRewardByRewardID((*Iter)->RewardID, TEXT("��ȡ�ʼ���Ʒ��¼"), (*Iter)->RewardSum);
			}
		}
	}

	for (auto& mem : mapItem)
	{

		tagItemOnce pOnce;
		pOnce.ItemID = mem.first;
		pOnce.ItemSum = mem.second;
		pOnce.LastMin = 0;
		m_pUser->GetItemManager().OnAddUserItem(pOnce);
	}

	for (auto& mem : mapReward)
	{
		m_pUser->OnAddRoleRewardByRewardID(mem.first, TEXT("��ȡ�ʼ���Ʒ��¼"), mem.second);
	}
	OnDelUserAllMail(type);
	LC_Cmd_GetAllMailItem msgClient;
	SetMsgInfo(msgClient, GetMsgType(Main_Mail, LC_GetAllMailItem), sizeof(LC_Cmd_GetMailItem));
	msgClient.Type = type;
	msgClient.Result = true;
	m_pUser->SendDataToClient(&msgClient);
	m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
	return true;
}

bool RoleMailManager::OnGetUserMailItem(DWORD MailID)//�ʼ���ȡ����
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//���ʼ����ȡ��Ʒ
	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	for (; Iter != m_RoleMailVec.end(); ++Iter)
	{
		if ((*Iter)->MailID == MailID /*&& (*Iter)->SrcUserID == 0*/)
		{
			if ((*Iter)->bIsExistsReward == false || (*Iter)->RewardID == 0 || (*Iter)->RewardSum == 0)
			{
				LC_Cmd_GetMailItem msgClient;
				SetMsgInfo(msgClient, GetMsgType(Main_Mail, LC_GetMailItem), sizeof(LC_Cmd_GetMailItem));
				msgClient.dwMailID = MailID;
				msgClient.Result = false;
				m_pUser->SendDataToClient(&msgClient);
				m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
				return true;
			}

			if ((*Iter)->SrcUserID != 0)//�����ʼ���ʾItemID
			{
				tagItemOnce pOnce;
				pOnce.ItemID = (*Iter)->RewardID;
				pOnce.ItemSum = (*Iter)->RewardSum;
				pOnce.LastMin = 0;
				m_pUser->GetItemManager().OnAddUserItem(pOnce);
			}
			else//�Ǻ����ʼ���ʾRewardID
			{
				m_pUser->OnAddRoleRewardByRewardID((*Iter)->RewardID, TEXT("��ȡ�ʼ���Ʒ��¼"), (*Iter)->RewardSum);
			}

			(*Iter)->RewardID = 0;
			(*Iter)->RewardSum = 0;
			(*Iter)->bIsExistsReward = false;//
			//3.�����ݿ��������
			DBR_Cmd_GetUserMailItem msg;
			SetMsgInfo(msg,DBR_GetUserMailItem, sizeof(DBR_Cmd_GetUserMailItem));
			msg.dwMailID = MailID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			//4.��������ͻ���ȥ
			LC_Cmd_GetMailItem msgClient;
			SetMsgInfo(msgClient,GetMsgType(Main_Mail, LC_GetMailItem), sizeof(LC_Cmd_GetMailItem));
			msgClient.dwMailID = MailID;
			msgClient.Result = true;
			m_pUser->SendDataToClient(&msgClient);

			OnDelUserMail(MailID);
			return true;
		}
	}
	LC_Cmd_GetMailItem msgClient;
	SetMsgInfo(msgClient,GetMsgType(Main_Mail, LC_GetMailItem), sizeof(LC_Cmd_GetMailItem));
	msgClient.dwMailID = MailID;
	msgClient.Result = false;
	m_pUser->SendDataToClient(&msgClient);
	m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
	return true;
}


bool RoleMailManager::OnDelUserAllMail(BYTE type)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//ɾ��һ���ʼ�
	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	bool IsNeedSendNewMail = false;
	for (; Iter != m_RoleMailVec.end(); )
	{
		//if ((*Iter)->MailID == MailID)
		if (type == 0)//ϵͳ�ʼ�
		{
			if ((*Iter)->SrcUserID != 0)
			{
				++Iter;
				continue;
			}
		}
		else
		{
			if ((*Iter)->SrcUserID == 0)
			{
				++Iter;
				continue;
			}
		}
		DWORD MailID = (*Iter)->MailID;
		{
			DBR_Cmd_DelUserMail msg;
			SetMsgInfo(msg, DBR_DelUserMail, sizeof(DBR_Cmd_DelUserMail));
			msg.dwMailID = MailID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			////2.��������ͻ���ȥ
			//LC_Cmd_DelMail msgClient;
			//SetMsgInfo(msgClient, GetMsgType(Main_Mail, LC_DelUserMail), sizeof(LC_Cmd_DelMail));
			//msgClient.dwMailID = MailID;
			//msgClient.Result = true;
			//m_pUser->SendDataToClient(&msgClient);
			delete *Iter;//��Ϊ�ʼ���New ������ ����Ҫ�����ڴ��
			Iter = m_RoleMailVec.erase(Iter);//Ϊ��֤˳�� ������н���ɾ��
		}
	}
	return true;
}

bool RoleMailManager::OnDelUserMail(DWORD MailID)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//ɾ��һ���ʼ�
	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	bool IsNeedSendNewMail = false;
	for (int i = 0; Iter != m_RoleMailVec.end(); ++Iter,++i)
	{
		if ((*Iter)->MailID == MailID)
		{
			//1.���͵����ݿ�
			DBR_Cmd_DelUserMail msg;
			SetMsgInfo(msg, DBR_DelUserMail, sizeof(DBR_Cmd_DelUserMail));
			msg.dwMailID = MailID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			//2.��������ͻ���ȥ
			LC_Cmd_DelMail msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Mail, LC_DelUserMail), sizeof(LC_Cmd_DelMail));
			msgClient.dwMailID = MailID;
			msgClient.Result = true;
			m_pUser->SendDataToClient(&msgClient);
			//3.�Ƴ�����
			delete *Iter;//��Ϊ�ʼ���New ������ ����Ҫ�����ڴ��
			//ListRemoveAt(m_RoleMailVec, i);//�Ƴ�ָ��λ�õ�����
			Iter = m_RoleMailVec.erase(Iter);//Ϊ��֤˳�� ������н���ɾ��
			//4.������Ҫ����һ���ʼ����ͻ���ȥ ��֤�ͻ��˱������µ�20���ʼ�
			IsNeedSendNewMail = true;

			--m_ClientMailSum;//ɾ���ͻ��˵�һ���ʼ�

			if (m_RoleMailVec.size() >= g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum && m_ClientMailSum<g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum)//�ʼ����� ������
			{
				Iter = m_RoleMailVec.begin();
				for (int i = 0; i < g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum - 1; ++i)
					++Iter;
				tagRoleMail* pMail = *Iter;
				OnSendAddMailToClient(pMail);//����һ���ʼ�
			}
			m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
			return true;
		}
	}
	LC_Cmd_DelMail msgClient;
	SetMsgInfo(msgClient,GetMsgType(Main_Mail, LC_DelUserMail), sizeof(LC_Cmd_DelMail));
	msgClient.dwMailID = MailID;
	msgClient.Result = false;
	m_pUser->SendDataToClient(&msgClient);
	return true;
}


bool RoleMailManager::OnAddUserMail(tagRoleMail* pMail, DWORD	dwDestUserID)
{
	//������������������� �����յ�һ���ʼ� ���� ϵͳ�ʼ���ʱ�� �����ȴ洢����������ȥ
	if (!pMail)
	{
		ASSERT(false);
		return false;
	}

	if (pMail->SrcUserID == 0)//ϵͳ�ʼ�
	{
		////ϵͳ�ʼ� ���� �ڱ��洦�� 
		////ϵͳ�ʼ���������������б���� 
		//tagRoleMail* pNewMail = new tagRoleMail;
		//if (!pNewMail)
		//{
		//	ASSERT(false);
		//	return false;
		//}
		//memcpy_s(pNewMail, sizeof(tagRoleMail), pMail, sizeof(tagRoleMail));
		//m_RoleMailVec.push_front(pNewMail);//�ŵ���ǰ��
		//OnSendAddMailToClient(pNewMail);
		//
		//m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
		//CRoleEx* pDestRole = g_FishServer.GetRoleManager()->QueryUser(dwDestUserID);
		//if (!pDestRole)
		//{
		//	//�����ʼ������������ȥ
		//	GC_Cmd_SendUserMail msg;
		//	SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMail), sizeof(CG_Cmd_SendUserMail));
		//	msg.DestUserID = dwDestUserID;
		//	msg.MailInfo = *pMail;
		//	m_pUser->SendDataToCenter(&msg);
		//}
		//else
		//{

		//��������ϵͳ�ʼ���Ȼ�����������
			DBR_Cmd_AddUserMail msg;
			SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
			msg.dwDestUserID = dwDestUserID;//m_pUser->GetUserID();
			msg.MailInfo = *pMail;
			g_FishServer.SendNetCmdToDB(&msg);
		//}	
	}
	else//�����ʼ�
	{
		CRoleEx* pDestRole = g_FishServer.GetRoleManager()->QueryUser(dwDestUserID);
		if (!pDestRole)
		{
			//�����ʼ������������ȥ
			GC_Cmd_SendUserMail msg;
			SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMail), sizeof(CG_Cmd_SendUserMail));
			msg.DestUserID = dwDestUserID;
			msg.MailInfo = *pMail;
			m_pUser->SendDataToCenter(&msg);
		}
		else
		{
			if (pMail->SrcUserID != 0 && pDestRole->GetMailManager().m_RoleMailVec.size() >= g_FishServer.GetFishConfig().GetFishMailConfig().MaxUserMailSum)
			{//���������ʼ� �������Ѿ�����
				//CRoleEx* pSrcRole = g_FishServer.GetRoleManager()->QueryUser(pMail->SrcUserID);
				//if (pSrcRole)//���ͻط������ԭ���
				//{
					LC_Cmd_SendUserMailResult msg;
					SetMsgInfo(msg, GetMsgType(Main_Mail, LC_SendUserMailResult), sizeof(LC_Cmd_SendUserMailResult));
					msg.DestUserID = dwDestUserID;// m_pUser->GetUserID();
					msg.Result = false;
					m_pUser->SendDataToClient(&msg);
					return false;
				//}
				//else
				//{
				//	//�����ʼ���������������ȥ
				//	GC_Cmd_SendUserMailResult msg;
				//	SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMailResult), sizeof(GC_Cmd_SendUserMailResult));
				//	msg.DestUserID = dwDestUserID;//m_pUser->GetUserID();
				//	msg.SrcUserID = pMail->SrcUserID;
				//	msg.Result = false;
				//	m_pUser->SendDataToCenter(&msg);
				//	return false;
				//}
			}
			DBR_Cmd_AddUserMail msg;
			SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
			msg.dwDestUserID = dwDestUserID;//m_pUser->GetUserID();//��ǰ���
			msg.MailInfo = *pMail;
			g_FishServer.SendNetCmdToDB(&msg);
		}
	}
	return true;
}

bool RoleMailManager::OnBeAddUserMail(tagRoleMail* pMail)
{
	//������������������� �����յ�һ���ʼ� ���� ϵͳ�ʼ���ʱ�� �����ȴ洢����������ȥ
	if (!pMail)
	{
		ASSERT(false);
		return false;
	}

    //if (pMail->SrcUserID == 0)//ϵͳ�ʼ�
	//{
		//ϵͳ�ʼ� ���� �ڱ��洦�� 
		//ϵͳ�ʼ���������������б���� 
		tagRoleMail* pNewMail = new tagRoleMail;
		if (!pNewMail)
		{
			ASSERT(false);
			return false;
		}
		memcpy_s(pNewMail, sizeof(tagRoleMail), pMail, sizeof(tagRoleMail));
		m_RoleMailVec.push_front(pNewMail);//�ŵ���ǰ��
		OnSendAddMailToClient(pNewMail);
		
		m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
	//}
	//else//�����ʼ�Ҳ����center server ����
	//{
	//	DBR_Cmd_AddUserMail msg;
	//	SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
	//	msg.dwDestUserID = m_pUser->GetUserID();//m_pUser->GetUserID();//��ǰ���
	//	msg.MailInfo = *pMail;
	//	g_FishServer.SendNetCmdToDB(&msg);
	//}
	return true;
}
bool RoleMailManager::OnBeAddUserMailResult(DBO_Cmd_AddUserMail* pMsg)
{
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	if (pMsg->Result)//���ͳɹ�
	{
		//�����ʼ��ɹ���
		tagRoleMail* pNewMail = new tagRoleMail;
		memcpy_s(pNewMail, sizeof(tagRoleMail), &pMsg->MailInfo, sizeof(tagRoleMail));
		m_RoleMailVec.push_front(pNewMail);//�ŵ���ǰ��
		OnSendAddMailToClient(pNewMail);
		m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
		//������ط���ȥ
		CRoleEx* pSrcRole = g_FishServer.GetRoleManager()->QueryUser(pNewMail->SrcUserID);
		if (pSrcRole)
		{	
			LC_Cmd_SendUserMailResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Mail, LC_SendUserMailResult), sizeof(LC_Cmd_SendUserMailResult));
			msg.DestUserID = m_pUser->GetUserID();
			msg.Result = true;
			pSrcRole->SendDataToClient(&msg);
		}
		else
		{
			GC_Cmd_SendUserMailResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMailResult), sizeof(GC_Cmd_SendUserMailResult));
			msg.DestUserID = m_pUser->GetUserID();
			msg.SrcUserID = pMsg->MailInfo.SrcUserID;
			msg.Result = true;
			m_pUser->SendDataToCenter(&msg);
		}
	}
	else//����ʧ��
	{
		if (pMsg->MailInfo.SrcUserID == 0)
			return false;
		CRoleEx* pSrcRole = g_FishServer.GetRoleManager()->QueryUser(pMsg->MailInfo.SrcUserID);
		if (pSrcRole)
		{
			LC_Cmd_SendUserMailResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Mail, LC_SendUserMailResult), sizeof(LC_Cmd_SendUserMailResult));
			msg.DestUserID = m_pUser->GetUserID();
			msg.Result = false;
			pSrcRole->SendDataToClient(&msg);
		}
		else
		{
			GC_Cmd_SendUserMailResult msg;
			SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMailResult), sizeof(GC_Cmd_SendUserMailResult));
			msg.DestUserID = m_pUser->GetUserID();
			msg.SrcUserID = pMsg->MailInfo.SrcUserID;
			msg.Result = false;
			m_pUser->SendDataToCenter(&msg);
		}
	}
	return true;
}
void RoleMailManager::OnSendAddMailToClient(tagRoleMail* pMail)
{
	if (!pMail || !m_pUser)
	{
		ASSERT(false);
		return;
	}
	//��������ͻ���ȥ
	if (pMail->SrcUserID == 0)
	{
		LC_Cmd_AddSystemMail msg;
		SetMsgInfo(msg,GetMsgType(Main_Mail, LC_AddSystemTitle), sizeof(LC_Cmd_AddSystemMail));
		msg.MailInfo.MailID = pMail->MailID;
		msg.MailInfo.bIsRead = pMail->bIsRead;
		__int64 diffDay = GetDiffDay(pMail->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
		if (diffDay > 250)
			diffDay = 250;
		msg.MailInfo.bDiffTime = (BYTE)diffDay;
		//if (pMail->RewardID != 0 && pMail->RewardSum != 0 && pMail->bIsExistsReward)
		//	msg.MailInfo.bIsExistsItem = true;
		//else
		//	msg.MailInfo.bIsExistsItem = false;

		TCHARCopy(msg.MailInfo.Context, CountArray(msg.MailInfo.Context), pMail->Context, _tcslen(pMail->Context));
		msg.MailInfo.RewardID = pMail->RewardID;
		msg.MailInfo.RewardSum = pMail->RewardSum;
		msg.MailInfo.bIsExistsReward = pMail->bIsExistsReward;


		m_pUser->SendDataToClient(&msg);
		++m_ClientMailSum;

		return;
	}
	else
	{
		if (pMail->SrcUserID == m_pUser->GetUserID())//�����Ը��Լ������ʼ�
			return;
		LC_Cmd_AddNormalMail msg;
		SetMsgInfo(msg,GetMsgType(Main_Mail, LC_AddNormalTitle), sizeof(LC_Cmd_AddNormalMail));
		msg.MailInfo.MailID = pMail->MailID;
		msg.MailInfo.bIsRead = pMail->bIsRead;
		msg.MailInfo.SrcFaceID = pMail->SrcFaceID;
		msg.MailInfo.SrcUserID = pMail->SrcUserID;
		TCHARCopy(msg.MailInfo.SrcNickName, CountArray(msg.MailInfo.SrcNickName), pMail->SrcNickName, _tcslen(pMail->SrcNickName));
		__int64 diffDay = GetDiffDay(pMail->SendTimeLog, g_FishServer.GetFishConfig().GetWriteSec());
		if (diffDay > 250)
			diffDay = 250;
		msg.MailInfo.bDiffTime = (BYTE)diffDay;
		msg.MailInfo.RewardID = pMail->RewardID;
		msg.MailInfo.RewardSum = pMail->RewardSum;
		TCHARCopy(msg.MailInfo.Context, CountArray(msg.MailInfo.Context), pMail->Context, _tcslen(pMail->Context));

		m_pUser->SendDataToClient(&msg);
		++m_ClientMailSum;
		return;
	}
}
//bool RoleMailManager::OnSendUserMail(CL_Cmd_SendUserMail* pMsg)
//{
//	//���ͷ���һ���ʼ� ��ҵ��ʼ�
//	if (!pMsg || !m_pUser)
//	{
//		ASSERT(false);
//		return false;
//	}
//	if (!g_FishServer.GetFishConfig().CheckStringIsError(pMsg->Context, MIN_MAIL_CONTEXT, MAX_MAIL_CONTEXT,SCT_Normal))//�ʼ����ݰ����Ƿ��ַ�
//	{
//		LC_Cmd_SendUserMailResult msg;
//		SetMsgInfo(msg, GetMsgType(Main_Mail, LC_SendUserMailResult), sizeof(LC_Cmd_SendUserMailResult));
//		msg.DestUserID =pMsg->DestUserID;
//		msg.Result = false;
//		m_pUser->SendDataToClient(&msg);
//		return false;
//	}
//	//���ͷ��ʼ����������
//	//�����ʼ�����
//	tagRoleMail pMail;
//	TCHARCopy(pMail.Context, CountArray(pMail.Context), pMsg->Context, _tcslen(pMsg->Context));
//	TCHARCopy(pMail.SrcNickName, CountArray(pMail.SrcNickName), m_pUser->GetRoleInfo().NickName, _tcslen(m_pUser->GetRoleInfo().NickName));
//	pMail.SrcUserID = m_pUser->GetUserID();
//	pMail.SrcFaceID = m_pUser->GetFaceID();
//	pMail.bIsRead = false;
//	pMail.RewardID = 0;
//	pMail.MailID = 0;
//	pMail.bIsExistsReward = false;//����ʼ���������Ʒ
//	pMail.SendTimeLog = time(NULL);
//
//	CRoleEx* pDestRole = g_FishServer.GetRoleManager()->QueryUser(pMsg->DestUserID);
//	if (pDestRole)//�����ͬһ��GameServer����
//	{
//		//������� ���ǽ��д���
//		pDestRole->GetMailManager().OnAddUserMail(&pMail);//���������ʼ�
//	}
//	else
//	{
//		//���͵����������ȥ
//		GC_Cmd_SendUserMail msg;
//		SetMsgInfo(msg, GetMsgType(Main_Mail, GC_SendUserMail), sizeof(GC_Cmd_SendUserMail));
//		msg.DestUserID = pMsg->DestUserID;
//		msg.MailInfo = pMail;
//		g_FishServer.SendNetCmdToCenter(&msg);
//	}
//	return true;
//}
void RoleMailManager::OnDayChange()
{
	//ɾ����������ʼ� �ͻ������д���
	time_t Now = time(NULL);
	BYTE NowClientSum = m_ClientMailSum;

	DBR_Cmd_DelUserMail msg;
	SetMsgInfo(msg, DBR_DelUserMail, sizeof(DBR_Cmd_DelUserMail));

	LC_Cmd_DelMail msgClient;
	SetMsgInfo(msgClient, GetMsgType(Main_Mail, LC_DelUserMail), sizeof(LC_Cmd_DelMail));
	msgClient.Result = true;

	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	for (size_t i = 0; Iter != m_RoleMailVec.end(); ++i)
	{
		int DiffDay = GetDiffDay((*Iter)->SendTimeLog,g_FishServer.GetFishConfig().GetWriteSec());//�����ʼ�������������� 
		if (DiffDay > g_FishServer.GetFishConfig().GetFishMailConfig().MailLimitDay && (*Iter)->SrcUserID != 0)
		{
			//��������ͻ���ȥɾ������
			msg.dwMailID = (*Iter)->MailID;
			g_FishServer.SendNetCmdToSaveDB(&msg);
			//2.��������ͻ���ȥ
			msgClient.dwMailID = (*Iter)->MailID;
			m_pUser->SendDataToClient(&msgClient);
			delete *Iter;
			Iter = m_RoleMailVec.erase(Iter);//�޷��仯˳�� ����һ����һ��ɾ�� 
			if (i < NowClientSum)
			{
				--m_ClientMailSum;
			}
		}
		else
		{
			++Iter;
		}
	}
	if (m_ClientMailSum<0)
		m_ClientMailSum = 0;//�ͻ����ʼ���
	if (m_ClientMailSum < g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum && m_RoleMailVec.size() > m_ClientMailSum)
	{
		//�������˻���ʣ����ʼ� ���ǽ����䲿���ʼ����ͻ���ȥ
		//ǰ�� m_ClientMailSum ���ʼ����뷢�� ֻ���ͺ���� ���Ҳ��䵽MAIL_CLIENTSUM �Ϳ�����
		int BeginIndex = m_ClientMailSum;
		int SendSum = g_FishServer.GetFishConfig().GetFishMailConfig().ClientShowMailSum - m_ClientMailSum;
		for (int i = 0; Iter != m_RoleMailVec.end(); ++i,++Iter)
		{
			if (i < BeginIndex)
				continue;
			if (i >= SendSum + BeginIndex)
				break;
			//�����ʼ�
			OnSendAddMailToClient(*Iter);
			++m_ClientMailSum;//�ͻ����ʼ��������
		}
	}
	m_pUser->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_Mail);
}
bool RoleMailManager::GetMailMessageStates()
{
	//�Ƿ���δ���ʼ� ������δ��ȥ��Ʒ���ʼ�
	std::list<tagRoleMail*>::iterator Iter = m_RoleMailVec.begin();
	for (; Iter != m_RoleMailVec.end(); ++Iter)
	{
		if (!(*Iter)->bIsRead)
			return true;
	}
	return false;
}