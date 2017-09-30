//���������
#pragma once
#include "Stdafx.h"
class CRoleEx;
class RoleAchievementBase;
class RoleAchievementManager  //���ڹ���������ϵ�ȫ��������
{
public:
	RoleAchievementManager();
	virtual ~RoleAchievementManager();
	bool OnInit(CRoleEx* pRole);//��ʼ��
	void OnLoadAllAchievementInfoByDB(DBO_Cmd_LoadRoleAchievement* pDB);//�����ݿ����ȫ��������
	void SendAllAchievementToClient();//����ǰ��ȡ�������͵��ͻ���ȥ
	bool IsSendClient(){ return m_IsSendClient; }//�Ƿ������ͻ���ȥ��
	bool OnFinishAchievement(BYTE AchievementID);//���ָ������
	//bool OnDelAchievement(BYTE AchievementID);//ɾ��ָ������
	void OnRoleLevelChange();//����ҵȼ��仯��ʱ��
	void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	//void OnSaveByUpdate();

	bool IsLoadDB(){ return m_IsLoadDB; }

	RoleAchievementBase* QueryAchievement(BYTE AchievementID);

	void ResetClientInfo(){ m_IsSendClient = false; }

	void GetAllNeedSaveAchievement(vector<tagRoleAchievementInfo>& pVec);

	bool GetAchievementMessageStates();

	void OnResetJoinAllAchievement();
private:
	void OnDestroy();//���ٶ���
	void SendLoadAllAchievementInfoToDB();//�����ݿ��������
	bool OnJoinAchievement(BYTE AchievementID,bool IsNeedSave);//��ȡָ������
	bool IsCanJoinAchievement(BYTE AchievementID);//�ж�ָ�������Ƿ���Խ�ȡ
	void OnJoinAchievementByConfig(bool IsNeedSave);//���Խ�ȡ����
	RoleAchievementBase* CreateAchievementByEventID(BYTE EventID);
	//void SaveAchievement();
private:
	bool						m_IsLoadDB;
	bool						m_IsSendClient;
	CRoleEx*					m_pRole;//���
	std::vector<RoleAchievementBase*>  m_AchievementVec;
	int256						m_JoinAchievementLog;//��ȡ����ļ�¼
};
class RoleAchievementBase //�������Ļ���
{
public:
	RoleAchievementBase();
	virtual ~RoleAchievementBase();
	virtual bool OnInit(CRoleEx* pRole, RoleAchievementManager* pManager, tagRoleAchievementInfo* pInfo);
	virtual bool OnInit(CRoleEx* pRole, RoleAchievementManager* pManager, BYTE AchievementID,bool IsNeedSave);
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual bool IsNeedSave(){ return m_IsNeedSave; }
	virtual void OnSave(){ m_IsNeedSave = false; }
	virtual bool IsEventFinish(){ return m_EventIsFinish; }
	virtual BYTE GetAchievementID(){ return m_AchievementInfo.AchievementID; }
	virtual tagRoleAchievementInfo& GetAchievementInfo(){ return m_AchievementInfo; }
	virtual void OnJoinAchievement();
protected:
	CRoleEx*					m_pRole;//���
	RoleAchievementManager*			m_pAchievementManager;
	bool						m_EventIsFinish;//�����Ƿ��Ѿ������
	bool						m_IsNeedSave;
	//tagAchievementConfig*				m_AchievementConfig;//�������������
	tagRoleAchievementInfo				m_AchievementInfo;//��������ݿ�����
};
//����ľ���ʵ����
class GetGlobelRoleAchievement : public RoleAchievementBase
{
public:
	GetGlobelRoleAchievement() : RoleAchievementBase(){}
	virtual ~GetGlobelRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetMadelRoleAchievement : public RoleAchievementBase
{
public:
	GetMadelRoleAchievement() : RoleAchievementBase(){}
	virtual ~GetMadelRoleAchievement() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class GetCurrenRoleAchievement : public RoleAchievementBase
{
public:
	GetCurrenRoleAchievement() : RoleAchievementBase(){}
	virtual ~GetCurrenRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UpperLevelRoleAchievement : public RoleAchievementBase
{
public:
	UpperLevelRoleAchievement() : RoleAchievementBase(){}
	virtual ~UpperLevelRoleAchievement() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class CatchFishRoleAchievement : public RoleAchievementBase
{
public:
	CatchFishRoleAchievement() : RoleAchievementBase(){}
	virtual ~CatchFishRoleAchievement() {}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class SendGiffRoleAchievement : public RoleAchievementBase
{
public:
	SendGiffRoleAchievement() : RoleAchievementBase(){}
	virtual ~SendGiffRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class UseSkillRoleAchievement : public RoleAchievementBase
{
public:
	UseSkillRoleAchievement() : RoleAchievementBase(){}
	virtual ~UseSkillRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class LauncherGlobelRoleAchievement : public RoleAchievementBase
{
public:
	LauncherGlobelRoleAchievement() : RoleAchievementBase(){}
	virtual ~LauncherGlobelRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
};
class MaxGlobelSumRoleAchievement : public RoleAchievementBase
{
public:
	MaxGlobelSumRoleAchievement() : RoleAchievementBase(){}
	virtual ~MaxGlobelSumRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAchievement();
};
class ToLevelRoleAchievement : public RoleAchievementBase
{
public:
	ToLevelRoleAchievement() : RoleAchievementBase(){}
	virtual ~ToLevelRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAchievement();
};
class MaxCurrenRoleAchievement : public RoleAchievementBase
{
public:
	MaxCurrenRoleAchievement() : RoleAchievementBase(){}
	virtual ~MaxCurrenRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAchievement();
};
class RechargeRoleAchievement : public RoleAchievementBase
{
public:
	RechargeRoleAchievement() : RoleAchievementBase(){}
	virtual ~RechargeRoleAchievement(){}
	virtual void OnHandleEvent(BYTE EventID, DWORD BindParam, DWORD Param);
	virtual void OnJoinAchievement();
};