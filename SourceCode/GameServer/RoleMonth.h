//������ϵı�������
//ֻ�������ʽ����������ӵ�ʱ�������Żᱻ���
#pragma once
#include "Stdafx.h"
class CRoleEx;
struct tagMonthConfig;
class RoleMont
{
public:
	RoleMont();
	virtual ~RoleMont();

	bool OnInit(CRoleEx* pRole);

	void ClearMonthInfo();
	void OnLoadMonthInfo(tagRoleMonthInfo* pInfo);

	bool IsInMonthTable();

	bool OnChangeRoleMonthGlobel(int MonthGlobel,bool IsSendToClient = false);
	bool OnChangeRoleMonthPoint(int Point, bool IsSendToClient = false);

	void OnChangeRoleMonthIndex(DWORD dwIndex, DWORD UpperSocre);//�޸���ҵ�����
	void OnRoleAddMonthGlobel();//��ҽ������Ҳ���
	bool IsCanAddMonthGlobel();

	void OnMonthEnd(BYTE Month,DWORD Index,DWORD MonthScores,DWORD VipMonthScores);//������������ʱ��

	void OnPlayerLeaveTable();

	BYTE GetMonthID(){ return m_MonthID; }
	
	tagRoleMonthInfo& GetMonthInfo(){ return m_MonthInfo; }

	void OnUpdate(DWORD dwTimer);
	void OnUpMonthInfo(BYTE MonthID);

	void OnLoadSignUpMonthInfo(CG_Cmd_LoadRoleSignUp* pMsg);
	void SendAllSignUpInfoToClient();
	void OnSignUpSucess(BYTE Month);

	void OnResetMonth(BYTE Month);

	void OnUseMonthSkill(BYTE Month);
private: 
	CRoleEx*					m_pRole;
	BYTE						m_MonthID;
	tagRoleMonthInfo			m_MonthInfo;//��ҵ���GameServer�ϵı�������Ϣ
	//tagMonthConfig*				m_MonthConfig;
	bool						m_IsCenterChange;
	DWORD						m_CenterUpdateTime;

	//����
	vector<BYTE>				m_SignUpMonthVec;
	bool						m_IsLoadSignUpFinish;
	bool						m_IsNeedSendToClient;
};