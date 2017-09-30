#pragma once
#include "PhoneSMSManager.h"
struct BindPhoneOnce
{
	DWORD				UserID;
	DWORD				GameServerClientID;
	unsigned __int64	PhoneNumber;
	DWORD				SendPhoneNumberOnlyID;//是否已经成功发送验证码了
	DWORD				BindNumber;//验证码;//验证码   1表示已经接收验证成功了 0依然失败
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
	std::vector<DWORD>   UserVec; //当前正在进行的玩家
};
class BindPhoneManager
{
public:
	BindPhoneManager();
	virtual ~BindPhoneManager();

	bool  OnGetPhoneVerificationNumber(ServerClientData* pClient, GO_Cmd_GetPhoneVerificationNum* pMsg);//获取手机的验证码
	void  OnCheckPhoneResult(DBO_Cmd_CheckPhone* pMsg);
	void  OnGetPhoneVerificationNumberResult(bool Result,BYTE ErrorID,DWORD dwUserID);//获取手机验证码的结果

	bool OnBindPhontByVerificationNumber(ServerClientData* pClient, GO_Cmd_BindPhone* pMsg);//对手机进行验证 

	bool CheckPhoneNumber(unsigned __int64  PhoneNumber);

	void OnUpdateSendPhoneSMS(DWORD dwTimer);

	void OnHandleSMSEvent(PhoneSMSOnceInfo pEvent, bool Result);
private:
	HashMap<DWORD, BindPhoneOnce>		m_BindPhoneMap;//手机绑定的订单
	SendPhoneList						m_SendPhoneList;//待发送验证码的手机集合
};