//����ʼ�����
#pragma once
#include "Stdafx.h"
#include <list>
class CRoleEx;
class RoleManager;
class RoleMailManager  //�������ȫ�����ʼ�ϵͳ ���ǿ��Կ�ʼ������
{
public:
	RoleMailManager();
	virtual ~RoleMailManager();

	bool OnInit(CRoleEx* pUser, RoleManager* pManager);//��ʼ��
	bool OnLoadUserMailByPage();//��ȡָ��ҳ�����ʼ�
	bool OnLoadUserMailByPageResult(DBO_Cmd_LoadUserMail* pDB);
	bool OnLoadUserMailRecordResult(DBO_Cmd_LoadUserMailRecord* pDB);
	
	//void OnLoadUserMailFinish();
	bool OnGetAllUserMail();
	//bool OnGetUserMailContext(DWORD MailID);
	bool OnGetUserMailItem(DWORD MailID);
	bool OnGetUserAllMailItem(BYTE type);
	bool OnDelUserMail(DWORD MailID);
	bool OnDelUserAllMail(BYTE type);
	//game server ����
	bool OnAddUserMail(tagRoleMail* pMail, DWORD	dwDestUserID);
	//bool OnAddUserMailResult(DBO_Cmd_AddUserMail* pMsg);
	//center server ����
	bool OnBeAddUserMail(tagRoleMail* pMail);
	bool OnBeAddUserMailResult(DBO_Cmd_AddUserMail* pMsg);
	//bool OnSendUserMail(CL_Cmd_SendUserMail* pMsg);

	//����ʼ������� ֻ�����������ڵ��ʼ�
	void OnDayChange();

	bool IsLoadDB(){ return m_IsLoadDB; }

	void ResetClientInfo(){ m_IsLoadToClient = false; }

	bool  GetMailMessageStates();
private:
	void OnSendAddMailToClient(tagRoleMail* pMail);
private:
	//std::list<tagRoleMail>		m_RoleMailVec;//����ʼ��ļ��� ʹ��List����  ��ȫ�����ʼ���˳���ȡ����
	std::list<tagRoleMail*>		m_RoleMailVec;//�ʼ��ļ���
	//std::list<tagRoleMailRecord*> m_RoleMailRecordVec;//�ʼ���¼����
	//����
	bool						m_IsLoadToClient;
	RoleManager*				m_RoleManager;
	CRoleEx*					m_pUser;
	bool						m_IsLoadDB;

	BYTE						m_ClientMailSum;
};