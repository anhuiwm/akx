#pragma once
#include "Stdafx.h"
#include "RoleItemManager.h"
#include "RoleRelation.h"
#include "RoleMailManager.h"
#include "RoleCheck.h"
#include "RoleTask.h"
#include "RoleAchievement.h"
#include "RoleMonth.h"
#include "RoleTitleManager.h"
#include "RoleIdEntity.h"
#include "RoleAction.h"
#include "RoleGiff.h"
#include "RoleFtpManager.h"
#include "RoleGameData.h"
#include "RoleLauncherManager.h"
#include "RoleRank.h"
#include "RoleMessageStates.h"
#include "RoleProtect.h"
#include "RoleVip.h"
#include "RoleMonthCard.h"
#include "RoleRate.h"
#include"FishLogic\FishCallbak.h"
#include "RoleCharManager.h"
#include "RoleRelationRequest.h"
class RoleManager;
enum CatchType;
class CRoleEx
{
public:
	
	CRoleEx();
	~CRoleEx();

	bool    OnInit(tagRoleInfo* pUserInfo, tagRoleServerInfo* pRoleServerInfo, RoleManager* pManager, DWORD dwSocketID, time_t pTime, bool LogobByGameServer, bool IsRobot);//��ҵ�½�ɹ���ʱ����� �������ݿ�����
	void	OnUserLoadFinish(bool IsLogonGameServer);//��ǰ��ҵ�ȫ�����ݿ������Ѿ���������� ���Խ����Logon�ɹ�����Ϣ���͵��ͻ���ȥ��
	bool    IsLoadFinish();

	void	UpdateRoleEvent();

	DWORD	GetUserID(){ return m_RoleInfo.dwUserID; }
	DWORD	GetGameID() { return m_RoleInfo.GameID; }

	BYTE    GetPageFriend() { return m_PageFriend++; }
	WORD    GetPageExchangeEntity() { return m_PageExchangeEntity; }
	WORD    GetPageExchangeItem() { return m_PageExchangeItem; }
	WORD    GetPageSendRecvItem() { return m_PageSendRecvItem; }
	void    SetPageFriend(BYTE page) { m_PageFriend = page; }
	void    SetPageExchangeEntity(WORD page) { m_PageExchangeEntity = page; }
	void    SetPageExchangeItem(WORD page) { m_PageExchangeItem = page; }
	void    SetPageSendRecvItem(WORD page) { m_PageSendRecvItem = page; }

	DWORD	GetGameSocketID(){ return m_dwGameSocketID; }
	void	ChangeRoleSocketID(DWORD SocketID);

	DWORD   GetFaceID(){ return m_RoleInfo.dwFaceID; }
	tagRoleInfo& GetRoleInfo(){ return m_RoleInfo; }
	tagRoleServerInfo& GetRoleServerInfo(){ return m_RoleServerInfo; }
	
	time_t				GetLastOnLineTime(){ return m_LastOnLineTime; }//��ȡ���������ߵ�ʱ�� �п���Ϊ0 ��ʾ��ҵ�һ������
	bool				IsOnceDayOnline();//�Ƿ���ͬһ�������µ�½
	bool				IsOnceMonthOnline();
	bool                IsOnceWeekOnline();

	RoleItemManger&		GetItemManager(){ return m_ItemManager; }
	RoleRelationManager&	GetRelation(){ return m_RelationManager; }
	RoleMailManager&	GetMailManager(){ return m_MailManager; }
	RoleCheck&			GetRoleCheckManager(){ return m_RoleCheck; }
	RoleTaskManager&	GetRoleTaskManager(){ return m_RoleTask; }
	RoleAchievementManager& GetRoleAchievementManager(){ return m_RoleAchievement; }
	RoleMont&	GetRoleMonth(){ return m_RoleMonth; }
	RoleTitleManager& GetRoleTitleManager(){ return m_RoleTitleManager; }
	RoleIDEntity& GetRoleEntity(){ return m_RoleIDEntity; }
	RoleActionManager& GetRoleActionManager(){ return m_RoleActionManager; }
	RoleGiffManager& GetRoleGiffManager(){ return m_RoleGiffManager; }
	RoleFtpManager&	GetRoleFtpManager(){ return m_RoleFtpManager; }
	RoleGameData& GetRoleGameData(){ return m_RoleGameData; }
	RoleLauncherManager& GetRoleLauncherManager(){ return m_RoleLauncherManager; }
	//RoleRank& GetRoleRank(){ return m_RoleRank; }
	RoleMessageStates& GetRoleMessageStates(){ return m_RoleMessageStates; }
	DWORD	GetRoleOnlineSec();
	std::vector<tagQueryRoleInfo>& GetQueryRoleVec(){ return m_QueryRoleVec; }
	std::vector<tagRoleExchangeInfo>& GetRoleExchangeEntityVec() { return m_RoleExchangeEntityVec; }
	std::vector<tagRoleExchangeInfo>& GetRoleExchangeItemVec() { return m_RoleExchangeItemVec; }
	std::vector<tagRoleSendItem>& GetRoleSendItemVec() { return m_RoleSendItemVec; }
	RoleProtect& GetaRoleProtect(){ return m_RoleProtect; }
	RoleVip& GetRoleVip(){ return m_RoleVip; }
	RoleMonthCard& GetRoleMonthCard(){ return m_RoleMonthCard; }
	RoleRate& GetRoleRate(){ return m_RoleRate; }
	RoleCharManager& GetRoleCharManager(){ return m_RoleCharManger; }
	RoleRelationRequest& GetRoleRelationRequest(){ return m_RoleRelationRequest; }

	void	SendDataToClient(NetCmd* pCmd);
	void	SendDataToCenter(NetCmd* pCmd);
	//void	SendDataToRank(NetCmd* pCmd);
	void	SendDataToTable(NetCmd* pCmd);
	//void	SendDataToLogon(NetCmd* pCmd);


	//bool	SetRoleIsOnline(bool IsOnline);//�����������õ�ǰ����Ѿ�������
	void    SendUserInfoToCenter();
	void    SendUserLeaveToCenter();

	bool	ChangeRoleExp(int AddExp, bool IsSendToClient);
	bool	ChangeRoleLevel(short AddLevel);
	bool    ChangeRoleGender(bool bGender);
	bool    ChangeRoleGuideStep(BYTE byStep);
	bool    ChangeRoleFaceID(DWORD FaceID);
	bool	ChangeRoleNickName(LPTSTR pNickName);
	void	ChangeRoleNickNameResult(DBO_Cmd_SaveRoleNickName* pMsg);
	bool    ChangeSaveRoleGlobe(__int64 saveGlobe);
	bool	ChangeRoleGlobe(int AddGlobe, bool IsSendToClient = true, bool IsSaveToDB = false,bool IsSendToMiniGame = false, bool IsCatchFish = false);
	bool	ChangeRoleMaxRate(BYTE rate, bool IsSendToClient = true);

	bool	ChangeRoleMedal(int AddMedal, const TCHAR *pcStr);
	bool	ChangeRoleCurrency(int AddCurrency, const TCHAR *pcStr);
	bool	ChangeRoleCashpoint(int AddCurrency, const TCHAR *pcStr);
	bool    ChangeRoleLastOnlineTime();
	bool	SaveRoleGoldBulletNum(DWORD GoldBulletNum, const TCHAR *pcStr);
	bool	ChangeRoleSendGoldBulletNum(int BulletNum);
	bool	ChangeRoleSendSilverBulletNum(int BulletNum);
	bool	ChangeRoleSendBronzeBulletNum(int BulletNum);
	void    SendLoginReward();
	bool	ChangeRoleProduction(DWORD dwProduction);
	bool	ChangeRoleGameTime(WORD wGameTime);
	bool	ChangeRoleTitle(BYTE TitleID);
	bool	ChangeRoleAchievementPoint(DWORD dwAchievementPoint);
	bool    ChangeRoleCharmValue(/*tagRoleCharmInfo* pInfo*/BYTE Index,int AddSum);
	bool    ChangeRoleSendGiffSum(int AddSum);
	bool	ChangeRoleAcceptGiffSum(int AddSum);
	bool	ChangeRoleTaskStates(BYTE Index, bool States);
	//bool	ClearRoleTaskStates();
	bool    ClearRoleDayTaskStates();
	bool    ClearRoleWeekTaskStates();
	void    ClearRoleTaskStatesBySign(BYTE Sign);
	bool	ChangeRoleAchievementStates(BYTE Index, bool States);
	bool	ChangeRoleActionStates(BYTE Index, bool States);
	bool	ChangeRoleOnlineRewardStates(DWORD States);
	bool    SaveRoleDayOnlineSec(bool clear = true);
	bool	ChangeRoleCheckData(DWORD CheckData);
	bool	ChangeRoleIsShowIpAddress(bool States);
	bool	ChangeRoleIsOnline(bool States);
	void	ChangeRoleOnlineTimeByDay(bool States);
	bool	ChangeRoleExChangeStates(DWORD States);
	bool	ChangeRoleTotalRechargeSum(DWORD AddSum);
    void 	ChangeRechargeRatioSum(DWORD AddSum);
	bool    ChangeRoleNobilityPoint(DWORD AddSum);
	bool    ChangeRoleCheck( bool bCheck);
	bool    ChangeRoleDayTaskActiviness(BYTE AddSum,bool clear = false);
	bool    ChangeRoleWeekTaskActiviness(BYTE AddSum, bool clear = false);
	bool    ChangeRoleWeekClobeNum(__int64 AddSum, bool IsSendToClient, bool IsSaveToDB);
	bool	ChangeRoleIsFirstPayGlobel();
	bool	ChangeRoleIsFirstPayCashpoint(BYTE Index);
	void    FirstCharge();
	bool    IsFirstPayCashpoint(BYTE Index);
	bool	ChangeRoleParticularStates(DWORD States);
	//bool	ChangeRoleLotteryScore(int AddScore);
	void	OnRoleCatchFishByLottery(BYTE FishTypeID, CatchType pType, byte subType);
	void	OnClearRoleLotteryInfo();
	void	OnChangeRoleSecPassword(DWORD Crc1, DWORD Crc2, DWORD Crc3,bool IsSaveToDB);
	
	//bool	SetRoleMonthCardInfo(BYTE MonthCardID);//���������¿����ұ��浽���ݿ�
	//bool	GetRoleMonthCardReward();

	//bool	ChangeRoleRateValue(BYTE AddRateIndex);//��ӽ����ĵȼ�
	//bool	ChangeRoleVipLevel(BYTE VipLevel,bool IsInit = false);
	//bool	IsCanUseRateIndex(BYTE RateIndex);

	bool	ChangeRoleCashSum(int AddSum);

	bool	ChangeRoleShareStates(bool States);

	//RoleServerInfo
	bool	ChangeRoleTotalFishGlobelSum(int AddSum);
	void	AddRoleProtectSum();

	
	//
	void	SaveRoleExInfo();

	bool    LostUserMoney(DWORD Globel, DWORD Medal, DWORD Currey, const TCHAR *pcStr);

	__int64 GetGlobel();
	DWORD GetExp();
	WORD  GetLevel();
	BYTE  GetVipLevel();
	DWORD GetProduction();
	DWORD GetGameTime();

	DWORD GetCurrency();
	DWORD GetCashpoint();
	void OnSaveInfoToDB();
	void UpdateByMin(bool IsHourChange, bool IsDayChange, bool IsWeekChange, bool IsMonthChange, bool IsYearChange);

	void OnHandleEvent(bool IsUpdateTask, bool IsUpdateAction, bool IsUpdateAchievement,BYTE EventID, DWORD BindParam, DWORD Param);


	void OnAddRoleRewardByRewardID(WORD RewardID, const TCHAR* pStr,DWORD RewardSum = 1);//��һ��ָ���Ľ���ID

	//��RoleEx ���� ������ �л�
	void SetRoleExLeaveServer();//����RoleEx �뿪�˷�����

	void ResetRoleInfoToClient();

	bool IsAFK(){ return m_IsAfk; }
	void SetAfkStates(bool States);

	bool LogonByGameServer(){ return m_LogobByGameServer; }

	void ResetClientInfo();

	bool IsRobot(){ return m_IsRobot; }

	bool IsExit(){ return m_IsExit; }
	void SetIsExit(bool States);

	bool SaveAllRoleInfo(bool IsExit);

	void SetRoleIsNeedSave(){ m_IsNeedSave = true; }//�޸���� ����Ҫ�����

	DWORD GetChannelID(){ return m_ChannelID; }

	void SendClientOpenShareUI();

	void UpdateOnlineStatesByMin(bool IsHourChange);

	bool GetOnlineRewardMessageStates();

	//bool IsGm();
	bool IsLogonGm();
	bool IsChargeGm();
	time_t GetLastRechargeRatioTime() { return m_LastRechargeRatioTime; }
	DWORD  GetRechargeRatioSum() { return m_RechargeRatioSum; }
private:
	void OnHandleRoleVersionChange();
	void ResetPerDay();
	BYTE GetLauncherType()
	{
		return m_RoleLauncherManager.GetUsingLauncher();
	};

private:
	
	RoleManager*				m_RoleManager;
	tagRoleInfo					m_RoleInfo;//��ҵ����� �����ݿ����ȡ�� ��һЩ����Ĳ��� ����������Ҫ���͵��ͻ���ȥ��
	tagRoleServerInfo			m_RoleServerInfo;
	bool					    m_IsNeedSave;
	bool						m_IsChangeClientIP;//��ʼ����ʱ��ʹ��
	
	//��ҵ����ݴ���
	DWORD						m_dwGameSocketID;//�����Game��Socket�϶�Ӧ��ID		
	time_t						m_LastOnLineTime;//�������ߵ�ʱ���¼
	time_t						m_LogonTime;//��ǰ��½��ʱ��
	time_t						m_LogonTimeByDay;//��������ʱ����
	//��ҵĹ�����
	RoleItemManger				m_ItemManager;//�����Ʒ������
	RoleRelationManager			m_RelationManager;//��ҹ�ϵ�б�
	RoleMailManager				m_MailManager;//����ʼ�������
	RoleCheck					m_RoleCheck;//���ǩ��������
	RoleTaskManager				m_RoleTask;//������������
	RoleAchievementManager		m_RoleAchievement;//��ҳɾ͹�����
	RoleMont					m_RoleMonth;
	RoleTitleManager			m_RoleTitleManager;
	RoleIDEntity				m_RoleIDEntity;
	RoleActionManager			m_RoleActionManager;
	RoleGiffManager				m_RoleGiffManager;
	RoleFtpManager				m_RoleFtpManager;
	RoleGameData				m_RoleGameData;
	RoleLauncherManager			m_RoleLauncherManager;
	//RoleRank					m_RoleRank;
	RoleMessageStates			m_RoleMessageStates;
	RoleProtect					m_RoleProtect;
	RoleVip						m_RoleVip; 
	RoleMonthCard				m_RoleMonthCard;
	RoleRate					m_RoleRate;
	RoleCharManager				m_RoleCharManger;
	RoleRelationRequest			m_RoleRelationRequest;

	bool						m_IsAfk;

	//��½��صĻ�������
	bool						m_LogobByGameServer;
	bool						m_IsRobot;

	bool						m_IsExit;//����Ƿ������˳�
	bool						m_IsOnline;

	//��Ҳ�ѯ������
	std::vector<tagQueryRoleInfo> m_QueryRoleVec;
	std::vector<tagRoleExchangeInfo> m_RoleExchangeEntityVec;
	std::vector<tagRoleExchangeInfo> m_RoleExchangeItemVec;
	std::vector<tagRoleSendItem>  m_RoleSendItemVec;

	//������ڵ�����ID
	DWORD							m_ChannelID;


	BYTE                            m_PageFriend = 0;//��������ҳ��
	WORD							m_PageExchangeEntity = 0;//ʵ��һ�
	WORD                            m_PageExchangeItem = 0;//���߶һ�
	WORD                            m_PageSendRecvItem = 0;//������ȡ��¼
	time_t                          m_LastRechargeRatioTime = 0;
	DWORD                           m_RechargeRatioSum = 0;
	//��������
	std::vector<TCHAR*>				m_ChannelUserInfo;
};