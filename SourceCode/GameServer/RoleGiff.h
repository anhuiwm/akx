//���֮����໥���͹���
//���ÿ����Է��Ͷ��ٴ����� 
//���ÿ����Խ��ܶ��ٴ����� 
//ÿ������Я���Ľ����
//��¼���ÿ�췢�͵����ݵĴ��� ÿ������
//��¼���ÿ����յĴ���  ÿ������ 
#pragma once
#include "Stdafx.h"
class CRoleEx;
class RoleGiffManager
{
public:
	RoleGiffManager();
	virtual ~RoleGiffManager();
	bool OnInit(CRoleEx* pRole);
	bool OnLoadRoleGiff();
	void OnLoadRoleGiffResult(DBO_Cmd_LoadRoleGiff* pDB);
	void OnLoadRoleSendGiffInfo(DBO_Cmd_GetNowDayGiff* pDB);
	void GetRoleGiffToClient();
	void SendGiffToOtherRole(DWORD dwUserID);
	void SendGiffToOtherRoleResult(DWORD dwDestUserID);
	void AcceptRoleGiff(DWORD GiffID);
	void AcceptAllGiff();
	void OnDayChange();
	void AddBeGiff(DWORD SrcUserID);
	void AddBeGiffResult(DBO_Cmd_AddRoleGiff* pMsg);

	void SendNowDaySendGiffToClient();
	bool IsLoadDB(){ return m_IsLoadDB; }

	void ResetClientInfo(){ m_IsSendClient = false; }

	bool GetGiffMessageStates();
private:
	CRoleEx*					m_pRole;
	std::vector<tagGiffInfo>	m_GiffVec;//���е����͵ļ���
	bool						m_IsLoadDB;
	bool						m_IsSendClient;
 
	HashMap<DWORD, tagNowDaySendGiffInfo> m_SendInfo;//�����ҷ��͵����͵ļ���
	//HashMap<DWORD, BYTE>		m_SendInfo;
};