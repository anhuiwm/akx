//���������
#pragma once
#include "Stdafx.h"
class CRoleEx;
class RoleActionBase;
//�������ɾ� ��ʱ������
//� �� ������ϵ ���� ��ʱ������ ����Ӧ�������� ���ʱ�����Ƶ��� ��ÿ����յ����� �ͻ��˽�ȡ�� ���ǿ�����ȡ��ȫ�������
class RoleActionManager  //���ڹ���������ϵ�ȫ��������
{
public:
	RoleActionManager();
	virtual ~RoleActionManager();
	bool OnInit(CRoleEx* pRole);//��ʼ��
	void OnLoadAllActionInfoByDB(DBO_Cmd_LoadRoleAction* pDB);//�����ݿ����ȫ��������
	void SendAllActionToClient();//����ǰ��ȡ�������͵��ͻ���ȥ
	bool IsSendClient(){ return m_IsSendClient; }//�Ƿ������ͻ���ȥ��
	bool OnFinishAction(BYTE ActionID, DWORD ActionOnceID);//���ָ������
	void OnRoleLevelChange();//����ҵȼ��仯��ʱ��
	void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	//void OnSaveByUpdate();
	void UpdateByHour();
	void OnDayChange();
	bool IsLoadDB(){ return m_IsLoadDB; }

	RoleActionBase* QueryAction(BYTE ActionID);

	void ResetClientInfo(){ m_IsSendClient = false; }

	void GetAllNeedSaveAction(vector<tagRoleActionInfo>& pVec);

	bool GetActionMessageStates();
	void OnResetJoinAllAction();
private:
	void OnDestroy();//���ٶ���
	void SendLoadAllActionInfoToDB();//�����ݿ��������
	bool OnJoinAction(BYTE ActionID, bool IsNeedSave);//��ȡָ������
	bool IsCanJoinAction(BYTE ActionID);//�ж�ָ�������Ƿ���Խ�ȡ
	void OnJoinActionByConfig(bool IsNeedSave);//���Խ�ȡ����
	RoleActionBase* CreateActionByEventID(BYTE EventID);
	//void SaveAction();
private:
	bool						m_IsLoadDB;
	bool						m_IsSendClient;
	CRoleEx*					m_pRole;//���
	std::vector<RoleActionBase*>  m_ActionVec;
	int256						m_JoinActionLog;//��ȡ����ļ�¼
};
class RoleActionBase //�������Ļ���
{
public:
	RoleActionBase();
	virtual ~RoleActionBase();
	virtual bool OnInit(CRoleEx* pRole, RoleActionManager* pManager, tagRoleActionInfo* pInfo);
	virtual bool OnInit(CRoleEx* pRole, RoleActionManager* pManager, BYTE ActionID, bool IsNeedSave);
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual bool IsNeedSave(){ return m_IsNeedSave; }
	virtual void OnSave(){ m_IsNeedSave = false; }
	//virtual bool IsEventFinish(){ return m_EventIsFinish; }
	virtual BYTE GetActionID(){ return m_ActionInfo.ActionID; }
	virtual tagRoleActionInfo& GetActionInfo(){ return m_ActionInfo; }
	virtual bool  IsInTime();
	virtual void  SetIsNeedSave(){ m_IsNeedSave = true; }
	virtual void OnJoinAction();
	virtual bool IsExistsFinishEvent();
protected:
	CRoleEx*					m_pRole;//���
	RoleActionManager*			m_pActionManager;
	//bool						m_EventIsFinish;//�����Ƿ��Ѿ������
	bool						m_IsNeedSave;
	//tagActionConfig*				m_ActionConfig;//�������������
	tagRoleActionInfo				m_ActionInfo;//��������ݿ�����
};
//����ľ���ʵ����
class GetGlobelRoleAction : public RoleActionBase
{
public:
	GetGlobelRoleAction() : RoleActionBase(){}
	virtual ~GetGlobelRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetMadelRoleAction : public RoleActionBase
{
public:
	GetMadelRoleAction() : RoleActionBase(){}
	virtual ~GetMadelRoleAction() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetCurrenRoleAction : public RoleActionBase
{
public:
	GetCurrenRoleAction() : RoleActionBase(){}
	virtual ~GetCurrenRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UpperLevelRoleAction : public RoleActionBase
{
public:
	UpperLevelRoleAction() : RoleActionBase(){}
	virtual ~UpperLevelRoleAction() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class CatchFishRoleAction : public RoleActionBase
{
public:
	CatchFishRoleAction() : RoleActionBase(){}
	virtual ~CatchFishRoleAction() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class SendGiffRoleAction : public RoleActionBase
{
public:
	SendGiffRoleAction() : RoleActionBase(){}
	virtual ~SendGiffRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UseSkillRoleAction : public RoleActionBase
{
public:
	UseSkillRoleAction() : RoleActionBase(){}
	virtual ~UseSkillRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class LauncherGlobelRoleAction : public RoleActionBase
{
public:
	LauncherGlobelRoleAction() : RoleActionBase(){}
	virtual ~LauncherGlobelRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class MaxGlobelSumRoleAction : public RoleActionBase
{
public:
	MaxGlobelSumRoleAction() : RoleActionBase(){}
	virtual ~MaxGlobelSumRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};
class ToLevelRoleAction : public RoleActionBase
{
public:
	ToLevelRoleAction() : RoleActionBase(){}
	virtual ~ToLevelRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};
class MaxCurrenRoleAction : public RoleActionBase
{
public:
	MaxCurrenRoleAction() : RoleActionBase(){}
	virtual ~MaxCurrenRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};
class RechargeRoleAction : public RoleActionBase
{
public:
	RechargeRoleAction() : RoleActionBase(){}
	virtual ~RechargeRoleAction(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};

class RechargeOneRoleAction : public RoleActionBase
{
public:
	RechargeOneRoleAction() : RoleActionBase(){}
	virtual ~RechargeOneRoleAction() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};


class RechargeFirstRoleAction : public RoleActionBase
{
public:
	RechargeFirstRoleAction() : RoleActionBase() {}
	virtual ~RechargeFirstRoleAction() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAction();
};
