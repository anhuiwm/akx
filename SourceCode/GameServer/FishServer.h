//GameServer������
#pragma once
#include "UserLogonManager.h"
#include "..\CommonFile\FishConfig.h"
#include "RoleManager.h"
#include "TableManager.h"
#include "ShopManager.h"
#include "EventManager.h"
#include "RoleOnlineReward.h"
#include "FishPackageManager.h"
//#include "GameServerManager.h"
#include "RoleCache.h"
#include "RoleLogonManager.h"
#include "TcpClientList.h"
#include "AnnouncementManager.h"
//#include "RobotManager.h"
#include "RoleQueueManager.h"
#include "ExChangeManager.h"
#include "LotteryManager.h"
#include "GameRobot.h"
struct DelClient
{
	DWORD		LogTime;
	DWORD		SocketID;
};

class FishServer : public INetHandler
{
public:
	FishServer();
	virtual ~FishServer();

	bool InitServer(int Index);
	void InitDir();
	void OnLoadFinish();
	bool MainUpdate();
	bool OnDestroy();

	void SetServerClose(){ m_IsClose = true; }
	void SetReloadConfig(){ m_IsReloadConfig = true; }
	void SetReloadFishConfig() { m_IsReloadFishConfig = true; }
	void ShowInfoToWin(const char *pcStr, ...);

	virtual uint CanConnected(BYTE SeverID, uint ip, short port, void *pData, uint recvSize, char* resData);;
	virtual bool NewClient(BYTE SeverID, ServerClientData *pClient, void *pData, uint recvSize);
	virtual void Disconnect(BYTE ServerID, ServerClientData *pClient, RemoveType rt);
	void OnAddClient();

	void SendNetCmdToDB(NetCmd* pCmd);
	void SendNetCmdToCenter(NetCmd* pCmd);
	//void SendNetCmdToRank(NetCmd* pCmd);
	void SendNetCmdToFTP(NetCmd* pCmd);
	void SendNetCmdToOperate(NetCmd* pCmd);
	void SendNetCmdToSaveDB(NetCmd* pCmd);
	void SendNetCmdToLogDB(NetCmd* pCmd);
	//void SendNetCmdToLogon(NetCmd* pCmd);
	void SendNewCmdToClient(ServerClientData* pClient, NetCmd* pCmd);
	void SendNewCmdToAllClient(NetCmd* pCmd);
	void SendNetCmdToControl(NetCmd*pCmd);
	void SendNetCmdToLogon(BYTE LogonID, NetCmd* pCmd);
	//void SendNetCmdToMiniGame(NetCmd* pCmd);

	ServerClientData* GetUserClientDataByIndex(DWORD IndexID);

	DWORD			GetOnlinePlayerSum(){ return m_ClintList.size(); }

	RoleManager*	GetRoleManager(){ return &m_RoleManager; }
	TableManager*	GetTableManager(){ return &m_TableManager; }
	EventManager&	GetEventManager(){ return m_EventManager; }
	FishConfig&		GetFishConfig(){ return m_FishConfig; }
	FishPackageManager& GetPackageManager(){ return m_PackageManager; }
	AnnouncementManager& GetAnnouncementManager(){ return m_AnnouncementManager; }
	GameRobotManager& GetRobotManager(){ return m_GameRobotManager; }
	RoleCache& GetRoleCache(){ return m_RoleCache; }

	void UpdateByMin(DWORD dwTimer);
	void OnSaveInfoToDB(DWORD dwTimer);

	void CloseClientSocket(DWORD SocketID);

	BYTE  GetGameIndex(){ return m_GameNetworkID; }

	//bool HandleLogonMsg(BYTE LogonID, NetCmd* pCmd);

	//bool IsConnectCenterServer(){ return m_CenterTcp.IsConnected(); }

	void GetAddressByIP(DWORD IP, TCHAR* pAddress,DWORD ArrayCount);

	void SendAllMonthPlayerSumToClient(DWORD dwUserID);
	DWORD GetAchievementIndex(DWORD dwUserID);

	RoleLogonManager& GetRoleLogonManager(){ return m_RoleLogonManager; }

	//bool RoleIsOnlineByCenter(DWORD dwUserID);
	//void DelRoleOnlineInfo(DWORD dwUserID);
	void AddDelRoleSocket(DelClient delCLient){ m_DelSocketVec.push_back(delCLient); }

	bool HandleDataBaseMsg(NetCmd* pCmd);

	ChannelUserInfo* GetChannelUserInfo(DWORD UserID);
	void OnDelChannelInfo(DWORD UserID);
	DWORD GetOperateIP(){ return m_OperateIP; }

	BYTE KickUserByID(DWORD dwUserID,DWORD FreezeMin);

	string GetUserMacAddress(DWORD dwUserID);
	string GetUserIpAddress(DWORD dwUserID);
	void SendBroadCast(CRoleEx* pRole, BYTE Type, const TCHAR* pName = nullptr, DWORD ItemID = 0, DWORD ItemNum=0 );
private:
	bool ConnectFTP();
	bool ConnectDB();
    bool ConnectSaveDB();
	bool ConnectLogDB();
	//bool ConnectRank();
	//bool ConnectLogon();
	bool ConnectCenter();
	bool ConnectClient();
	bool ConnectOperate();
	bool ConnectControl();
	//bool ConnectMiniGame();

	void OnHandleAllMsg();

	void OnTcpServerLeave(BYTE ServerID, ServerClientData* pClient);
	void OnTcpServerJoin(BYTE ServerID, ServerClientData* pClient);
	void OnTcpClientConnect(TCPClient* pClient);
	void OnTcpClientLeave(TCPClient* pClient);

	void OnConnectionCenterServer();
	void OnLeaveCenterServer();

	bool HandleControlMsg(NetCmd* pCmd);
	bool HandleMiniGameMsg(NetCmd* pCmd);
	bool HandleNiuNiuMsg(NetCmd* pCmd);
	bool HandleDialMsg(NetCmd* pCmd);
	bool HandleCarMsg(NetCmd* pCmd);

	bool HandleClientMsg(ServerClientData* pClient, NetCmd* pCmd);
	
	//bool HandleRankMsg(NetCmd* pCmd);
	
	bool HandleCenterMsg(NetCmd* pCmd);
	bool HandleFtpMsg(NetCmd* pCmd);

	bool HandleOperateMsg(NetCmd* pCmd);

	//1.��½ģ��
	bool OnHandLogonLogonMsg(NetCmd* pCmd);
	bool OnHandClientLogonMsg(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandDBLogonMsg(NetCmd* pCmd);
	bool OnHandleResetPasswordResult(NetCmd* pCmd);
	//bool OnHandleRoleOnline(NetCmd* pCmd);
	//bool OnHandleRoleAchievementIndex(NetCmd* pCmd);
	bool OnHandleResetAccountName(NetCmd* pCmd);
	//�û�������
	//Center
	bool OnHandleSocketCenter(NetCmd* pCmd);
	//���ӽ���
	bool OnHandleTCPNetworkTable(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleRoleJoinTable(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleRoleLeaveTable(ServerClientData* pClient, NetCmd* pCmd);
	//��Ϸ�ڲ���Ϣ
	bool OnHandleTCPNetworkGame(ServerClientData* pClient, NetCmd* pCmd);
	//���ش���
	bool OnGateJoinGameRoom(DWORD dwSocketID);//��һ�����ؼ��������ʱ��
	bool OnGateLeaveGameRoom(DWORD dwSocketID);
	//��Ʒ����
	bool OnHandleTCPNetworkItem(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDateBaseLoadItem(NetCmd* pCmd);
	//bool OnHandleDataBaseLoadItemFinish(NetCmd* pCmd);
	bool OnHandleDataBaseAddItemResult(NetCmd* pCmd);
	//��ϵ
	bool OnHandleTCPNetworkRelation(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketRelation(NetCmd* pCmd);
	bool OnHandleDateBaseLoadRelation(NetCmd* pCmd);
	bool OnHandleDataBaseLoadBeRelation(NetCmd* pCmd);
	//bool OnHandleDataBaseLoadBeRelationFinish(NetCmd* pCmd);
	bool OnHandleDataBaseAddRelation(NetCmd* pCmd);
	//Mail
	bool OnHandleTCPNetworkMail(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserMail(NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserMailRecord(NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserMailSendItem(NetCmd* pCmd);
	//bool OnHandleDataBaseLoadUserMailFinish(NetCmd* pCmd);
	bool OnHandleDataBaseSendUserMail(NetCmd* pCmd);
	bool OnHandleSocketMail(NetCmd* pCmd);
	//Role
	bool OnHandleTCPNetworkRole(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleForge(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSaveRoleAllInfo(NetCmd* pCmd);
	bool OnHandleChangeRoleNickName(NetCmd* pCmd);
	bool OnHandleChangeRoleSecPassword(NetCmd* pCmd);
	bool OnHandleGameIDConvertUserID(NetCmd* pCmd);
	//��ѯ
	bool OnHandleTCPNetworkQuery(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadQueryUserInfo(NetCmd* pCmd);
	//bool OnHandleDataBaseLoadQueryUserInfoByUserID(NetCmd* pCmd);
	bool OnHandleDataBaseLoadQueryUserInfoByGameID(NetCmd* pCmd);
	bool OnHandleDataBaseLoadQueryUserInfoByIP(NetCmd* pCmd);
	//ǩ��
	bool OnHandleTCPNetworkCheck(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserCheckInfo(NetCmd* pCmd);
	//����
	bool OnHandleTCPNetworkTask(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketTask(NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserTaskInfo(NetCmd* pCmd);
	//�ɾ�
	bool OnHandleTCPNetworkAchievement(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserAchievementInfo(NetCmd* pCmd);
	//����
	bool OnHandleTCPNetworkMonth(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketMonth(NetCmd* pCmd);
	//�ƺ�
	bool OnHandleTCPNetworkTitle(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserTitleInfo(NetCmd* pCmd);
	//���а�
	bool OnHandleTCPNetworkRank(ServerClientData* pClient, NetCmd* pCmd);
	//����
	bool OnHandleTCPNetworkChest(ServerClientData* pClient, NetCmd* pCmd);
	//����
	bool OnHandleTCPNetworkCharm(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketCharm(NetCmd* pCmd);
	//�̵�
	bool OnHandleTCPNetworkShop(ServerClientData* pClient, NetCmd* pCmd);
	//Entity
	bool OnHandleTCPNetworlEntity(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserEntityInfo(NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserExchangeEntity(NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserExchangeItem(NetCmd* pCmd);
	//�
	bool OnHandleTCPNetworkAction(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserActionInfo(NetCmd* pCmd);
	//����
	bool OnHandleTCPNetworkGiff(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseLoadUserGiff(NetCmd* pCmd);
	bool OnHnaldeDataBaseLoadUserSendGiffInfo(NetCmd* pCmd);
	bool OnHandleDataBaseAddUserGiff(NetCmd* pCmd);
	bool OnHandleSocketGiff(NetCmd* pCmd);
	//����̵�
	//bool OnHandleTCPNetworkGlobelShop(ServerClientData* pClient, NetCmd* pCmd);
	//���߽���
	bool OnHandleTCPNetworkOnlineReward(ServerClientData* pClient, NetCmd* pCmd);
	//���GameData
	bool OnHandleDataBaseLoadGameData( NetCmd* pCmd);
	//
	bool OnHandleTCPNetworkPackage(ServerClientData* pClient, NetCmd* pCmd);
	//GameData
	bool OnHandleTCPNetworkGameData(ServerClientData* pClient, NetCmd* pCmd);
	//Message
	bool OnHandleTCPNetworkMessage(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketMessage(NetCmd* pCmd);
	//void HandlePlayerSum(DWORD dwTimer);
	void SendMessageByType(TCHAR* pMessage,WORD MessageSize, BYTE MessageType, DWORD MessageColor, BYTE StepNum, BYTE StepSec, DWORD Param,bool IsCenterMessage);
	//Recharge
	bool OnHandleTCPNetworkRecharge(ServerClientData* pClient, NetCmd* pCmd);
	//Announcement
	bool OnHandleTCPNetworkAnnouncement(ServerClientData* pClient, NetCmd* pCmd);
	//Operate
	bool OnHandleTCpNetworkOperate(ServerClientData* pClient, NetCmd* pCmd);

	void CheckDelSocket(DWORD dwTimer);

	void OnHandleUseRMB(CG_Cmd_UseRMB* pMsg);
	void OnHandlePhonePay(CG_Cmd_PhonePay* pMsg);

	//���а�
	bool OnHandleLoadWeekRankInfo(NetCmd* pCmd);

	//ExChange
	bool OnHandleTCPNetworkExChange(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleDataBaseQueryExChange(NetCmd* pCmd);
	bool OnHandleGetRechargeOrderID(NetCmd* pCmd);

	//Lottery
	bool OnHandleTCPNetwordLottery(ServerClientData* pClient, NetCmd* pCmd);


	void OnAddChannelInfo(DWORD UserID, ChannelUserInfo* pInfo);
	
	void OnClearAllChannelInfo();
	
	void OnReloadConfig();
	void OnReloadFishConfig();

	//MiNiGame
	//bool OnHandleTCPNetworkMiniGame(ServerClientData* pClient, NetCmd* pCmd);
	//bool OnHandleTCPNetworkNiuNiu(ServerClientData* pClient, NetCmd* pCmd);
	//bool OnHandleTCPNetworkDial(ServerClientData* pClient, NetCmd* pCmd);
	//bool OnHandleTCPNetworkCar(ServerClientData* pClient, NetCmd* pCmd);

	//Char
	//bool OnHandleTCPNetworkChar(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleLoadCharInfo(NetCmd* pCmd);
	bool OnHandleSocketChar(NetCmd* pCmd);

	//RelationRequest
	bool OnHandleTCPNerwordRelationRequest(ServerClientData* pClient, NetCmd* pCmd);
	bool OnHandleSocketRelationRequest(NetCmd* pCmd);
	bool OnHandleDataBaseLoadRelationRequest(NetCmd* pCmd);
	bool OnHandleDataBaseAddRelationRequest(NetCmd* pCmd);

	void UpdateInfoToControl(DWORD dwTimer);
private:
	Dump						m_pDump;
	volatile bool	 			m_IsClose;
	volatile bool				m_IsReloadFishConfig;
	volatile bool				m_IsReloadConfig;
	BYTE						m_GameNetworkID;
	//�����
	NewUDPServer				m_ClientTcp;//�Կͻ��˵������
	HashMap<DWORD, ServerClientData*> m_ClintList;
	//vector<ServerClientData*>	m_ClintList;//�ͻ��˵�����

	TCPClient			m_CenterTcp;//���ӵ����������������
	bool				m_CenterTcpStates;

	//DBServer
	TCPClient			m_DBTcp;//���ӵ����ݿ������
	bool				m_DBTcpStates;
	//DBSaveServer ר�����ڱ�������ݿ�����
	TCPClient			m_DBSaveTcp;
	bool				m_DBSaveTcpStates;
	//Log
	TCPClient			m_DBLogTcp;
	bool				m_DBLogTcpStates;

	//TcpClientList		m_DBTcpList;

	//TCPClient			m_RankTcp;//���ӵ����а������������
	//bool				m_RankTcpStates;

	//TCPClient			m_LogonTcp;//���ӵ���¼������������ ��ΪLogon���ж�� 
	//bool				m_LogonTcpStates;

	TCPClient			m_FtpTcp;//���ӵ�FTP������
	bool				m_FtpTcpStates;

	TCPClient			m_OperatorTcp;
	bool				m_OperatorStates;

	//����Ĺ�����
	DWORD				m_UserIndex;
	UserLogonManager	m_LogonManager;

	//�����ļ�
	FishConfig			m_FishConfig;

	RoleManager			m_RoleManager;
	TableManager		m_TableManager;
	ShopManager			m_ShopManager;
	//GlobelShop			m_GlobelShopManager;
	RoleOnlineReward	m_OnlineRewardManager;
	FishPackageManager	m_PackageManager;
	EventManager		m_EventManager;
	bool				m_IsSendUserInfo;
	//FTP
	/*std::vector<ImgNetCmdResult>			m_MsgVec;*/

	
	

	/*DWORD									m_LogPlayerTime;
	DWORD									m_LogPlayerSum;*/

	//GameServerManager						m_GameServerManager;
	RoleCache								m_RoleCache;
	SafeList<AfxNetworkClientOnce*>			m_AfxAddClient;


	//���������ͬ������
	//���������޸�
	HashMap<BYTE, WORD>						m_MonthInfo;
	HashMap<DWORD, WORD>					m_AchjievementList;
	//ȫ�ֻ�����Ҽ���
	//HashMap<DWORD, DWORD>					m_OnlineRoleMap;//UserID -> GameServerConfigID

	//std::vector<tagMonthRoleSum>			m_MonthInfo;
	//�û���½Ψһƾ֤
	RoleLogonManager						m_RoleLogonManager;

	AnnouncementManager						m_AnnouncementManager;

	vector<DelClient>						m_DelSocketVec;

	//RobotManager							m_RobotManager;

	RoleQueueManager						m_RoleQueueManager;//�Ŷ���

	//
	ExChangeManager							m_ExChangeManager;

	//
	HashMap<DWORD, ChannelUserInfo*>		m_ChannelInfo;

	//
	DWORD									m_OperateIP;


	//���ӵ�������
	TCPClient									m_ControlTcp;
	bool										m_ControlIsConnect;

	//
	LotteryManager								m_LotteryManager;

	//���ӵ�MiniGame
	TCPClient									m_MiniGameTcp;
	bool										m_MiniGameIsConnect;

	//��¼��½��� Mac��ַ
	HashMap<DWORD, string>						m_UserMacLog;

	GameRobotManager							m_GameRobotManager;
};
extern FishServer g_FishServer;