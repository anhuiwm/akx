//���������
#pragma once
#include "Stdafx.h"

class CRoleEx;
class RoleTaskBase;
class RoleTaskManager  //���ڹ���������ϵ�ȫ��������
{
public:
	RoleTaskManager();
	virtual ~RoleTaskManager();
	bool OnInit(CRoleEx* pRole);//��ʼ��
	void OnLoadAllTaskInfoByDB(DBO_Cmd_LoadRoleTask* pDB);//�����ݿ����ȫ��������
	void OnDayChange( bool join = true );//�������仯��ʱ��
	void OnWeekChange(bool join = true );//�ܱ仯
	void SendAllTaskToClient();//����ǰ��ȡ�������͵��ͻ���ȥ
	bool IsSendClient(){ return m_IsSendClient; }//�Ƿ������ͻ���ȥ��
	bool OnFinishTask(BYTE TaskID);//���ָ������
	bool OnDelTask(BYTE TaskID);//ɾ��ָ������
	void OnRoleLevelChange();//����ҵȼ��仯��ʱ��
	void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	//void OnSaveByUpdate();

	bool IsLoadDB(){ return m_IsLoadDB; }

	RoleTaskBase* QueryTask(BYTE TaskID);

	void ResetClientInfo(){ m_IsSendClient = false; }

	void GetAllNeedSaveTask(vector<tagRoleTaskInfo>& pVec);

	bool GetTaskMessageStates();
	bool GetWeekTaskMessageStates();
	void OnResetJoinAllTask();
	static std::vector<BYTE>& GetTasksBySign( BYTE sign);
	BYTE GetTaskSign( BYTE taskID);
private:
	//void OnDestroy();//���ٶ���
	void OnDestroyDay();
	void OnDestroyWeek();

	void ClearJoinTaskLogBySign(BYTE Sign);

	void SendLoadAllTaskInfoToDB();//�����ݿ��������
	//bool OnClearAllTask();//������н�ȡ������
	bool OnClearAllDayTask();//����ճ�����
	bool OnClearAllWeekTask();//����ܳ�����
	bool OnJoinTask(BYTE TaskID,bool IsNeedSave);//��ȡָ������
	bool IsCanJoinTask(BYTE TaskID);//�ж�ָ�������Ƿ���Խ�ȡ
	void OnJoinTaskByConfig(bool IsNeedSave);//���Խ�ȡ����
	RoleTaskBase* CreateTaskByEventID(BYTE EventID);
	//void SaveTask();
private:
	bool						m_IsLoadDB;
	bool						m_IsSendClient;
	CRoleEx*					m_pRole;//���
	std::vector<RoleTaskBase*>  m_TaskVec;
	int256						m_JoinTaskLog;//��ȡ����ļ�¼
	static std::vector<BYTE>    g_VecEmpty;
};
class RoleTaskBase //�������Ļ���
{
public:
	RoleTaskBase();
	virtual ~RoleTaskBase();
	virtual bool OnInit(CRoleEx* pRole, RoleTaskManager* pManager, tagRoleTaskInfo* pInfo);
	virtual bool OnInit(CRoleEx* pRole, RoleTaskManager* pManager, BYTE TaskID,bool IsSave);
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual bool IsNeedSave(){ return m_IsNeedSave; }
	virtual void OnSave(){ m_IsNeedSave = false; }
	virtual bool IsEventFinish(){ return m_EventIsFinish; }
	virtual BYTE GetTaskID(){ return m_TaskInfo.TaskID; }
	virtual tagRoleTaskInfo& GetTaskInfo(){ return m_TaskInfo; }
	virtual void OnJoinTask();
protected:
	CRoleEx*					m_pRole;//���
	RoleTaskManager*			m_pTaskManager;
	bool						m_EventIsFinish;//�����Ƿ��Ѿ������
	bool						m_IsNeedSave;
	//tagTaskConfig*				m_TaskConfig;//�������������
	tagRoleTaskInfo				m_TaskInfo;//��������ݿ�����
};
//����ľ���ʵ����
class GetGlobelRoleTask : public RoleTaskBase
{
public:
	GetGlobelRoleTask() : RoleTaskBase(){}
	virtual ~GetGlobelRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetMadelRoleTask : public RoleTaskBase
{
public:
	GetMadelRoleTask() : RoleTaskBase(){}
	virtual ~GetMadelRoleTask() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetCurrenRoleTask : public RoleTaskBase
{
public:
	GetCurrenRoleTask() : RoleTaskBase(){}
	virtual ~GetCurrenRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UpperLevelRoleTask : public RoleTaskBase
{
public:
	UpperLevelRoleTask() : RoleTaskBase(){}
	virtual ~UpperLevelRoleTask() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class CatchFishRoleTask : public RoleTaskBase
{
public:
	CatchFishRoleTask() : RoleTaskBase(){}
	virtual ~CatchFishRoleTask() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class SendGiffRoleTask : public RoleTaskBase
{
public:
	SendGiffRoleTask() : RoleTaskBase(){}
	virtual ~SendGiffRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UseSkillRoleTask : public RoleTaskBase
{
public:
	UseSkillRoleTask() : RoleTaskBase(){}
	virtual ~UseSkillRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class LauncherGlobelRoleTask : public RoleTaskBase
{
public:
	LauncherGlobelRoleTask() : RoleTaskBase(){}
	virtual ~LauncherGlobelRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class MaxGlobelSumRoleTask : public RoleTaskBase
{
public:
	MaxGlobelSumRoleTask() : RoleTaskBase(){}
	virtual ~MaxGlobelSumRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};
class ToLevelRoleTask : public RoleTaskBase
{
public:
	ToLevelRoleTask() : RoleTaskBase(){}
	virtual ~ToLevelRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};
class MaxCurrenRoleTask : public RoleTaskBase
{
public:
	MaxCurrenRoleTask() : RoleTaskBase(){}
	virtual ~MaxCurrenRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};
class RechargeRoleTask : public RoleTaskBase
{
public:
	RechargeRoleTask() : RoleTaskBase(){}
	virtual ~RechargeRoleTask(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};

class CompleteDayTaskRoleTask : public RoleTaskBase
{
public:
	CompleteDayTaskRoleTask() : RoleTaskBase() {}
	virtual ~CompleteDayTaskRoleTask() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};

class CompleteWeekTaskRoleTask : public RoleTaskBase
{
public:
	CompleteWeekTaskRoleTask() : RoleTaskBase() {}
	virtual ~CompleteWeekTaskRoleTask() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinTask();
};

