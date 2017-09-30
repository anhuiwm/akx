#include "stdafx.h"
#include "FishServer.h"
#include "..\CommonFile\FishServerConfig.h"

const static char DBErrorSqlFileName[] = "WmSqlErr";
const static char DBSqlFileName[] = "WmSql";
FishServer g_FishServer;
struct Thread3Info
{
	void* pPoint;
	BYTE   Index;
};
struct Thread4nfo
{
	void* pPoint;
	BYTE   Index;
};
inline UINT WINAPI ThreadFunc3(void *p)
{
	//�߳�3 ���ڴ�����������
	Thread3Info* pInfo = (Thread3Info*)p;
	((FishServer*)pInfo->pPoint)->HandleThreadMsg(pInfo->Index);
	delete pInfo;
	return 0;
}

FishServer::FishServer()
{

}
FishServer::~FishServer()
{

}
bool FishServer::InitServer(BYTE DBNetworkID)
{


	m_pDump.OnInit();
	if (!g_FishServerConfig.LoadServerConfigFile())
	{
		ShowInfoToWin("�����������ļ���ȡʧ��");
		return false;
	}
	if (!m_Config.LoadConfigFilePath())
	{
		ShowInfoToWin("��ȡ�����ļ�ʧ��");
		return false;
	}
	DBServerConfig* pDBConfig = g_FishServerConfig.GetDBServerConfig(DBNetworkID);
	if (!pDBConfig)
	{
		ASSERT(false);
		return false;
	}
	m_DBNetworkID = DBNetworkID;
	g_ServerID = DBNetworkID;
	SQLInitData pMySQl;
	strncpy_s(pMySQl.IP, CountArray(pMySQl.IP), pDBConfig->MySQlIP, CountArray(pDBConfig->MySQlIP));
	strncpy_s(pMySQl.DB, CountArray(pMySQl.DB), pDBConfig->MySQlDBName, CountArray(pDBConfig->MySQlDBName));
	strncpy_s(pMySQl.UID, CountArray(pMySQl.UID), pDBConfig->MySQlUserName, CountArray(pDBConfig->MySQlUserName));
	strncpy_s(pMySQl.PWD, CountArray(pMySQl.PWD), pDBConfig->MySQlUserPassword, CountArray(pDBConfig->MySQlUserPassword));

	m_bRun = true;

	for (BYTE i = 0; i < DB_ThreadSum; ++i)
	{
		if (!m_Sql[i].Init(pMySQl))
		{
			Log("���ݿ�����ʧ��:%s", pMySQl.IP);
			ShowInfoToWin("���ݿ�����ʧ��");
			return false;
		}
	}

	for (BYTE i = 0; i < DB_ThreadSum; ++i)
	{
		Thread3Info* pInfo = new Thread3Info;
		pInfo->Index = i;
		pInfo->pPoint = this;
		::_beginthreadex(0, 0, ThreadFunc3, pInfo, 0, 0);
	}

	//���ӵ�ControlServer
	if (!ConnectControl())
	{
		while (true)
		{
			Sleep(5);
			if (ConnectControl())
				break;
		}
	}
	if (!ConnectClient())
	{
		while (true)
		{
			Sleep(5);
			if (ConnectClient())
				break;
		}
	}
	return true;
}
bool FishServer::ConnectClient()
{
	DBServerConfig* pDBConfig = g_FishServerConfig.GetDBServerConfig(m_DBNetworkID);
	if (!pDBConfig)
	{
		ASSERT(false);
		return false;
	}
	ServerInitData siid;
	siid.ServerID = pDBConfig->NetworkID;
	siid.BuffSize = pDBConfig->BuffSize;
	siid.CmdHearbeat = DB_HEARBEAT;
	siid.Port = pDBConfig->DBListenPort;
	siid.SocketRecvSize = pDBConfig->RecvBuffSize;
	siid.SocketSendSize = pDBConfig->SendBuffSize;
	siid.Timeout = pDBConfig->TimeOut;
	siid.RecvThreadNum = pDBConfig->RecvThreadSum;
	siid.SendThreadNum = pDBConfig->SendThreadSum;
	siid.SleepTime = pDBConfig->SleepTime;
	siid.AcceptSleepTime = pDBConfig->AcceptSleepTime;
	siid.AcceptRecvData = true;

	siid.SceneHearbeatTick = pDBConfig->SceneHearbeatTick;
	siid.MaxSendCmdCount = pDBConfig->MaxSendCmdCount;
	siid.MaxAcceptNumPerFrame = pDBConfig->MaxAcceptNumPerFrame;
	siid.MaxRecvCmdCount = pDBConfig->MaxRecvCmdCount;
	siid.MaxSendCountPerFrame = pDBConfig->MaxSendCountPerFrame;
	siid.ListenCount = pDBConfig->ListenCount;

	m_Server.SetCmdHandler(this);
	if (!m_Server.Init(siid))
	{
		ShowInfoToWin("�������������ʧ��");
		return false;
	}

	return true;
}
bool FishServer::ConnectControl()
{
	//�Կ��Ʒ������Ķ˿� �����ⲿ���ӵ���������������е�½���� Я��DWORD ��ΨһID
	ControlServerConfig* pConfig = g_FishServerConfig.GetControlServerConfig();
	if (!pConfig)
	{
		ASSERT(false);
		return false;
	}
	ClientInitData pControlData;
	pControlData.BuffSize = pConfig->ControlServerBuffSize;
	pControlData.Port = pConfig->ControlServerListenPort;
	pControlData.SleepTime = pConfig->ControlServerSleepTime;
	pControlData.SocketRecvSize = pConfig->ControlServerRecvBuffSize;
	pControlData.SocketSendSize = pConfig->ControlServerSendBuffSize;
	pControlData.ThreadNum = 1;
	pControlData.Timeout = pConfig->ControlServerTimeOut;
	pControlData.CmdHearbeat = 0;
	pControlData.SendArraySize = pConfig->ControlServerMaxSendCmdCount;
	pControlData.RecvArraySize = pConfig->ControlServerMaxRecvCmdCount;

	if (!m_ControlTcp.Init(pConfig->ControlServerListenIP, pControlData, (void*)&m_DBNetworkID, sizeof(BYTE)))
	{
		ShowInfoToWin("ControlServer����ʧ��");
		return false;
	}
	OnTcpClientConnect(&m_ControlTcp);
	return true;
}
ServerClientData* FishServer::GetClientData(BYTE ClientID)
{
	HashMap<BYTE, ServerClientData*>::iterator Iter = m_ClintList.find(ClientID);
	if (Iter == m_ClintList.end())
		return NULL;
	else
		return Iter->second;
}
bool FishServer::MainUpdate() //��ѭ��
{
	while (m_bRun)
	{
		OnAddClient();

		DWORD dwTimer = timeGetTime();

		UpdateInfoToControl(dwTimer);
		//��������
		__int64 MsgTotalSum = 0;
		HashMap<BYTE, ServerClientData*>::iterator Iter = m_ClintList.begin();
		for (; Iter != m_ClintList.end();)
		{
			ServerClientData *pClient = Iter->second;
			if (!pClient)
			{
				++Iter;
				continue;
			}
			int Sum = 0;
			while (pClient->RecvList.HasItem() && (Sum < Msg_OnceSum || pClient->Removed))//����ͻ�������Ҫ�Ƴ��� �Ͳ�����������
			{
				NetCmd *pcmd = pClient->RecvList.GetItem();
				if (!pcmd)
				{
					ASSERT(false);
					continue;
				}
				//��������뵽���߳�����ȥ
				RcveMsg* pRecvInfo = new RcveMsg;
				pRecvInfo->pCmd = pcmd;
				pRecvInfo->ClientID = ConvertDWORDToBYTE(pClient->OutsideExtraData);
				m_RecvMsgList[MsgTotalSum% DB_ThreadSum].AddItem(pRecvInfo);
				++MsgTotalSum;
				++Sum;
			}
			if (pClient->Removed)//���뿪ǰ �ȴ����굱ǰ�ͻ���ȫ��������
			{
				ShowInfoToWin("һ��Client�뿪��DBServer");
				Iter = m_ClintList.erase(Iter);
				m_Server.Kick(pClient);
				continue;
			}
			++Iter;
		}
		//����������ͳ�ȥ
		for (BYTE i = 0; i < DB_ThreadSum; ++i)
		{
			while (m_SendMsgList[i].HasItem())
			{
				RcveMsg *pRecvInfo = m_SendMsgList[i].GetItem();
				if (!pRecvInfo)
					continue;
				ServerClientData* pClient = GetClientData(pRecvInfo->ClientID);
				if (!pClient)
				{
					delete pRecvInfo;
					continue;
				}
				if (!m_Server.Send(pClient, pRecvInfo->pCmd))
				{
					ASSERT(false);
				}
				free(pRecvInfo->pCmd);//New���������� ��Ҫfree��
				delete pRecvInfo;
			}
		}
		//������Ʒ�����������
		if (m_ControlTcp.IsConnected())
		{
			if (!m_ControlIsConnect)
				OnTcpClientConnect(&m_ControlTcp);
			NetCmd* pCmd = m_ControlTcp.GetCmd();
			int Sum = 0;
			while (pCmd && Sum < Msg_OnceSum)
			{
				HandleControlMsg(pCmd);
				free(pCmd);
				++Sum;
				pCmd = m_ControlTcp.GetCmd();
			}
			if (pCmd)
			{
				HandleControlMsg(pCmd);
				free(pCmd);
			}
		}
		else
		{
			if (m_ControlIsConnect)
				OnTcpClientLeave(&m_ControlTcp);

			//��������DB
			ConnectControl();
		}

		Sleep(1);
	}
	::InterlockedIncrement(&m_ExitCount);
	return OnDestroy();
}
void FishServer::HandleThreadMsg(BYTE ThreadIndex)//����߳̽��̴���
{
	if (ThreadIndex >= DB_ThreadSum)
		return;
	while (m_bRun)
	{
		RcveMsg *pRecvInfo = m_RecvMsgList[ThreadIndex].GetItem();
		while (pRecvInfo)
		{
			HandleDataBaseMsg(ThreadIndex, pRecvInfo->ClientID, pRecvInfo->pCmd);
			free(pRecvInfo->pCmd);
			delete pRecvInfo;
			pRecvInfo = m_RecvMsgList[ThreadIndex].GetItem();
		}
		Sleep(1);
	}
	::InterlockedIncrement(&m_ExitCount);
}
bool FishServer::OnDestroy()
{
	OnHandleAllMsg();//������ȫ��������
	m_ExitCount = 0;
	m_bRun = false;
	while (m_ExitCount != DB_ThreadSum)
		Sleep(1);
	while (m_AfxAddClient.HasItem())
	{
		AfxNetworkClientOnce* pOnce = m_AfxAddClient.GetItem();
		delete pOnce;
	}
	return true;
}
bool FishServer::CommitTable(SqlTable &table)
{
	vector<SqlUpdateStr*> strList;
	table.CreateSqlStr(strList, true);
	for (UINT i = 0; i < strList.size(); ++i)
		m_SqlList.AddItem(strList[i]);

	return true;
}
void FishServer::ShowInfoToWin(const char *pcStr, ...)
{
	if (!pcStr)
	{
		ASSERT(false);
		return;
	}
	va_list var;
	va_start(var, pcStr);
	UINT nCount = _vscprintf(pcStr, var);
	char *pBuffer = new char[nCount + 1];
	vsprintf_s(pBuffer, nCount + 1, pcStr, var);
	std::cout << pBuffer;
	std::cout << "\n";
	SAFE_DELETE_ARR(pBuffer);
	va_end(var);
}
void FishServer::Disconnect(BYTE ServerID, ServerClientData *pClient, RemoveType rt)
{
	if (!pClient)
	{
		ASSERT(false);
		return;
	}
	//��� �ͻ����뿪��״̬
	switch (rt)
	{
	case REMOVE_NONE:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��ΪNone",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_NORMAL:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ��ͨ",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_RECV_ERROR:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ���մ���",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_SEND_ERROR:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ���ʹ���",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_TIMEOUT:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ��ʱ",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_CMD_SEND_OVERFLOW:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ����̫��",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_CMD_RECV_OVERFLOW:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ����̫��",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_CMD_SIZE_ERROR:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ�����С",pClient->OutsideExtraData);
		}
		break;
	case REMOVE_RECVBACK_NOT_SPACE:
		{
			ShowInfoToWin("�ͻ��� ID:%d �뿪ԭ��Ϊ���ջ��������",pClient->OutsideExtraData);
		}
		break;
	}
}
void FishServer::OnAddClient()
{
	while (m_AfxAddClient.HasItem())
	{
		AfxNetworkClientOnce* pOnce = m_AfxAddClient.GetItem();
		if (!pOnce)
		{
			//ASSERT(false);
			continue;
		}
		if (!pOnce->pPoint || pOnce->Length != sizeof(BYTE))
		{
			m_Server.Kick(pOnce->pClient);
			//ASSERT(false);
			if (pOnce->pPoint)
				free(pOnce->pPoint);
			delete pOnce;
			continue;
		}
		BYTE ClientID = *((BYTE*)pOnce->pPoint);
		if (ClientID == 0)
		{
			m_Server.Kick(pOnce->pClient);
			//ASSERT(false);
			if (pOnce->pPoint)
				free(pOnce->pPoint);
			delete pOnce;
			continue;
		}
		pOnce->pClient->OutsideExtraData = ClientID;

		HashMap<BYTE, ServerClientData*>::iterator Iter = m_ClintList.find(ClientID);
		if (Iter != m_ClintList.end())
		{
			ASSERT(false);
			m_Server.Kick(pOnce->pClient);
			if (pOnce->pPoint)
				free(pOnce->pPoint);
			delete pOnce;
			continue;
		}
		m_ClintList.insert(HashMap<BYTE, ServerClientData*>::value_type(ClientID, pOnce->pClient));
		//ShowInfoToWin("һ��Client������DBServer");

		if (pOnce->pPoint)
			free(pOnce->pPoint);
		delete pOnce;
	}
}
void FishServer::OnAddDBResult(BYTE Index, BYTE ClientID, NetCmd*pCmd)
{
	if (Index >= DB_ThreadSum || !pCmd)
	{
		if (pCmd)
			free(pCmd);
		return;
	}
	RcveMsg* pRecvInfo = new RcveMsg;
	pRecvInfo->ClientID = ClientID;
	pRecvInfo->pCmd = pCmd;
	m_SendMsgList[Index].AddItem(pRecvInfo);
}
void FishServer::OnHandleAllMsg()
{
	//���ݿ����˳�֮ǰ �ȴ�����ȫ�������ݿ����� ���˳�
	for (BYTE i = 0; i < DB_ThreadSum; ++i)
	{
		RcveMsg *pRecvInfo = m_RecvMsgList[i].GetItem();
		while (pRecvInfo)
		{
			HandleDataBaseMsg(i, pRecvInfo->ClientID, pRecvInfo->pCmd);
			free(pRecvInfo->pCmd);
			delete pRecvInfo;
			pRecvInfo = m_RecvMsgList[i].GetItem();
		}
	}
	NetCmd* pCmd = m_ControlTcp.GetCmd();
	while (pCmd)
	{
		HandleControlMsg(pCmd);
		free(pCmd);
		pCmd = m_ControlTcp.GetCmd();
	}
}
void FishServer::OnTcpServerLeave(BYTE ServerID, ServerClientData* pClient)
{
	if (!pClient)
	{
		ASSERT(false);
		return;
	}
	if (ServerID == m_DBNetworkID)
	{
		ShowInfoToWin("һ��Client�뿪DB"); 
	}
	else
	{
		ASSERT(false);
	}
	return;
}
void FishServer::OnTcpServerJoin(BYTE ServerID, ServerClientData* pClient)
{
	if (!pClient)
	{
		ASSERT(false);
		return;
	}
	if (ServerID == m_DBNetworkID)
	{
		ShowInfoToWin("һ��Client����DB");
	}
	else
	{
		ASSERT(false);
	}
	return;
}
void FishServer::OnTcpClientConnect(TCPClient* pClient)
{
	if (!pClient)
	{
		ASSERT(false);
		return;
	}
	if (pClient == &m_ControlTcp)
	{
		m_ControlIsConnect = true;
		ShowInfoToWin("�Ѿ���ControlServer���ӳɹ���");
	}
	else
	{
		ASSERT(false);
	}
	return;
}
void FishServer::OnTcpClientLeave(TCPClient* pClient)
{
	if (!pClient)
	{
		ASSERT(false);
		return;
	}
	//�����Ͽ����ӵ�ʱ��
	if (pClient == &m_ControlTcp)
	{
		m_ControlIsConnect = false;
		m_ControlTcp.Shutdown();
		ShowInfoToWin("�Ѿ���ControlServer�Ͽ�������");
	}
	else
	{
		ASSERT(false);
	}
	return;
}
void FishServer::OnReloadConfig()
{
	FishConfig pNewConfig;
	if (!pNewConfig.LoadConfigFilePath())
	{
		ShowInfoToWin("���¼���FishConfig.xmlʧ��");
		return;
	}
	m_Config = pNewConfig;
	ShowInfoToWin("���¼���FishConfig.xml�ɹ�");
}
uint FishServer::CanConnected(BYTE SeverID, uint ip, short port, void *pData, uint recvSize, char* resData)
{
	return CONNECT_CHECK_OK;
}
bool FishServer::NewClient(BYTE SeverID, ServerClientData *pClient, void *pData, uint recvSize)
{
	if (!pClient)
	{
		ASSERT(false);
		return false;
	}
	if (!pData || recvSize != sizeof(BYTE))
	{
		m_Server.Kick(pClient);
		ASSERT(false);
		return false;
	}
	AfxNetworkClientOnce* pOnce = new AfxNetworkClientOnce();
	if (!pOnce)
	{
		m_Server.Kick(pClient);
		ASSERT(false);
		return false;
	}
	pOnce->SeverID = SeverID;
	pOnce->pClient = pClient;
	pOnce->pPoint = (void*)malloc(recvSize);
	if (!pOnce->pPoint)
	{
		m_Server.Kick(pClient);
		ASSERT(false);
		delete pOnce;
		return false;
	}
	memcpy_s(pOnce->pPoint, recvSize, pData, recvSize);//���ƽ���
	pOnce->Length = recvSize;
	m_AfxAddClient.AddItem(pOnce);
	return true;
}
void FishServer::SendNetCmdToControl(NetCmd*pCmd)
{
	if (!pCmd)
	{
		ASSERT(false);
		return;
	}
	if (!m_ControlTcp.Send(pCmd, false))
	{
		ASSERT(false);
	}
}
//Control������
bool FishServer::HandleControlMsg(NetCmd* pCmd)
{
	//������Ӫ������������������
	if (!pCmd)
		return false;
	if (pCmd->CmdType != Main_Control)
	{
		ASSERT(false);
		return false;
	}
	switch (pCmd->SubCmdType)
	{
	case CL_ReloadConfig:
		{
			OnReloadConfig();
			return true;
		}
		break;
	}
	return true;
}

//���ݿ������
bool FishServer::HandleDataBaseMsg(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�������������ݿ������ ���Ҳ���DBO����ط���ȥ
	if (!pCmd)//pClient ����Ϊ�� ������ط������ʱ�� ������DB�˳���ʱ��
	{
		ASSERT(false);
		return false;
	}
	switch (pCmd->GetCmdType())
	{
		//��½   OK
	case DBR_AccountLogon:
		return OnHandleAccountLogon(Index, ClientID, pCmd);
	case DBR_QueryLogon:
		return OnHandleQueryLogon(Index, ClientID, pCmd);
	case DBR_AccountRsg:
		return OnHandleAccountRsg(Index, ClientID, pCmd);
	case DBR_ResetAccount:
		return OnHandleAccountReset(Index, ClientID, pCmd);
	case DBR_ChangeAccountPassword:
		return OnHandleChangeAccountPassword(Index, ClientID, pCmd);
	case DBR_GetAccountInfoByUserID:
		return OnHandleGetRoleInfoByUserID(Index, ClientID, pCmd);
	case DBR_SetOnline:
		return OnHandleChangeAccountOnline(Index, ClientID, pCmd);
	case DBR_SetRoleClientInfo:
		return OnHandleRoleClientInfo(Index, ClientID, pCmd);//����������򻰵Ĵ���
	case DBR_GetNewAccount:
		return OnHandleGetNewAccount(Index, ClientID, pCmd);
	case DBR_SaveChannelInfo:
		return OnHandleSaveChannelInfo(Index, ClientID, pCmd);
	case DBR_SetAccountFreeze:
		return OnHandleSetAccountFreeze(Index, ClientID, pCmd);
	case DBR_PhoneLogon:
		return OnHandlePhoneLogon(Index, ClientID, pCmd);
	case DBR_PhoneSecPwd:
		return OnHandlePhoneSecPwdLogon(Index, ClientID, pCmd);
		//Item
	case DBR_LoadUserItem:
		return OnHandleGetUserItem(Index, ClientID, pCmd);//�ְ�
	case DBR_AddUserItem:
		return OnHandleAddUserItem(Index, ClientID, pCmd);
	case DBR_DelUserItem:
		return OnHandleDelUserItem(Index, ClientID, pCmd);//����������򻰵Ĵ���
	case DBR_ChangeUserItem:
		return OnHandleChangeUserItem(Index, ClientID, pCmd);//��������򻰵Ĵ���  ����޸�ǰ������ �����ж�
		//Relation
	case DBR_LoadUserRelation:
		return OnHandleGetUserRelation(Index, ClientID, pCmd);//�����ְ�
	case DBR_AddUserRelation:
		return OnHandleAddUserRelation(Index, ClientID, pCmd);
	case DBR_DelUserRelation:
		return OnHandleDelUserRelation(Index, ClientID, pCmd);
		//Mail
	case DBR_LoadUserMail:
		return OnHandleGetUserMail(Index, ClientID, pCmd);//�ְ�����
	case DBR_AddUserMail:
		return OnHandleAddUserMail(Index, ClientID, pCmd);
	case DBR_AddAllUserMail:
		return OnHandleAddAllUserMail(Index, ClientID, pCmd);
	case DBR_DelUserMail:
		return OnHandleDelUserMail(Index, ClientID, pCmd);
	case DBR_GetUserMailItem:
		return OnHandleGetUserMailItem(Index, ClientID, pCmd);
	case DBR_ChangeUserMailIsRead:
		return OnHandleChangeUserMailIsRead(Index, ClientID, pCmd);
	case DBR_LoadUserSendItem:
		return OnHandleLoadUserMailSendItem(Index, ClientID, pCmd);
		//Role
	case DBR_SaveRoleExInfo:
		return OnHandleSaveRoleInfo(Index, ClientID, pCmd);
	case DBR_SaveRoleNickName:
		return OnHandleSaveRoleNickName(Index, ClientID, pCmd);
	case DBR_SaveRoleLevel:
		return OnHandleSaveRoleLevel(Index, ClientID, pCmd);
	case DBR_SaveRoleGender:
		return OnHandleSaveRoleGender(Index, ClientID, pCmd);
	case DBR_SaveRoleGuideStep:
		return OnHandleSaveRoleGuideStep(Index, ClientID, pCmd);
	case DBR_SaveRoleAchievementPoint:
		return OnHandleSaveRoleAchievementPoint(Index, ClientID, pCmd);
	case DBR_SaveRoleCurTitle:
		return OnHandleSaveRoleCurTitle(Index, ClientID, pCmd);
	case DBR_SaveRoleCharmArray:
		return OnHandleSaveRoleCharmArray(Index, ClientID, pCmd);
	case DBR_SaveRoleCurrency:
		return OnHandleSaveRoleCurency(Index, ClientID, pCmd);
	case DBR_SaveRoleCashpoint:
		return OnHandleSaveRoleCashpoint(Index, ClientID, pCmd);
	case DBR_SaveRoleLastOnlineTime:
		return OnHandleSaveRoleLastOnlineTime(Index, ClientID, pCmd);
	case DBR_SaveRoleGoldBulletNum:
		return OnHandleSaveRoleGoldBulletNum(Index, ClientID, pCmd);

	case DBR_SaveRoleSendGoldBulletNum:
		return OnHandleSaveRoleSendGoldBulletNum(Index, ClientID, pCmd);
	case DBR_SaveRoleSendSilverBulletNum: 
		return OnHandleSaveRoleSendSilverBulletNum(Index, ClientID, pCmd);
	case DBR_SaveRoleSendBronzeBulletNum:
		return OnHandleSaveRoleSendBronzeBulletNum(Index, ClientID, pCmd);

	case DBR_SaveRoleUsingLauncher:
		return OnHandleSaveRoleUsingLauncher(Index, ClientID, pCmd);
	case DBR_SaveRoleFaceID:
		return OnHandleSaveRoleFaceID(Index, ClientID, pCmd);
	case DBR_SaveRoleMedal:
		return OnHandleSaveRoleMedal(Index, ClientID, pCmd);
	case DBR_SaveRoleSendGiffSum:
		return OnHandleSaveRoleSendGiffSum(Index, ClientID, pCmd);
	case DBR_SaveRoleAcceptGiffSum:
		return OnHandleSaveRoleAcceptGiffSum(Index, ClientID, pCmd);
	case DBR_SaveRoleTaskStates:
		return OnHandleSaveRoleTaskStates(Index, ClientID, pCmd);
	case DBR_SaveRoleAchievementStates:
		return OnHandleSaveRoleAchievementStates(Index, ClientID, pCmd);
	case DBR_SaveRoleActionStates:
		return OnHandleSaveRoleActionStates(Index, ClientID, pCmd);
	case DBR_SaveRoleOnlineStates:
		return OnHandleSaveRoleOnlineStates(Index, ClientID, pCmd);
	case DBR_SaveRoleCheckData:
		return OnHandleSaveRoleCheckData(Index, ClientID, pCmd);
	case DBR_SaveRoleIsShowIpAddress:
		return OnHandleSaveRoleIsShopIpAddress(Index, ClientID, pCmd);
	case DBR_SaveRoleAllInfo:
		return OnHandleSaveRoleAllInfo(Index, ClientID, pCmd);//������ҵ�ȫ������
	case DBR_SaveRoleGlobel:
		return OnHandleSaveRoleGlobel(Index, ClientID, pCmd);
	case DBR_SaveRoleOnlineMin:
		return OnHandleSaveRoleOnLineMin(Index, ClientID, pCmd);
	case DBR_SaveRoleExChangeStates:
		return OnHandleSaveRoleExChangeStates(Index, ClientID, pCmd);
	case DBR_SaveRoleTotalRecharge:
		return OnHandleSaveRoleTotalRecharge(Index, ClientID, pCmd);
	case DBR_SaveRoleNobilityPoint:
		return OnHandleSaveRoleNobilityPoint(Index, ClientID, pCmd);
	case DBR_SaveRoleAddupCheckNum:
		return OnHandleSaveRoleAddupCheckNum(Index, ClientID, pCmd);
	case DBR_SaveRoleDayTaskActiviness:
		return OnHandleSaveRoleDayTaskActiviness(Index, ClientID, pCmd);
	case DBR_SaveRoleWeekTaskActiviness:
		return OnHandleSaveRoleWeekTaskActiviness(Index, ClientID, pCmd);
	case DBR_SaveRoleWeekGlobeNum:
		return OnHandleSaveRoleWeekGlobeNum(Index, ClientID, pCmd);
	case DBR_AddRoleTotalRecharge:
		return OnHandleAddRoleTotalRecharge(Index, ClientID, pCmd);
	/*case DBR_SaveRoleLockInfo:
		return OnHandleSaveRoleLockInfo(Index, ClientID, pCmd);*/
	case DBR_SaveRoleIsFirstPayGlobel:
		return OnHandleSaveRoleIsFirstPayGlobel(Index, ClientID, pCmd);
	case DBR_SaveRoleIsFirstPayCurrcey:
		return OnHandleSaveRoleIsFirstPayCurrcey(Index, ClientID, pCmd);
	case DBR_ChangeRoleMoney:
		return OnHandleChangeRoleMoney(Index, ClientID, pCmd);
	case DBR_TableChange:
		return OnHandleTableChange(Index, ClientID, pCmd);
	case DBR_RoleEntityItem:
		return OnHandleRoleEntityItem(Index, ClientID, pCmd);
	case DBR_DelRoleEntityItem:
		return OnHandleDelRoleEntityItem(Index, ClientID, pCmd);
	case DBR_SaveRoleMonthCardInfo:
		return OnHandleSaveRoleMonthCardInfo(Index, ClientID, pCmd);
	case DBR_SaveRoleRateValue:
		return OnHandleSaveRoleRateValue(Index, ClientID, pCmd);
	case DBR_SaveRoleMaxRateValue:
		return OnHandleSaveRoleMaxRate(Index, ClientID, pCmd);
	case DBR_SaveRoleGetMonthCardRewardTime:
		return OnHandleSaveRoleGetMonthCardRewardTime(Index, ClientID, pCmd);
	case DBR_SaveRoleVipLevel:
		return OnHandleSaveRoleVipLevel(Index, ClientID, pCmd);
	case DBR_SaveRoleCashSum:
		return OnHandleSaveRoleCashSum(Index, ClientID, pCmd);
	case DBR_DelRoleCashSum:
		return OnHandleDelRoleCashSum(Index, ClientID, pCmd);
	case DBR_SaveRoleParticularStates:
		return OnHandleSaveRoleParticularStates(Index, ClientID, pCmd);
	case DBR_SaveRoleSecPassword:
		return OnHandleSaveRoleSecPassword(Index, ClientID, pCmd);
	case DBR_ChangeRoleSecPassword:
		return OnHandleChangeRoleSecPassword(Index, ClientID, pCmd);
	case DBR_ChangeRoleShareStates:
		return OnHandleChangeRoleShareStates(Index, ClientID, pCmd);
	case DBR_GameIDConvertToUserID:
		return OnHandleGameIDConvertUserID(Index, ClientID, pCmd);
		//Query
	case DBR_Query_RoleInfoByNickName:
		return OnHandleQueryRoleInfoByNickName(Index, ClientID, pCmd);//���Ʋ�ѯ����
	//case DBR_Query_RoleInfoByUserID:
	//	return OnHandleQueryRoleInfoByUserID(Index, ClientID, pCmd);
	case DBR_Query_RoleInfoByGameID:
		return OnHandleQueryRoleInfoByGameID(Index, ClientID, pCmd);
	case DBR_Query_RoleInfoByIP:
		return OnHandleQueryRoleInfoByIP(Index, ClientID, pCmd);
		//����
	case DBR_LoadRoleTask:
		return OnHandleLoadRoleTask(Index, ClientID, pCmd);
	case DBR_SaveRoleTask:
		return OnHandleSaveRoleTask(Index, ClientID, pCmd);
	case DBR_DelRoleTask:
		return OnHandleDelRoleTask(Index, ClientID, pCmd);
	//case DBR_ClearRoleTask:
	//	return OnHandleClearRoleTask(Index, ClientID, pCmd);
	case DBR_ClearRoleDayTask:
		return OnHandleClearRoleDayTask(Index, ClientID, pCmd);
	case DBR_ClearRoleWeekTask:
		return OnHandleClearRoleWeekTask(Index, ClientID, pCmd);
	case DBR_SaveAllTask:
		return OnHandleSaveAllTask(Index, ClientID, pCmd);
		//�ɾ�
	case DBR_LoadRoleAchievement:
		return OnHandleLoadRoleAchievement(Index, ClientID, pCmd);
	case DBR_SaveRoleAchievement:
		return OnHandleSaveRoleAchievement(Index, ClientID, pCmd);
	case DBR_DelRoleAchievement:
		return OnHandleDelRoleAchievement(Index, ClientID, pCmd);
	case DBR_SaveAllAchievement:
		return OnHandleSaveAllAchievement(Index, ClientID, pCmd);
		//�
	case DBR_LoadRoleAction:
		return OnHandleLoadRoleAction(Index, ClientID, pCmd);
	case DBR_SaveRoleAction:
		return OnHandleSaveRoleAction(Index, ClientID, pCmd);
	case DBR_DelRoleAction:
		return OnHandleDelRoleAction(Index, ClientID, pCmd);
	case DBR_SaveAllAction:
		return OnHandleSaveAllAction(Index, ClientID, pCmd);
		//�ƺ�
	case DBR_LoadRoleTitle:
		return OnHandleLoadRoleTitle(Index, ClientID, pCmd);
	case DBR_AddRoleTitle:
		return OnHandleAddRoleTitle(Index, ClientID, pCmd);
	case DBR_DelRoleTitle:
		return OnHandleDelRoleTitle(Index, ClientID, pCmd);
		//�̵�
	case DBR_AddRoleEntityItem:
		return OnHandleSaveRoleEntityItem(Index, ClientID, pCmd);
	case DBR_AddRoleShopNormalItem:
		return OnHandleSaveRoleShopNormalItem(Index, ClientID, pCmd);
		//ʵ��
	case DBR_LoadRoleEntity:
		return OnHandleLoadRoleEntity(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityName:
		return OnHandleSaveRoleEntityName(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityPhone:
		return OnHandleSaveRoleEntityPhone(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityEmail:
		return OnHandleSaveRoleEntityEmail(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityIdentityID:
		return OnHandleSaveRoleEntityIdentityID(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityItemUseName:
		return OnHandleSaveRoleEntityItemUseName(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityItemUsePhone:
		return OnHandleSaveRoleEntityItemUsePhone(Index, ClientID, pCmd);
	case DBR_SaveRoleEntityItemUseAddress:
		return OnHandleSaveRoleEntityItemUseAddress(Index, ClientID, pCmd);
	case DBR_LoadRoleExchangeEntity:
		return OnHandleLoadRoleExchangeEntity(Index, ClientID, pCmd);
	case DBR_LoadRoleExchangeItem:
		return OnHandleLoadRoleExchangeItem(Index, ClientID, pCmd);
		//����
	case DBR_LoadRoleGiff:
		return OnHandleLoadRoleGiff(Index, ClientID, pCmd);
	case DBR_AddRoleGiff:
		return OnHandleAddRoleGiff(Index, ClientID, pCmd);
	case DBR_DelRoleGiff:
		return OnHandleDelRoleGiff(Index, ClientID, pCmd);
	case DBR_GetNowDayGiff:
		return OnHandleLoadNowDayGiff(Index, ClientID, pCmd);
	case DBR_ClearNowDayGiff:
		return OnHandleClearNowDayGiff(Index, ClientID, pCmd);
		//����
	case DBO_SaveRoleCharm:
		return OnHandleChangeRoleCharm(Index, ClientID, pCmd);
		//���а�
	case DBR_LoadRankInfo:
		return OnHandleLoadRankInfo(Index, ClientID, pCmd);
	case DBR_LoadWeekRankInfo:
		return OnHandleLoadWeekRankInfo(Index, ClientID, pCmd);
	case DBR_SaveWeekRankInfo:
		return OnHandleSaveWeekRankInfo(Index, ClientID, pCmd);
	case DBR_ChangeWeekInfo:
		return OnHandleChangeWeekInfo(Index, ClientID, pCmd);
		//���GameData
	case DBR_LoadGameData:
		return OnHandleLoadRoleGameData(Index, ClientID, pCmd);
	case DBR_SaveGameData:
		return OnHandleSaveRoleGameData(Index, ClientID, pCmd);
	case DBR_ChangeGameDataByMonth:
		return OnHandleChangeRoleGameDataByMonth(Index, ClientID, pCmd);
		//
	case DBR_LoadAllUserAchievementPointList:
		return OnHandleAchievementPointList(Index, ClientID, pCmd);
		//
	case DBR_LoadAllAccountInfo:
		return OnHandleLoadAllAccountInfo(Index, ClientID, pCmd);
		//Log
	case DBR_LogRecharge:
		return OnHandleLogRechargeInfo(Index, ClientID, pCmd);
	case DBR_LogInfo:
		return OnHandleLogInfo(Index, ClientID, pCmd);
	case DBR_LogOnlineInfo:
		return OnHandleLogRoleOnlineInfo(Index, ClientID, pCmd);
	case DBR_LogPhonePay:
		return OnHandleLogPhonePayInfo(Index, ClientID, pCmd);
	case DBR_LogRoleTableInfo:
		return OnHandleLogRoleTableInfo(Index, ClientID, pCmd);
	case DBR_LogEntityItemInfo:
		return OnHandleLogRoleEntityItemInfo(Index, ClientID, pCmd);
	case DBR_CreateLogTable:
		return OnHandleCreateLogTable(Index, ClientID, pCmd);
	case DBR_LogNiuNiuTableInfo:
		return OnHandleLogNiuNiuTableInfo(Index, ClientID, pCmd);
	case DBR_LogExChangeInfo:
		return OnHandleLogExChangeInfo(Index, ClientID, pCmd);
	case DBR_LogLotteryInfo:
		return OnHandleLogLotteryInfo(Index, ClientID, pCmd);
	case DBR_LogMonthInfo:
		return OnHandleLogMonthInfo(Index, ClientID, pCmd);
	case DBR_LogDialInfo:
		return OnHandleLogDialInfo(Index, ClientID, pCmd);
	case DBR_LogCarInfo:
		return OnHandleLogCarInfo(Index, ClientID, pCmd);
	case DBR_LogStockScore:
		return OnHandleLogStockScore(Index, ClientID, pCmd);
		//Announcement
	case DBR_LoadAllAnnouncement:
		return OnHandleLoadAllAnnouncementInfo(Index, ClientID, pCmd);
	case DBR_AddAnnouncement:
		return OnHandleAddAnnouncementInfo(Index, ClientID, pCmd);
		//����
	case DBR_SaveRoleEmail:
		return OnHandleChangeRoleEmail(Index, ClientID, pCmd);
		//Center
	case DBR_ClearFishDB:
		return OnHandleClearFishDB(Index, ClientID, pCmd);
		//ExChange
	case DBR_QueryExChange:
		return OnHandleQueryExChange(Index, ClientID, pCmd);
	case DBR_DelExChange:
		return OnDelExChange(Index, ClientID, pCmd);
		//Recharege
	case DBR_AddRecharge:
		return OnAddRecharge(Index, ClientID, pCmd);
	case DBR_LoadRechageOrderID:
		return OnLoadRechargeOrderID(Index, ClientID, pCmd);
	case DBR_AddRechageOrderID:
		return OnAddRechargeOrderID(Index, ClientID, pCmd);
	case DBR_CheckPhone:
		return OnCheckPhone(Index, ClientID, pCmd);
	case DBR_CheckEntityID:
		return OnCheckEntityID(Index, ClientID, pCmd);
	case DBR_LoadPhonePayOrderID:
		return OnLoadPhonePayOrderList(Index, ClientID, pCmd);
	case DBR_AddPhonePayOrderID:
		return OnAddPhonePayOrderID(Index, ClientID, pCmd);
	case DBR_GetRechargeOrderID:
		return OnGetRechargeOrderID(Index, ClientID, pCmd);
	case DBR_QueryRechargeOrderInfo:
		return OnQueryRechargeOrderInfo(Index, ClientID, pCmd);
	case DBR_DelRechargeOrderInfo:
		return OnDleRechargeOrderInfo(Index, ClientID, pCmd);
	//Control
	case DBR_ResetUserPassword:
		return OnResetUserPassword(Index, ClientID, pCmd);
	case DBR_FreezeUser:
		return OnHandleFreezeUser(Index, ClientID, pCmd);
	//Robot
	case DBR_LoadRobotInfo:
		return OnHandelLoadRobotInfo(Index, ClientID, pCmd);
	//char
	case DBR_LoadCharInfo:
		return OnHandleLoadCharInfo(Index, ClientID, pCmd);
	case DBR_DelCharInfo:
		return OnHandleDelCharInfo(Index, ClientID, pCmd);
	case DBR_AddCharInfo:
		return OnHandleAddCharInfo(Index, ClientID, pCmd);
	//��ϵ����
	case DBR_LoadRelationRequest:
		return OnHandleLoadRelationRequest(Index, ClientID, pCmd);
	case DBR_AddRelationRequest:
		return OnHandleAddRelationRequest(Index, ClientID, pCmd);
	case DBR_DelRelationRequest:
		return OnHandleDelRelationRequest(Index, ClientID, pCmd);
	default:
		return false;
	}
	return true;
}
bool FishServer::OnHandleAccountLogon(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��������
	DBR_Cmd_AccountLogon* pMsg = (DBR_Cmd_AccountLogon*)pCmd;
	//���ݲ���������ִ�д洢���� ���� ���մ洢���̷��ص����� 
	//����洢����ʧ��  �ͷ����쳣 ��������ʧ��
	//��������� ������DBO���� ���ҷ��ظ������������ķ�����
	DBO_Cmd_AccountLogon* msg = (DBO_Cmd_AccountLogon*)CreateCmd(DBO_AccountLogon, sizeof(DBO_Cmd_AccountLogon));
	msg->ClientID = pMsg->ClientID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* AccountName = WCharToChar(pMsg->AccountName, Size);

	char* DestAccountName = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(AccountName, Size, DestAccountName);


	Size = 0;
	char* MacAddress = WCharToChar(pMsg->MacAddress, Size);

	char* DestMacAddress = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(MacAddress, Size, DestMacAddress);


	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAccountLogon('%s','%s','%u','%u','%u');", DestAccountName, DestMacAddress, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3);
	free(DestAccountName);
	free(AccountName);
	free(DestMacAddress);
	free(MacAddress);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->dwUserID = pTable1.GetUint(0, 0);
		msg->IsFreeze = pTable1.GetBit(0, 1);
		msg->FreezeEndTime = pTable1.GetDateTime(0, 2);

		time_t Now = time(null);
		if (msg->IsFreeze && Now > msg->FreezeEndTime)
		{
			msg->IsFreeze = false;
			msg->FreezeEndTime = 0;
		}
	}
	else
	{
		msg->dwUserID = 0;
		msg->IsFreeze = false;
		msg->FreezeEndTime = 0;
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	//��������
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleQueryLogon(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	ShowInfoToWin("���յ����ٵ�½");

	DBR_Cmd_QueryLogon* pMsg = (DBR_Cmd_QueryLogon*)pCmd;
	DBO_Cmd_QueryLogon* msg = (DBO_Cmd_QueryLogon*)CreateCmd(DBO_QueryLogon, sizeof(DBO_Cmd_QueryLogon));
	msg->ClientID = pMsg->ClientID;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* AccountName = WCharToChar(pMsg->AccountName, Size);

	char* DestAccountName = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(AccountName, Size, DestAccountName);

	Size = 0;
	char* MacAddress = WCharToChar(pMsg->MacAddress, Size);
	char* DestMacAddress = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(MacAddress, Size, DestMacAddress);

	//�û�����
	//����һ��������ȵ� ���� 8������  �ο�+ '����Ϊ6�� ����'
	string strNickName = "�û�";
	for (size_t i = 0; i < MAX_NICKNAME - 2; ++i)
	{
		BYTE ID = RandUInt() % 62;//0-61
		char ch = 0;
		if (ID >= 0 && ID < 10)
		{
			ch = ID + 48;
			//48-57
		}
		else if (ID >= 10 && ID < 36)
		{
			ch = ID + 55;
			//65-90
		}
		else
		{
			ch = ID + 61;
			//97-122
		}
		strNickName = strNickName + ch;
	}

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	int256 RateValue;
	int256Handle::Clear(RateValue);
	DWORD InitRateValue = m_Config.GetSystemConfig().InitRateValue;
	for (BYTE i = 0; i < 32; ++i)//��ҵ�Ĭ�ϱ��� ֻ������ǰ32��
	{
		if ((InitRateValue& (1 << i)) != 0)
		{
			int256Handle::SetBitStates(RateValue, i, true);
		}
		else if ((InitRateValue& (1 << i)) == 0)
			break;
	}

	char pRateStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&RateValue, sizeof(RateValue), pRateStates);


	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryLogon('%s','%s','%u','%u','%u','%s',%d,'%u','%s','%d.%d.%d.%d','%d-%d-%d %d:%d:%d');", DestAccountName, DestMacAddress, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3, strNickName.c_str(), 1, m_Config.GetSystemConfig().RsgInitGlobelSum, pRateStates, pMsg->ClientIP & 0xff, (pMsg->ClientIP >> 8) & 0xff, (pMsg->ClientIP >> 16) & 0xff, (pMsg->ClientIP >> 24) & 0xff, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
	free(DestAccountName);
	free(AccountName);
	free(DestMacAddress);
	free(MacAddress);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->IsNewRsg = pTable1.GetBit(0, 0) == 1 ? true : false;
		msg->dwUserID = pTable1.GetUint(0, 1);
		msg->IsFreeze = pTable1.GetBit(0, 2);
		msg->FreezeEndTime = pTable1.GetDateTime(0, 3);
		time_t Now = time(null);
		if (msg->IsFreeze && Now > msg->FreezeEndTime)
		{
			msg->IsFreeze = false;
			msg->FreezeEndTime = 0;
		}
		msg->LogonType = pMsg->LogonType;
	}
	else
	{
		msg->IsNewRsg = false;
		msg->dwUserID = 0;
		msg->IsFreeze = false;
		msg->FreezeEndTime = 0;
		msg->LogonType = pMsg->LogonType;
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	//��������
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleAccountRsg(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��������˺�ע��
	DBR_Cmd_AccountRsg* pMsg = (DBR_Cmd_AccountRsg*)pCmd;
	DBO_Cmd_AccountRsg* msg = (DBO_Cmd_AccountRsg*)CreateCmd(DBO_AccountRsg, sizeof(DBO_Cmd_AccountRsg));
	msg->ClientID = pMsg->ClientID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* AccountName = WCharToChar(pMsg->AccountName, Size);
	char* DestAccountName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestAccountName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestAccountName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(AccountName, Size, DestAccountName);

	Size = 0;
	char* MacAddress = WCharToChar(pMsg->MacAddress, Size);
	char* DestMacAddress = (char*)malloc((Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(MacAddress, Size, DestMacAddress);

	//����һ��������ȵ� ���� 8������  �ο�+ '����Ϊ6�� ����'
	string strNickName = "�û�";
	for (size_t i = 0; i < MAX_NICKNAME - 2; ++i)
	{
		BYTE ID = RandUInt() % 62;//0-61
		char ch = 0;
		if (ID >= 0 && ID < 10)
		{
			ch = ID + 48;
			//48-57
		}
		else if (ID >= 10 && ID < 36)
		{
			ch = ID + 55;
			//65-90
		}
		else
		{
			ch = ID + 61;
			//97-122
		}
		strNickName = strNickName + ch;
	}

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	int256 RateValue;
	int256Handle::Clear(RateValue);
	DWORD InitRateValue = m_Config.GetSystemConfig().InitRateValue;
	for (BYTE i = 0; i < 32; ++i)//��ҵ�Ĭ�ϱ��� ֻ������ǰ32��
	{
		if ((InitRateValue& (1 << i)) != 0)
		{
			int256Handle::SetBitStates(RateValue, i, true);
		}
		else if ((InitRateValue& (1 << i)) == 0)
			break;
	}

	char pRateStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&RateValue, sizeof(RateValue), pRateStates);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAccountRsg('%s','%s','%u','%u','%u','%s',%d,'%u','%s','%d.%d.%d.%d','%d-%d-%d %d:%d:%d');", DestAccountName, DestMacAddress, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3, strNickName.c_str(), 1, m_Config.GetSystemConfig().RsgInitGlobelSum, pRateStates, pMsg->ClientIP & 0xff, (pMsg->ClientIP >> 8) & 0xff, (pMsg->ClientIP >> 16) & 0xff, (pMsg->ClientIP >> 24) & 0xff, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
	free(DestAccountName);
	free(AccountName);
	free(DestMacAddress);
	free(MacAddress);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->dwUserID = pTable1.GetUint(0, 0);
	}
	else
	{
		msg->dwUserID = 0;
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleAccountReset(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//���ڿ��ٵ�½���˺� ���ǿ����ṩһ����ҽ������������û���������Ļ��� �����˺�ת��Ϊ��ͨ�˺�
	DBR_Cmd_ResetAccount* pMsg = (DBR_Cmd_ResetAccount*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* AccountName = WCharToChar(pMsg->AccountName, Size);
	char* DestAccountName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestAccountName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestAccountName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(AccountName, Size, DestAccountName);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeAccountName('%s','%u','%u','%u','%u');", DestAccountName, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3, pMsg->dwUserID);
	free(DestAccountName);
	free(AccountName);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0, 0) == 1);//����ֵ Ϊ 1
	DBO_Cmd_ResetAccount* msg = (DBO_Cmd_ResetAccount*)CreateCmd(DBO_ResetAccount, sizeof(DBO_Cmd_ResetAccount));
	msg->dwUserID = pMsg->dwUserID;
	msg->Result = Result;
	msg->PasswordCrc1 = pMsg->PasswordCrc1;
	msg->PasswordCrc2 = pMsg->PasswordCrc2;
	msg->PasswordCrc3 = pMsg->PasswordCrc3;
	TCHARCopy(msg->AccountName, CountArray(msg->AccountName), pMsg->AccountName, _tcslen(pMsg->AccountName));
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleChangeAccountPassword(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�޸ĵ�ǰ�˺ŵ�����
	DBR_Cmd_ChangeAccountPassword* pMsg = (DBR_Cmd_ChangeAccountPassword*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeAccountPassword('%u','%u','%u','%u','%u','%u','%u');", pMsg->dwUserID, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3, pMsg->OldPasswordCrc1, pMsg->OldPasswordCrc2, pMsg->OldPasswordCrc3);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0, 0) == 1);
	DBO_Cmd_ChangeAccountPassword* msg = (DBO_Cmd_ChangeAccountPassword*)CreateCmd(DBO_ChangeAccountPassword, sizeof(DBO_Cmd_ChangeAccountPassword));
	msg->dwUserID = pMsg->dwUserID;
	msg->Result = Result;
	if (msg->Result)
	{
		msg->PasswordCrc1 = pMsg->PasswordCrc1;
		msg->PasswordCrc2 = pMsg->PasswordCrc2;
		msg->PasswordCrc3 = pMsg->PasswordCrc3;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleChangeAccountOnline(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����˺����߻�������״̬
	DBR_Cmd_RoleOnline* pMsg = (DBR_Cmd_RoleOnline*)pCmd;
	//DBO_Cmd_RoleOnline* msg = (DBO_Cmd_RoleOnline*)CreateCmd(DBO_RoleOnline, sizeof(DBO_Cmd_RoleOnline));
	//SqlTable pTable1;
	//char SqlStr[MAXSQL_LENGTH] = { 0 };
	//sprintf_s(SqlStr, sizeof(SqlStr), "call FishSetUserOnline('%u',%d);",pMsg->dwUserID,pMsg->IsOnline?1:0);
	//msg->dwUserID = pMsg->dwUserID;
	//msg->Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	//msg->IsOnline = pMsg->IsOnline;
	//if (!msg->Result)
	//	ASSERT(false);
	//OnAddDBResult(Index, ClientID, msg);
	//return true;

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSetUserOnline('%u',%d,'%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->IsOnline ? 1 : 0, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	return true;
}
//bool FishServer::OnHandleRoleAchievementIndex(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	DBR_Cmd_GetRoleAchievementIndex* pMsg = (DBR_Cmd_GetRoleAchievementIndex*)pCmd;
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRoleAcievementIndex('%u');", pMsg->dwUserID);
//	//��ȡ�洢���̵�����
//	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
//	{
//		DBO_Cmd_GetRoleAchievementIndex* msg = (DBO_Cmd_GetRoleAchievementIndex*)CreateCmd(DBO_GetRoleAchievementIndex, sizeof(DBO_Cmd_GetRoleAchievementIndex));
//		msg->dwUserID = pMsg->dwUserID;
//		msg->AchievementIndex = pTable1.GetUint(0, 0);
//		OnAddDBResult(Index, ClientID, msg);
//		return true;
//	}
//	else
//	{
//		ASSERT(false);
//		return false;
//	}
//}
bool FishServer::OnHandleRoleClientInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����������ͳ��  
	DBR_Cmd_SetRoleClientInfo* pMsg = (DBR_Cmd_SetRoleClientInfo*)pCmd;
	//�޸�һ����Ʒ������
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveUserClientInfo('%u','%u','%u','%u');", pMsg->dwUserID, pMsg->PlateFormID, pMsg->ScreenPoint >> 16, (UINT16)pMsg->ScreenPoint);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	return true;
}
bool FishServer::OnHandleSaveChannelInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveChannelInfo* pMsg = (DBR_Cmd_SaveChannelInfo*)pCmd;
	//��������
	vector<StringArrayData*> pVec;
	GetStringArrayVecByData(pVec, &pMsg->pInfo);
	if (pVec.size() != pMsg->pInfo.HandleSum)
	{
		//FreeVec(pVec);
		ASSERT(false);
		return false;
	}

//sdk  String  ��  09CE2B99C22E6D06  �������׽ӷ������ϵ�ID�����׽��ṩ��      channlid
//app  String  ��  800018D72E2761D0  CP��Ϸ���׽ӷ������ϵ�ID�����׽��ṩ��    productcode
//uin  String  ��  123456  �������ϵ��û�ID��                                  channluid
//sess  String  ��  abcdefghijklmn  �û��ڵ�½���������������ظ��ͻ��˵�SessionId��  token
//
	UINT Size = 0;
	char* Info1 = WCharToChar(pVec[0]->Array, pVec[0]->Length / sizeof(TCHAR), Size);
	char* sdk_channlid = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info1, Size, sdk_channlid);

	char* Info2 = WCharToChar(pVec[1]->Array, pVec[1]->Length / sizeof(TCHAR), Size);
	char* productcode = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info2, Size, productcode);

	char* Info3 = WCharToChar(pVec[2]->Array, pVec[2]->Length / sizeof(TCHAR), Size);
	char* channluid = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info3, Size, channluid);

	//char* Info4 = WCharToChar(pVec[3]->Array, pVec[3]->Length / sizeof(TCHAR), Size);
	//char* token = (char*)malloc((Size * 2 + 1)*sizeof(char));
	//m_Sql[Index].GetMySqlEscapeString(Info4, Size, token);

	//char* Info5 = WCharToChar(pVec[4]->Array, pVec[4]->Length / sizeof(TCHAR), Size);
	//char* DestInfo5 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	//m_Sql[Index].GetMySqlEscapeString(Info5, Size, DestInfo5);

	//char* Info6 = WCharToChar(pVec[5]->Array, pVec[5]->Length / sizeof(TCHAR), Size);
	//char* DestInfo6 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	//m_Sql[Index].GetMySqlEscapeString(Info6, Size, DestInfo6);

	/*char* Info7 = WCharToChar(pVec[6], Size);
	char* DestInfo7 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info7, Size, DestInfo7);*/

	/*vector<TCHAR*>::iterator Iter = pVec.begin();
	for (; Iter != pVec.end(); ++Iter)
	{
	free(*Iter);
	}
	pVec.clear();*/

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	//CREATE PROCEDURE `FishSaveChannelUserInfo`
	//(IN `UserID` int unsigned, IN `uid` varchar(2000), IN `channelLabel` varchar(2000),
	//IN `channeluid` varchar(2000), IN `productCode` varchar(2000), IN `channelCode` varchar(2000), IN `UserName` varchar(2000))

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveChannelUserInfo('%u','%s','%s','%s','%s','%s','%s');",
		pMsg->dwUserID, channluid, sdk_channlid, channluid, productcode, sdk_channlid, channluid);

	free(Info1);
	free(sdk_channlid);

	free(Info2);
	free(productcode);

	free(Info3);
	free(channluid);

	//free(Info4);
	//free(token);

	//FreeVec(pVec);

	/*free(Info7);
	free(DestInfo7);*/

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	return true;
}
bool FishServer::OnHandleSetAccountFreeze(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SetAccountFreeze* pMsg = (DBR_Cmd_SetAccountFreeze*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	time_t pNow = time(null) + pMsg->FreezeMin*60;
	tm pNowTm;
	errno_t pError= localtime_s(&pNowTm, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSetAccountFreeze('%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pNowTm.tm_year + 1900, pNowTm.tm_mon + 1, pNowTm.tm_mday, pNowTm.tm_hour, pNowTm.tm_min, pNowTm.tm_sec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandlePhoneSecPwdLogon(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����ͼ���� �ֻ� ���������½
	DBR_Cmd_PhoneSecPwd* pMsg = (DBR_Cmd_PhoneSecPwd*)pCmd;

	DBO_Cmd_PhoneSecPwd* msg = (DBO_Cmd_PhoneSecPwd*)CreateCmd(DBO_PhoneSecPwd, sizeof(DBO_Cmd_PhoneSecPwd));
	msg->ClientID = pMsg->ClientID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };


	UINT Size = 0;
	char* MacAddress = WCharToChar(pMsg->MacAddress, Size);

	char* DestMacAddress = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(MacAddress, Size, DestMacAddress);


	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAccountPhoneLogon('%llu','%s','%u','%u','%u');", pMsg->PhoneNumber, DestMacAddress, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3);
	free(DestMacAddress);
	free(MacAddress);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->dwUserID = pTable1.GetUint(0, 0);

		const TCHAR* pStr = pTable1.GetStr(0, 1);
		TCHARCopy(msg->AccountName, CountArray(msg->AccountName), pStr, _tcslen(pStr));

		msg->IsFreeze = pTable1.GetBit(0, 2);
		msg->FreezeEndTime = pTable1.GetDateTime(0, 3);

		time_t Now = time(null);
		if (msg->IsFreeze && Now > msg->FreezeEndTime)
		{
			msg->IsFreeze = false;
			msg->FreezeEndTime = 0;
		}
	}
	else
	{
		msg->dwUserID = 0;
		msg->IsFreeze = false;
		msg->FreezeEndTime = 0;
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	//��������
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandlePhoneLogon(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����ͼ�����ֻ���½
	DBR_Cmd_PhoneLogon* pMsg = (DBR_Cmd_PhoneLogon*)pCmd;


	////1.�ж��ֻ��Ƿ���� 
	//string strAccountName = "";
	//for (size_t i = 0; i < (ACCOUNT_LENGTH / 2); ++i)
	//{
	//	BYTE ID = RandUInt() % 62;//0-61
	//	char ch = 0;
	//	if (ID >= 0 && ID < 10)
	//	{
	//		ch = ID + 48;
	//		//48-57
	//	}
	//	else if (ID >= 10 && ID < 36)
	//	{
	//		ch = ID + 55;
	//		//65-90
	//	}
	//	else
	//	{
	//		ch = ID + 61;
	//		//97-122
	//	}
	//	strAccountName = strAccountName + ch;
	//}
	////����
	//string strNickName = "�ο�";;
	//for (size_t i = 0; i < MAX_NICKNAME - 2; ++i)
	//{
	//	BYTE ID = RandUInt() % 62;//0-61
	//	char ch = 0;
	//	if (ID >= 0 && ID < 10)
	//	{
	//		ch = ID + 48;
	//		//48-57
	//	}
	//	else if (ID >= 10 && ID < 36)
	//	{
	//		ch = ID + 55;
	//		//65-90
	//	}
	//	else
	//	{
	//		ch = ID + 61;
	//		//97-122
	//	}
	//	strNickName = strNickName + ch;
	//}
	//����һ��Ĭ�ϵ����� 
	DWORD Password1 = pMsg->PasswordCrc1;
	DWORD Password2 = pMsg->PasswordCrc2;
	DWORD Password3 = pMsg->PasswordCrc3;

	UINT Size = 0;
	char* Mac = WCharToChar(pMsg->MacAddress, _tcslen(pMsg->MacAddress), Size);
	char* DestMacAddress = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Mac, Size, DestMacAddress);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	//int256 RateValue;
	//int256Handle::Clear(RateValue);
	//DWORD InitRateValue = m_Config.GetSystemConfig().InitRateValue;
	//for (BYTE i = 0; i < 32; ++i)//��ҵ�Ĭ�ϱ��� ֻ������ǰ32��
	//{
	//	if ((InitRateValue& (1 << i)) != 0)
	//	{
	//		int256Handle::SetBitStates(RateValue, i, true);
	//	}
	//	else if ((InitRateValue& (1 << i)) == 0)
	//		break;
	//}

	/*char pRateStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&RateValue, sizeof(RateValue), pRateStates);*/

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishPhoneLogon('%llu','%s',%d,'%u','%u','%u');",
		pMsg->PhoneNumber,  DestMacAddress, pMsg->IsFirOrSec?1:0,Password1, Password2, Password3);

	free(DestMacAddress);
	free(Mac);

	//������Ϻ�
	DBO_Cmd_PhoneLogon* msg = (DBO_Cmd_PhoneLogon*)CreateCmd(DBO_PhoneLogon, sizeof(DBO_Cmd_PhoneLogon));
	if (!msg)
	{
		ASSERT(false);
		return false;
	}

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->dwUserID = pTable1.GetUint(0, 0);
		const TCHAR* pStr = pTable1.GetStr(0, 1);
		TCHARCopy(msg->AccountName, CountArray(msg->AccountName), pStr, _tcslen(pStr));
		msg->PasswordCrc1 = pTable1.GetUint(0, 2);
		msg->PasswordCrc2 = pTable1.GetUint(0, 3);
		msg->PasswordCrc3 = pTable1.GetUint(0, 4);

		bool IsFreeze = pTable1.GetBit(0, 5);
		time_t FreezeEndTime = pTable1.GetDateTime(0, 6);
		if (IsFreeze && time(null) > FreezeEndTime)
		{
			IsFreeze = false;
			FreezeEndTime = 0;
		}
		msg->IsFreeze = IsFreeze;
		msg->FreezeEndTime = FreezeEndTime;
		msg->ClientID = pMsg->ClientID;
	}
	else
	{
		msg->dwUserID = 0;
		msg->ClientID = pMsg->ClientID;
		msg->IsFreeze = false;
		msg->FreezeEndTime = 0;
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleGetNewAccount(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_GetNewAccount* pMsg = (DBR_Cmd_GetNewAccount*)pCmd;
	//����һ���˺�����
	string strAccountName = "";
	for (size_t i = 0; i < (ACCOUNT_LENGTH / 2); ++i)
	{
		BYTE ID = RandUInt() % 62;//0-61
		char ch = 0;
		if (ID >= 0 && ID < 10)
		{
			ch = ID + 48;
			//48-57
		}
		else if (ID >= 10 && ID < 36)
		{
			ch = ID + 55;
			//65-90
		}
		else
		{
			ch = ID + 61;
			//97-122
		}
		strAccountName = strAccountName + ch;
	}
	//����
	string strNickName = "�ο�";;
	for (size_t i = 0; i < MAX_NICKNAME - 2; ++i)
	{
		BYTE ID = RandUInt() % 62;//0-61
		char ch = 0;
		if (ID >= 0 && ID < 10)
		{
			ch = ID + 48;
			//48-57
		}
		else if (ID >= 10 && ID < 36)
		{
			ch = ID + 55;
			//65-90
		}
		else
		{
			ch = ID + 61;
			//97-122
		}
		strNickName = strNickName + ch;
	}

	UINT Size = 0;
	char* MacAddress = WCharToChar(pMsg->MacAddress, Size);
	char* DestMacAddress = (char*)malloc((Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(MacAddress, Size, DestMacAddress);

	//ִ�д洢����
	DBO_Cmd_GetNewAccount* msg = (DBO_Cmd_GetNewAccount*)CreateCmd(DBO_GetNewAccount, sizeof(DBO_Cmd_GetNewAccount));
	if (!msg)
	{
		ASSERT(false);
		return false;
	}
	msg->ClientID = pMsg->ClientID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		free(msg);

		ASSERT(false);
		return false;
	}

	int256 RateValue;
	int256Handle::Clear(RateValue);
	DWORD InitRateValue = m_Config.GetSystemConfig().InitRateValue;
	for (BYTE i = 0; i < 32; ++i)//��ҵ�Ĭ�ϱ��� ֻ������ǰ32��
	{
		if ((InitRateValue& (1 << i)) != 0)
		{
			int256Handle::SetBitStates(RateValue, i, true);
		}
		else if ((InitRateValue& (1 << i)) == 0)
			break;
	}

	char pRateStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&RateValue, sizeof(RateValue), pRateStates);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishGetNewAccount('%s','%s','%u','%u','%u','%s',%d ,'%u','%d.%d.%d.%d','%d-%d-%d %d:%d:%d','%s');", strAccountName.c_str(), DestMacAddress, pMsg->PasswordCrc1, pMsg->PasswordCrc2, pMsg->PasswordCrc3, strNickName.c_str(), 1, m_Config.GetSystemConfig().RsgInitGlobelSum, pMsg->ClientIP & 0xff, (pMsg->ClientIP >> 8) & 0xff, (pMsg->ClientIP >> 16) & 0xff, (pMsg->ClientIP >> 24) & 0xff, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec, pRateStates);

	free(DestMacAddress);
	free(MacAddress);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->Result = true;
		const TCHAR* pStr = pTable1.GetStr(0, 0);
		TCHARCopy(msg->AccountName, CountArray(msg->AccountName), pStr, _tcslen(pStr));
		msg->PasswordCrc1 = pTable1.GetUint(0, 1);
		msg->PasswordCrc2 = pTable1.GetUint(0, 2);
		msg->PasswordCrc3 = pTable1.GetUint(0, 3);
		msg->dwUserID = pTable1.GetUint(0, 4);
	}
	else
	{
		msg->Result = false;
		ASSERT(false);
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleGetRoleInfoByUserID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ȡ��Ҿ�������� ������ҵ�ID
	DBR_Cmd_GetAccountInfoByUserID* pMsg = (DBR_Cmd_GetAccountInfoByUserID*)pCmd;
	DBO_Cmd_GetAccountInfoByUserID* msg = (DBO_Cmd_GetAccountInfoByUserID*)CreateCmd(DBO_GetAccountInfoByUserID, sizeof(DBO_Cmd_GetAccountInfoByUserID));
	msg->ClientID = pMsg->ClientID;
	msg->RoleInfo.dwUserID = pMsg->dwUserID;
	msg->Result = false;
	msg->LogonByGameServer = pMsg->LogonByGameServer;
	//ִ�д洢����

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		free(msg);
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadUserInfo('%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
	//��ȡ�洢���̵�����
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		//��ȡ���ݳɹ��� ���ǽ��������õ���������ȥ
		msg->RoleInfo.dwUserID = pTable1.GetUint(0, 0);
		const TCHAR* pNickName = pTable1.GetStr(0, 1);
		TCHARCopy(msg->RoleInfo.NickName, CountArray(msg->RoleInfo.NickName), pNickName, _tcslen(pNickName));
		msg->RoleInfo.dwFaceID = pTable1.GetUint(0, 2);
		msg->RoleInfo.wLevel = pTable1.GetUshort(0, 3);
		msg->RoleInfo.dwExp = pTable1.GetUint(0, 4);
		msg->RoleInfo.bGender = pTable1.GetBit(0, 5);
		msg->RoleInfo.dwGlobeNum = pTable1.GetInt64(0, 6);
		msg->RoleInfo.dwMedalNum = pTable1.GetUint(0, 7);
		msg->RoleInfo.dwCurrencyNum = pTable1.GetUint(0, 8);
		msg->RoleInfo.dwProduction = pTable1.GetUint(0, 9);
		msg->RoleInfo.dwGameTime = pTable1.GetUint(0, 10);
		msg->LastOnlineTime = pTable1.GetDateTime(0, 11);
		msg->RoleInfo.TitleID = pTable1.GetByte(0, 12);
		msg->RoleInfo.dwAchievementPoint = pTable1.GetUint(0, 13);
		msg->RoleInfo.SendGiffSum = pTable1.GetByte(0, 14);
		msg->RoleInfo.AcceptGiffSum = pTable1.GetByte(0, 15);
		//blob
		memcpy_s(&msg->RoleInfo.TaskStates, sizeof(msg->RoleInfo.TaskStates), pTable1.GetField((UINT)0, (UINT)16), sizeof(msg->RoleInfo.TaskStates));
		memcpy_s(&msg->RoleInfo.AchievementStates, sizeof(msg->RoleInfo.AchievementStates), pTable1.GetField((UINT)0, (UINT)17), sizeof(msg->RoleInfo.AchievementStates));
		memcpy_s(&msg->RoleInfo.ActionStates, sizeof(msg->RoleInfo.ActionStates), pTable1.GetField((UINT)0, (UINT)18), sizeof(msg->RoleInfo.ActionStates));
		msg->RoleInfo.OnlineSec = pTable1.GetUint(0, 19);//pTable1.GetUshort(0, 19)*60;
		msg->RoleInfo.OnlineRewardStates = pTable1.GetUint(0, 20);
		memcpy_s(&msg->RoleInfo.CharmArray, sizeof(msg->RoleInfo.CharmArray), pTable1.GetField((UINT)0, (UINT)22), sizeof(msg->RoleInfo.CharmArray));
		msg->RoleInfo.IsCanResetAccount = pTable1.GetBit(0, 23);
		msg->RoleInfo.ClientIP = pTable1.GetUint(0, 24);
		msg->RoleInfo.CheckData = pTable1.GetUint(0, 25);
		msg->RoleInfo.IsShowIPAddress = pTable1.GetByte(0, 26) == 1 ? true : false;//�Ƿ���ʾIP��ַ
		msg->IsRobot = pTable1.GetByte(0, 27) == 1 ? true : false;//�Ƿ�Ϊ������
		msg->RoleInfo.AchievementPointIndex = 0;
		msg->RoleInfo.ExChangeStates = pTable1.GetUint(0, 28);
		msg->RoleInfo.TotalRechargeSum = pTable1.GetUint(0, 29);
		//Server����
		msg->RoleServerInfo.TotalFishGlobelSum = pTable1.GetInt64(0, 30);
		msg->RoleServerInfo.RoleProtectSum = pTable1.GetUint(0, 31);
		msg->RoleServerInfo.RoleProtectLogTime = pTable1.GetDateTime(0, 32);
		msg->RoleInfo.dwCashpoint = pTable1.GetUint(0, 33);
		//msg->RoleInfo.bIsFirstPayCurrcey = pTable1.GetByte(0, 34) == 1 ? true : false;
		msg->RoleInfo.FirstPayCashpointStates = pTable1.GetByte(0, 34);
		bool IsFreeze = pTable1.GetBit(0, 35);
		time_t FreezeEndTime = pTable1.GetDateTime(0, 36);
		if (IsFreeze && time(null) > FreezeEndTime)
		{
			IsFreeze = false;
			FreezeEndTime = 0;
		}
		msg->IsFreeze = IsFreeze;
		msg->FreezeEndTime = FreezeEndTime;
		if (IsFreeze)
		{
			msg->Result = false;
			OnAddDBResult(Index, ClientID, msg);
			return true;
		}
		msg->RoleInfo.LotteryScore = pTable1.GetUint(0, 37);
		msg->RoleInfo.LotteryFishSum = pTable1.GetByte(0, 38);

		msg->RoleInfo.MonthCardID = pTable1.GetByte(0, 39);
		msg->RoleInfo.MonthCardEndTime = pTable1.GetDateTime(0, 40);
		msg->RoleInfo.GetMonthCardRewardTime = pTable1.GetDateTime(0, 41);
		memcpy_s(&msg->RoleInfo.RateValue, sizeof(msg->RoleInfo.RateValue), pTable1.GetField((UINT)0, (UINT)42), sizeof(msg->RoleInfo.RateValue));//�޸���ҵ���̨״̬
		msg->RoleInfo.VipLevel = pTable1.GetByte(0, 43);
		msg->RoleInfo.CashSum = pTable1.GetByte(0, 44);//����һ�����
		msg->RoleInfo.TotalUseMedal = pTable1.GetUint(0, 45);

		__int64 OnLineLogonTime = pTable1.GetDateTime(0, 46);
		//����¼��ʱ�� �������¼��ʱ��Ϊ 1971�� ���ǽ��д���
		DWORD InitTimeValue = 365 * 24 * 60 * 60 * 2;//2��
		if (msg->LastOnlineTime <= InitTimeValue)
		{
			//Ϊ 1971�� 
			msg->LastOnlineTime = max(msg->LastOnlineTime, OnLineLogonTime);
		}

		msg->RoleInfo.ParticularStates = pTable1.GetUint(0, 47);//����״ֵ̬
		msg->RoleInfo.GameID = pTable1.GetUint(0, 48);//�����ϷID


		msg->RoleServerInfo.SecPasswordCrc1 = pTable1.GetUint(0, 49);
		msg->RoleServerInfo.SecPasswordCrc2 = pTable1.GetUint(0, 50);
		msg->RoleServerInfo.SecPasswordCrc3 = pTable1.GetUint(0, 51);


		msg->RoleInfo.bShareStates = pTable1.GetBit(0, 52);
		msg->RoleInfo.TotalCashSum = pTable1.GetUint(0, 53);
		msg->RoleInfo.dwGoldBulletNum = pTable1.GetUint(0, 54);;//�ƽ�ͷ����
		msg->RoleInfo.byUsingLauncher = pTable1.GetByte(0, 55);;//��ǰʹ�õ���̨
		msg->RoleInfo.NobilityPoint = pTable1.GetUint(0, 56);
		msg->RoleInfo.AddupCheckNum = pTable1.GetByte(0, 57);
		//msg->RoleInfo.LockStates = pTable1.GetUint(0,47);
		//msg->RoleInfo.LockEndTime = pTable1.GetDateTime(0, 48);
		msg->RoleInfo.DayTaskActiviness = pTable1.GetByte(0, 58);
		msg->RoleInfo.WeekTaskActiviness = pTable1.GetByte(0, 59);
		msg->RoleInfo.WeekGlobeNum = pTable1.GetInt64(0, 60);
		msg->RoleInfo.IsCheckToday = pTable1.GetBit(0, 61);

		msg->RoleInfo.SendGoldBulletNum = pTable1.GetUint(0, 62);;//�ƽ�ͷ����
		msg->RoleInfo.SendSilverBulletNum = pTable1.GetUint(0, 63);;//�ƽ�ͷ����
		msg->RoleInfo.SendBronzeBulletNum = pTable1.GetUint(0, 64);;//�ƽ�ͷ����
		msg->RoleInfo.GuideStep = pTable1.GetUint(0, 65);;//�ƽ�ͷ����
		/*msg->TableID = pTable1.GetUint(0, 25);
		msg->MonthID = pTable1.GetUint(0, 26);*/
		msg->Result = true;
		bool IsOnceDayLoad = !(m_Config.GetFishUpdateConfig().IsChangeUpdate(msg->LastOnlineTime, time(null)));
		DWORD dwUserID = msg->RoleInfo.dwUserID;
		OnAddDBResult(Index, ClientID, msg);
		return OnHandleLoadAllUserInfo(IsOnceDayLoad, dwUserID, Index, ClientID);
		//return true;
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
		OnAddDBResult(Index, ClientID, msg);
		return true;
	}
}
bool FishServer::OnHandleLoadAllUserInfo(bool IsOnceDay, DWORD dwUserID, BYTE Index, BYTE ClientID)
{
	//���д���
	{
		DBR_Cmd_LoadUserRelation msg;
		SetMsgInfo(msg, DBR_LoadUserRelation, sizeof(DBR_Cmd_LoadUserRelation));
		msg.dwUserID = dwUserID;
		OnHandleGetUserRelation(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadUserItem msg;
		SetMsgInfo(msg, DBR_LoadUserItem, sizeof(DBR_Cmd_LoadUserItem));
		msg.dwUserID = dwUserID;
		OnHandleGetUserItem(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadUserMail msg;
		SetMsgInfo(msg, DBR_LoadUserMail, sizeof(DBR_Cmd_LoadUserMail));
		msg.dwUserID = dwUserID;
		OnHandleGetUserMail(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleTask msg;
		SetMsgInfo(msg, DBR_LoadRoleTask, sizeof(DBR_Cmd_LoadRoleTask));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleTask(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleAchievement msg;
		SetMsgInfo(msg, DBR_LoadRoleAchievement, sizeof(DBR_Cmd_LoadRoleAchievement));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleAchievement(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleTitle msg;
		SetMsgInfo(msg, DBR_LoadRoleTitle, sizeof(DBR_Cmd_LoadRoleTitle));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleTitle(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleEntity msg;
		SetMsgInfo(msg, DBR_LoadRoleEntity, sizeof(DBR_Cmd_LoadRoleEntity));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleEntity(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleAction msg;
		SetMsgInfo(msg, DBR_LoadRoleAction, sizeof(DBR_Cmd_LoadRoleAction));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleAction(Index, ClientID, &msg);
	}
	if (IsOnceDay)
	{
		//ͬһ���½ ��ȡ����
		DBR_Cmd_GetNowDayGiff msg;
		SetMsgInfo(msg, DBR_GetNowDayGiff, sizeof(DBR_Cmd_GetNowDayGiff));
		msg.dwUserID = dwUserID;
		OnHandleLoadNowDayGiff(Index, ClientID, &msg);
	}
	else
	{
		DBR_Cmd_ClearNowDayGiff msg;
		SetMsgInfo(msg, DBR_ClearNowDayGiff, sizeof(DBR_Cmd_ClearNowDayGiff));
		msg.dwUserID = dwUserID;
		OnHandleClearNowDayGiff(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRoleGiff msg;
		SetMsgInfo(msg, DBR_LoadRoleGiff, sizeof(DBR_Cmd_LoadRoleGiff));
		msg.LimitDay = m_Config.GetGiffConfig().LimitDay;
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleGiff(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadGameData msg;
		SetMsgInfo(msg, DBR_LoadGameData, sizeof(DBR_Cmd_LoadGameData));
		msg.dwUserID = dwUserID;
		OnHandleLoadRoleGameData(Index, ClientID, &msg);
	}
	{//�ܰ�Ŀǰȡ��
		//DBR_Cmd_LoadWeekRankInfo msg;
		//SetMsgInfo(msg, DBR_LoadWeekRankInfo, sizeof(DBR_Cmd_LoadWeekRankInfo));
		//msg.dwUserID = dwUserID;
		//msg.IsInit = true;
		//OnHandleLoadWeekRankInfo(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadCharInfo msg;
		SetMsgInfo(msg, DBR_LoadCharInfo, sizeof(DBR_Cmd_LoadCharInfo));
		msg.dwUserID = dwUserID;
		OnHandleLoadCharInfo(Index, ClientID, &msg);
	}
	{
		DBR_Cmd_LoadRelationRequest msg;
		SetMsgInfo(msg, DBR_LoadRelationRequest, sizeof(DBR_Cmd_LoadRelationRequest));
		msg.dwUserID = dwUserID;
		OnHandleLoadRelationRequest(Index, ClientID, &msg);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ҵ����ݱ�������
	DBR_Cmd_SaveRoleExInfo* pMsg = (DBR_Cmd_SaveRoleExInfo*)pCmd;
	//���ô洢���̵Ĳ���
	SqlTable pTable1;

	char SqlStr[MAXSQL_LENGTH] = { 0 };

	char pTaskStates[MAXBLOB_LENGTH] = { 0 };
	char pAchievementStates[MAXBLOB_LENGTH] = { 0 };
	char pActionStates[MAXBLOB_LENGTH] = { 0 };
	char pCharmArray[MAXBLOB_LENGTH] = { 0 };

	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->RoleInfo.TaskStates, sizeof(pMsg->RoleInfo.TaskStates), pTaskStates);
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->RoleInfo.AchievementStates, sizeof(pMsg->RoleInfo.AchievementStates), pAchievementStates);
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->RoleInfo.ActionStates, sizeof(pMsg->RoleInfo.ActionStates), pActionStates);
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->RoleInfo.CharmArray, sizeof(pMsg->RoleInfo.CharmArray), pCharmArray);

	UINT Size = 0;
	char* NickName = WCharToChar(pMsg->RoleInfo.NickName, Size);
	char* DestNickName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestNickName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestNickName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(NickName, Size, DestNickName);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveUserInfo('%u','%s','%u','%u','%u',%u,'%lld','%lld','%u','%u','%u','%u','%u','%u','%u','%u','%s','%s','%s','%u','%u','%d','%s','%u','%u','%u');",
		pMsg->RoleInfo.dwUserID, DestNickName, pMsg->RoleInfo.dwFaceID, pMsg->RoleInfo.wLevel, pMsg->RoleInfo.dwExp,
		pMsg->RoleInfo.bGender, pMsg->RoleInfo.dwGlobeNum, pMsg->RoleInfo.WeekGlobeNum,pMsg->RoleInfo.dwMedalNum, pMsg->RoleInfo.dwCurrencyNum, pMsg->RoleInfo.dwProduction,
		pMsg->RoleInfo.dwGameTime, pMsg->RoleInfo.TitleID, pMsg->RoleInfo.dwAchievementPoint, pMsg->RoleInfo.SendGiffSum, pMsg->RoleInfo.AcceptGiffSum,
		pTaskStates, pAchievementStates, pActionStates, pMsg->RoleInfo.OnlineSec, pMsg->RoleInfo.OnlineRewardStates,
		pMsg->RoleCharmValue, pCharmArray, pMsg->RoleInfo.ClientIP, pMsg->RoleInfo.CheckData, pMsg->RoleInfo.IsShowIPAddress ? 1 : 0);
	free(DestNickName);
	free(NickName);


	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleNickName(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleNickName
	DBR_Cmd_SaveRoleNickName* pMsg = (DBR_Cmd_SaveRoleNickName*)pCmd;
	DBO_Cmd_SaveRoleNickName* msg = (DBO_Cmd_SaveRoleNickName*)CreateCmd(DBO_SaveRoleNickName, sizeof(DBO_Cmd_SaveRoleNickName));
	msg->Result = false;
	msg->dwUserID = pMsg->dwUserID;
	TCHARCopy(msg->NickName, CountArray(msg->NickName), pMsg->NickName, _tcslen(pMsg->NickName));

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* NickName = WCharToChar(pMsg->NickName, Size);
	char* DestNickName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestNickName)
	{
		free(msg);
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestNickName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(NickName, Size, DestNickName);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleNickName('%u','%s');", pMsg->dwUserID, DestNickName);
	free(DestNickName);
	free(NickName);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0, 0) == 1);
	msg->Result = Result;

	OnAddDBResult(Index, ClientID, msg);//��������
	return true;
}
bool FishServer::OnHandleSaveRoleLevel(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleLevel
	DBR_Cmd_SaveRoleLevel* pMsg = (DBR_Cmd_SaveRoleLevel*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleLevel('%u','%u');", pMsg->dwUserID, pMsg->wLevel);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleGender(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleGender
	DBR_Cmd_SaveRoleGender* pMsg = (DBR_Cmd_SaveRoleGender*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleGender('%u',%d);", pMsg->dwUserID, pMsg->bGender ? 1 : 0);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleGuideStep(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleGender
	DBR_Cmd_SaveRoleGuideStep* pMsg = (DBR_Cmd_SaveRoleGuideStep*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.GuideStep = '%u' where accountinfo.UserID = '%u'", pMsg->byStep, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleAchievementPoint(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleAchievementPoint
	DBR_Cmd_SaveRoleAchievementPoint* pMsg = (DBR_Cmd_SaveRoleAchievementPoint*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleAchievementPoint('%u','%u');", pMsg->dwUserID, pMsg->dwAchievementPoint);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleCurTitle(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleTitle
	DBR_Cmd_SaveRoleCurTitle* pMsg = (DBR_Cmd_SaveRoleCurTitle*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleTitle('%u','%u');", pMsg->dwUserID, pMsg->TitleID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleCharmArray(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleCharmArray
	DBR_Cmd_SaveRoleCharmArray* pMsg = (DBR_Cmd_SaveRoleCharmArray*)pCmd;
	int CharmValue = m_Config.GetCharmValue(pMsg->CharmArray);
	char pCharmArray[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->CharmArray, sizeof(pMsg->CharmArray), pCharmArray);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCharmArray('%u','%d','%s');", pMsg->dwUserID, CharmValue, pCharmArray);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleCurency(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleCurency
	DBR_Cmd_SaveRoleCurrency* pMsg = (DBR_Cmd_SaveRoleCurrency*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCurency('%u','%u');", pMsg->dwUserID, pMsg->dwCurrencyNum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleCashpoint(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleCurency
	DBR_Cmd_SaveRoleCashpoint* pMsg = (DBR_Cmd_SaveRoleCashpoint*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCashpoint('%u','%u');", pMsg->dwUserID, pMsg->dwCashpointNum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleSaveRoleLastOnlineTime(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleLastOnlineTime* pMsg = (DBR_Cmd_SaveRoleLastOnlineTime*)pCmd;
	
	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.LastLogonTime = '%d-%d-%d %d:%d:%d' where accountinfo.UserID = '%u'", NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec,pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleGoldBulletNum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleGoldBulletNum* pMsg = (DBR_Cmd_SaveRoleGoldBulletNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.GoldBulletNum = '%u' where accountinfo.UserID = '%u'", pMsg->dwGoldBulletNum, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleSendGoldBulletNum(BYTE Index, BYTE ClientID, NetCmd* pCmd) 
{
	DBR_Cmd_SaveRoleSendGoldBulletNum* pMsg = (DBR_Cmd_SaveRoleSendGoldBulletNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.SendGoldBulletNum = '%u' where accountinfo.UserID = '%u'", pMsg->BulletNum, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleSendSilverBulletNum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleSendSilverBulletNum* pMsg = (DBR_Cmd_SaveRoleSendSilverBulletNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.SendSilverBulletNum = '%u' where accountinfo.UserID = '%u'", pMsg->BulletNum, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleSendBronzeBulletNum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleSendBronzeBulletNum* pMsg = (DBR_Cmd_SaveRoleSendBronzeBulletNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.SendBronzeBulletNum = '%u' where accountinfo.UserID = '%u'", pMsg->BulletNum, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleSaveRoleUsingLauncher(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleUsingLauncher* pMsg = (DBR_Cmd_SaveRoleUsingLauncher*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set accountinfo.UsingLauncher = '%u' where accountinfo.UserID = '%u'", pMsg->byUsingLauncher, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleSaveRoleFaceID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishSaveRoleFaceID
	DBR_Cmd_SaveRoleFaceID* pMsg = (DBR_Cmd_SaveRoleFaceID*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleFaceID('%u','%u');", pMsg->dwUserID, pMsg->dwFaceID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleGlobel(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleGlobel* pMsg = (DBR_Cmd_SaveRoleGlobel*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleGlobel('%u','%lld','%lld');", pMsg->dwUserID, pMsg->dwGlobel, pMsg->WeekGlobeNum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleOnLineMin(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleOnlineSec * pMsg = (DBR_Cmd_SaveRoleOnlineSec*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleOnLineMin('%u','%u');", pMsg->dwUserID, pMsg->OnLineSec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleExChangeStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleExChangeStates * pMsg = (DBR_Cmd_SaveRoleExChangeStates*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleExChangeStates('%u','%u');", pMsg->dwUserID, pMsg->States);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleTotalRecharge(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleTotalRecharge * pMsg = (DBR_Cmd_SaveRoleTotalRecharge*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleTotalRecharge('%u','%u');", pMsg->dwUserID, pMsg->Sum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleNobilityPoint(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleNobilityPoint * pMsg = (DBR_Cmd_SaveRoleNobilityPoint*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleNobilityPoint('%u','%u');", pMsg->dwUserID, pMsg->Sum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleAddupCheckNum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleAddupCheckNum * pMsg = (DBR_Cmd_SaveRoleAddupCheckNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update accountinfo set AddupCheckNum='%u', CheckData='%u',IsCheckToday = %d ,LastLogonTime = NOW() where UserID = '%u'",pMsg->AddupCheckNum,pMsg->CheckData,pMsg->IsCheckToday, pMsg->dwUserID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleSaveRoleDayTaskActiviness(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleDayTaskActiviness * pMsg = (DBR_Cmd_SaveRoleDayTaskActiviness*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleDayTaskActiviness('%u','%u');", pMsg->dwUserID, pMsg->Sum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleWeekTaskActiviness(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleWeekTaskActiviness * pMsg = (DBR_Cmd_SaveRoleWeekTaskActiviness*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleWeekTaskActiviness('%u','%u');", pMsg->dwUserID, pMsg->Sum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleWeekGlobeNum(BYTE Index, BYTE ClientID, NetCmd *pCmd)
{
	DBR_Cmd_SaveRoleWeekGlobeNum * pMsg = (DBR_Cmd_SaveRoleWeekGlobeNum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleWeekClobeNum('%u','%d');", pMsg->dwUserID, pMsg->Num);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddRoleTotalRecharge(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddRoleTotalRecharge * pMsg = (DBR_Cmd_AddRoleTotalRecharge*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleTotalRecharge('%u','%u');", pMsg->dwUserID, pMsg->Sum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
//bool FishServer::OnHandleSaveRoleLockInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	DBR_Cmd_SaveRoleLockInfo* pMsg = (DBR_Cmd_SaveRoleLockInfo*)pCmd;
//
//	tm NowTime;
//	errno_t Err = localtime_s(&NowTime, &pMsg->LockEndTime);
//	if (Err != 0)
//	{
//		ASSERT(false);
//		return false;
//	}
//
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleLockInfo('%u','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->LockStates,
//		NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
//	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
//	if (!Result)
//	{
//		LogInfoToFile(DBErrorSqlFileName, SqlStr);
//		ASSERT(false);
//	}
//	return true;
//}

bool FishServer::OnHandleSaveRoleIsFirstPayGlobel(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�޸�����Ƿ��׳�����
	DBR_Cmd_SaveRoleIsFirstPayGlobel * pMsg = (DBR_Cmd_SaveRoleIsFirstPayGlobel*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleIsFirstPayGlobel('%u');", pMsg->dwUserID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;

}
bool FishServer::OnHandleSaveRoleIsFirstPayCurrcey(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�޸�����Ƿ��׳���ʯ��
	DBR_Cmd_SaveRoleIsFirstPayCurrcey * pMsg = (DBR_Cmd_SaveRoleIsFirstPayCurrcey*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleIsFirstPayCurrcey('%u','%u');",pMsg->FirstPayCurrceyStates, pMsg->dwUserID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeRoleMoney(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҵĽ���޸�
	DBR_Cmd_ChangeRoleMoney * pMsg = (DBR_Cmd_ChangeRoleMoney*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeRoleMoney('%u','%d','%d','%d');", pMsg->dwUserID, pMsg->GlobelSum, pMsg->MedalSum, pMsg->CurrceySum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleTableChange(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҽ������ӵ�����
	DBR_Cmd_TableChange * pMsg = (DBR_Cmd_TableChange*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishRoleTableInfo('%u','%u','%u','%u','%u','%u','%d-%d-%d %d:%d:%d',%d);", pMsg->dwUserID, pMsg->TableTypeID, pMsg->TableMonthID, pMsg->GlobelSum, pMsg->MedalSum, pMsg->CurrceySum
		, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec, pMsg->JoinOrLeave ? 1 : 0);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleRoleEntityItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�������ʵ����Ʒ�Ĳ���
	//1.ֱ��ִ�д洢����
	//FishGetEntityItemInfo
	DBR_Cmd_RoleEntityItem* pMsg = (DBR_Cmd_RoleEntityItem*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	DBO_Cmd_RoleEntityItem* msg = (DBO_Cmd_RoleEntityItem*)CreateCmd(DBO_RoleEntityItem, sizeof(DBO_Cmd_RoleEntityItem));
	if (!msg)
	{
		ASSERT(false);
		return false;
	}
	msg->ClientID = pMsg->ClientID;
	msg->ClientIP = pMsg->ClientIP;
	strncpy_s(msg->OrderNumber, CountArray(msg->OrderNumber), pMsg->OrderNumber, strlen(pMsg->OrderNumber));

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishGetEntityItemInfo('%u');", pMsg->ID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->Result = true;
		//�����ݶ�ȡ����
		//ID,ItemID,ItemSum,MedalNum,`Name`,Address,IDEntity,NowMedalNum,Phone,ShopTime,UserID
		msg->ID = pTable1.GetUint(0, 0);
		msg->ItemID = pTable1.GetUint(0, 1);
		msg->ItemSum = pTable1.GetUint(0, 2);
		msg->MedalNum = pTable1.GetUint(0, 3);
		TCHARCopy(msg->Name, CountArray(msg->Name), pTable1.GetStr(0, 4),_tcslen(pTable1.GetStr(0, 4)));
		TCHARCopy(msg->Address, CountArray(msg->Address), pTable1.GetStr(0, 5), _tcslen(pTable1.GetStr(0, 5)));
		TCHARCopy(msg->IDEntity, CountArray(msg->IDEntity), pTable1.GetStr(0, 6), _tcslen(pTable1.GetStr(0, 6)));
		msg->NowMedalNum = pTable1.GetUint(0, 7);
		msg->Phone = pTable1.GetUint64(0, 8);
		msg->ShopTime = pTable1.GetDateTime(0, 9);
		msg->UserID = pTable1.GetUint(0, 10);
	}
	else
	{
		msg->Result = false;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleDelRoleEntityItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��Ϊ�޸����״̬Ϊ 1
	DBR_Cmd_DelRoleEntityItem * pMsg = (DBR_Cmd_DelRoleEntityItem*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelRoleEntityItem('%u');", pMsg->ID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleMonthCardInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleMonthCardInfo * pMsg = (DBR_Cmd_SaveRoleMonthCardInfo*)pCmd;

	tm pEndTime;
	errno_t Error= localtime_s(&pEndTime, &pMsg->MonthCardEndTime);
	if (Error != 0)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleMonthCardInfo('%u','%u','%d-%d-%d %d:%d:%d');", pMsg->UserID, pMsg->MonthCardID, pEndTime.tm_year + 1900, pEndTime.tm_mon + 1, pEndTime.tm_mday, pEndTime.tm_hour, pEndTime.tm_min, pEndTime.tm_sec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleMaxRate(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleMaxRateValue* pMsg = (DBR_Cmd_SaveRoleMaxRateValue*)pCmd;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleMaxRate('%u','%u');", pMsg->dwUserID,pMsg->MaxRate);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleRateValue(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleRateValue * pMsg = (DBR_Cmd_SaveRoleRateValue*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	char pRateStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->RateValue, sizeof(pMsg->RateValue), pRateStates);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleRateValue('%u','%s','%u');", pMsg->UserID, pRateStates,pMsg->MaxRateValue);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleGetMonthCardRewardTime(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleGetMonthCardRewardTime* pMsg = (DBR_Cmd_SaveRoleGetMonthCardRewardTime*)pCmd;

	tm pEndTime;
	errno_t Error = localtime_s(&pEndTime, &pMsg->LogTime);
	if (Error != 0)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleMonthCardRewardTime('%u','%d-%d-%d %d:%d:%d');", pMsg->UserID, pEndTime.tm_year + 1900, pEndTime.tm_mon + 1, pEndTime.tm_mday, pEndTime.tm_hour, pEndTime.tm_min, pEndTime.tm_sec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleVipLevel(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҵ�VIP�ȼ�
	DBR_Cmd_SaveRoleVipLevel* pMsg = (DBR_Cmd_SaveRoleVipLevel*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleVipLevel('%u','%u');", pMsg->UserID, pMsg->VipLevel);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleCashSum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҽ���һ��Ĵ���
	DBR_Cmd_SaveRoleCashSum* pMsg = (DBR_Cmd_SaveRoleCashSum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCashSum('%u','%u','%u');", pMsg->UserID, pMsg->CashSum,pMsg->TotalCashSum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleParticularStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҽ���һ��Ĵ���
	DBR_Cmd_SaveRoleParticularStates* pMsg = (DBR_Cmd_SaveRoleParticularStates*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleParticularStates('%u','%u');", pMsg->dwUserID, pMsg->ParticularStates);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleSecPassword(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleSecPassword* pMsg = (DBR_Cmd_SaveRoleSecPassword*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveSecPassword('%u','%u','%u','%u');", pMsg->dwUserID, pMsg->SecPasswordCrc1, pMsg->SecPasswordCrc2, pMsg->SecPasswordCrc3);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeRoleShareStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ChangeRoleShareStates* pMsg = (DBR_Cmd_ChangeRoleShareStates*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleShareStates('%u',%d);", pMsg->dwUserID, pMsg->States?1:0);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleGameIDConvertUserID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_GameIDConvertToUserID* pMsg = (DBR_Cmd_GameIDConvertToUserID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	NetCmd* msg = (NetCmd*)&pMsg->NetCmd;
	if (!msg)
	{
		ASSERT(false);
		return false;
	}
	WORD PageSize = sizeof(DBO_Cmd_GameIDConvertToUserID) + msg->GetCmdSize() - sizeof(BYTE);
	DBO_Cmd_GameIDConvertToUserID* msgDB = (DBO_Cmd_GameIDConvertToUserID*)malloc(PageSize);
	if (!msgDB)
	{
		ASSERT(false);
		return false;
	}
	msgDB->SetCmdSize(PageSize);
	msgDB->SetCmdType(DBO_GameIDConvertToUserID);
	msgDB->dwGameID = pMsg->dwGameID;
	msgDB->SrcUserID = pMsg->SrcUserID;
	memcpy_s((void*)&msgDB->NetCmd, msg->GetCmdSize(), msg, msg->GetCmdSize());

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishGameIDConvertUserID('%u');", pMsg->dwGameID);
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msgDB->dwUserID = pTable1.GetUint(0, 0);
	}
	else
	{
		msgDB->dwUserID = 0;
	}
	OnAddDBResult(Index, ClientID, msgDB);
	return true;
	
}
bool FishServer::OnHandleChangeRoleSecPassword(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�޸���ҵĶ�������  FishChangeRoleSecPassword
	DBR_Cmd_ChangeRoleSecPassword* pMsg = (DBR_Cmd_ChangeRoleSecPassword*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	DBO_Cmd_ChangeRoleSecPassword* msg = (DBO_Cmd_ChangeRoleSecPassword*)CreateCmd(DBO_ChangeRoleSecPassword, sizeof(DBO_Cmd_ChangeRoleSecPassword));
	if (!msg)
	{
		ASSERT(false);
		return false;
	}
	msg->dwUserID = pMsg->dwUserID;
	msg->Result = false;
	msg->dwNewSecPasswordCrc1 = pMsg->dwNewSecPasswordCrc1;
	msg->dwNewSecPasswordCrc2 = pMsg->dwNewSecPasswordCrc2;
	msg->dwNewSecPasswordCrc3 = pMsg->dwNewSecPasswordCrc3;

	
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeRoleSecPassword('%u','%u','%u','%u','%u','%u','%u');", pMsg->dwUserID,pMsg->dwOldSecPasswordCrc1,pMsg->dwOldSecPasswordCrc2,pMsg->dwOldSecPasswordCrc3,
		pMsg->dwNewSecPasswordCrc1,pMsg->dwNewSecPasswordCrc2,pMsg->dwNewSecPasswordCrc3);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->Result = true;
	}
	else
	{
		msg->Result = false;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleDelRoleCashSum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRoleCashSum* pMsg = (DBR_Cmd_DelRoleCashSum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelRoleCashSum('%u','%u');", pMsg->UserID, pMsg->DelCashSum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleMedal(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleMedal* pMsg = (DBR_Cmd_SaveRoleMedal*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleMedal('%u','%u','%u');", pMsg->dwUserID, pMsg->dwMedalSum,pMsg->TotalUseMedal);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleSendGiffSum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleSendGiffSum* pMsg = (DBR_Cmd_SaveRoleSendGiffSum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleSendGiffSum('%u','%u');", pMsg->dwUserID, pMsg->SendSum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleAcceptGiffSum(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleAcceptGiffSum* pMsg = (DBR_Cmd_SaveRoleAcceptGiffSum*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleAcceptGiffSum('%u','%u');", pMsg->dwUserID, pMsg->AcceptSum);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleTaskStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleTaskStates* pMsg = (DBR_Cmd_SaveRoleTaskStates*)pCmd;

	char pTaskStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->States, sizeof(pMsg->States), pTaskStates);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleTaskStates('%u','%s');", pMsg->dwUserID, pTaskStates);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleAchievementStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleAchievementStates* pMsg = (DBR_Cmd_SaveRoleAchievementStates*)pCmd;

	char pAchievementStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->States, sizeof(pMsg->States), pAchievementStates);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleAchievementStates('%u','%s');", pMsg->dwUserID, pAchievementStates);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleActionStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleActionStates* pMsg = (DBR_Cmd_SaveRoleActionStates*)pCmd;

	char pActionStates[MAXBLOB_LENGTH] = { 0 };
	m_Sql[Index].GetMySqlEscapeString((char*)&pMsg->States, sizeof(pMsg->States), pActionStates);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleActionStates('%u','%s');", pMsg->dwUserID, pActionStates);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleOnlineStates(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleOnlineStates* pMsg = (DBR_Cmd_SaveRoleOnlineStates*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleOnlineStates('%u','%u');", pMsg->dwUserID, pMsg->OnlineStates);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleCheckData(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleCheckData* pMsg = (DBR_Cmd_SaveRoleCheckData*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCheckData('%u','%u');", pMsg->dwUserID, pMsg->CheckData);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleIsShopIpAddress(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleIsShowIpAddress* pMsg = (DBR_Cmd_SaveRoleIsShowIpAddress*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleIsShopIpAddress('%u','%u');", pMsg->dwUserID, pMsg->IsShowIP ? 1 : 0);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleAllInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleAllInfo* pMsg = (DBR_Cmd_SaveRoleAllInfo*)pCmd;
	//һ���Ա������ȫ��������
	//1.��ұ��� FishSaveRoleOtherInfo
	if ((pMsg->States & MsgBegin) != 0)
	{
		{
			//��Ҫ���������ʱ�䱣������
			SYSTEMTIME pTime;
			GetLocalTime(&pTime);

			tm pLogTime;
			errno_t Error = localtime_s(&pLogTime, &pMsg->RoleServerInfo.RoleProtectLogTime);
			if (Error != 0)
			{
				ASSERT(false);
				return false;
			}

			SqlTable pTable1;
			char SqlStr[MAXSQL_LENGTH] = { 0 };
			sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleOtherInfo(%d,'%u','%lld','%lld','%u','%u','%u','%u','%u','%lld','%u','%d-%d-%d %d:%d:%d','%d-%d-%d %d:%d:%d','%u','%u');", pMsg->IsNeedResult, pMsg->dwUserID, pMsg->dwGlobeNum, pMsg->WeekGlobeNum, pMsg->OnlineSec, pMsg->dwExp, pMsg->dwProduction, pMsg->dwGameTime, pMsg->ClientIP,
				pMsg->RoleServerInfo.TotalFishGlobelSum, pMsg->RoleServerInfo.RoleProtectSum,
				pLogTime.tm_year + 1900, pLogTime.tm_mon + 1, pLogTime.tm_mday, pLogTime.tm_hour, pLogTime.tm_min, pLogTime.tm_sec,
				pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond,pMsg->dwLotteryScore,pMsg->bLotteryFishSum);
			bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
			if (!Result)
			{
				LogInfoToFile(DBErrorSqlFileName, SqlStr);
				ASSERT(false);
			}
		}
		//2.GameData
		{
		DBR_Cmd_SaveGameData msg;
		SetMsgInfo(msg, DBR_SaveGameData, sizeof(DBR_Cmd_SaveGameData));
		msg.dwUserID = pMsg->dwUserID;
		msg.GameData = pMsg->GameData;
		OnHandleSaveRoleGameData(Index, ClientID, &msg);
	}
	}
	for (int i = 0; i < pMsg->Sum; ++i)
	{
		SaveEventInfo pInfo = pMsg->Array[i];
		switch (pInfo.InfoStates)
		{
		case 1:
		{
				  //����
				  DBR_Cmd_SaveRoleTask msg;
				  SetMsgInfo(msg, DBR_SaveRoleTask, sizeof(DBR_Cmd_SaveRoleTask));
				  msg.dwUserID = pMsg->dwUserID;
				  msg.TaskInfo = pInfo.EventInfo.TaskInfo;
				  OnHandleSaveRoleTask(Index, ClientID, &msg);
		}
			break;
		case 2:
		{
				  DBR_Cmd_SaveRoleAchievement msg;
				  SetMsgInfo(msg, DBR_SaveRoleAchievement, sizeof(DBR_Cmd_SaveRoleAchievement));
				  msg.dwUserID = pMsg->dwUserID;
				  msg.AchievementInfo = pInfo.EventInfo.AchievementInfo;
				  OnHandleSaveRoleAchievement(Index, ClientID, &msg);
		}
			break;
		case 3:
		{
				  DBR_Cmd_SaveRoleAction msg;
				  SetMsgInfo(msg, DBR_SaveRoleAction, sizeof(DBR_Cmd_SaveRoleAction));
				  msg.dwUserID = pMsg->dwUserID;
				  msg.ActionInfo = pInfo.EventInfo.ActionInfo;
				  OnHandleSaveRoleAction(Index, ClientID, &msg);
		}
			break;
		}
	}
	if ((pMsg->States & MsgEnd) != 0)
	{
		if (pMsg->IsNeedResult)
		{
			DBO_Cmd_SaveRoleAllInfo* msg = (DBO_Cmd_SaveRoleAllInfo*)CreateCmd(DBO_SaveRoleAllInfo, sizeof(DBO_Cmd_SaveRoleAllInfo));
			msg->dwUserID = pMsg->dwUserID;
			OnAddDBResult(Index, ClientID, msg);
		}
	}
	return true;
}
bool FishServer::OnHandleGetUserItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//���������Ʒ 
	//�����Ʒ û�취�������� ������зְ�����
	DBR_Cmd_LoadUserItem* pMsg = (DBR_Cmd_LoadUserItem*)pCmd;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadItem('%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadUserItem)+(Rows - 1)*sizeof(tagItemInfo);
		DBO_Cmd_LoadUserItem * msg = (DBO_Cmd_LoadUserItem*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadUserItem);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].ItemOnlyID = pTable1.GetUint(i, 0);
			msg->Array[i].ItemID = pTable1.GetUint(i, 1);
			msg->Array[i].ItemSum = pTable1.GetUint(i, 2);
			msg->Array[i].EndTime = pTable1.GetDateTime(i, 3);
		}
		//����׼����� ���Ƿָ�����
		std::vector<DBO_Cmd_LoadUserItem*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadUserItem*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		//ֱ�ӷ�������
		DBO_Cmd_LoadUserItem * msg = (DBO_Cmd_LoadUserItem*)CreateCmd(DBO_LoadUserItem, sizeof(DBO_Cmd_LoadUserItem)-sizeof(tagItemInfo));
		if (!msg)
		{
			ASSERT(false);
			return false;
		}
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);//��ʼ�ͽ���
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}

	LogInfoToFile("WmItemSql", SqlStr);
	return true;
}
bool FishServer::OnHandleAddUserItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddUserItem* pMsg = (DBR_Cmd_AddUserItem*)pCmd;
	DBO_Cmd_AddUserItem* msg = (DBO_Cmd_AddUserItem*)CreateCmd(DBO_AddUserItem, sizeof(DBO_Cmd_AddUserItem));

	msg->dwUserID = pMsg->dwUserID;
	msg->ItemInfo.ItemID = pMsg->ItemInfo.ItemID;
	msg->ItemInfo.ItemSum = pMsg->ItemInfo.ItemSum;
	//����ұ��������һ����Ʒ
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	if (g_FishServer.GetFishConfig().IsTimeItem(pMsg->ItemInfo.ItemID))//ʱ�����
	{
		if (pMsg->ItemInfo.LastMin == 0)//ʱ��Ϊ0
		{
			LogInfoToFile("WmItemError", "db time itemid=%u, itemsum=%u ,lastmin=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.LastMin);
			return true;
		}
		msg->ItemInfo.EndTime = time(NULL) + pMsg->ItemInfo.LastMin * pMsg->ItemInfo.ItemSum * 60;
		pMsg->ItemInfo.ItemSum = 1;
		tm NowTime;
		errno_t Err = localtime_s(&NowTime, &msg->ItemInfo.EndTime);
		if (Err != 0)
		{
			free(msg);
			ASSERT(false);
			return false;
		}
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddItem('%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum,
			NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);

	}
	else
	{
		if (pMsg->ItemInfo.ItemSum == 0)//��Ʒ����Ϊ0
		{
			LogInfoToFile("WmItemError", "db no time itemid=%u, itemsum=%u ,lastmin=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.LastMin);
			return true;
		}
		msg->ItemInfo.EndTime = 0;//�洢���̸��ݴ��ж�
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddItem('%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, 0, 0, 0, 0, 0, 0);

	}

	LogInfoToFile("WmItemSql", SqlStr);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->ItemInfo.ItemOnlyID = pTable1.GetUint(0, 0);
		//�����Ʒ�ɹ� ���غ���
		OnAddDBResult(Index, ClientID, msg);
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
		free(msg);
	}
	return true;
}
bool FishServer::OnHandleDelUserItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelUserItem* pMsg = (DBR_Cmd_DelUserItem*)pCmd;
	//���ɾ��ָ������Ʒ
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelItem('%u');", pMsg->ItemOnlyID);
	LogInfoToFile("WmItemSql", SqlStr);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeUserItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ChangeUserItem* pMsg = (DBR_Cmd_ChangeUserItem*)pCmd;
	//�޸�һ����Ʒ������
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	//if (pMsg->EndTime > 0)
	if (g_FishServer.GetFishConfig().IsTimeItem(pMsg->ItemID))//ʱ�����
	{
		tm NowTime;
		const time_t ConstEndTime = pMsg->EndTime;
		errno_t Err = localtime_s(&NowTime, &ConstEndTime);
		if (Err != 0)
		{
			ASSERT(false);
			return false;
		}
		sprintf_s(SqlStr, sizeof(SqlStr), "update fishitem set fishitem.EndTime = '%d-%d-%d %d:%d:%d' where fishitem.ItemOnlyID = '%d'", NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec, pMsg->ItemOnlyID);
		bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr)  /*&& pTable1.GetUint(0, 0) == 1*/));

		if (!Result)
		{
			LogInfoToFile(DBErrorSqlFileName, SqlStr);
			ASSERT(false);
		}
	}
	else
	{
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeUserItem('%u','%u');", pMsg->ItemOnlyID, pMsg->ItemSum);
		bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);

		if (!Result)
		{
			LogInfoToFile(DBErrorSqlFileName, SqlStr);
			ASSERT(false);
		}
	}
	LogInfoToFile("WmItemSql", SqlStr);

	return true;
}
bool FishServer::OnHandleGetUserRelation(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadUserRelation* pMsg = (DBR_Cmd_LoadUserRelation*)pCmd;
	//������ҵĹ�ϵ ��2��  �������������ϵ �� ��ҵı�����ϵ
	SqlTable pTable1, pTable2;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadUserRelation('%u','%u');", pMsg->dwUserID, m_Config.GetRelation().MaxRelationSum);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadUserRelation)+(Rows - 1)*sizeof(tagRoleRelation);
		DBO_Cmd_LoadUserRelation * msg = (DBO_Cmd_LoadUserRelation*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadUserRelation);
		msg->dwUserID = pMsg->dwUserID;
		time_t pNow = time(null);
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].dwDestRoleID = pTable1.GetUint(i, 0);
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 1);
			msg->Array[i].wLevel = pTable1.GetUshort(i, 2);
			msg->Array[i].bGender = pTable1.GetBit(i, 3);
			msg->Array[i].LastLogonTime = pTable1.GetInt64(i, 4);
			msg->Array[i].IsOnline = pTable1.GetBit(i, 5);
			const TCHAR* pNickName = pTable1.GetStr(i, 6);
			TCHARCopy(msg->Array[i].DestNickName, CountArray(msg->Array[i].DestNickName), pNickName, _tcslen(pNickName));
			msg->Array[i].bRelationType = pTable1.GetByte(i, 7);
			msg->Array[i].TitleID = pTable1.GetByte(i, 8);
			msg->Array[i].dwAchievementPoint = pTable1.GetUint(i, 9);
			memcpy_s(&msg->Array[i].CharmArray, sizeof(msg->Array[i].CharmArray), pTable1.GetField((UINT)i, (UINT)10), sizeof(msg->Array[i].CharmArray));
			msg->Array[i].ClientIP = pTable1.GetUint(i, 11);
			msg->Array[i].IsShowIpAddress = pTable1.GetByte(i, 12) == 1 ? true : false;

			msg->Array[i].VipLevel = pTable1.GetByte(i, 13);
			BYTE MonthCardID = pTable1.GetByte(i, 14);
			__int64 MonthCardEndTime = pTable1.GetDateTime(i, 15);
			msg->Array[i].IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
			msg->Array[i].WeekGoldNum = pTable1.GetInt64(i, 16);
			msg->Array[i].byUsingLauncher = pTable1.GetByte(i, 17);
			msg->Array[i].GameID = pTable1.GetUint(i, 18);
			//msg->Array[i].RateIndex = pTable1.GetByte(i, 19);
		}
		//�������Ϻ� �ְ�����
		std::vector<DBO_Cmd_LoadUserRelation*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadUserRelation*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadUserRelation* msg = (DBO_Cmd_LoadUserRelation*)CreateCmd(DBO_LoadUserRelation, sizeof(DBO_Cmd_LoadUserRelation)-sizeof(tagRoleRelation));
		msg->dwUserID = pMsg->dwUserID;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	//ִ���µĴ洢����  ������ϵ������ �޷��������� ������Ҫ���зְ����� ���ƺ�����
	ZeroMemory(SqlStr, sizeof(SqlStr));
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadBeRelation('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 1, pTable2, true))
	{
		DWORD Rows = pTable2.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadBeUserRelation)+(Rows - 1)*sizeof(tagBeRoleRelation);
		DBO_Cmd_LoadBeUserRelation* msgBe = (DBO_Cmd_LoadBeUserRelation*)malloc(PageSize);
		if (!msgBe)
		{
			ASSERT(false);
			return false;
		}
		msgBe->SetCmdType(DBO_LoadBeUserRelation);
		msgBe->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Rows; ++i)
		{
			msgBe->Array[i].dwUserID = pTable2.GetUint(i, 0);
			msgBe->Array[i].bRelationType = pTable2.GetByte(i, 1);
		}
		std::vector<DBO_Cmd_LoadBeUserRelation*> pVec;
		SqlitMsg(msgBe, PageSize, Rows, false, pVec);
		free(msgBe);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadBeUserRelation*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadBeUserRelation* msgBeFinish = (DBO_Cmd_LoadBeUserRelation*)CreateCmd(DBO_LoadBeUserRelation, sizeof(DBO_Cmd_LoadBeUserRelation)-sizeof(tagBeRoleRelation));
		msgBeFinish->dwUserID = pMsg->dwUserID;
		msgBeFinish->Sum = 0;
		msgBeFinish->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msgBeFinish);

		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddUserRelation(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����ҵĹ�ϵ����
	DBR_Cmd_AddUserRelation* pMsg = (DBR_Cmd_AddUserRelation*)pCmd;
	DBO_Cmd_AddUserRelation* msg = (DBO_Cmd_AddUserRelation*)CreateCmd(DBO_AddUserRelation, sizeof(DBO_Cmd_AddUserRelation));
	msg->dwUserID = pMsg->dwSrcUserID;
	msg->RelationInfo.dwDestRoleID = pMsg->dwDestUserID;
	msg->RelationInfo.bRelationType = pMsg->bRelationType;
	msg->Result = false;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddUserRelation('%u','%u','%u');", pMsg->dwSrcUserID, pMsg->dwDestUserID, pMsg->bRelationType);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->Result = true;
		msg->RelationInfo.dwDestRoleID = pTable1.GetUint(0, 0);
		msg->RelationInfo.dwFaceID = pTable1.GetUint(0, 1);
		msg->RelationInfo.wLevel = pTable1.GetUshort(0, 2);
		msg->RelationInfo.bGender = pTable1.GetBit(0, 3);
		msg->RelationInfo.LastLogonTime = pTable1.GetInt64(0, 4);
		msg->RelationInfo.IsOnline = pTable1.GetBit(0, 5);
		const TCHAR* pNickName = pTable1.GetStr(0, 6);
		TCHARCopy(msg->RelationInfo.DestNickName, CountArray(msg->RelationInfo.DestNickName), pNickName, _tcslen(pNickName));
		msg->RelationInfo.bRelationType = pTable1.GetByte(0, 7);
		msg->RelationInfo.TitleID = pTable1.GetByte(0, 8);
		msg->RelationInfo.dwAchievementPoint = pTable1.GetUint(0, 9);
		memcpy_s(&msg->RelationInfo.CharmArray, sizeof(msg->RelationInfo.CharmArray), pTable1.GetField((UINT)0, (UINT)10), sizeof(msg->RelationInfo.CharmArray));
		msg->RelationInfo.ClientIP = pTable1.GetUint(0, 11);
		msg->RelationInfo.IsShowIpAddress = pTable1.GetByte(0, 12) == 1 ? true : false;
		msg->RelationInfo.GameID = pTable1.GetUint(0, 13);
		msg->RelationInfo.WeekGoldNum = pTable1.GetInt64(0, 14);
		OnAddDBResult(Index, ClientID, msg);
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleDelUserRelation(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelUserRelation* pMsg = (DBR_Cmd_DelUserRelation*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelUserRelation('%u','%u');", pMsg->dwSrcUserID, pMsg->dwDestUserID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
//bool FishServer::OnHandleChangeUserRelation(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	//�޸���ҵĹ�ϵ����
//	DBR_Cmd_ChangeUserRelation* pMsg = (DBR_Cmd_ChangeUserRelation*)pCmd;
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeUserRelation('%u','%u','%u');", pMsg->dwSrcUserID, pMsg->dwDestUserID, pMsg->bRelationType);
//	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*/*&& pTable1.GetUint(0, 0) == 1*/*/);
//	if (!Result)
//		ASSERT(false);
//	return true;
//}
bool FishServer::OnHandleGetUserMail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ȡ���ȫ�����ʼ�
	DBR_Cmd_LoadUserMail* pMsg = (DBR_Cmd_LoadUserMail*)pCmd;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadUserMail('%u','%u','%u', '%d-%d-%d %d:%d:%d');", pMsg->dwUserID, m_Config.GetFishMailConfig().MailLimitDay, m_Config.GetFishMailConfig().MaxUserMailSum
		, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		//�����ʼ�
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadUserMail)+(Rows - 1)*sizeof(tagRoleMail);
		DBO_Cmd_LoadUserMail* msg = (DBO_Cmd_LoadUserMail*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return false;
		}
		msg->SetCmdType(DBO_LoadUserMail);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].MailID = pTable1.GetUint(i, 0);
			msg->Array[i].SrcUserID = pTable1.GetUint(i, 1);
			const TCHAR* pContext = pTable1.GetStr(i, 2);
			TCHARCopy(msg->Array[i].Context, CountArray(msg->Array[i].Context), pContext, _tcslen(pContext));
			msg->Array[i].RewardID = pTable1.GetUshort(i, 3);
			msg->Array[i].RewardSum = pTable1.GetUint(i, 4);
			msg->Array[i].bIsRead = pTable1.GetBit(i, 5);
			msg->Array[i].bIsExistsReward = pTable1.GetBit(i, 6);
			if (msg->Array[i].RewardID == 0 || msg->Array[i].RewardSum == 0)
				msg->Array[i].bIsExistsReward = false;
			msg->Array[i].SendTimeLog = pTable1.GetDateTime(i, 7);
			msg->Array[i].SrcFaceID = pTable1.GetUint(i, 8);
			const TCHAR* pNickName = pTable1.GetStr(i, 9);
			TCHARCopy(msg->Array[i].SrcNickName, CountArray(msg->Array[i].SrcNickName), pNickName, _tcslen(pNickName));
		}

		std::vector<DBO_Cmd_LoadUserMail*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadUserMail*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadUserMail * msgFinish = (DBO_Cmd_LoadUserMail*)CreateCmd(DBO_LoadUserMail, sizeof(DBO_Cmd_LoadUserMail)-sizeof(tagRoleMail));
		msgFinish->dwUserID = pMsg->dwUserID;
		msgFinish->Sum = 0;
		msgFinish->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msgFinish);

		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddUserMailSendItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ָ����� ���һ���ʼ�
	DBR_Cmd_AddUserMail * pMsg = (DBR_Cmd_AddUserMail*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	sprintf_s(SqlStr, sizeof(SqlStr), "insert into  FishSendItem(MailID, SrcUserID, DestUserID, IsRead, RewardID, RewardSum, SendTime) values('%u','%u','%u', 0, '%u','%u','%d-%d-%d %d:%d:%d');",pMsg->MailInfo.MailID, pMsg->MailInfo.SrcUserID, pMsg->dwDestUserID, pMsg->MailInfo.RewardID, pMsg->MailInfo.RewardSum,pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadUserMailSendItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadUserSendItem* pMsg = (DBR_Cmd_LoadUserSendItem*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	BYTE Count = PAGE_COUNT;

	sprintf_s(SqlStr, sizeof(SqlStr), "	select fsm.SrcUserID, fsm.DestUserID, fsm.RewardID, fsm.RewardSum, fsm.SendTime, fsm.IsRead, srcaco.NickName as SrcNickName, destaco.NickName as DestNickName \
		from fishsenditem as fsm  \
		left join  accountinfo as srcaco on(srcaco.UserID = fsm.SrcUserID)   \
		left join  accountinfo as destaco on(destaco.UserID = fsm.DestUserID)  \
	    where  fsm.SrcUserID = '%u' or fsm.DestUserID = '%u'  limit %u, %u", pMsg->dwUserID, pMsg->dwUserID, /*Count**/pMsg->Page, Count);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadUserSendItem) + (Row - 1) * sizeof(tagRoleSendItem);
		DBO_Cmd_LoadUserSendItem* msg = (DBO_Cmd_LoadUserSendItem*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadUserSendItem);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].SrcUserID = pTable1.GetUint(i, 0);
			msg->Array[i].DestUserID = pTable1.GetUint(i, 1);
			msg->Array[i].RewardID = pTable1.GetUint(i, 2);
			msg->Array[i].RewardSum = pTable1.GetUint(i, 3);
			msg->Array[i].SendTime = pTable1.GetDateTime(i, 4);
			msg->Array[i].bIsRead = pTable1.GetBit(i, 5);
			const TCHAR* pNickName = pTable1.GetStr(i, 6);
			TCHARCopy(msg->Array[i].SrcNickName, CountArray(msg->Array[i].SrcNickName), pNickName, _tcslen(pNickName));
			pNickName = pTable1.GetStr(i, 7);
			TCHARCopy(msg->Array[i].DestNickName, CountArray(msg->Array[i].DestNickName), pNickName, _tcslen(pNickName));
		}

		std::vector<DBO_Cmd_LoadUserSendItem*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadUserSendItem*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadUserSendItem* msg = (DBO_Cmd_LoadUserSendItem*)CreateCmd(DBO_LoadUserSendItem, sizeof(DBO_Cmd_LoadUserSendItem) - sizeof(tagRoleSendItem));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleAddAllUserMail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ָ����� ���һ���ʼ�
	DBR_Cmd_AddUserMail * pMsg = (DBR_Cmd_AddUserMail*)pCmd;
	DBO_Cmd_AddUserMail* msg = (DBO_Cmd_AddUserMail*)CreateCmd(DBO_AddAllUserMail, sizeof(DBO_Cmd_AddUserMail));
	msg->dwDestUserID = pMsg->dwDestUserID;
	msg->Result = false;
	msg->MailInfo = pMsg->MailInfo;
	msg->MailInfo.MailID = 0;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	UINT Size = 0;
	char* Context = WCharToChar(pMsg->MailInfo.Context, Size);
	char* DestContext = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestContext)
	{
		free(msg);
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestContext, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Context, Size, DestContext);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);


	sprintf_s(SqlStr, sizeof(SqlStr), "insert into FishMail(SrcUserID,DestUserID,Context,RewardID,RewardSum,IsRead,IsExistsReward,SendTime) \
		select 0, accountinfo.UserID, '%s', '%d', '%d', 0, ( '%d'!= 0), '%d-%d-%d %d:%d:%d' from accountinfo where accountinfo.IsRobot != 1; ", DestContext,
		pMsg->MailInfo.RewardID, pMsg->MailInfo.RewardSum, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	free(DestContext);
	free(Context);

	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr)) );
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	else
	{
		msg->Result = true;
	}

	OnAddDBResult(Index, ClientID, msg);

	return true;
}


bool FishServer::OnHandleAddUserMail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ָ����� ���һ���ʼ�
	DBR_Cmd_AddUserMail * pMsg = (DBR_Cmd_AddUserMail*)pCmd;
	DBO_Cmd_AddUserMail* msg = (DBO_Cmd_AddUserMail*)CreateCmd(DBO_AddUserMail, sizeof(DBO_Cmd_AddUserMail));
	msg->dwDestUserID = pMsg->dwDestUserID;
	msg->Result = false;
	msg->MailInfo = pMsg->MailInfo;
	msg->MailInfo.MailID = 0;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	UINT Size = 0;
	char* Context = WCharToChar(pMsg->MailInfo.Context, Size);
	char* DestContext = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestContext)
	{
		free(msg);
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestContext, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Context, Size, DestContext);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);


	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddUserMail('%u','%u','%s','%u','%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->MailInfo.SrcUserID, pMsg->dwDestUserID, DestContext,
		pMsg->MailInfo.RewardID, pMsg->MailInfo.RewardSum, m_Config.GetFishMailConfig().MaxUserMailSum, m_Config.GetFishMailConfig().MailLimitDay
		, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	free(DestContext);
	free(Context);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		if (pTable1.GetUint(0, 0) == 0)
		{
			msg->Result = false;
		}
		else
		{
			msg->Result = true;
			msg->MailInfo.MailID = pTable1.GetUint(0, 0);
			msg->MailInfo.SendTimeLog = time(NULL);
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}

	if (pMsg->MailInfo.SrcUserID != 0 && msg->Result)//����
	{
		OnHandleAddUserMailSendItem(Index, ClientID, pCmd);
	}

	OnAddDBResult(Index, ClientID, msg);

	return true;
}
bool FishServer::OnHandleDelUserMail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�Ƴ���һ���ʼ�
	DBR_Cmd_DelUserMail* pMsg = (DBR_Cmd_DelUserMail*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelUserMail('%u');", pMsg->dwMailID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleGetUserMailItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//���ʼ�����ȥ��Ʒ 
	DBR_Cmd_GetUserMailItem* pMsg = (DBR_Cmd_GetUserMailItem*)pCmd;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishGetMailItem('%u');", pMsg->dwMailID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeUserMailIsRead(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//���ʼ�����Ϊ�Ѷ�
	DBR_Cmd_ChangeUserMailIsRead* pMsg = (DBR_Cmd_ChangeUserMailIsRead*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeUserMailIsRead('%u',%d);", pMsg->dwMailID, pMsg->bIsRead ? 1 : 0);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleQueryRoleInfoByNickName(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ҽ��в�ѯ
	DBR_Cmd_QueryRoleInfoByNickName* pMsg = (DBR_Cmd_QueryRoleInfoByNickName*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* NickName = WCharToChar(pMsg->QueryNickName, Size);
	char* DestNickName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestNickName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestNickName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(NickName, Size, DestNickName);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryUserInfoByNickName('%u','%s','%u','%u',%d);", pMsg->dwUserID, DestNickName, pMsg->Page*QUERY_ROLE_SUM, QUERY_ROLE_SUM, pMsg->IsOnline ? 1 : 0);
	free(DestNickName);
	free(NickName);
	time_t pNow = time(null);
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_QueryRoleInfo)+(Row - 1)*sizeof(tagQueryRoleInfo);
		DBO_Cmd_QueryRoleInfo* msg = (DBO_Cmd_QueryRoleInfo*)malloc(PageSize);
		msg->SetCmdType(DBO_Query_RoleInfo);
		msg->dwUserID = pMsg->dwUserID;
		msg->Page = pMsg->Page;
		for (DWORD i = 0; i < Row; ++i)
		{
			//����ѯ�����ݼ��ؽ���
			msg->Array[i].dwUserID = pTable1.GetUint(i, 0);
			const TCHAR* pNickName = pTable1.GetStr(i, 1);
			TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), pNickName, _tcslen(pNickName));
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 2);
			msg->Array[i].bGender = pTable1.GetBit(i, 3);
			msg->Array[i].wLevel = pTable1.GetUshort(i, 4);
			msg->Array[i].bIsOnline = pTable1.GetBit(i, 5);
			msg->Array[i].TitleID = pTable1.GetByte(i, 6);
			msg->Array[i].dwAchievementPoint = pTable1.GetUint(i, 7);
			memcpy_s(&msg->Array[i].CharmArray, sizeof(msg->Array[i].CharmArray), pTable1.GetField((UINT)i, (UINT)8), sizeof(msg->Array[i].CharmArray));
			msg->Array[i].ClientIP = pTable1.GetUint(i, 9);
			msg->Array[i].IsShowIpAddress = pTable1.GetByte(i, 10) == 1 ? true : false;
			msg->Array[i].VipLevel = pTable1.GetByte(i, 11);
			BYTE MonthCardID = pTable1.GetByte(i, 12);
			__int64 MonthCardEndTime = pTable1.GetDateTime(i, 13);
			msg->Array[i].GameID = pTable1.GetUint(i, 14);
			msg->Array[i].IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
		}

		std::vector<DBO_Cmd_QueryRoleInfo*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_QueryRoleInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_QueryRoleInfo* msg = (DBO_Cmd_QueryRoleInfo*)CreateCmd(DBO_Query_RoleInfo, sizeof(DBO_Cmd_QueryRoleInfo)-sizeof(tagQueryRoleInfo));
		msg->dwUserID = pMsg->dwUserID;
		msg->Page = pMsg->Page;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleQueryRoleInfoByIP(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ҽ��в�ѯ

	DBR_Cmd_Query_RoleInfoByIP* pMsg = (DBR_Cmd_Query_RoleInfoByIP*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	BYTE Count = PAGE_COUNT * 2;
	bool bUp = (pMsg->Page % 2) == 0 ? true : false;
	BYTE page = pMsg->Page / 2;
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryUserInfoByIP('%u','%u','%u','%u',%d);", pMsg->dwUserID, pMsg->ClientIP, page*Count, Count, bUp);

	time_t pNow = time(null);
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_Query_RoleInfoByIP) + (Row - 1) * sizeof(tagQueryRoleInfo);
		DBO_Cmd_Query_RoleInfoByIP* msg = (DBO_Cmd_Query_RoleInfoByIP*)malloc(PageSize);
		msg->SetCmdType(DBO_Query_RoleInfoByIP);
		msg->dwUserID = pMsg->dwUserID;
		msg->Page = pMsg->Page;
		for (DWORD i = 0; i < Row; ++i)
		{
			//����ѯ�����ݼ��ؽ���
			msg->Array[i].dwUserID = pTable1.GetUint(i, 0);
			const TCHAR* pNickName = pTable1.GetStr(i, 1);
			TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), pNickName, _tcslen(pNickName));
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 2);
			msg->Array[i].bGender = pTable1.GetBit(i, 3);
			msg->Array[i].wLevel = pTable1.GetUshort(i, 4);
			msg->Array[i].bIsOnline = pTable1.GetBit(i, 5);
			msg->Array[i].TitleID = pTable1.GetByte(i, 6);
			msg->Array[i].dwAchievementPoint = pTable1.GetUint(i, 7);
			memcpy_s(&msg->Array[i].CharmArray, sizeof(msg->Array[i].CharmArray), pTable1.GetField((UINT)i, (UINT)8), sizeof(msg->Array[i].CharmArray));
			msg->Array[i].ClientIP = pTable1.GetUint(i, 9);
			msg->Array[i].IsShowIpAddress = pTable1.GetByte(i, 10) == 1 ? true : false;
			msg->Array[i].VipLevel = pTable1.GetByte(i, 11);
			BYTE MonthCardID = pTable1.GetByte(i, 12);
			__int64 MonthCardEndTime = pTable1.GetDateTime(i, 13);
			msg->Array[i].GameID = pTable1.GetUint(i, 14);
			msg->Array[i].IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
		}

		std::vector<DBO_Cmd_Query_RoleInfoByIP*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_Query_RoleInfoByIP*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_Query_RoleInfoByIP* msg = (DBO_Cmd_Query_RoleInfoByIP*)CreateCmd(DBO_Query_RoleInfoByIP, sizeof(DBO_Cmd_Query_RoleInfoByIP) - sizeof(tagQueryRoleInfo));
		msg->dwUserID = pMsg->dwUserID;
		msg->Page = pMsg->Page;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}


bool FishServer::OnHandleQueryRoleInfoByGameID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_Query_RoleInfoByGameID * pMsg = (DBR_Cmd_Query_RoleInfoByGameID*)pCmd;

	DBO_Cmd_Query_RoleInfoByGameID* msg = (DBO_Cmd_Query_RoleInfoByGameID*)CreateCmd(DBO_Query_RoleInfoByGameID, sizeof(DBO_Cmd_Query_RoleInfoByGameID));
	msg->dwUserID = pMsg->dwUserID;
	msg->RoleInfo.dwUserID = 0;
	msg->Type = pMsg->Type;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	if (pMsg->IsUserID)
	{
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryUserInfoByUserID('%u');", pMsg->dwDestID);
	}
	else
	{
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryUserInfoByGameID('%u');", pMsg->dwDestID);
	}
	

	time_t pNow = time(null);
	
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		msg->RoleInfo.dwUserID = pTable1.GetUint(0, 0);
		const TCHAR* pNickName = pTable1.GetStr(0, 1);
		TCHARCopy(msg->RoleInfo.NickName, CountArray(msg->RoleInfo.NickName), pNickName, _tcslen(pNickName));
		msg->RoleInfo.wLevel = pTable1.GetUshort(0, 2);
		msg->RoleInfo.dwFaceID = pTable1.GetUint(0, 3);
		msg->RoleInfo.bGender = pTable1.GetBit(0, 4);
		msg->RoleInfo.bIsOnline = pTable1.GetBit(0, 5);
		msg->RoleInfo.dwAchievementPoint = pTable1.GetUint(0, 6);
		msg->RoleInfo.TitleID = pTable1.GetByte(0, 7);
		memcpy_s(&msg->RoleInfo.CharmArray, sizeof(msg->RoleInfo.CharmArray), pTable1.GetField((UINT)0, (UINT)8), sizeof(msg->RoleInfo.CharmArray));
		msg->RoleInfo.ClientIP = pTable1.GetUint(0, 9);
		msg->RoleInfo.IsShowIpAddress = pTable1.GetByte(0, 10) == 1 ? true : false;
		msg->RoleInfo.VipLevel = pTable1.GetByte(0, 11);
		BYTE MonthCardID = pTable1.GetByte(0, 12);
		__int64 MonthCardEndTime = pTable1.GetDateTime(0, 13);
		msg->RoleInfo.GameID = pTable1.GetUint(0, 14);
		msg->RoleInfo.IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
		msg->RoleInfo.dwCashpoint = pTable1.GetUint(0, 15);       //��ȯ
		msg->RoleInfo.byUsingLauncher = pTable1.GetByte(0, 16);;//��ǰʹ�õ���̨
		msg->RoleInfo.MaxRateIndex = pTable1.GetByte(0, 17);;//����
		msg->RoleInfo.dwCurrencyNum = pTable1.GetUint(0, 18);;
		msg->RoleInfo.dwGlobeNum = pTable1.GetUint(0, 19);;
		msg->RoleInfo.MedalNum = pTable1.GetUint(0, 20);

		//tagRoleGameData GameData;
		msg->RoleInfo.GameData.RoleMonthRewardSum = pTable1.GetUint(0, 21);
		msg->RoleInfo.GameData.RoleMonthFirstSum = pTable1.GetUint(0, 22);
		msg->RoleInfo.GameData.RoleMonthSecondSum = pTable1.GetUint(0, 23);
		msg->RoleInfo.GameData.RoleMonthThreeSum = pTable1.GetUint(0, 24);
		msg->RoleInfo.GameData.RoleCatchFishTotalSum = pTable1.GetUint(0, 25);
		msg->RoleInfo.GameData.RoleGetGlobelSum = pTable1.GetInt64(0, 26);
		msg->RoleInfo.GameData.RoleMonthSigupSum = pTable1.GetUint(0, 27);
		msg->RoleInfo.GameData.NonMonthGameSec = pTable1.GetUint(0, 28);
		msg->RoleInfo.GameData.TotalGameSec = pTable1.GetUint(0, 29);
		//9 18 20 1 3 19
		msg->RoleInfo.GameData.CatchFishSum_9 = pTable1.GetUint(0, 30);
		msg->RoleInfo.GameData.CatchFishSum_18 = pTable1.GetUint(0, 31);
		msg->RoleInfo.GameData.CatchFishSum_20 = pTable1.GetUint(0, 32);
		msg->RoleInfo.GameData.CatchFishSum_1 = pTable1.GetUint(0, 33);
		msg->RoleInfo.GameData.CatchFishSum_3 = pTable1.GetUint(0, 34);
		msg->RoleInfo.GameData.CatchFishSum_19 = pTable1.GetUint(0, 35);
		msg->RoleInfo.GameData.MaxCombSum = pTable1.GetUint(0, 36);
		msg->Success = 0;//�ɹ�
	}
	else
	{
		msg->Success = 1;
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
//bool FishServer::OnHandleQueryRoleInfoByUserID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	//��Ұ�ID���в�ѯ
//	DBR_Cmd_Query_RoleInfoByUserID* pMsg = (DBR_Cmd_Query_RoleInfoByUserID*)pCmd;
//	DBO_Cmd_Query_RoleInfoByUserID* msg = (DBO_Cmd_Query_RoleInfoByUserID*)CreateCmd(DBO_Query_RoleInfoByUserID, sizeof(DBO_Cmd_Query_RoleInfoByUserID));
//	msg->dwUserID = pMsg->dwUserID;
//	msg->RoleInfo.dwUserID = 0;
//
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryUserInfoByUserID('%u');", pMsg->dwDestUserID);
//
//	time_t pNow = time(null);
//
//	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
//	{
//		msg->RoleInfo.dwUserID = pTable1.GetUint(0, 0);
//		const TCHAR* pNickName = pTable1.GetStr(0, 1);
//		TCHARCopy(msg->RoleInfo.NickName, CountArray(msg->RoleInfo.NickName), pNickName, _tcslen(pNickName));
//		msg->RoleInfo.dwFaceID = pTable1.GetUint(0, 2);
//		msg->RoleInfo.bGender = pTable1.GetBit(0, 3);
//		msg->RoleInfo.wLevel = pTable1.GetUshort(0, 4);
//		msg->RoleInfo.bIsOnline = pTable1.GetBit(0, 5);
//		msg->RoleInfo.TitleID = pTable1.GetByte(0, 6);
//		msg->RoleInfo.dwAchievementPoint = pTable1.GetUint(0, 7);
//		memcpy_s(&msg->RoleInfo.CharmArray, sizeof(msg->RoleInfo.CharmArray), pTable1.GetField((UINT)0, (UINT)8), sizeof(msg->RoleInfo.CharmArray));
//		msg->RoleInfo.ClientIP = pTable1.GetUint(0, 9);
//		msg->RoleInfo.IsShowIpAddress = pTable1.GetByte(0, 10) == 1 ? true : false;
//
//		msg->RoleInfo.VipLevel = pTable1.GetByte(0, 11);
//		BYTE MonthCardID = pTable1.GetByte(0, 12);
//		__int64 MonthCardEndTime = pTable1.GetDateTime(0, 13);
//		msg->RoleInfo.GameID = pTable1.GetUint(0, 14);
//		msg->RoleInfo.IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
//	}
//	else
//	{
//
//		LogInfoToFile(DBErrorSqlFileName, SqlStr);
//		ASSERT(false);
//	}
//	OnAddDBResult(Index, ClientID, msg);
//	return true;
//}
//bool FishServer::OnHandleLoadRoleCheckInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	DBR_Cmd_LoadRoleCheckInfo* pMsg = (DBR_Cmd_LoadRoleCheckInfo*)pCmd;
//	DBO_Cmd_LoadRoleCheckInfo* msg = (DBO_Cmd_LoadRoleCheckInfo*)CreateCmd(DBO_LoadRoleCheckInfo, sizeof(DBO_Cmd_LoadRoleCheckInfo));
//	msg->dwUserID = pMsg->dwUserID;
//	//ִ�д洢����
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRoleCheckInfo('%u');",pMsg->dwUserID);
//	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
//	{
//		if (pTable1.Rows() == 0)
//		{
//			msg->CheckInfo.bCheckSum = 0;
//			msg->CheckInfo.bLastLogDay = 0;
//			msg->CheckInfo.bLastLogMonth = 0;
//			msg->CheckInfo.bLastLogYear = 0;
//			msg->CheckInfo.bMontheCheckSum = 0;
//		}
//		else if (pTable1.Rows() == 1)
//		{
//			msg->CheckInfo.bCheckSum = pTable1.GetByte(0, 0);
//			msg->CheckInfo.bMontheCheckSum = pTable1.GetByte(0, 1);
//			__int64 LogTime = pTable1.GetDateTime(0, 2);
//			tm* pTime = localtime(&LogTime);
//			msg->CheckInfo.bLastLogDay = pTime->tm_mday;
//			msg->CheckInfo.bLastLogMonth = pTime->tm_mon + 1;
//			msg->CheckInfo.bLastLogYear = pTime->tm_year - 100;//1900 ��2000
//		}
//		else
//		{
//			msg->CheckInfo.bCheckSum = 0;
//			msg->CheckInfo.bLastLogDay = 0;
//			msg->CheckInfo.bLastLogMonth = 0;
//			msg->CheckInfo.bLastLogYear = 0;
//			msg->CheckInfo.bMontheCheckSum = 0;
//			ASSERT(false);
//		}
//	}
//	else
//	{
//		msg->CheckInfo.bCheckSum = 0;
//		msg->CheckInfo.bLastLogDay = 0;
//		msg->CheckInfo.bLastLogMonth = 0;
//		msg->CheckInfo.bLastLogYear = 0;
//		msg->CheckInfo.bMontheCheckSum = 0;
//		ASSERT(false);
//	}
//	OnAddDBResult(Index, ClientID, msg);
//	return true;
//}
//bool FishServer::OnHandleUpdateRoleCheckInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	//������ҵ�ǩ������
//	DBR_Cmd_ChangeRoleCheckInfo * pMsg = (DBR_Cmd_ChangeRoleCheckInfo*)pCmd;
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCheckInfo('%u','%u','%u','%04d-%02d-%02d %02d:%02d:%02d');", pMsg->dwUserID, pMsg->CheckInfo.bCheckSum, pMsg->CheckInfo.bMontheCheckSum, 
//		pMsg->CheckInfo.bLastLogYear + 2000, pMsg->CheckInfo.bLastLogMonth, pMsg->CheckInfo.bLastLogDay,0,0,0);
//	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0,0) == 1);
//	if (!Result)
//		ASSERT(false);
//	return true;
//}
bool FishServer::OnHandleLoadRoleTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleTask* pMsg = (DBR_Cmd_LoadRoleTask*)pCmd;
	//DBO_Cmd_LoadRoleTask* msg = (DBO_Cmd_LoadRoleTask)
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadTask('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleTask)+(Row - 1)*sizeof(tagRoleTaskInfo);
		DBO_Cmd_LoadRoleTask* msg = (DBO_Cmd_LoadRoleTask*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleTask);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].TaskID = pTable1.GetByte(i, 0);
			msg->Array[i].TaskValue = pTable1.GetUint(i, 1);
			msg->Array[i].Sign = pTable1.GetByte(i, 2);
		}

		std::vector<DBO_Cmd_LoadRoleTask*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleTask*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRoleTask* msg = (DBO_Cmd_LoadRoleTask*)CreateCmd(DBO_LoadRoleTask, sizeof(DBO_Cmd_LoadRoleTask)-sizeof(tagRoleTaskInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����һ������
	DBR_Cmd_SaveRoleTask* pMsg = (DBR_Cmd_SaveRoleTask*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveTask('%u','%u','%u', '%u');", pMsg->dwUserID, pMsg->TaskInfo.TaskID, pMsg->TaskInfo.TaskValue, pMsg->TaskInfo.Sign);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}

	/*char IpStr[64] = { 0 };
	sprintf_s(IpStr, sizeof(IpStr), "��� %d ���ڱ�������\n", pMsg->dwUserID);
	std::cout << IpStr;*/

	return true;
}
bool FishServer::OnHandleSaveAllTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveAllTask* pMsg = (DBR_Cmd_SaveAllTask*)pCmd;
	DBR_Cmd_SaveRoleTask msg;
	SetMsgInfo(msg, DBR_SaveRoleTask, sizeof(DBR_Cmd_SaveRoleTask));
	for (BYTE i = 0; i < pMsg->Sum; ++i)
	{
		msg.dwUserID = pMsg->dwUserID;
		msg.TaskInfo = pMsg->Array[i];
		OnHandleSaveRoleTask(Index, ClientID, &msg);
	}
	return true;
}
bool FishServer::OnHandleDelRoleTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRoleTask* pMsg = (DBR_Cmd_DelRoleTask*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelTask('%u','%u');", pMsg->dwUserID, pMsg->bTaskID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
//bool FishServer::OnHandleClearRoleTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
//{
//	//����ǰ���ݿ��ȫ�������� ��ǰ��ҵ�
//	DBR_Cmd_ClearRoleTask* pMsg = (DBR_Cmd_ClearRoleTask*)pCmd;
//	SqlTable pTable1;
//	char SqlStr[MAXSQL_LENGTH] = { 0 };
//	sprintf_s(SqlStr, sizeof(SqlStr), "call FishClearRoleTask('%u');", pMsg->dwUserID);
//
//	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
//	if (!Result)
//	{
//		LogInfoToFile(DBErrorSqlFileName, SqlStr);
//		ASSERT(false);
//	}
//	return true;
//}

bool FishServer::OnHandleClearRoleDayTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ǰ���ݿ��ȫ�������� ��ǰ��ҵ�
	DBR_Cmd_ClearRoleDayTask* pMsg = (DBR_Cmd_ClearRoleDayTask*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "delete from fishtask where UserID='%u' and ( Sign = '%u' or Sign = '%u') ", pMsg->dwUserID, pMsg->Sign, pMsg->AcSign);

	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr)) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleClearRoleWeekTask(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ǰ���ݿ��ȫ�������� ��ǰ��ҵ�
	DBR_Cmd_ClearRoleWeekTask* pMsg = (DBR_Cmd_ClearRoleWeekTask*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "delete from fishtask where UserID='%u' and ( Sign = '%u' or Sign = '%u') ", pMsg->dwUserID, pMsg->Sign, pMsg->AcSign);

	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr)) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadRoleAchievement(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleAchievement* pMsg = (DBR_Cmd_LoadRoleAchievement*)pCmd;
	//DBO_Cmd_LoadRoleAchievement* msg = (DBO_Cmd_LoadRoleAchievement)
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadAchievement('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleAchievement)+(Row - 1)*sizeof(tagRoleAchievementInfo);

		DBO_Cmd_LoadRoleAchievement* msg = (DBO_Cmd_LoadRoleAchievement*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleAchievement);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].AchievementID = pTable1.GetByte(i, 0);
			msg->Array[i].AchievementValue = pTable1.GetUint(i, 1);
		}

		std::vector<DBO_Cmd_LoadRoleAchievement*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleAchievement*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadRoleAchievement* msg = (DBO_Cmd_LoadRoleAchievement*)CreateCmd(DBO_LoadRoleAchievement, sizeof(DBO_Cmd_LoadRoleAchievement)-sizeof(tagRoleAchievementInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleAchievement(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����һ������
	DBR_Cmd_SaveRoleAchievement* pMsg = (DBR_Cmd_SaveRoleAchievement*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveAchievement('%u','%u','%u');", pMsg->dwUserID, pMsg->AchievementInfo.AchievementID, pMsg->AchievementInfo.AchievementValue);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveAllAchievement(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveAllAchievement* pMsg = (DBR_Cmd_SaveAllAchievement*)pCmd;
	DBR_Cmd_SaveRoleAchievement msg;
	SetMsgInfo(msg, DBR_SaveRoleAchievement, sizeof(DBR_Cmd_SaveRoleAchievement));
	for (BYTE i = 0; i < pMsg->Sum; ++i)
	{
		msg.dwUserID = pMsg->dwUserID;
		msg.AchievementInfo = pMsg->Array[i];
		OnHandleSaveRoleAchievement(Index, ClientID, &msg);
	}
	return true;
}
bool FishServer::OnHandleDelRoleAchievement(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRoleAchievement* pMsg = (DBR_Cmd_DelRoleAchievement*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelAchievement('%u','%u');", pMsg->dwUserID, pMsg->bAchievementID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadRoleAction(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleAction* pMsg = (DBR_Cmd_LoadRoleAction*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadAction('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleAction)+(Row - 1)*sizeof(tagRoleActionInfo);
		DBO_Cmd_LoadRoleAction* msg = (DBO_Cmd_LoadRoleAction*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleAction);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].ActionID = pTable1.GetByte(i, 0);
			msg->Array[i].ActionValue = pTable1.GetUint(i, 1);
			msg->Array[i].ActionStates = pTable1.GetUint(i, 2);
		}

		std::vector<DBO_Cmd_LoadRoleAction*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleAction*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadRoleAction* msg = (DBO_Cmd_LoadRoleAction*)CreateCmd(DBO_LoadRoleAction, sizeof(DBO_Cmd_LoadRoleAction)-sizeof(tagRoleActionInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleAction(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����һ������
	DBR_Cmd_SaveRoleAction* pMsg = (DBR_Cmd_SaveRoleAction*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveAction('%u','%u','%u','%u');", pMsg->dwUserID, pMsg->ActionInfo.ActionID, pMsg->ActionInfo.ActionValue, pMsg->ActionInfo.ActionStates);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveAllAction(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveAllAction* pMsg = (DBR_Cmd_SaveAllAction*)pCmd;
	DBR_Cmd_SaveRoleAction msg;
	SetMsgInfo(msg, DBR_SaveRoleAction, sizeof(DBR_Cmd_SaveRoleAction));
	for (BYTE i = 0; i < pMsg->Sum; ++i)
	{
		msg.dwUserID = pMsg->dwUserID;
		msg.ActionInfo = pMsg->Array[i];
		OnHandleSaveRoleAction(Index, ClientID, &msg);
	}
	return true;
}
bool FishServer::OnHandleDelRoleAction(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRoleAction* pMsg = (DBR_Cmd_DelRoleAction*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelAction('%u','%u');", pMsg->dwUserID, pMsg->bActionID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadRoleTitle(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�������ȫ���ĳƺ�
	DBR_Cmd_LoadRoleTitle* pMsg = (DBR_Cmd_LoadRoleTitle*)pCmd;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadTitle('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		int Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleTitle)+(Row - 1)*sizeof(BYTE);
		DBO_Cmd_LoadRoleTitle* msg = (DBO_Cmd_LoadRoleTitle*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleTitle);
		msg->dwUserID = pMsg->dwUserID;
		for (int i = 0; i < Row; ++i)
		{
			msg->Array[i] = pTable1.GetByte(i, 0);
		}

		std::vector<DBO_Cmd_LoadRoleTitle*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleTitle*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadRoleTitle* msg = (DBO_Cmd_LoadRoleTitle*)CreateCmd(DBO_LoadRoleTitle, sizeof(DBO_Cmd_LoadRoleTitle)-sizeof(BYTE));
		msg->dwUserID = pMsg->dwUserID;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddRoleTitle(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddRoleTitle* pMsg = (DBR_Cmd_AddRoleTitle*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddTitle('%u','%u');", pMsg->dwUserID, pMsg->TitleID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleDelRoleTitle(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRoleTitle* pMsg = (DBR_Cmd_DelRoleTitle*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelTitle('%u','%u');", pMsg->dwUserID, pMsg->TitleID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��������
	DBR_Cmd_AddRoleEntityItem* pMsg = (DBR_Cmd_AddRoleEntityItem*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* pAddress = WCharToChar(pMsg->Addres, Size);
	char* DestAddress = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestAddress)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestAddress, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pAddress, Size, DestAddress);

	char* pName = WCharToChar(pMsg->Name, Size);
	char* DestName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pName, Size, DestName);

	char* pIDEntity = WCharToChar(pMsg->IDEntity, Size);
	char* DestIDEntity = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestIDEntity)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestIDEntity, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pIDEntity, Size, DestIDEntity);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleEntityItem('%u','%u','%u','%u','%u','%s','%llu','%s', '%d-%d-%d %d:%d:%d','%s');", pMsg->dwUserID, pMsg->ItemID, pMsg->ItemSum, pMsg->UseMedal, pMsg->NowMedalSum, DestAddress, pMsg->Phone, DestName
		, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond, DestIDEntity);
	free(DestAddress);
	free(pAddress);
	free(DestName);
	free(pName);
	free(DestIDEntity);
	free(pIDEntity);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleSaveRoleShopNormalItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��������
	DBR_Cmd_AddRoleShopNormalItem* pMsg = (DBR_Cmd_AddRoleShopNormalItem*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleShopNormalItem('%u','%u','%u','%u','%u', '%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->ItemID, pMsg->ItemSum, pMsg->UseMedal, pMsg->NowMedalSum, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadRoleEntity(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleEntity* pMsg = (DBR_Cmd_LoadRoleEntity*)pCmd;
	DBO_Cmd_LoadRoleEntity* msg = (DBO_Cmd_LoadRoleEntity*)CreateCmd(DBO_LoadRoleEntity, sizeof(DBO_Cmd_LoadRoleEntity));
	msg->dwUserID = pMsg->dwUserID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadUserEntity('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		TCHARCopy(msg->RoleInfo.Name, CountArray(msg->RoleInfo.Name), pTable1.GetStr(0, 0), _tcslen(pTable1.GetStr(0, 0)));
		msg->RoleInfo.Phone = pTable1.GetUint64(0, 1);
		TCHARCopy(msg->RoleInfo.Email, CountArray(msg->RoleInfo.Email), pTable1.GetStr(0, 2), _tcslen(pTable1.GetStr(0, 2)));
		TCHARCopy(msg->RoleInfo.IdentityID, CountArray(msg->RoleInfo.IdentityID), pTable1.GetStr(0, 3), _tcslen(pTable1.GetStr(0, 3)));
		TCHARCopy(msg->RoleInfo.EntityItemUseName, CountArray(msg->RoleInfo.EntityItemUseName), pTable1.GetStr(0, 4), _tcslen(pTable1.GetStr(0, 4)));
		msg->RoleInfo.EntityItemUsePhone = pTable1.GetUint64(0, 5);
		TCHARCopy(msg->RoleInfo.EntityItemUseAddres, CountArray(msg->RoleInfo.EntityItemUseAddres), pTable1.GetStr(0, 6), _tcslen(pTable1.GetStr(0, 6)));

		TCHARCopy(msg->RoleInfo.EmailCode, CountArray(msg->RoleInfo.EmailCode), pTable1.GetStr(0, 7), _tcslen(pTable1.GetStr(0, 7)));
		TCHARCopy(msg->RoleInfo.EntityItemStates, CountArray(msg->RoleInfo.EntityItemStates), pTable1.GetStr(0, 8), _tcslen(pTable1.GetStr(0, 8)));
		TCHARCopy(msg->RoleInfo.EntityItemCity, CountArray(msg->RoleInfo.EntityItemCity), pTable1.GetStr(0, 9), _tcslen(pTable1.GetStr(0, 9)));
		msg->RoleInfo.isMale = pTable1.GetBit(0, 10);
	}
	else
	{
		TCHARCopy(msg->RoleInfo.EntityItemUseName, CountArray(msg->RoleInfo.EntityItemUseName), TEXT(""), 0);
		msg->RoleInfo.Phone = 0;
		TCHARCopy(msg->RoleInfo.EntityItemUseAddres, CountArray(msg->RoleInfo.EntityItemUseAddres), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.Email, CountArray(msg->RoleInfo.Email), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.IdentityID, CountArray(msg->RoleInfo.IdentityID), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.Name, CountArray(msg->RoleInfo.Name), TEXT(""), 0);
		msg->RoleInfo.EntityItemUsePhone = 0;

		TCHARCopy(msg->RoleInfo.EmailCode, CountArray(msg->RoleInfo.EmailCode), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.EntityItemStates, CountArray(msg->RoleInfo.EntityItemStates), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.EntityItemCity, CountArray(msg->RoleInfo.EntityItemCity), TEXT(""), 0);
		TCHARCopy(msg->RoleInfo.EntityItemUseAddres, CountArray(msg->RoleInfo.EntityItemUseAddres), TEXT(""), 0);
		msg->RoleInfo.isMale = 0;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleSaveRoleEntityName(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityName* pMsg = (DBR_Cmd_SaveRoleEntityName*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* pName = WCharToChar(pMsg->Name, Size);
	char* DestName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pName, Size, DestName);
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityName('%u','%s');", pMsg->dwUserID, DestName);
	free(DestName);
	free(pName);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityPhone(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityPhone* pMsg = (DBR_Cmd_SaveRoleEntityPhone*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityPhone('%u','%llu');", pMsg->dwUserID, pMsg->Phone);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityEmail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityEmail* pMsg = (DBR_Cmd_SaveRoleEntityEmail*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;

	char* pEmail = WCharToChar(pMsg->Email, Size);
	char* DestEmail = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestEmail)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestEmail, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEmail, Size, DestEmail);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityEmail('%u','%s');", pMsg->dwUserID, DestEmail);
	free(DestEmail);
	free(pEmail);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityIdentityID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityIdentityID* pMsg = (DBR_Cmd_SaveRoleEntityIdentityID*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;

	char* pIdentityID = WCharToChar(pMsg->IdentityID, Size);
	char* DestIdentityID = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestIdentityID)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestIdentityID, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pIdentityID, Size, DestIdentityID);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityIdentityID('%u','%s');", pMsg->dwUserID, DestIdentityID);
	free(DestIdentityID);
	free(pIdentityID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityItemUseName(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityItemUseName* pMsg = (DBR_Cmd_SaveRoleEntityItemUseName*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;

	char* pEntityItemUseName = WCharToChar(pMsg->EntityItemUseName, Size);
	char* DestEntityItemUseName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestEntityItemUseName)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestEntityItemUseName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemUseName, Size, DestEntityItemUseName);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityItemUseName('%u','%s');", pMsg->dwUserID, DestEntityItemUseName);
	free(DestEntityItemUseName);
	free(pEntityItemUseName);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityItemUsePhone(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityItemUsePhone* pMsg = (DBR_Cmd_SaveRoleEntityItemUsePhone*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityItemUsePhone('%u','%llu');", pMsg->dwUserID, pMsg->EntityItemUsePhone);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntityItemUseAddress(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEntityItemUseAddress* pMsg = (DBR_Cmd_SaveRoleEntityItemUseAddress*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;

	char* pEntityItemUseAddres = WCharToChar(pMsg->EntityItemUseAddres, Size);
	char* DestEntityItemUseAddres = (char*)malloc((Size * 2 + 1) * sizeof(char));
	if (!DestEntityItemUseAddres)
	{
		ASSERT(false);
		return false;
	}
	ZeroMemory(DestEntityItemUseAddres, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemUseAddres, Size, DestEntityItemUseAddres);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntityItemUseAddress('%u','%s');", pMsg->dwUserID, DestEntityItemUseAddres);

	free(DestEntityItemUseAddres);
	free(pEntityItemUseAddres);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadRoleExchangeEntity(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleExchangeEntity* pMsg = (DBR_Cmd_LoadRoleExchangeEntity*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	BYTE Count = PAGE_COUNT;

	sprintf_s(SqlStr, sizeof(SqlStr), "	select ItemID, ItemSum, ShopTime, MedalNum, IsCheck from fishentityitem where UserID = '%u' limit %u,%u", pMsg->dwUserID, /*Count**/pMsg->Page, Count);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleExchangeEntity) + (Row - 1) * sizeof(tagRoleExchangeInfo);
		DBO_Cmd_LoadRoleExchangeEntity* msg = (DBO_Cmd_LoadRoleExchangeEntity*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleExchangeEntity);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].ItemID = pTable1.GetByte(i, 0);
			msg->Array[i].ItemSum = pTable1.GetUint(i, 1);
			msg->Array[i].ShopTime = pTable1.GetDateTime(i, 2);
			msg->Array[i].MedalNum = pTable1.GetUint(i, 3);
			msg->Array[i].bCheck = pTable1.GetBit(i, 4);
		}

		std::vector<DBO_Cmd_LoadRoleExchangeEntity*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleExchangeEntity*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadRoleExchangeEntity* msg = (DBO_Cmd_LoadRoleExchangeEntity*)CreateCmd(DBO_LoadRoleExchangeEntity, sizeof(DBO_Cmd_LoadRoleExchangeEntity) - sizeof(tagRoleExchangeInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadRoleExchangeItem(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRoleExchangeItem* pMsg = (DBR_Cmd_LoadRoleExchangeItem*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	BYTE Count = PAGE_COUNT;

	sprintf_s(SqlStr, sizeof(SqlStr), "	select ItemID, ItemSum, ShopTime, MedalNum, IsCheck from fishshopnormalitem where UserID = '%u' limit %u,%u", pMsg->dwUserID, /*Count**/pMsg->Page, Count);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleExchangeItem) + (Row - 1) * sizeof(tagRoleExchangeInfo);
		DBO_Cmd_LoadRoleExchangeItem* msg = (DBO_Cmd_LoadRoleExchangeItem*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleExchangeItem);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].ItemID = pTable1.GetByte(i, 0);
			msg->Array[i].ItemSum = pTable1.GetUint(i, 1);
			msg->Array[i].ShopTime = pTable1.GetDateTime(i, 2);
			msg->Array[i].MedalNum = pTable1.GetUint(i, 3);
			msg->Array[i].bCheck = pTable1.GetBit(i, 4);
		}

		std::vector<DBO_Cmd_LoadRoleExchangeItem*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleExchangeItem*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);

		DBO_Cmd_LoadRoleExchangeItem* msg = (DBO_Cmd_LoadRoleExchangeItem*)CreateCmd(DBO_LoadRoleExchangeItem, sizeof(DBO_Cmd_LoadRoleExchangeItem) - sizeof(tagRoleExchangeInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveRoleEntity(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//������ҵ�ʵ������
	DBR_Cmd_SaveRoleEntity* pMsg = (DBR_Cmd_SaveRoleEntity*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	UINT Size = 0;
	char* pName		= WCharToChar(pMsg->RoleInfo.Name, Size);
	char* DestName = (char*)malloc((Size * 2  + 1) * sizeof(char));
	ZeroMemory(DestName, (Size * 2  + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pName, Size, DestName);

	//char* pPhone	= WCharToChar(pMsg->RoleInfo.Phone, Size);
	//char* DestPhone = (char*)malloc((Size * 2  + 1) * sizeof(char));
	//ZeroMemory(DestPhone, (Size * 2  + 1) * sizeof(char));
	//m_Sql[Index].GetMySqlEscapeString(pPhone, Size, DestPhone);

	char* pEmail	= WCharToChar(pMsg->RoleInfo.Email, Size);
	char* DestEmail = (char*)malloc((Size * 2  + 1) * sizeof(char));
	ZeroMemory(DestEmail, (Size * 2  + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEmail, Size, DestEmail);

	char* pIdentityID = WCharToChar(pMsg->RoleInfo.IdentityID, Size);
	char* DestIdentityID = (char*)malloc((Size * 2  + 1) * sizeof(char));
	ZeroMemory(DestIdentityID, (Size * 2  + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pIdentityID, Size, DestIdentityID);


	char* pEntityItemUseAddres = WCharToChar(pMsg->RoleInfo.EntityItemUseAddres, Size);
	char* DestEntityItemUseAddres = (char*)malloc((Size * 2 + 1) * sizeof(char));
	ZeroMemory(DestEntityItemUseAddres, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemUseAddres, Size, DestEntityItemUseAddres);

	char* pEntityItemUseName = WCharToChar(pMsg->RoleInfo.EntityItemUseName, Size);
	char* DestEntityItemUseName = (char*)malloc((Size * 2 + 1) * sizeof(char));
	ZeroMemory(DestEntityItemUseName, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemUseName, Size, DestEntityItemUseName);

	//char* pEntityItemUsePhone = WCharToChar(pMsg->RoleInfo.EntityItemUsePhone, Size);
	//char* DestEntityItemUsePhone = (char*)malloc((Size * 2 + 1) * sizeof(char));
	//ZeroMemory(DestEntityItemUsePhone, (Size * 2 + 1) * sizeof(char));
	//m_Sql[Index].GetMySqlEscapeString(pEntityItemUsePhone, Size, DestEntityItemUsePhone);

	char* pEmailCode = WCharToChar(pMsg->RoleInfo.EmailCode, Size);
	char* DestEmailCode = (char*)malloc((Size * 2 + 1) * sizeof(char));
	ZeroMemory(DestEmailCode, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEmailCode, Size, DestEmailCode);

	char* pEntityItemStates = WCharToChar(pMsg->RoleInfo.EntityItemStates, Size);
	char* DestEntityItemStates = (char*)malloc((Size * 2 + 1) * sizeof(char));
	ZeroMemory(DestEntityItemStates, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemStates, Size, DestEntityItemStates);

	char* pEntityItemCity = WCharToChar(pMsg->RoleInfo.EntityItemCity, Size);
	char* DestEntityItemCity = (char*)malloc((Size * 2 + 1) * sizeof(char));
	ZeroMemory(DestEntityItemCity, (Size * 2 + 1) * sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pEntityItemCity, Size, DestEntityItemCity);

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEntity('%u','%s',%d,'%s','%s','%s',%d,'%s');", pMsg->dwUserID, DestName, pMsg->RoleInfo.Phone, DestEmail, DestIdentityID, DestEntityItemUseName, pMsg->RoleInfo.EntityItemUsePhone, DestEntityItemUseAddres);
	free(DestName);
	free(pName);
	//free(DestPhone);
	//free(pPhone);
	free(DestEmail);
	free(pEmail);
	free(DestIdentityID);
	free(pIdentityID);
	free(DestEntityItemUseAddres);
	free(pEntityItemUseAddres);
	free(DestEntityItemUseName);
	free(pEntityItemUseName);
	//free(DestEntityItemUsePhone);
	//free(pEntityItemUsePhone);
	free(pEmailCode);
	free(DestEmailCode);
	free(pEntityItemStates);
	free(DestEntityItemStates);
	free(pEntityItemCity);
	free(DestEntityItemCity);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
		ASSERT(false);
	return true;
}
bool FishServer::OnHandleLoadRoleGiff(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//���� ���͸�ֱ�ӵ����� ���Ҽ��� ��ҽ��췢�������͵����� �����͹���Щ��
	//1.�����Ѿ����յ�������
	DBR_Cmd_LoadRoleGiff* pMsg = (DBR_Cmd_LoadRoleGiff*)pCmd;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadGiff('%u','%u','%u', '%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->LimitDay, m_Config.GetGiffConfig().MaxUserGiffSum
		, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadRoleGiff)+(Row - 1)*sizeof(tagGiffInfo);
		DBO_Cmd_LoadRoleGiff* msg = (DBO_Cmd_LoadRoleGiff*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRoleGiff);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].OnlyID = pTable1.GetUint(i, 0);
			msg->Array[i].dwUserID = pTable1.GetUint(i, 1);
			msg->Array[i].SendTime = pTable1.GetInt64(i, 2);
			TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), pTable1.GetStr(i, 3), _tcslen(pTable1.GetStr(i, 3)));
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 4);
		}

		std::vector<DBO_Cmd_LoadRoleGiff*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRoleGiff*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRoleGiff* msg = (DBO_Cmd_LoadRoleGiff*)CreateCmd(DBO_LoadRoleGiff, sizeof(DBO_Cmd_LoadRoleGiff)-sizeof(tagGiffInfo));
		msg->dwUserID = pMsg->dwUserID;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	////2.������ҽ��췢�͹����б�����
	//ZeroMemory(SqlStr, sizeof(SqlStr));
	//sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadNowDaySendGiff('%u','%u','%u');", pMsg->dwUserID, m_Config.GetGiffConfig().SendSumLimitByDay,m_Config.GetWriteSec());

	//if (m_Sql[Index].Select(SqlStr, 1, pTable2, true))
	//{
	//	int Row = pTable2.Rows();

	//	DWORD PageSize = sizeof(DBO_Cmd_GetNowDayGiff)+(Row - 1)*sizeof(tagNowDaySendGiffInfo);
	//	if (PageSize >= DB_Msg_PageSum)
	//	{
	//		ASSERT(false);
	//		return false;
	//	}

	//	DBO_Cmd_GetNowDayGiff* msg = (DBO_Cmd_GetNowDayGiff*)CreateCmd(DBO_GetNowDayGiff, ConvertDWORDToWORD(PageSize));
	//	msg->dwUserID = pMsg->dwUserID;
	//	msg->GiffSum = ConvertIntToBYTE(Row);
	//	for (int i = 0; i < Row; ++i)
	//	{
	//		msg->GiffArray[i].dwUserID = pTable2.GetUint(i, 0);
	//		msg->GiffArray[i].SendSum = pTable2.GetByte(i, 1);
	//	}
	//	OnAddDBResult(Index, ClientID, msg);
	//}
	//else
	//{
	//	DBO_Cmd_GetNowDayGiff* msg = (DBO_Cmd_GetNowDayGiff*)CreateCmd(DBO_GetNowDayGiff, sizeof(DBO_Cmd_GetNowDayGiff)-sizeof(tagNowDaySendGiffInfo));
	//	msg->dwUserID = pMsg->dwUserID;
	//	msg->GiffSum = 0;
	//	OnAddDBResult(Index, ClientID, msg);
	//	ASSERT(false);
	//}
	return true;
}
bool FishServer::OnHandleLoadNowDayGiff(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_GetNowDayGiff* pMsg = (DBR_Cmd_GetNowDayGiff*)pCmd;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadNowDaySendGiff('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 1, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_GetNowDayGiff)+(Row - 1)*sizeof(tagNowDaySendGiffInfo);
		DBO_Cmd_GetNowDayGiff* msg = (DBO_Cmd_GetNowDayGiff*)malloc(PageSize);
		msg->SetCmdType(DBO_GetNowDayGiff);
		msg->dwUserID = pMsg->dwUserID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].dwUserID = pTable1.GetUint(i, 0);
			msg->Array[i].SendSum = pTable1.GetByte(i, 1);
		}

		std::vector<DBO_Cmd_GetNowDayGiff*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_GetNowDayGiff*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_GetNowDayGiff* msg = (DBO_Cmd_GetNowDayGiff*)CreateCmd(DBO_GetNowDayGiff, sizeof(DBO_Cmd_GetNowDayGiff)-sizeof(tagNowDaySendGiffInfo));
		msg->dwUserID = pMsg->dwUserID;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleClearNowDayGiff(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ClearNowDayGiff* pMsg = (DBR_Cmd_ClearNowDayGiff*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishClearNowDaySendGiff('%u');", pMsg->dwUserID);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddRoleGiff(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����һ�����͸���� ��¼�����ݿ�ȥ
	DBR_Cmd_AddRoleGiff* pMsg = (DBR_Cmd_AddRoleGiff*)pCmd;
	DBO_Cmd_AddRoleGiff* msg = (DBO_Cmd_AddRoleGiff*)CreateCmd(DBO_AddRoleGiff, sizeof(DBO_Cmd_AddRoleGiff));
	msg->Result = false;
	msg->dwDestUserID = pMsg->DestUserID;
	msg->GiffInfo.dwUserID = pMsg->SrcUserID;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddGiff('%u','%u','%u','%u','%u', '%d-%d-%d %d:%d:%d');", pMsg->SrcUserID, pMsg->DestUserID, m_Config.GetGiffConfig().MaxUserGiffSum, m_Config.GetGiffConfig().LimitDay, m_Config.GetGiffConfig().SendOnceUserLimitByDay
		, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)	
	{
		if (pTable1.GetUint(0, 0) == 0)
		{
			msg->Result = false;
			OnAddDBResult(Index, ClientID, msg);
		}
		else
		{
			msg->Result = true;
			msg->GiffInfo.OnlyID = pTable1.GetUint(0, 0);
			msg->GiffInfo.dwUserID = pTable1.GetUint(0, 1);
			msg->GiffInfo.SendTime = pTable1.GetInt64(0, 2);
			TCHARCopy(msg->GiffInfo.NickName, CountArray(msg->GiffInfo.NickName), pTable1.GetStr(0, 3), _tcslen(pTable1.GetStr(0, 3)));
			msg->GiffInfo.dwFaceID = pTable1.GetUint(0, 4);
			OnAddDBResult(Index, ClientID, msg);
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleDelRoleGiff(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//ɾ������
	DBR_Cmd_DelRoleGiff * pMsg = (DBR_Cmd_DelRoleGiff*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelGiff('%u');", pMsg->GiffID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 /*&& pTable1.GetUint(0, 0) == 1*/);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeRoleCharm(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleCharm* pMsg = (DBR_Cmd_SaveRoleCharm*)pCmd;
	DBO_Cmd_SaveRoleCharm * msg = (DBO_Cmd_SaveRoleCharm*)CreateCmd(DBO_SaveRoleCharm, sizeof(DBO_Cmd_SaveRoleCharm));
	msg->dwUserID = pMsg->dwUserID;
	msg->dwDestUserID = pMsg->dwDestUserID;
	msg->bIndex = pMsg->bIndex;
	msg->ItemSum = pMsg->ItemSum;
	msg->Result = false;
	msg->dwDestUserCharmValue = 0;
	//1.�ȶ�ȡ��ҵ���������
	SqlTable pTable1, pTable2;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadUserCharm('%u');", pMsg->dwDestUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD			CharmArray[MAX_CHARM_ITEMSUM] = { 0 };//��ҵ���������
		int				CharmValue = 0;
		CharmValue = pTable1.GetInt(0, 0);
		memcpy_s(&CharmArray, sizeof(CharmArray), pTable1.GetField((UINT)0, (UINT)1), sizeof(CharmArray));
		//���ݻ�ȡ�ɹ���
		CharmArray[pMsg->bIndex] += 1;
		CharmValue += pMsg->AddCharmValue;
		//�����Value�� ���ǽ��м���
		CharmValue = m_Config.GetCharmValue(CharmArray);

		//��������  FishSaveRoleCharm
		ZeroMemory(SqlStr, sizeof(SqlStr));
		char pCharmArray[MAXBLOB_LENGTH] = { 0 };
		m_Sql[Index].GetMySqlEscapeString((char*)&CharmArray, sizeof(CharmArray), pCharmArray);
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleCharm('%u','%d','%s');", pMsg->dwDestUserID, CharmValue, pCharmArray);
		if (m_Sql[Index].Select(SqlStr, 1, pTable2, true) && pTable2.Rows() == 1 && pTable2.GetUint(0, 0) == 1)
		{
			msg->Result = true;
			msg->dwDestUserCharmValue = CharmValue;
			OnAddDBResult(Index, ClientID, msg);
			return true;
		}
		else
		{
			LogInfoToFile(DBErrorSqlFileName, SqlStr);
			OnAddDBResult(Index, ClientID, msg);
			ASSERT(false);
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadRankInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�������а�����
	DBR_Cmd_LoadRankInfo* pMsg = (DBR_Cmd_LoadRankInfo*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishGetRankInfo('%u','%u');", pMsg->RankID, pMsg->RandRowCount);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadRankInfo)+(Row - 1)*sizeof(tagRankInfo);
		DBO_Cmd_LoadRankInfo* msg = (DBO_Cmd_LoadRankInfo*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRankInfo);
		msg->RankID = pMsg->RankID;
		for (DWORD i = 0; i < Row; ++i)
		{
			msg->Array[i].dwUserID = pTable1.GetUint(i, 0);
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 1);
			msg->Array[i].TitleID = pTable1.GetByte(i, 2);
			TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), pTable1.GetStr(i, 3), _tcslen(pTable1.GetStr(i, 3)));
			msg->Array[i].Gender = pTable1.GetBit(i, 4);
			msg->Array[i].Param = pTable1.GetInt64(i, 5);
			msg->Array[i].VipLevel = pTable1.GetByte(i, 6);
		}
		std::vector<DBO_Cmd_LoadRankInfo*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRankInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRankInfo* msg = (DBO_Cmd_LoadRankInfo*)CreateCmd(DBO_LoadRankInfo, sizeof(DBO_Cmd_LoadRankInfo)-sizeof(tagRankInfo));
		msg->RankID = pMsg->RankID;
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadWeekRankInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����ݿ�������а������
	DBR_Cmd_LoadWeekRankInfo* pMsg = (DBR_Cmd_LoadWeekRankInfo*)pCmd;
	//��Ҽ����Լ��������а������
	time_t NowTime = time(NULL) - m_Config.GetWriteSec();//��ȡ��ǰӦ����ʾ��ʱ��
	tm pNowTime;
	errno_t Error = localtime_s(&pNowTime, &NowTime);
	if (Error != 0)
	{
		ASSERT(false);
		return false;
	}
	//��ȡ ���� �������ܵ�����
	BYTE DiffDay = ConvertIntToBYTE((pNowTime.tm_wday >= m_Config.GetRankConfig().RankWeekDay) ? (pNowTime.tm_wday - m_Config.GetRankConfig().RankWeekDay) : (7 - m_Config.GetRankConfig().RankWeekDay + pNowTime.tm_wday));
	time_t WeekTime = NowTime - DiffDay * 24 * 3600;
	tm pWeekTime;
	Error = localtime_s(&pWeekTime, &WeekTime);
	if (Error != 0)
	{
		ASSERT(false);
		return false;
	}
	DWORD VersionID = (pWeekTime.tm_year + 1900) * 10000 + (pWeekTime.tm_mon + 1) * 100 + pWeekTime.tm_mday;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadWeekRankInfo('%u','%u');", VersionID, pMsg->dwUserID);
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		//������ȫ�������� ���ǽ���׼�� ��Լ100��  * (7)
		//����ְ�����
		int Row = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadWeekRankInfo)+(Row - 1)*sizeof(tagRankWeekRankClient);
		DBO_Cmd_LoadWeekRankInfo* msg = (DBO_Cmd_LoadWeekRankInfo*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadWeekRankInfo);
		msg->IsInit = pMsg->IsInit;
		msg->dwUserID = pMsg->dwUserID;
		for (int i = 0; i < Row; ++i)
		{
			//��ȡ����
			msg->Array[i].RankID = pTable1.GetByte(i, 0);
			msg->Array[i].RankIndex = pTable1.GetByte(i, 1);
			msg->Array[i].IsReward = pTable1.GetBit(i, 2) == 1 ? true : false;
		}

		std::vector<DBO_Cmd_LoadWeekRankInfo*> pVec;
		SqlitMsg(msg, PageSize, Row, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadWeekRankInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadWeekRankInfo* msg = (DBO_Cmd_LoadWeekRankInfo*)CreateCmd(DBO_LoadWeekRankInfo, sizeof(DBO_Cmd_LoadWeekRankInfo)-sizeof(tagRankWeekRankClient));
		msg->Sum = 0;
		msg->IsInit = pMsg->IsInit;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleSaveWeekRankInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����һ�����ܵ���������
	DBR_Cmd_SaveWeekRankInfo* pMsg = (DBR_Cmd_SaveWeekRankInfo*)pCmd;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	for (WORD i = 0; i < pMsg->Sum; ++i)
	{
		sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveWeekRankInfo('%u','%u','%u','%u',%u);", pMsg->Array[i].VersionID, pMsg->Array[i].dwUserID, pMsg->Array[i].RankID, pMsg->Array[i].RankIndex, pMsg->Array[i].IsReward);
		SqlTable pTable1;
		bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
		if (!Result)
		{
			LogInfoToFile(DBErrorSqlFileName, SqlStr);
			ASSERT(false);
		}
	}
	return true;
}
bool FishServer::OnHandleChangeWeekInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ChangeWeekInfo* pMsg = (DBR_Cmd_ChangeWeekInfo*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishChangeWeekRankInfo('%u','%u','%u',%u);", pMsg->dwUserID, pMsg->RankInfo.RankID, pMsg->RankInfo.RankIndex, pMsg->RankInfo.IsReward ? 1 : 0);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadRoleGameData(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadGameData* pMsg = (DBR_Cmd_LoadGameData*)pCmd;
	DBO_Cmd_LoadGameData * msg = (DBO_Cmd_LoadGameData*)CreateCmd(DBO_LoadGameData, sizeof(DBO_Cmd_LoadGameData));
	msg->dwUserID = pMsg->dwUserID;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRoleGameData('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		if (pTable1.Rows() == 1)
		{
			msg->GameData.RoleMonthRewardSum = pTable1.GetUint(0, 0);
			msg->GameData.RoleMonthFirstSum = pTable1.GetUint(0, 1);
			msg->GameData.RoleMonthSecondSum = pTable1.GetUint(0, 2);
			msg->GameData.RoleMonthThreeSum = pTable1.GetUint(0, 3);
			msg->GameData.RoleCatchFishTotalSum = pTable1.GetUint(0, 4);
			msg->GameData.RoleGetGlobelSum = pTable1.GetInt64(0, 5);
			msg->GameData.RoleMonthSigupSum = pTable1.GetUint(0, 6);
			msg->GameData.NonMonthGameSec = pTable1.GetUint(0, 7);
			msg->GameData.TotalGameSec = pTable1.GetUint(0, 8);
			//9 18 20 1 3 19
			msg->GameData.CatchFishSum_9 = pTable1.GetUint(0, 9);
			msg->GameData.CatchFishSum_18 = pTable1.GetUint(0, 10);
			msg->GameData.CatchFishSum_20 = pTable1.GetUint(0, 11);
			msg->GameData.CatchFishSum_1 = pTable1.GetUint(0, 12);
			msg->GameData.CatchFishSum_3 = pTable1.GetUint(0, 13);
			msg->GameData.CatchFishSum_19 = pTable1.GetUint(0, 14);

			msg->GameData.MaxCombSum = pTable1.GetUint(0, 15);
			OnAddDBResult(Index, ClientID, msg);
		}
		else
		{
			msg->GameData.RoleMonthRewardSum = 0;
			msg->GameData.RoleMonthFirstSum = 0;
			msg->GameData.RoleMonthSecondSum = 0;
			msg->GameData.RoleMonthThreeSum = 0;
			msg->GameData.RoleCatchFishTotalSum = 0;
			msg->GameData.RoleGetGlobelSum = 0;
			msg->GameData.RoleMonthSigupSum = 0;
			msg->GameData.NonMonthGameSec = 0;
			msg->GameData.TotalGameSec = 0;
			msg->GameData.CatchFishSum_9   =0;
			msg->GameData.CatchFishSum_18  =0;
			msg->GameData.CatchFishSum_20  =0;
			msg->GameData.CatchFishSum_1   =0;
			msg->GameData.CatchFishSum_3   =0;
			msg->GameData.CatchFishSum_19  =0;
			msg->GameData.MaxCombSum = 0;
			OnAddDBResult(Index, ClientID, msg);
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;

}
bool FishServer::OnHandleSaveRoleGameData(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveGameData* pMsg = (DBR_Cmd_SaveGameData*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleGameData('%u','%u','%u','%u','%u','%u','%lld','%u','%u','%u','%u','%u','%u','%u','%u','%u','%u');", pMsg->dwUserID, pMsg->GameData.RoleMonthRewardSum, pMsg->GameData.RoleMonthFirstSum, pMsg->GameData.RoleMonthSecondSum, pMsg->GameData.RoleMonthThreeSum, pMsg->GameData.RoleCatchFishTotalSum, pMsg->GameData.RoleGetGlobelSum
		, pMsg->GameData.RoleMonthSigupSum, pMsg->GameData.NonMonthGameSec, pMsg->GameData.TotalGameSec, pMsg->GameData.CatchFishSum_9, pMsg->GameData.CatchFishSum_18, pMsg->GameData.CatchFishSum_20, pMsg->GameData.CatchFishSum_1, pMsg->GameData.CatchFishSum_3, pMsg->GameData.CatchFishSum_19, pMsg->GameData.MaxCombSum);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeRoleGameDataByMonth(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ChangeGameDataByMonth* pMsg = (DBR_Cmd_ChangeGameDataByMonth*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleGameDataMonthReward('%u','%u');", pMsg->dwUserID, pMsg->MonthIndex);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAchievementPointList(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadAllUserAchievementPointList* pMsg = (DBR_Cmd_LoadAllUserAchievementPointList*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAchievementPointList('%u');", pMsg->RankSum);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadAllUserAchievementPointList)+(pTable1.Rows() - 1)* sizeof(DWORD);
		DBO_Cmd_LoadAllUserAchievementPointList * msg = (DBO_Cmd_LoadAllUserAchievementPointList*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadAllUserAchievementPointList);
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i] = pTable1.GetUint(i, 0);
		}

		std::vector<DBO_Cmd_LoadAllUserAchievementPointList*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadAllUserAchievementPointList*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadAllUserAchievementPointList* msg = (DBO_Cmd_LoadAllUserAchievementPointList*)CreateCmd(DBO_LoadAllUserAchievementPointList, sizeof(DBO_Cmd_LoadAllUserAchievementPointList)-sizeof(DWORD));
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
		return false;
	}
	return true;
}
bool FishServer::OnHandleLoadAllAccountInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ȡ FishLogonCache
	DBR_Cmd_LoadAllAccountInfo* pMsg = (DBR_Cmd_LoadAllAccountInfo*)pCmd;
	//������ҵĹ�ϵ ��2��  �������������ϵ �� ��ҵı�����ϵ
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	string Sql = "call FishLogonCache();";
	strncpy_s(SqlStr, sizeof(SqlStr), Sql.c_str(), Sql.length());

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadAllAccountInfo)+(Rows - 1)*sizeof(AccountCacheInfo);
		//�����Ʒ���� ���Ƿְ�����
		DBO_Cmd_LoadAllAccountInfo * msg = (DBO_Cmd_LoadAllAccountInfo*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return false;
		}
		msg->SetCmdType(DBO_LoadAllAcoountInfo);
		for (DWORD i = 0; i < Rows; ++i)
		{
			TCHARCopy(msg->Array[i].AccountName, CountArray(msg->Array[i].AccountName), pTable1.GetStr(i, 0), _tcslen(pTable1.GetStr(i, 0)));
			msg->Array[i].dwUserID = pTable1.GetUint(i, 1);
			msg->Array[i].PasswordCrc1 = pTable1.GetUint(i, 2);
			msg->Array[i].PasswordCrc2 = pTable1.GetUint(i, 3);
			msg->Array[i].PasswordCrc3 = pTable1.GetUint(i, 4);
			msg->Array[i].IsFreeze = pTable1.GetBit(i, 5) == 1 ? true : false;
			msg->Array[i].FreezeEndTime = pTable1.GetDateTime(i, 6);
			TCHARCopy(msg->Array[i].MacAddress, CountArray(msg->Array[i].MacAddress), pTable1.GetStr(i, 7), _tcslen(pTable1.GetStr(i, 7)));
		}
		std::vector<DBO_Cmd_LoadAllAccountInfo*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadAllAccountInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadAllAccountInfo* msgFinish = (DBO_Cmd_LoadAllAccountInfo*)CreateCmd(DBO_LoadAllAcoountInfo, sizeof(DBO_Cmd_LoadAllAccountInfo)-sizeof(AccountCacheInfo));
		msgFinish->States = (MsgBegin | MsgEnd);
		msgFinish->Sum = 0;
		OnAddDBResult(Index, ClientID, msgFinish);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogRechargeInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogRecharge* pMsg = (DBR_Cmd_LogRecharge*)pCmd;
	//��ֵ��¼
	if (pMsg->RechargeInfo.Sum == 0 || pMsg->RechargeInfo.HandleSum == 0)
	{
		ASSERT(false);
		return false;
	}
	vector<char*> pVec;
	GetStringArrayVecByData(pVec, &pMsg->RechargeInfo);
	if (pVec.size() != pMsg->RechargeInfo.HandleSum)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}
	if (pVec.size() <= 4)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}
	//��ȡ�����ݺ� ���ǽ�����д�뵽���ݿ�ȥ
	//FishAddRechargeLog
	//IN `OrderStates` varchar(256), IN `OrderID` varchar(256), IN `UserID`  int unsigned, `ChannelCode` varchar(256), 
	//IN `ChannelOrderID` varchar(256), IN `ChannelLabel` varchar(256), IN `ShopItemID` int unsigned, 
	//IN `Price` int unsigned, IN `OldGlobelNum` int unsigned, IN `OldCurrceyNum` int unsigned, 
	//IN `AddGlobelNum` int unsigned, IN `AddCurrceyNum` int unsigned, IN `LogTime` datetime

	// char			OrderString
	//char			OrderID
	//char			OrderCode
	//char			ChannelOrderID
	//char			channelLabel

	UINT Size = strlen(pVec[0]);
	char* DestInfo1 = (char*)malloc((2*Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[0], Size, DestInfo1);

	Size = strlen(pVec[1]);
	char* DestInfo2 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[1], Size, DestInfo2);

	Size = strlen(pVec[2]);
	char* DestInfo3 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[2], Size, DestInfo3);

	Size = strlen(pVec[3]);
	char* DestInfo4 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[3], Size, DestInfo4);

	Size = strlen(pVec[4]);
	char* DestInfo5 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[4], Size, DestInfo5);


	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRechargeLog('%s','%s','%u','%s','%s','%s','%u','%u','%u','%u','%u','%u','%u','%d-%d-%d %d:%d:%d','%u');", 
		DestInfo1, DestInfo2, pMsg->UserID, DestInfo3, DestInfo4, DestInfo5, pMsg->ShopItemID, pMsg->Price,pMsg->FreePrice, pMsg->OldGlobelNum, pMsg->OldCurrceyNum, pMsg->AddGlobel, pMsg->AddCurrcey,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond,pMsg->AddRewardID);
	free(DestInfo1);
	free(DestInfo2);
	free(DestInfo3);
	free(DestInfo4);
	free(DestInfo5);
	FreeVec(pVec);
	//��Ҫ��SQL������ת�봦��
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogRoleOnlineInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����LOG��¼
	DBR_Cmd_LogOnlineInfo* pMsg = (DBR_Cmd_LogOnlineInfo*)pCmd;
	//ִ�д洢����
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	//������ҵ�½������
	//������� �������ߵ�����
	//FishAddRoleOnlineLog
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleOnlineLog('%u',%d,'%u','%u','%u','%s','%s','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->IsOnline ? 1 : 0,pMsg->GlobelSum,pMsg->MadelSum,pMsg->CurrceySum,pMsg->MacAddress,pMsg->IpAddress, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogPhonePayInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogPhonePay* pMsg = (DBR_Cmd_LogPhonePay*)pCmd;

	if (pMsg->PhonePayInfo.Sum == 0 || pMsg->PhonePayInfo.HandleSum == 0)
	{
		ASSERT(false);
		return false;
	}
	vector<char*> pVec;
	GetStringArrayVecByData(pVec, &pMsg->PhonePayInfo);
	if (pVec.size() != pMsg->PhonePayInfo.HandleSum)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}
	if (pVec.size() < 1)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);


	UINT Size = strlen(pVec[0]);
	char* DestInfo1 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[0], Size, DestInfo1);

	//IN `OrderID` bigint unsigned,IN `FacePrice` int unsigned,IN `UserID` int unsigned,IN `PhoneNumber` bigint unsigned,IN `OrderStates` varchar(256),IN `LogTime` datetime

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddPhonePayLog('%llu','%u','%u','%llu','%s','%d-%d-%d %d:%d:%d');", 
		pMsg->OrderID, pMsg->FacePrice, pMsg->UserID, pMsg->PhoneNumber, DestInfo1,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	free(DestInfo1);
	FreeVec(pVec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogRoleEntityItemInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��¼���ʵ����Ʒ������
	DBR_Cmd_LogEntityItemInfo* pMsg = (DBR_Cmd_LogEntityItemInfo*)pCmd;

	if (pMsg->EntityInfo.Sum == 0 || pMsg->EntityInfo.HandleSum == 0)
	{
		ASSERT(false);
		return false;
	}
	vector<char*> pVec;
	GetStringArrayVecByData(pVec, &pMsg->EntityInfo);
	if (pVec.size() != pMsg->EntityInfo.HandleSum)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}
	if (pVec.size() < 5)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	//IN `ID` int unsigned, IN `OrderStates` varchar(256), IN `ShopID` int unsigned, 
	//IN `ShopItemID` int unsigned, IN `ShopItemSum` int unsigned, 
	//IN `ShopLogTime` datetime, IN `HandleTime` datetime, IN `UserID` int unsigned, 
	//IN `Address` varchar(256), IN `Phone` bigint unsigned, IN `IDEntity` varchar(256), IN `Name` varchar(256)

	//char				OrderStates
	//char				Address
	//char				IDEntity
	//char				Name

	UINT Size = strlen(pVec[0]);
	char* DestInfo1 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[0], Size, DestInfo1);

	Size = strlen(pVec[1]);
	char* DestInfo2 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[1], Size, DestInfo2);

	Size = strlen(pVec[2]);
	char* DestInfo3 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[2], Size, DestInfo3);

	Size = strlen(pVec[3]);
	char* DestInfo4 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[3], Size, DestInfo4);

	Size = strlen(pVec[4]);
	char* DestInfo5 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pVec[4], Size, DestInfo5);


	tm pLogTime;
	errno_t Error = localtime_s(&pLogTime, &pMsg->ShopLogTime);
	if (Error != 0)
	{
		FreeVec(pVec);
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddEntityItemLog('%u','%s','%u','%u','%d-%d-%d %d:%d:%d','%d-%d-%d %d:%d:%d','%u','%s','%llu','%s','%s','%s','%u','%u','%d.%d.%d.%d');",
		pMsg->ID, DestInfo1, pMsg->ItemID, pMsg->ItemSum,
		pLogTime.tm_year + 1900, pLogTime.tm_mon + 1, pLogTime.tm_mday, pLogTime.tm_hour, pLogTime.tm_min,pLogTime.tm_sec,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond,
		pMsg->UserID, DestInfo2, pMsg->Phone, DestInfo3, DestInfo4, DestInfo5, pMsg->Medal, pMsg->NowMedal, pMsg->HandleIP & 0xff, (pMsg->HandleIP >> 8) & 0xff, (pMsg->HandleIP >> 16) & 0xff, (pMsg->HandleIP >> 24) & 0xff);
	free(DestInfo1);
	free(DestInfo2);
	free(DestInfo3);
	free(DestInfo4);
	free(DestInfo5);
	FreeVec(pVec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);

	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleCreateLogTable(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����µ�Log��
	DBR_Cmd_CreateLogTable* pMsg = (DBR_Cmd_CreateLogTable*)pCmd;
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishCreateLogTable('%u','%u','%u');",pMsg->Year,pMsg->Month,pMsg->Day);
	m_Sql[Index].Select(SqlStr, 0, pTable1, true);
	return true;
}
bool FishServer::OnHandleLogNiuNiuTableInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��¼����
	//��¼����
	DBR_Cmd_LogNiuNiuTableInfo * pMsg = (DBR_Cmd_LogNiuNiuTableInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	string   BrandStr[MAX_NIUNIU_ClientSum + 1];
	//ƴ���ַ���
	for (BYTE i = 0; i < MAX_NIUNIU_ClientSum + 1; ++i)
	{
		BrandStr[i] = "";
		for (BYTE j = 0; j < MAX_NIUNIU_BrandSum; ++j)
		{
			BYTE BrandValue = pMsg->BrandValue[i][j];
			BYTE Value = ((BrandValue - 1) / 4) + 1;
			BYTE Type = ((BrandValue - 1) % 4);

			char ValueStrArray[10] = { 0 };
			errno_t pError = _itoa_s(Value, ValueStrArray, 10, 10);

			if (pError != 0)
			{
				ASSERT(false);
				continue;
			}

			string TypeStr = (Type == 0 ? "����" : (Type == 1 ? "÷��" : (Type == 2 ? "����" : (Type == 3 ? "����" : "δ֪"))));
			string ValueStr = (Value <= 10 ? ValueStrArray : (Value == 11 ? "J" : (Value == 12 ? "Q" : (Value == 13 ? "K" : "δ֪"))));

			if (j == MAX_NIUNIU_BrandSum - 1)
				BrandStr[i] += (TypeStr + ValueStr);
			else
				BrandStr[i] += (TypeStr + ValueStr + ",");
		}
	}
	//�ַ����������.
	UINT Size = strlen(BrandStr[0].c_str());
	char* DestInfo1 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)BrandStr[0].c_str(), Size, DestInfo1);

	Size = strlen(BrandStr[1].c_str());
	char* DestInfo2 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)BrandStr[1].c_str(), Size, DestInfo2);

	Size = strlen(BrandStr[2].c_str());
	char* DestInfo3 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)BrandStr[2].c_str(), Size, DestInfo3);

	Size = strlen(BrandStr[3].c_str());
	char* DestInfo4 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)BrandStr[3].c_str(), Size, DestInfo4);

	Size = strlen(BrandStr[4].c_str());
	char* DestInfo5 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)BrandStr[4].c_str(), Size, DestInfo5);

	tm pNowTime;
	errno_t pError= localtime_s(&pNowTime, &pMsg->NowTime);
	if (pError != 0)
	{	
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLogNiuNiuTableInfo('%s','%s','%s','%s','%s','%u','%lld','%lld','%d-%d-%d %d:%d:%d');",
		DestInfo1, DestInfo2, DestInfo3, DestInfo4, DestInfo5, pMsg->UserSum, pMsg->BrandGlobelSum, pMsg->SystemGlobelSum,
		pNowTime.tm_year+1900, pNowTime.tm_mon+1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);

	free(DestInfo1);
	free(DestInfo2);
	free(DestInfo3);
	free(DestInfo4);
	free(DestInfo5);

	m_Sql[Index].Select(SqlStr, 0, pTable1, true);

	return true;
}
bool FishServer::OnHandleLogExChangeInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��¼���ʹ�öһ����״̬
	DBR_Cmd_LogExChangeInfo* pMsg = (DBR_Cmd_LogExChangeInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	time_t pNow = time(NULL);
	tm pNowTime;
	errno_t pError = localtime_s(&pNowTime, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}

	UINT Count;
	char* pExChangeCode = WCharToChar(pMsg->ExChangeCode, Count);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLogExChangeInfo('%u','%u','%s','%d-%d-%d %d:%d:%d');",
		pMsg->dwUserID, pMsg->ExChangeTypeID, pExChangeCode,
		pNowTime.tm_year + 1900, pNowTime.tm_mon + 1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);

	free(pExChangeCode);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogLotteryInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogLotteryInfo* pMsg = (DBR_Cmd_LogLotteryInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	time_t pNow = time(NULL);
	tm pNowTime;
	errno_t pError = localtime_s(&pNowTime, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLotteryInfo('%u','%u','%u','%d-%d-%d %d:%d:%d');",pMsg->dwUserID, pMsg->LotteryID,pMsg->RewardID,pNowTime.tm_year + 1900, pNowTime.tm_mon + 1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogMonthInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��¼��������
	DBR_Cmd_LogMonthInfo* pMsg = (DBR_Cmd_LogMonthInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	time_t pNow = time(NULL);
	tm pNowTime;
	errno_t pError = localtime_s(&pNowTime, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLogMonthInfo('%u','%u','%u','%u','%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, 
		pMsg->MonthID,pMsg->MonthIndex,pMsg->MonthScore,pMsg->SkillSum,pMsg->AddGlobelSum,pMsg->RewardID,
		pNowTime.tm_year + 1900, pNowTime.tm_mon + 1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);
	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogDialInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��¼ɭ����������
	DBR_Cmd_LogDialInfo* pMsg = (DBR_Cmd_LogDialInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	time_t pNow = time(NULL);
	tm pNowTime;
	errno_t pError = localtime_s(&pNowTime, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}
	//��¼����  
	string AreaDate = ""; 
	for (BYTE i = 0; i < MAX_DIAL_GameSum; ++i)
	{
		char Dest[8] = { 0 };
		errno_t Error = _itoa_s(pMsg->AreaData[i], Dest, CountArray(Dest), 10);
		if (Error != 0)
		{
			ASSERT(false);
			return false;
		}
		AreaDate += Dest;
		if (i != MAX_DIAL_GameSum - 1)
			AreaDate += ",";
	}
	
	string AreaGlobel = "";
	for (BYTE i = 0; i < MAX_DIAL_ClientSum; ++i)
	{
		char Dest[32] = { 0 };
		errno_t Error = _ui64toa_s(pMsg->AreaGlobel[i], Dest, CountArray(Dest), 10);
		if (Error != 0)
		{
			ASSERT(false);
			return false;
		}
		AreaGlobel += Dest;
		if (i != MAX_DIAL_ClientSum - 1)
			AreaGlobel += ",";
	}

	UINT Size = strlen(AreaDate.c_str());
	char* DestInfo1 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)AreaDate.c_str(), Size, DestInfo1);

	Size = strlen(AreaGlobel.c_str());
	char* DestInfo2 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)AreaGlobel.c_str(), Size, DestInfo2);

	//�ַ���׼����� ��¼�����ݿ�ȥ
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLogDialTableInfo('%u','%s','%s','%u','%lld','%lld','%u','%d-%d-%d %d:%d:%d');",
		pMsg->BanderUserID, DestInfo1, DestInfo2, pMsg->ResultIndex, pMsg->BrandGlobel, pMsg->SystemGlobel, pMsg->RoleSum,
		pNowTime.tm_year + 1900, pNowTime.tm_mon + 1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);

	free(DestInfo1);
	free(DestInfo2);

	m_Sql[Index].Select(SqlStr, 0, pTable1, true);

	return true;
}

bool FishServer::OnHandleLogStockScore(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogStockScore* pMsg = (DBR_Cmd_LogStockScore*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "update fishstocklog set fishstocklog.StockScore = %lld,fishstocklog.TaxScore = %lld where fishstocklog.TableType = %u and fishstocklog.ServerID=%u",pMsg->StockScore, pMsg->TaxScore, pMsg->TableType,pMsg->ServerID);

	//sprintf_s(SqlStr, sizeof(SqlStr), "update fishstocklog set StockScore='%d' where TableType='%d' and ServerID='%d';",pMsg->StockScore,pMsg->TableType,pMsg->ServerID);
	bool Result = (m_Sql[Index].RealExcute(SqlStr, strlen(SqlStr) /*&& pTable1.Rows() == 1*/ /*&& pTable1.GetUint(0, 0) == 1*/));
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLogCarInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogCarInfo* pMsg = (DBR_Cmd_LogCarInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	time_t pNow = time(NULL);
	tm pNowTime;
	errno_t pError = localtime_s(&pNowTime, &pNow);
	if (pError != 0)
	{
		ASSERT(false);
		return false;
	}
	//��¼����  
	string AreaGlobel = "";
	for (BYTE i = 0; i < MAX_CAR_ClientSum; ++i)
	{
		char Dest[32] = { 0 };
		errno_t Error = _ui64toa_s(pMsg->AreaGlobel[i], Dest, CountArray(Dest), 10);
		if (Error != 0)
		{
			ASSERT(false);
			return false;
		}
		AreaGlobel += Dest;
		if (i != MAX_DIAL_ClientSum - 1)
			AreaGlobel += ",";
	}

	UINT Size = strlen(AreaGlobel.c_str());
	char* DestInfo2 = (char*)malloc((2 * Size + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString((char*)AreaGlobel.c_str(), Size, DestInfo2);

	//�ַ���׼����� ��¼�����ݿ�ȥ
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLogCarTableInfo('%u','%s','%u','%lld','%lld','%u','%d-%d-%d %d:%d:%d');",
		pMsg->BanderUserID, DestInfo2, pMsg->ResultIndex, pMsg->BrandGlobel, pMsg->SystemGlobel, pMsg->RoleSum,
		pNowTime.tm_year + 1900, pNowTime.tm_mon + 1, pNowTime.tm_mday, pNowTime.tm_hour, pNowTime.tm_min, pNowTime.tm_sec);

	free(DestInfo2);

	m_Sql[Index].Select(SqlStr, 0, pTable1, true);

	return true;
}
bool FishServer::OnHandleLogRoleTableInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LogRoleTableInfo* pMsg = (DBR_Cmd_LogRoleTableInfo*)pCmd;
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRoleTableLog('%u','%u','%u','%u','%u','%u',%d,'%d-%d-%d %d:%d:%d');",
		pMsg->dwUserID,pMsg->TableID,pMsg->TableMonthID,pMsg->GlobelSum,pMsg->MedalSum,pMsg->CurrceySum,pMsg->JionOrLeave?1:0,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnAddRecharge(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��Ҳ����� ���в��� ���ǽ��м�¼��
	DBR_Cmd_AddRecharge* pMsg = (DBR_Cmd_AddRecharge*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	vector<StringArrayData*> pVec;
	GetStringArrayVecByData(pVec, &pMsg->rechargeInfo);
	if (pVec.size() != pMsg->rechargeInfo.HandleSum)
	{
		//FreeVec(pVec);
		ASSERT(false);
		return false;
	}

	UINT Size = 0;
	char* Info1 = WCharToChar(pVec[0]->Array, pVec[0]->Length / sizeof(TCHAR), Size);
	char* DestInfo1 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info1, Size, DestInfo1);

	char* Info2 = WCharToChar(pVec[1]->Array, pVec[1]->Length / sizeof(TCHAR), Size);
	char* DestInfo2 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info2, Size, DestInfo2);

	char* Info3 = WCharToChar(pVec[2]->Array, pVec[2]->Length / sizeof(TCHAR), Size);
	char* DestInfo3 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info3, Size, DestInfo3);

	char* Info4 = WCharToChar(pVec[3]->Array, pVec[3]->Length / sizeof(TCHAR), Size);
	char* DestInfo4 = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info4, Size, DestInfo4);

	/*vector<TCHAR*>::iterator Iter = pVec.begin();
	for (; Iter != pVec.end(); ++Iter)
	{
	free(*Iter);
	}
	pVec.clear();*/

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRecharge('%u','%u',%d,'%s','%s','%s','%s','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->rechargeInfo.UserID, pMsg->rechargeInfo.Price, pMsg->IsFirst ? 1 : 0, DestInfo1, DestInfo2,
		DestInfo3, DestInfo4, pMsg->AddGlobelSum, pMsg->AddCurrceySum,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	free(Info1);
	free(DestInfo1);

	free(Info2);
	free(DestInfo2);

	free(Info3);
	free(DestInfo3);

	free(Info4);
	free(DestInfo4);

	//FreeVec(pVec);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnLoadPhonePayOrderList(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadPhonePayOrderID* pMsg = (DBR_Cmd_LoadPhonePayOrderID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	//
	BYTE DiffDay = pMsg->DiffDay;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadPhonePayOrderId(%d,'%d-%d-%d %d:%d:%d');", pMsg->DiffDay, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadPhonePayOrderID)+(Rows - 1)*sizeof(unsigned __int64);
		//�����Ʒ���� ���Ƿְ�����
		DBO_Cmd_LoadPhonePayOrderID * msg = (DBO_Cmd_LoadPhonePayOrderID*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return false;
		}
		msg->SetCmdType(DBO_LoadPhonePayOrderID);
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i] = pTable1.GetUint64(i, 0);
		}
		std::vector<DBO_Cmd_LoadPhonePayOrderID*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadPhonePayOrderID*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadPhonePayOrderID* msgFinish = (DBO_Cmd_LoadPhonePayOrderID*)CreateCmd(DBO_LoadPhonePayOrderID, sizeof(DBO_Cmd_LoadPhonePayOrderID)-sizeof(__int64));
		msgFinish->States = (MsgBegin | MsgEnd);
		msgFinish->Sum = 0;
		OnAddDBResult(Index, ClientID, msgFinish);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnLoadRechargeOrderID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ȫ���Ķ����� ���ڴ���ȥ
	DBR_Cmd_LoadRechageOrderID* pMsg = (DBR_Cmd_LoadRechageOrderID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	//
	BYTE DiffDay = pMsg->DiffDay;

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRechargeOrderId(%d,'%d-%d-%d %d:%d:%d');", pMsg->DiffDay, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();

		DWORD PageSize = sizeof(DBO_Cmd_LoadRechageOrderID)+(Rows - 1)*sizeof(OrderSetInfo);
		//�����Ʒ���� ���Ƿְ�����
		DBO_Cmd_LoadRechageOrderID * msg = (DBO_Cmd_LoadRechageOrderID*)malloc(PageSize);
		if (!msg)
		{
			ASSERT(false);
			return false;
		}
		msg->SetCmdType(DBO_LoadRechageOrderID);
		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].OrderID = pTable1.GetUint64(i, 0);
			msg->Array[i].OrderType = pTable1.GetByte(i, 1);
		}
		std::vector<DBO_Cmd_LoadRechageOrderID*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRechageOrderID*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRechageOrderID* msgFinish = (DBO_Cmd_LoadRechageOrderID*)CreateCmd(DBO_LoadRechageOrderID, sizeof(DBO_Cmd_LoadRechageOrderID)-sizeof(OrderSetInfo));
		msgFinish->States = (MsgBegin | MsgEnd);
		msgFinish->Sum = 0;
		OnAddDBResult(Index, ClientID, msgFinish);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnAddRechargeOrderID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�´�����һ������  ��¼�����ݿ�ȥ
	DBR_Cmd_AddRechageOrderID* pMsg = (DBR_Cmd_AddRechageOrderID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRechargeOrderID('%u','%llu','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->OrderOnlyID.OrderID,pMsg->OrderOnlyID.OrderType, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnAddPhonePayOrderID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddPhonePayOrderID* pMsg = (DBR_Cmd_AddPhonePayOrderID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddPhonePayOrderID('%u','%llu','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, pMsg->OrderOnlyID, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnGetRechargeOrderID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ȡ������ΨһID
	DBR_Cmd_GetRechargeOrderID* pMsg = (DBR_Cmd_GetRechargeOrderID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishCreateRechargeOrderID('%u','%u');", pMsg->dwUserID,pMsg->ShopID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	DBO_Cmd_GetRechargeOrderID* msg = (DBO_Cmd_GetRechargeOrderID*)CreateCmd(DBO_GetRechargeOrderID, sizeof(DBO_Cmd_GetRechargeOrderID));

	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);

		msg->dwUserID = pMsg->dwUserID;
		msg->OrderID = 0;
		msg->ShopID = pMsg->ShopID;
		msg->IsPC = pMsg->IsPC;
	}
	else
	{
		msg->dwUserID = pMsg->dwUserID;
		msg->OrderID = pTable1.GetUint(0, 0);
		msg->ShopID = pMsg->ShopID;
		msg->IsPC = pMsg->IsPC;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnQueryRechargeOrderInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_QueryRechargeOrderInfo* pMsg = (DBR_Cmd_QueryRechargeOrderInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryRechargeOrderInfo('%u');", pMsg->OrderID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	DBO_Cmd_QueryRechargeOrderInfo* msg = (DBO_Cmd_QueryRechargeOrderInfo*)CreateCmd(DBO_QueryRechargeOrderInfo, sizeof(DBO_Cmd_QueryRechargeOrderInfo));

	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);

		msg->dwUserID = 0;
		msg->OrderID = pMsg->OrderID;
		msg->ShopID = 0;
		msg->CheckShopID = pMsg->CheckShopID;
		msg->CheckUserID = pMsg->CheckUserID;
		msg->Price = pMsg->Price;
	}
	else
	{
		msg->OrderID = pTable1.GetUint(0, 0);
		msg->dwUserID = pTable1.GetUint(0, 1);
		msg->ShopID = pTable1.GetUint(0, 2);
		msg->CheckShopID = pMsg->CheckShopID;
		msg->CheckUserID = pMsg->CheckUserID;
		msg->Price = pMsg->Price;
	}
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnDleRechargeOrderInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelRechargeOrderInfo* pMsg = (DBR_Cmd_DelRechargeOrderInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelRechargeOrderInfo('%u');", pMsg->OrderID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	return true;
}
bool FishServer::OnCheckPhone(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ֻ��Ƿ����ʹ��
	DBR_Cmd_CheckPhone* pMsg = (DBR_Cmd_CheckPhone*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishCheckPhone('%llu');", pMsg->PhoneNumber);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0, 0) == 0);

	DBO_Cmd_CheckPhone* msg = (DBO_Cmd_CheckPhone*)CreateCmd(DBO_CheckPhone, sizeof(DBO_Cmd_CheckPhone));
	msg->dwUserID = pMsg->dwUserID;
	msg->Result = Result;
	msg->PhoneNumber = pMsg->PhoneNumber;
	msg->BindNumber = pMsg->BindNumber;
	msg->ClientID = pMsg->ClientID;
	msg->IsBindOrLogon = pMsg->IsBindOrLogon;
	msg->Zone = pMsg->Zone;
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnCheckEntityID(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_CheckEntityID* pMsg = (DBR_Cmd_CheckEntityID*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishCheckEntityID('%s');", pMsg->EntityID);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1 && pTable1.GetUint(0, 0) == 0);
	DBO_Cmd_CheckEntityID* msg = (DBO_Cmd_CheckEntityID*)CreateCmd(DBO_CheckEntityID, sizeof(DBO_Cmd_CheckEntityID));
	msg->dwUserID = pMsg->dwUserID;
	msg->EntityCrcID = pMsg->EntityCrcID;
	msg->Result = Result;
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleLogInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����������Log���ݼ�¼����  FishLogInfo
	DBR_Cmd_LogInfo* pMsg = (DBR_Cmd_LogInfo*)pCmd;

	UINT Size = 0;
	char* Info = WCharToChar(pMsg->Info, Size);
	char* DestInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info, Size, DestInfo);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddLogInfo('%u','%u','%d','%u','%u','%s','%d-%d-%d %d:%d:%d');", pMsg->UserID, pMsg->Type, pMsg->TypeSum, pMsg->Param,pMsg->EndParam, DestInfo, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	free(Info);
	free(DestInfo);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
//Announcement
bool FishServer::OnHandleLoadAllAnnouncementInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//����ȫ���Ĺ���
	//�ְ�����
	//ִ���µĴ洢����  ������ϵ������ �޷��������� ������Ҫ���зְ����� ���ƺ�����
	DBR_Cmd_LoadAllAnnouncement* pMsg = (DBR_Cmd_LoadAllAnnouncement*)pCmd;

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadAllAnnouncementInfo('%u');", m_Config.GetSystemConfig().AnnouncementSum);

	if (m_Sql[Index].Select(SqlStr, 1, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadALlAnnouncement)+(Rows - 1)*sizeof(AnnouncementOnce);
		DBO_Cmd_LoadALlAnnouncement* msgBe = (DBO_Cmd_LoadALlAnnouncement*)malloc(PageSize);
		if (!msgBe)
		{
			ASSERT(false);
			return false;
		}
		msgBe->SetCmdType(DBO_LoadALlAnnouncement);
		for (DWORD i = 0; i < Rows; ++i)
		{
			TCHARCopy(msgBe->Array[i].NickName, CountArray(msgBe->Array[i].NickName), pTable1.GetStr(i, 0), _tcslen(pTable1.GetStr(i, 0)));
			msgBe->Array[i].ShopID = pTable1.GetByte(i, 1);
			msgBe->Array[i].ShopOnlyID = pTable1.GetByte(i, 2);
		}
		std::vector<DBO_Cmd_LoadALlAnnouncement*> pVec;
		SqlitMsg(msgBe, PageSize, Rows, false, pVec);
		free(msgBe);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadALlAnnouncement*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{

		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadALlAnnouncement* msgBeFinish = (DBO_Cmd_LoadALlAnnouncement*)CreateCmd(DBO_LoadALlAnnouncement, sizeof(DBO_Cmd_LoadALlAnnouncement)-sizeof(AnnouncementOnce));
		msgBeFinish->Sum = 0;
		msgBeFinish->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msgBeFinish);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddAnnouncementInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddAnnouncement* pMsg = (DBR_Cmd_AddAnnouncement*)pCmd;

	UINT Size = 0;
	char* Info = WCharToChar(pMsg->AnnouncementInfo.NickName, Size);
	char* DestInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info, Size, DestInfo);

	SYSTEMTIME pTime;
	GetLocalTime(&pTime);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddAnnouncementInfo('%s','%u','%u','%d-%d-%d %d:%d:%d');", DestInfo, pMsg->AnnouncementInfo.ShopID, pMsg->AnnouncementInfo.ShopOnlyID, pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	free(Info);
	free(DestInfo);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleChangeRoleEmail(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_SaveRoleEmail* pMsg = (DBR_Cmd_SaveRoleEmail*)pCmd;

	UINT Size = 0;
	char* Info = WCharToChar(pMsg->Email, Size);
	char* DestInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info, Size, DestInfo);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishSaveRoleEmail('%u','%s');", pMsg->dwUserID, DestInfo);
	free(Info);
	free(DestInfo);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleClearFishDB(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�����������������ʱ�� ���������ݿ�
	DBR_Cmd_ClearFishDB* pMsg = (DBR_Cmd_ClearFishDB*)pCmd;
	SYSTEMTIME pTime;
	GetLocalTime(&pTime);
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishServerInit(%d,'%u','%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->IsInit ? 1 : 0,
		m_Config.GetSystemConfig().AnnouncementSum, m_Config.GetGiffConfig().LimitDay, m_Config.GetFishMailConfig().MailLimitDay,m_Config.GetRelation().RelationRequestLimitDay,
		pTime.wYear, pTime.wMonth, pTime.wDay, pTime.wHour, pTime.wMinute, pTime.wSecond);
	bool Result = m_Sql[Index].Select(SqlStr, 0, pTable1, true);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	DBO_Cmd_ClearFishDB* msg = (DBO_Cmd_ClearFishDB*)CreateCmd(DBO_ClearFishDB, sizeof(DBO_Cmd_ClearFishDB));
	OnAddDBResult(Index, ClientID, msg);
	return true;
}
bool FishServer::OnHandleQueryExChange(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//��ѯһ���һ����Ƿ���� �Ƿ����ʹ��
	DBR_Cmd_QueryExChange* pMsg = (DBR_Cmd_QueryExChange*)pCmd;
	DBO_Cmd_QueryExChange* msg = (DBO_Cmd_QueryExChange*)CreateCmd(DBO_QueryExChange, sizeof(DBO_Cmd_QueryExChange));
	msg->dwUserID = pMsg->dwUserID;
	TCHARCopy(msg->ExChangeCode, CountArray(msg->ExChangeCode), pMsg->ExChangeCode, _tcslen(pMsg->ExChangeCode));
	msg->RewardID = 0;
	msg->dwUseUserID = 0;

	UINT Size = 0;
	char* Info = WCharToChar(pMsg->ExChangeCode, Size);
	char* DestInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info, Size, DestInfo);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishQueryExChangeCode('%s','%d');", DestInfo,pMsg->dwUserID);
	free(Info);
	free(DestInfo);

	if (m_Sql[Index].Select(SqlStr, 1, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		if (Rows == 1)
		{
			msg->RewardID = pTable1.GetUint(0, 0);
			msg->dwUseUserID = pTable1.GetUint(0, 1);
			OnAddDBResult(Index, ClientID, msg);
		}
		else
		{
			msg->RewardID = 0;
			msg->dwUseUserID = 0;
			OnAddDBResult(Index, ClientID, msg);
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
		OnAddDBResult(Index, ClientID, msg);
	}
	return true;
}
bool FishServer::OnDelExChange(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//�Ƴ�һ���Ѿ�ʹ���˵Ķһ���
	DBR_Cmd_DelExChange* pMsg = (DBR_Cmd_DelExChange*)pCmd;

	UINT Size = 0;
	char* Info = WCharToChar(pMsg->ExChangeCode, Size);
	char* DestInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(Info, Size, DestInfo);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelExChangeCode('%s','%d');", DestInfo,pMsg->dwUserID);
	free(Info);
	free(DestInfo);

	bool Result = (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
void FishServer::UpdateInfoToControl(DWORD dwTimer)
{
	static DWORD LogUpdateCenterTime = 0;
	if (LogUpdateCenterTime == 0 || dwTimer - LogUpdateCenterTime >= 10000)
	{
		LogUpdateCenterTime = dwTimer;
		LC_Cmd_DBInfo msg;
		SetMsgInfo(msg, GetMsgType(Main_Control, LC_DBInfo), sizeof(LC_Cmd_DBInfo));
		msg.DBID = m_DBNetworkID;
		SendNetCmdToControl(&msg);
	}
}
bool FishServer::OnResetUserPassword(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_ResetUserPassword* pMsg = (DBR_Cmd_ResetUserPassword*)pCmd;
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishResetRolePassword('%u','%u','%u','%u');", pMsg->dwUserID,pMsg->Password1,pMsg->Password2,pMsg->Password3);
	bool Result = m_Sql[Index].Select(SqlStr, 0, pTable1, true);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleFreezeUser(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	return true;
}

//����ָ�������Ļ�����
bool FishServer::OnHandelLoadRobotInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRobotInfo* pMsg = (DBR_Cmd_LoadRobotInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRobotInfo('%u','%u','%d-%d-%d %d:%d:%d');", pMsg->BeginUserID,pMsg->EndUserID, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		DWORD PageSize = sizeof(DBO_Cmd_LoadRobotInfo)+(Rows - 1)*sizeof(RobotUserInfo);
		DBO_Cmd_LoadRobotInfo * msg = (DBO_Cmd_LoadRobotInfo*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRobotInfo);
		time_t pNow = time(null);

		vector<DWORD> pVecLog;

		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].dwUserID = pTable1.GetUint(i, 0);
			pVecLog.push_back(msg->Array[i].dwUserID);
			const TCHAR* pNickName = pTable1.GetStr(i, 1);
			TCHARCopy(msg->Array[i].NickName, CountArray(msg->Array[i].NickName), pNickName, _tcslen(pNickName));
			msg->Array[i].wLevel = pTable1.GetUshort(i, 2);
			msg->Array[i].dwExp = pTable1.GetUint(i, 3);
			msg->Array[i].dwFaceID = pTable1.GetUint(i, 4);
			msg->Array[i].bGender = pTable1.GetBit(i, 5);
			msg->Array[i].dwGlobeNum = pTable1.GetInt64(i, 6);
			msg->Array[i].dwMedalNum = pTable1.GetUint(i, 7);
			msg->Array[i].dwCurrencyNum = pTable1.GetUint(i, 8);
			msg->Array[i].dwAchievementPoint = pTable1.GetUint(i, 9);
			msg->Array[i].TitleID = pTable1.GetByte(i, 10);
			memcpy_s(&msg->Array[i].CharmArray, sizeof(msg->Array[i].CharmArray), pTable1.GetField((UINT)i, (UINT)11), sizeof(msg->Array[i].CharmArray));
			msg->Array[i].VipLevel = pTable1.GetByte(i, 12);
			BYTE MonthCardID = pTable1.GetByte(i, 13);
			__int64 MonthCardEndTime = pTable1.GetDateTime(i, 14);
			msg->Array[i].IsInMonthCard = (MonthCardID != 0 && pNow >= MonthCardEndTime);
		}

		//g_FishServer.ShowInfoToWin("���ػ����� �����˽���IDΪ:%u", msg->Array[Rows - 1].dwUserID);

		//�������Ϻ� �ְ�����
		std::vector<DBO_Cmd_LoadRobotInfo*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRobotInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRobotInfo* msg = (DBO_Cmd_LoadRobotInfo*)CreateCmd(DBO_LoadRobotInfo, sizeof(DBO_Cmd_LoadRobotInfo)-sizeof(RobotUserInfo));
		msg->Sum = 0;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleLoadCharInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadCharInfo* pMsg = (DBR_Cmd_LoadCharInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadCharInfo('%u');", pMsg->dwUserID);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		if (Rows == 0)
		{
			DBO_Cmd_LoadCharInfo* msg = (DBO_Cmd_LoadCharInfo*)CreateCmd(DBO_LoadCharInfo, sizeof(DBO_Cmd_LoadCharInfo)-sizeof(tagRoleCharInfo));
			msg->Sum = 0;
			msg->dwUserID = pMsg->dwUserID;
			msg->States = (MsgBegin | MsgEnd);
			OnAddDBResult(Index, ClientID, msg);
			return true;
		}
		DWORD PageSize = sizeof(DBO_Cmd_LoadCharInfo)+(Rows - 1)*sizeof(tagRoleCharInfo);
		DBO_Cmd_LoadCharInfo * msg = (DBO_Cmd_LoadCharInfo*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadCharInfo);
		msg->SetCmdSize(static_cast<WORD>(PageSize));
		msg->dwUserID = pMsg->dwUserID;
		time_t pNow = time(null);

		vector<DWORD> pVecLog;

		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].SrcUserID = pTable1.GetUint(i, 0);
			msg->Array[i].DestUserID = pTable1.GetUint(i, 1);
			const TCHAR* pMessageInfo = pTable1.GetStr(i, 2);
			TCHARCopy(msg->Array[i].MessageInfo, CountArray(msg->Array[i].MessageInfo), pMessageInfo, _tcslen(pMessageInfo));
			msg->Array[i].LogTime = pTable1.GetDateTime(i, 3);
		}
		//�������Ϻ� �ְ�����
		std::vector<DBO_Cmd_LoadCharInfo*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadCharInfo*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadCharInfo* msg = (DBO_Cmd_LoadCharInfo*)CreateCmd(DBO_LoadCharInfo, sizeof(DBO_Cmd_LoadCharInfo)-sizeof(tagRoleCharInfo));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleDelCharInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_DelCharInfo* pMsg = (DBR_Cmd_DelCharInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelCharInfo('%u','%u');", pMsg->dwSrcUserID, pMsg->dwDestUserID);
	bool Result = m_Sql[Index].Select(SqlStr, 0, pTable1, true);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddCharInfo(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_AddCharInfo* pMsg = (DBR_Cmd_AddCharInfo*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	tm NowTime;
	errno_t Err = localtime_s(&NowTime, &pMsg->MessageInfo.LogTime);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	UINT Size = 0;
	char* pMessageInfo = WCharToChar(pMsg->MessageInfo.MessageInfo, Size);
	char* DestMessageInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pMessageInfo, Size, DestMessageInfo);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddCharInfo('%u','%u','%s','%d-%d-%d %d:%d:%d');", pMsg->MessageInfo.SrcUserID, pMsg->MessageInfo.DestUserID, DestMessageInfo, NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);
	bool Result = m_Sql[Index].Select(SqlStr, 0, pTable1, true);

	free(pMessageInfo);
	free(DestMessageInfo);

	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}

bool FishServer::OnHandleLoadRelationRequest(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	DBR_Cmd_LoadRelationRequest* pMsg = (DBR_Cmd_LoadRelationRequest*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	
	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishLoadRelationRequest('%u','%u','%u','%u','%d-%d-%d %d:%d:%d');", pMsg->dwUserID, m_Config.GetRelation().RelationRequestLimitDay, m_Config.GetRelation().MaxRelationRequestSum, m_Config.GetWriteSec(), NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec);

	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true))
	{
		DWORD Rows = pTable1.Rows();
		if (Rows == 0)
		{
			DBO_Cmd_LoadRelationRequest* msg = (DBO_Cmd_LoadRelationRequest*)CreateCmd(DBO_LoadRelationRequest, sizeof(DBO_Cmd_LoadRelationRequest)-sizeof(tagRelationRequest));
			msg->Sum = 0;
			msg->dwUserID = pMsg->dwUserID;
			msg->States = (MsgBegin | MsgEnd);
			OnAddDBResult(Index, ClientID, msg);
			return true;
		}
		DWORD PageSize = sizeof(DBO_Cmd_LoadRelationRequest)+(Rows - 1)*sizeof(tagRelationRequest);
		DBO_Cmd_LoadRelationRequest * msg = (DBO_Cmd_LoadRelationRequest*)malloc(PageSize);
		msg->SetCmdType(DBO_LoadRelationRequest);
		msg->SetCmdSize(static_cast<WORD>(PageSize));
		msg->dwUserID = pMsg->dwUserID;
		time_t pNow = time(null);

		vector<DWORD> pVecLog;

		for (DWORD i = 0; i < Rows; ++i)
		{
			msg->Array[i].ID = pTable1.GetUint(i, 0);
			msg->Array[i].SrcUserID = pTable1.GetUint(i, 1);
			msg->Array[i].SrcFaceID = pTable1.GetUint(i, 2);
			const TCHAR* pNickName = pTable1.GetStr(i, 3);
			TCHARCopy(msg->Array[i].SrcNickName, CountArray(msg->Array[i].SrcNickName), pNickName, _tcslen(pNickName));
			msg->Array[i].SrcGender = pTable1.GetBit(i, 4);
			msg->Array[i].SrcLevel = pTable1.GetUshort(i, 5);
			msg->Array[i].SrcVipLevel = pTable1.GetByte(i, 6);
			const TCHAR* pIpAddress = pTable1.GetStr(i, 7);
			TCHARCopy(msg->Array[i].SrcIPAddress, CountArray(msg->Array[i].SrcIPAddress), pIpAddress, _tcslen(pIpAddress));
			msg->Array[i].DestUserID = pTable1.GetUint(i, 8);
			msg->Array[i].RelationType = pTable1.GetByte(i, 9);
			const TCHAR* pMessageInfo = pTable1.GetStr(i, 10);
			TCHARCopy(msg->Array[i].MessageInfo, CountArray(msg->Array[i].MessageInfo), pMessageInfo, _tcslen(pMessageInfo));
			msg->Array[i].SendTime = pTable1.GetDateTime(i, 11);
		}
		//�������Ϻ� �ְ�����
		std::vector<DBO_Cmd_LoadRelationRequest*> pVec;
		SqlitMsg(msg, PageSize, Rows, false, pVec);
		free(msg);
		if (!pVec.empty())
		{
			std::vector<DBO_Cmd_LoadRelationRequest*>::iterator Iter = pVec.begin();
			for (; Iter != pVec.end(); ++Iter)
			{
				OnAddDBResult(Index, ClientID, *Iter);
			}
			pVec.clear();
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		DBO_Cmd_LoadRelationRequest* msg = (DBO_Cmd_LoadRelationRequest*)CreateCmd(DBO_LoadRelationRequest, sizeof(DBO_Cmd_LoadRelationRequest)-sizeof(tagRelationRequest));
		msg->Sum = 0;
		msg->dwUserID = pMsg->dwUserID;
		msg->States = (MsgBegin | MsgEnd);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleAddRelationRequest(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishAddRelationRequest
	DBR_Cmd_AddRelationRequest* pMsg = (DBR_Cmd_AddRelationRequest*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}

	tm NowTime;
	time_t Now = time(null);
	errno_t Err = localtime_s(&NowTime, &Now);
	if (Err != 0)
	{
		ASSERT(false);
		return false;
	}

	UINT Size = 0;
	char* pMessageInfo = WCharToChar(pMsg->Info.MessageInfo, Size);
	char* DestMessageInfo = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pMessageInfo, Size, DestMessageInfo);

	char* pIpAddress = WCharToChar(pMsg->Info.SrcIPAddress, Size);
	char* DestIpAddress = (char*)malloc((Size * 2 + 1)*sizeof(char));
	m_Sql[Index].GetMySqlEscapeString(pIpAddress, Size, DestIpAddress);

	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };

	sprintf_s(SqlStr, sizeof(SqlStr), "call FishAddRelationRequest('%u','%u','%u','%s','%s','%d-%d-%d %d:%d:%d','%u','%u','%u','%u');", pMsg->Info.SrcUserID, pMsg->Info.DestUserID, pMsg->Info.RelationType, DestIpAddress, DestMessageInfo, 
		NowTime.tm_year + 1900, NowTime.tm_mon + 1, NowTime.tm_mday, NowTime.tm_hour, NowTime.tm_min, NowTime.tm_sec
		, m_Config.GetRelation().RelationRequestLimitDay, m_Config.GetRelation().MaxRelationRequestSum, m_Config.GetWriteSec(), m_Config.GetRelation().MaxRelationSum);

	free(pMessageInfo);
	free(DestMessageInfo);
	free(pIpAddress);
	free(DestIpAddress);


	DBO_Cmd_AddRelationRequest * msg = (DBO_Cmd_AddRelationRequest*)CreateCmd(DBO_AddRelationRequest, sizeof(DBO_Cmd_AddRelationRequest));
	msg->dwUserID = pMsg->dwUserID;
	msg->Result =false;

	//ִ��sql���
	if (m_Sql[Index].Select(SqlStr, 0, pTable1, true) && pTable1.Rows() == 1)
	{
		DWORD ID = pTable1.GetUint(0, 0);
		if (ID == 0)
		{
			msg->Result = false;
			msg->ErrorID = pTable1.GetByte(0, 1);
			OnAddDBResult(Index, ClientID, msg);
		}
		else
		{
			msg->Info.ID = ID;
			msg->Info.SrcUserID = pTable1.GetUint(0, 1);
			msg->Info.SrcFaceID = pTable1.GetUint(0, 2);
			const TCHAR* pNickName = pTable1.GetStr(0, 3);
			TCHARCopy(msg->Info.SrcNickName, CountArray(msg->Info.SrcNickName), pNickName, _tcslen(pNickName));


			msg->Info.SrcGender = pTable1.GetBit(0, 4);
			msg->Info.SrcLevel = pTable1.GetUshort(0, 5);
			msg->Info.SrcVipLevel = pTable1.GetByte(0, 6);
			const TCHAR* pIpAddress = pTable1.GetStr(0, 7);
			TCHARCopy(msg->Info.SrcIPAddress, CountArray(msg->Info.SrcIPAddress), pIpAddress, _tcslen(pIpAddress));

			msg->Info.DestUserID = pTable1.GetUint(0, 8);
			msg->Info.RelationType = pTable1.GetByte(0, 9);
			const TCHAR* pMessageInfo = pTable1.GetStr(0, 10);
			TCHARCopy(msg->Info.MessageInfo, CountArray(msg->Info.MessageInfo), pMessageInfo, _tcslen(pMessageInfo));
			msg->Info.SendTime = pTable1.GetDateTime(0, 11);

			msg->Result = true;
			OnAddDBResult(Index, ClientID, msg);
		}
	}
	else
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		OnAddDBResult(Index, ClientID, msg);
		ASSERT(false);
	}
	return true;
}
bool FishServer::OnHandleDelRelationRequest(BYTE Index, BYTE ClientID, NetCmd* pCmd)
{
	//FishDelRelationRequest
	DBR_Cmd_DelRelationRequest* pMsg = (DBR_Cmd_DelRelationRequest*)pCmd;
	if (!pMsg)
	{
		ASSERT(false);
		return false;
	}
	SqlTable pTable1;
	char SqlStr[MAXSQL_LENGTH] = { 0 };
	sprintf_s(SqlStr, sizeof(SqlStr), "call FishDelRelationRequest('%u');", pMsg->ID);
	bool Result = m_Sql[Index].Select(SqlStr, 0, pTable1, true);
	if (!Result)
	{
		LogInfoToFile(DBErrorSqlFileName, SqlStr);
		ASSERT(false);
	}
	return true;
}