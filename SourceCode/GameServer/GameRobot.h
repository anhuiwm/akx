//��Ϸ������
#pragma once
#include "GameTable.h"
class GameRobotManager;

struct tagGameRobotCharm
{
	DWORD		dwUserID;
	BYTE		CharmIndexID;
	WORD		CharmItemSum;
	DWORD		BeginTimeLog;
};
struct tagGameRobotCharmEvent
{
	DWORD		SrcUserID;
	int			CharmValue;
	DWORD		TimeLog;
};
struct tagGameRobotCharmEventList
{
	std::list<tagGameRobotCharmEvent>	 EventList;
	int									 NowChangeCharmValue;
	tagGameRobotCharmEventList()
	{
		EventList.clear();
		NowChangeCharmValue = 0;
	}
	void Clear()
	{
		EventList.clear();
		NowChangeCharmValue = 0;
	}
	int AddCharmEvent(DWORD SrcUserID, int ChanrmValue,DWORD DiffTime)
	{
		DWORD NowTime = timeGetTime();
		std::list<tagGameRobotCharmEvent>::iterator Iter = EventList.begin();
		for (; Iter != EventList.end();)
		{
			if (NowTime - Iter->TimeLog >= DiffTime)
			{
				NowChangeCharmValue -= Iter->CharmValue;
				Iter = EventList.erase(Iter);
			}
			else
				++Iter;
		}
		tagGameRobotCharmEvent pEvent;
		pEvent.CharmValue = ChanrmValue;
		pEvent.SrcUserID = SrcUserID;
		pEvent.TimeLog = NowTime;
		EventList.push_back(pEvent);
		NowChangeCharmValue += pEvent.CharmValue;
		return NowChangeCharmValue;
	}
};
struct tagGameRobotCharmChangeManager
{
	HashMap<DWORD, tagGameRobotCharmEventList>	EventMap;
	void Clear()
	{
		EventMap.clear();
	}
	void Clear(DWORD dwUserID)
	{
		EventMap.erase(dwUserID);
	}
	int AddCharmEvent(DWORD SrcUserID, int ChanrmValue, DWORD DiffTime)
	{
		HashMap<DWORD, tagGameRobotCharmEventList>::iterator Iter = EventMap.find(SrcUserID);
		if (Iter == EventMap.end())
		{
			tagGameRobotCharmEventList pInfo;
			int Value=pInfo.AddCharmEvent(SrcUserID, ChanrmValue, DiffTime);
			EventMap.insert(HashMap<DWORD, tagGameRobotCharmEventList>::value_type(SrcUserID,pInfo));
			return Value;
		}
		else
		{
			return Iter->second.AddCharmEvent(SrcUserID, ChanrmValue, DiffTime);
		}
	}
};
class GameRobot
{
public:
	GameRobot(GameRobotManager* pManager,CRoleEx* pRole);
	virtual ~GameRobot();

	void Update();
	void SetRobotUse(DWORD RobotID);
	void ResetRobot();
	DWORD GetRobotUserID();
	CRoleEx* GetRoleInfo();
	void OnChangeCharmValue(DWORD SrcUserID, BYTE CharmIndexID, DWORD CharmItemSum, int ChangeCharmValue);
private:
	void UpdateRobotRoom();
	void UpdateRobotRate();
	void UpdateRobotSkill();
	void UpdateRobotLauncher();
	void UpdateRobotOpenFire();
	void UpdateRobotInfo();
	void UpdateRobotIsCanOpenFire();
	void UpdateRobotCharm();
	void UpdateRobotSendCharm();
private:
	GameRobotManager*   m_pManager;
	CRoleEx*			m_pRole;
	DWORD				m_RobotID;
	bool				m_IsUse;
	//1.��������ļ�¼
	DWORD				m_RoomTimeLog;
	DWORD				m_LeaveTableTimeLog;
	//2.�������л�����
	DWORD				m_RateTimeLog;
	BYTE				m_NowRate;
	DWORD				m_ChangeRateTimeLog;
	//3.������ʹ�ü���
	DWORD				m_SkillTimeLog;
	DWORD				m_UseSkillTimeLog;
	//4.�������л���̨
	DWORD				m_LauncherTimeLog;
	DWORD				m_ChangeLauncherTimeLog;
	//5.�����˿���
	DWORD				m_OpenFireTimeLog;
	WORD				m_LockFishID;
	DWORD				m_Angle;
	//
	DWORD				m_StopOpenFireTimeLog;
	//
	DWORD				m_RobotInfoTimeLog;
	//
	DWORD				m_RobotOpenFireTimeLog;
	bool				m_IsOpenFire;

	//
	DWORD				m_RobotSendCharmItemTimeLog;
	DWORD				m_dwRobotSendCharmTimeLog;
	list<tagGameRobotCharm> m_vecSendCharmArray;//�����͵���������
	tagGameRobotCharmChangeManager m_RobotCharmEvent;
	//
	DWORD				m_dwSendCharmTineLog;

};
struct tagRobotWrite
{
	WORD		TableID;
	DWORD		TimeLog;
};

class GameRobotManager//��Ϸ�����˹�����
{
public:
	GameRobotManager();
	virtual ~GameRobotManager();

	void  OnLoadAllGameRobot(DWORD BeginUserID,DWORD EndUserID);//����ȫ���Ļ�����
	void  OnAddRobot(DWORD dwUserID);
	
	GameRobot* GetFreeRobot(DWORD RobotID);//��ȡһ�����еĻ�����
	void ResetGameRobot(GameRobot* pRobot);//�黹һ��������
	void ResetGameRobot(DWORD dwUserID);
	//
	bool GameRobotIsCanJoinTable(GameTable* pTable);//�жϻ������Ƿ���Խ��뵱ǰ������
	//
	void  OnRoleCreateNormalRoom(GameTable* pTable);//����ͨ��Ҵ���һ���Ǳ��������ʱ��
	void  OnRoleLeaveNormalRoom(GameTable* pTable);//����ͨ����뿪һ���Ǳ��������ʱ��
	void  OnRoleJoinNormalRoom(GameTable* pTable);//����ͨ��ҽ���һ���Ǳ��������ʱ��
	//
	void  OnHandleMonthStates(BYTE MonthID, BYTE MonthStates);
	//
	void  Update();
	//
	void  AdddWriteRobot(WORD TableID, DWORD WriteTime);
	void  UpdateWriteList();
	//
	void  AddResetRobot(DWORD dwUserID);
	void  UpdateResetVec();

	void OnChangeCharmValue(DWORD SrcUserID, DWORD DestUserID, BYTE CharmIndexID, DWORD CharmItemSum, int ChangeCharmValue);
	void SetRobotClose(bool bClose) { m_RobotClose = bClose; }
private:
	void  OnHandleAllMonthStates();
	void  OnMonthBeginSign(BYTE MonthID);
	void  OnMonthEndSign(BYTE MonthID);
	void  OnMonthBeginStar(BYTE MonthID);
	void  OnMonthEndStar(BYTE MonthID);
private:
	bool                        m_RobotClose;
	DWORD						m_RobotConfigSum;
	bool						m_IsLoadFinish;
	HashMap<BYTE, BYTE>          m_MonthStatesMap;

	HashMap<DWORD, GameRobot*>	m_RobotMap;//�Ѿ�ʹ�õĻ�����
	vector<GameRobot*>			m_FreeRobot;//���л�����
	//
	HashMap<BYTE, vector<GameRobot*>> m_MonthSignMap;//���������˵ļ�¼
	//tableID Time  WORD DWORD
	list<tagRobotWrite>			m_WriteList;
	//
	vector<DWORD>				m_ResetUserVec;
};