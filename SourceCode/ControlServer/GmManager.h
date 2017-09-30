#pragma once
//#include "stdafx.h"

#define TYPE     "type"
#define ID		 "id"
#define NUM		 "num"
#define CONTENT  "content"
#define TARGET	 "target"

struct GmCrcInfo
{
    DWORD      type  ;
	DWORD      id	;
	DWORD      num	;
	DWORD      content;
	DWORD      target;

	GmCrcInfo()
	{
		type  = AECrc32(TYPE,strlen(TYPE),0);
		id = AECrc32(ID, strlen(ID), 0);
		num = AECrc32(NUM, strlen(NUM), 0);
		content = AECrc32(CONTENT, strlen(CONTENT), 0);
		target = AECrc32(TARGET, strlen(TARGET), 0);
	}
};

enum GM_TYPE
{
	GT_Charge = 1,
	GT_Mail = 2,
	GT_SendBroad = 3,
	GT_Kick = 4,
	GT_ReloadConfig = 5,
	GT_HandleEntityItem = 6,

};
class GmManager
{
public:
	GmManager();
	virtual ~GmManager();

	GmCrcInfo& GetGmCrcInfo(){ return m_GmCrcInfo; }

	static unsigned char GmManager::ToHex(unsigned char x);
	static unsigned char GmManager::FromHex(unsigned char x);
	static std::string GmManager::UrlEncode(const std::string& str);
	static std::string GmManager::UrlDecode(const std::string& str);
	TCHAR* TransformUTF8ToUnicodeM(const char* _str);

	void OnInit();
	bool OnHandleHttpInfoByGm(string type, string id, string num, string content, string target);
	bool GmCharge(string& id, string& num, string& content, string& target);
	bool GmMail(string& id, string& num, string& content, string& target);
	bool GmSendBroad(string& id, string& num, string& content, string& target);
	bool GmKick(string& id, string& num, string& content, string& target);
	bool GmReloadconfig(string& id, string& num, string& content, string& target);
	bool GmHandleEntityItem(string& id, string& num, string& content, string& target);

private:
	GmCrcInfo						m_GmCrcInfo;

};