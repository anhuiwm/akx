//���ڷ���Log��¼���ļ�
#pragma once
enum LogType
{
	LT_Cashpoint = 1,//��ȯ
	LT_Medal = 2,//����
	LT_Reward = 3,//����
	LT_NiuNiu = 4,//ţţ
	LT_GlobelBag=5,//������Ʒ
	LT_Lottery=6,//�齱
	LT_Skill = 7,//����
	LT_Dial = 8,//ɭ�����
	LT_Car = 9,
	LT_BULLET = 10,//�ƽ�ͷ
	LT_NORMAL = 11,//������Ʒ
};
typedef void(SendMsgFun)(NetCmd* pCmd);

class DBLogManager
{
public:
	DBLogManager();
	virtual ~DBLogManager();
	//���ߵ�Log��¼
	void LogItemToDB(DWORD dwUserID, int ItemID, int ItemSum, int EndItemSum, const TCHAR *pcStr, SendMsgFun pSend);

	//��ͨ��Log��¼
	void LogToDB(DWORD dwUserID, LogType Type, int TypeSum,DWORD Param, const TCHAR *pcStr, SendMsgFun pSend);//�������Ϸ�е���Ϊ ���ձ����Ϊ
	//��ֵLog��¼
	void LogUserRechargeLogToDB(string OrderStates, string OrderID, DWORD UserID, string ChannelCode, string ChannelOrderID, string ChannelLabel, DWORD ShopItemID, DWORD Price, DWORD FreePrice, DWORD OldGlobelNum, DWORD OldCurrceyNum, DWORD AddGlobel, DWORD AddCurrcey, WORD RewardID, SendMsgFun pSend);
	//ʵ����ƷLog��¼
	void LogUserEntityItemLogToDB(string OrderStates, DWORD ID, DWORD ItemID, DWORD ItemSum, time_t ShopLogTime, DWORD UserID, string Address, UINT64 Phone, string IDEntity, string Name, string OrderNumber, DWORD MedalSum, DWORD NowMedalSum,DWORD HandleIP, SendMsgFun pSend);
	//���������Log��¼
	void LogRoleOnlineInfo(DWORD dwUserID, bool IsOnlineOrLeaveOnline, string MacAddress,string IpAddress, DWORD GlobelNum, DWORD CurrceyNum, DWORD MedalNum,  SendMsgFun pSend);//����������߼�¼
	//��ҽ�������Log��¼
	void LogRoleJoinOrLeaveTableInfo(DWORD dwUserID, DWORD GlobelNum, DWORD CurrceyNum, DWORD MedalNum, BYTE TableID, BYTE MonthID, bool IsJoinOrLeave, SendMsgFun pSend);//��ҽ������ӵļ�¼
	//��ҳ�ֵ�ֻ�����Log��¼
	void LogRolePhonePayLogToDB(string OrderStates, UINT64 OrderID, DWORD UserID, UINT64 PhoneNumber, DWORD FacePrice, SendMsgFun pSend);
	//��¼ϵͳÿ�ֵ���Ϸ״̬ 
	void LogNiuNiuTableInfoToDB(BYTE BrandValue[MAX_NIUNIU_ClientSum + 1][MAX_NIUNIU_BrandSum], INT64 BrandGlobel, INT64 SystemGlobel, DWORD RoleSum, SendMsgFun pSend);
	//��¼���ʹ�öһ���
	void LogExChangeInfoToDB(DWORD dwUserID, BYTE ExChangeTypeID, TCHAR ExChange[ExChangeCode_Length + 1], SendMsgFun pSend);
	//��¼���ʹ�ó齱��¼
	void LogLotteryInfoToDB(DWORD dwUserID, BYTE LotteryID, WORD RewardID, SendMsgFun pSend);
	//��¼��ұ�����¼
	void LogMonthInfoToDB(DWORD dwUserID, BYTE MonthID, DWORD MonthScore, DWORD MonthIndex, BYTE AddMonthGlobelSum, DWORD MonthSkillSum, WORD MonthRewardID, SendMsgFun pSend);
	//��¼���ɭ������¼
	void LogDialInfoToDB(DWORD BanderUserID, BYTE AreaData[MAX_DIAL_GameSum], UINT64 AreaGlobel[MAX_DIAL_ClientSum], WORD ResultIndex, INT64 BrandGlobel, INT64 SystemGlobel, DWORD RoleSum, SendMsgFun pSend);
	//��¼ת������
	void LogCarInfoToDB(DWORD BanderUserID, UINT64 AreaGlobel[MAX_CAR_ClientSum], BYTE ResultIndex, INT64 BrandGlobel, INT64 SystemGlobel, DWORD RoleSum, SendMsgFun pSend);

	void LogStockScoreToDB(WORD ServerID, BYTE TableType, __int64 StockScore,__int64 taxScore,SendMsgFun pSend);
};
extern DBLogManager g_DBLogManager;