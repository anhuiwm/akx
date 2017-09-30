#pragma once
#include "PhoneSMSManager.h"
struct BindPhoneOnce
{
	DWORD				UserID;
	DWORD				GameServerClientID;
	unsigned __int64	PhoneNumber;
	DWORD				SendPhoneNumberOnlyID;//�Ƿ��Ѿ��ɹ�������֤����
	DWORD				BindNumber;//��֤��;//��֤��   1��ʾ�Ѿ�������֤�ɹ��� 0��Ȼʧ��
	WORD                Zone;
	DWORD			SecPasswordCrc1;
	DWORD			SecPasswordCrc2;
	DWORD			SecPasswordCrc3;
	BindPhoneOnce()
	{
		UserID = 0;
		GameServerClientID = 0;
		PhoneNumber = 0;
		SendPhoneNumberOnlyID = 0;
		BindNumber = 0;
		SecPasswordCrc1 = 0;
		SecPasswordCrc2 = 0;
		SecPasswordCrc3 = 0;
		Zone = 86;
	}
};
struct SendPhoneList
{
	std::vector<DWORD>   UserVec; //��ǰ���ڽ��е����
};
class BindPhoneManager
{
public:
	BindPhoneManager();
	virtual ~BindPhoneManager();

	bool  OnGetPhoneVerificationNumber(ServerClientData* pClient, GO_Cmd_GetPhoneVerificationNum* pMsg);//��ȡ�ֻ�����֤��
	void  OnCheckPhoneResult(DBO_Cmd_CheckPhone* pMsg);
	void  OnGetPhoneVerificationNumberResult(bool Result,BYTE ErrorID,DWORD dwUserID);//��ȡ�ֻ���֤��Ľ��

	bool OnBindPhontByVerificationNumber(ServerClientData* pClient, GO_Cmd_BindPhone* pMsg);//���ֻ�������֤ 

	bool CheckPhoneNumber(unsigned __int64  PhoneNumber);

	void OnUpdateSendPhoneSMS(DWORD dwTimer);

	void OnHandleSMSEvent(PhoneSMSOnceInfo pEvent, bool Result);
private:
	HashMap<DWORD, BindPhoneOnce>		m_BindPhoneMap;//�ֻ��󶨵Ķ���
	SendPhoneList						m_SendPhoneList;//��������֤����ֻ�����
};