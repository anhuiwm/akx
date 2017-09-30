#include "stdafx.h"
#include "GmManager.h"
#include "MainServer.h"
#include "..\CommonFile\FishServerConfig.h"
extern void SendLogDB(NetCmd* pCmd);
GmManager::GmManager()
{
}
GmManager::~GmManager()
{
}
void GmManager::OnInit()
{

}

unsigned char GmManager::ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char GmManager::FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string GmManager::UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ')
			strTemp += "+";
		else
		{
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] % 16);
		}
	}
	return strTemp;
}

std::string GmManager::UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

TCHAR* GmManager::TransformUTF8ToUnicodeM(const char* _str)
{
	int textlen = 0;
	wchar_t * result = NULL;

	if (_str)
	{
		textlen = MultiByteToWideChar(CP_UTF8, 0, _str, -1, NULL, 0);
		result = (wchar_t *)malloc((textlen + 1) * sizeof(wchar_t));
		memset(result, 0, (textlen + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, _str, -1, (LPWSTR)result, textlen);
	}

	return result;
}

bool GmManager::GmCharge(string& id, string& num, string& content, string& target)
{
	//DWORD dPrice = strtoul(type.c_str(), null, 10);
	DWORD UserID = strtoul(id.c_str(), null, 10);
	DWORD ShopItemID = strtoul(num.c_str(), null, 10);

	//DWORD dPrice = 0;
	DWORD FreePrice = 0;

	string ChannelCode = "gm";
	string channelOrderid = "gm" + std::to_string(time(0));
	string channelLabel = ChannelCode;
	string orderid = channelOrderid;
	//st 是否成功标志，1标示成功，其余都表示失败
	HashMap<DWORD, tagFishRechargeInfo>::iterator Iter = g_FishServer.GetFishConfig().GetFishRechargesConfig().m_FishRechargeMap.find(ShopItemID);
	if (Iter == g_FishServer.GetFishConfig().GetFishRechargesConfig().m_FishRechargeMap.end())//物品不存在
	{
		LogInfoToFile("WmLogErrorGm.txt", "UserID=%u,ShopItemID=%u is not exists",UserID, ShopItemID);
		//g_DBLogManager.LogUserRechargeLogToDB("运营服务器验证失败:物品不存在", orderid, UserID, ChannelCode, channelOrderid, channelLabel, ShopItemID, dPrice, FreePrice, 0, 0, 0, 0, 0, SendLogDB);
		return false;
	}
	DWORD dPrice = Iter->second.dDisCountPrice * 100;
	//if (Iter->second.dDisCountPrice * 100 != dPrice)//价格不正确
	//{
	//	g_DBLogManager.LogUserRechargeLogToDB("运营服务器验证失败:价格不正确", orderid, UserID, ChannelCode, channelOrderid, channelLabel, ShopItemID, dPrice, FreePrice, 0, 0, 0, 0, 0, SendLogDB);
	//	return false;
	//}

	////验证签名
	//OperateConfig* pConfig = g_FishServerConfig.GetOperateConfig();
	//if (!pConfig)
	//{
	//	//g_DBLogManager.LogUserRechargeLogToDB("运营服务器验证失败:系统错误", orderid, UserID, ChannelCode, channelOrderid, channelLabel, ShopItemID, dPrice, FreePrice, 0, 0, 0, 0, 0, SendLogDB);
	//	return false;
	//}

	//string Product = pConfig->ProductSecret;


	//string CheckStr = "app=" + app + "&cbi=" + cbi + "&ct=" + ct + "&fee=" + fee + "&pt=" + pt + "&sdk=" + sdk + "&ssid=" + ssid + "&st=" + st + "&tcd=" + tcd + "&uid=" + uid + "&ver=" + ver;
	//string Key = "1LB8K19BXX2XCWXYXSX1X4XD5REHEF9Q";
	//CheckStr = CheckStr + Key;
	//string Md5Str = md5(CheckStr);//加密后的签名

	//LogInfoToFile("WmRechargeInfo", "checkstr=%s,Md5str=%s,sign=%s",
	//	CheckStr.c_str(), Md5Str.c_str(), sign.c_str());

	//if (Md5Str.compare(sign) != 0)//验证签名是否正确
	//{
	//	g_DBLogManager.LogUserRechargeLogToDB("运营服务器验证失败:MD5验证失败", orderid, UserID, ChannelCode, channelOrderid, channelLabel, ShopItemID, dPrice, FreePrice, 0, 0, 0, 0, 0, SendLogDB);
	//	return false;
	//}


	//AE_CRC_PAIRS pThree;
	//AECrc32(pThree, orderid.c_str(), orderid.length(), 0, 0x73573);
	////订单转化的CRC 数据
	//DWORD Crc1 = pThree.Crc1;
	//DWORD Crc2 = pThree.Crc2;
	//unsigned __int64 i64Value = Crc1;
	//i64Value = (i64Value << 32);
	//i64Value += Crc2;
	//获得订单的唯一编号 我们写入数据库 或者进行判断

	//if (g_FishServer.GetOrderOnlyManager().IsExists(i64Value, OT_SDK))
	//{
	//	g_DBLogManager.LogUserRechargeLogToDB("运营服务器验证失败:重复的订单", orderid, UserID, ChannelCode, channelOrderid, channelLabel, ShopItemID, dPrice, FreePrice, 0, 0, 0, 0, 0, SendLogDB);
	//	//LogInfoToFile("RechargeInfo", "充值异步回调 订单重复执行: orderid=%s", orderid.c_str());
	//	return false;//订单号存在
	//}

	//g_FishServer.GetOrderOnlyManager().OnAddOrderInfo(UserID, i64Value, OT_SDK);

	UINT Count = 0;
	TCHAR* pOrderID = CharToWChar(orderid.c_str(), Count);
	TCHAR* pChannelCode = CharToWChar(ChannelCode.c_str(), Count);
	TCHAR* pChannelOrderid = CharToWChar(channelOrderid.c_str(), Count);
	TCHAR* pChannelLabel = CharToWChar(channelLabel.c_str(), Count);

	DWORD Length = sizeof(StringArrayData) + (orderid.length() - 1) * sizeof(TCHAR) +
		sizeof(StringArrayData) + (ChannelCode.length() - 1) * sizeof(TCHAR) +
		sizeof(StringArrayData) + (channelOrderid.length() - 1) * sizeof(TCHAR) +
		sizeof(StringArrayData) + (channelLabel.length() - 1) * sizeof(TCHAR);


	DWORD PageSize = sizeof(OC_Cmd_UseRMB) - sizeof(BYTE) + Length;
	OC_Cmd_UseRMB* msg = (OC_Cmd_UseRMB*)malloc(PageSize);
	CheckMsgSizeReturn(PageSize);
	msg->SetCmdType(GetMsgType(Main_Control, LC_GM_Charge));
	msg->SetCmdSize(static_cast<WORD>(PageSize));
	msg->rechargeInfo.Price = dPrice;
	msg->rechargeInfo.UserID = UserID;
	msg->rechargeInfo.ShopItemID = ShopItemID;
	msg->rechargeInfo.HandleSum = 4;
	msg->rechargeInfo.Sum = Length;
	msg->rechargeInfo.FreePrice = FreePrice;
	DWORD BeginIndex = 0;
	{
		StringArrayData* pString = (StringArrayData*)&msg->rechargeInfo.Array[BeginIndex];
		pString->Length = ConvertDWORDToBYTE(orderid.length() * sizeof(TCHAR));
		memcpy_s(pString->Array, orderid.length() * sizeof(TCHAR), pOrderID, orderid.length() * sizeof(TCHAR));
		BeginIndex += sizeof(StringArrayData) + (orderid.length() - 1) * sizeof(TCHAR);
	}
	{
		StringArrayData* pString = (StringArrayData*)&msg->rechargeInfo.Array[BeginIndex];
		pString->Length = ConvertDWORDToBYTE(ChannelCode.length() * sizeof(TCHAR));
		memcpy_s(pString->Array, ChannelCode.length() * sizeof(TCHAR), pChannelCode, ChannelCode.length() * sizeof(TCHAR));
		BeginIndex += sizeof(StringArrayData) + (ChannelCode.length() - 1) * sizeof(TCHAR);
	}
	{
		StringArrayData* pString = (StringArrayData*)&msg->rechargeInfo.Array[BeginIndex];
		pString->Length = ConvertDWORDToBYTE(channelOrderid.length() * sizeof(TCHAR));
		memcpy_s(pString->Array, channelOrderid.length() * sizeof(TCHAR), pChannelOrderid, channelOrderid.length() * sizeof(TCHAR));
		BeginIndex += sizeof(StringArrayData) + (channelOrderid.length() - 1) * sizeof(TCHAR);
	}
	{
		StringArrayData* pString = (StringArrayData*)&msg->rechargeInfo.Array[BeginIndex];
		pString->Length = ConvertDWORDToBYTE(channelLabel.length() * sizeof(TCHAR));
		memcpy_s(pString->Array, channelLabel.length() * sizeof(TCHAR), pChannelLabel, channelLabel.length() * sizeof(TCHAR));
		BeginIndex += sizeof(StringArrayData) + (channelLabel.length() - 1) * sizeof(TCHAR);
	}

	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);//将玩家充值的数据发送到 GameServer去

	return true;
}
bool GmManager::GmMail(string& id, string& num, string& content, string& target)
{
	string decontent = UrlDecode(content);
	DWORD RewardID = strtoul(id.c_str(), null, 10);
	DWORD Num = strtoul(num.c_str(), null, 10);
	DWORD DestUserID = strtoul(target.c_str(), null, 10);

	DWORD Size = sizeof(CL_Cmd_SendSystemEmail) + (content.length() - 1) * sizeof(TCHAR);;
	CL_Cmd_SendSystemEmail* msg = (CL_Cmd_SendSystemEmail*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_SendSystemEmail));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->dwUserID = DestUserID;
	msg->RewardID = RewardID;
	msg->RewardSum = Num;

	TCHAR* con = GmManager::TransformUTF8ToUnicodeM(decontent.c_str());
	msg->ContextSize = _tcslen(con);
	memcpy_s(msg->EmailContext, msg->ContextSize * sizeof(TCHAR), con, msg->ContextSize * sizeof(TCHAR));
	g_FishServer.SendNetCmdToCenter(msg);
	free(con);
	free(msg);

	return true;
}

bool GmManager::GmSendBroad(string& id, string& num, string& content, string& target)
{
	//DWORD RewardID = strtoul(id.c_str(), null, 10);
	DWORD Num = strtoul(num.c_str(), null, 10);
	//DWORD DestUserID = strtoul(target.c_str(), null, 10);

	DWORD Size = sizeof(CL_Cmd_SendMsgToAllGame)+ (content.length()-1) * sizeof(TCHAR);
	CL_Cmd_SendMsgToAllGame* msg = (CL_Cmd_SendMsgToAllGame*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_SendMsgToAllGame));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->MessageColor = 4294967295;
	msg->Param = 0;
	msg->StepSec = 3;
	msg->StepNum = Num;

	string decontent = UrlDecode(content);
	TCHAR* con = GmManager::TransformUTF8ToUnicodeM(decontent.c_str());
	msg->MessageSize = _tcslen(con);
	memcpy_s(msg->CenterMessage, msg->MessageSize * sizeof(TCHAR), con, msg->MessageSize * sizeof(TCHAR));
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	free(con);

	return true;
}

bool GmManager::GmKick(string& id, string& num, string& content, string& target)
{
	//DWORD RewardID = strtoul(id.c_str(), null, 10);
	DWORD FreezeMin = strtoul(num.c_str(), null, 10);
	DWORD DestUserID = strtoul(target.c_str(), null, 10);
	DWORD Size = sizeof(CL_Cmd_KickUserByID);
	CL_Cmd_KickUserByID* msg = (CL_Cmd_KickUserByID*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_KickUserByID));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->dwUserID = DestUserID;
	msg->ClientID = 0;
	msg->FreezeMin = FreezeMin;
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	return true;
}

bool GmManager::GmReloadconfig(string& id, string& num, string& content, string& target)
{
	DWORD Size = sizeof(NetCmd);
	NetCmd* msg = (NetCmd*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_ReloadConfig));
	msg->SetCmdSize(static_cast<WORD>(Size));
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	return true;
}

bool GmManager::GmHandleEntityItem(string& id, string& num, string& content, string& target)
{
	DWORD EID = strtoul(id.c_str(), null, 10);
	DWORD ClientIP = strtoul(target.c_str(), null, 10);
	DWORD Size = sizeof(CL_Cmd_HandleEntityItem);
	CL_Cmd_HandleEntityItem* msg = (CL_Cmd_HandleEntityItem*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_HandleEntityItem));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->EID  = EID;
	msg->ClientIP = ClientIP;
	msg->ClientID = 0;
	memcpy_s(msg->OrderNumber, 32, target.c_str(), 32);
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	return true;
}


bool GmManager::GmSetBlackList(string& id, string& num, string& content, string& target)
{
	DWORD UserID = strtoul(target.c_str(), null, 10);
	DWORD Size = sizeof(LC_CMD_SetFishBlackList);
	LC_CMD_SetFishBlackList* msg = (LC_CMD_SetFishBlackList*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_SetBlackList));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->dwUserID = UserID;
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	return true;
}

bool GmManager::GmUnSetBlackList(string& id, string& num, string& content, string& target)
{
	DWORD UserID = strtoul(target.c_str(), null, 10);
	DWORD Size = sizeof(LC_CMD_UnSetFishBlackList);
	LC_CMD_UnSetFishBlackList* msg = (LC_CMD_UnSetFishBlackList*)malloc(Size);
	CheckMsgSizeReturn(Size);
	msg->SetCmdType(GetMsgType(Main_Control, CL_UnSetBlackList));
	msg->SetCmdSize(static_cast<WORD>(Size));
	msg->dwUserID = UserID;
	g_FishServer.SendNetCmdToCenter(msg);
	free(msg);
	return true;
}


bool GmManager::OnHandleHttpInfoByGm(string type, string id, string num, string content, string target,string sign)
{
	LogInfoToFile("WmLogGm.txt", "GM:type=%s,id=%s,num=%s,content=%s",	type.c_str(), id.c_str(),num.c_str(),content.c_str());
	string CheckStr = type + "DMQby321MiniCMS";
	string Md5Str = md5(CheckStr);//加密后的签名
	if (Md5Str.compare(sign) != 0)//验证签名是否正确
	{
		LogInfoToFile("WmLogErrorGm.txt", "GM:type=%s,id=%s,num=%s,content=%s,sign=%s", type.c_str(), id.c_str(), num.c_str(), content.c_str(),sign.c_str());
		return false;
	}

	DWORD type_value = strtoul(type.c_str(), null, 10);
	switch (type_value)
	{
	case  GT_Charge:
	{
		return GmCharge(id, num, content, target);
	}
	case GT_Mail:
	{
		return GmMail(id, num, content, target);
	}
	case GT_SendBroad:
	{
		return GmSendBroad(id, num, content, target);
	}
	case GT_Kick:
	{
		return GmKick(id, num, content, target);
	}
	case GT_ReloadConfig:
	{
		return GmReloadconfig(id, num, content, target);
	}
	case GT_HandleEntityItem:
	{
		return GmHandleEntityItem(id, num, content, target);
	} 
	case GT_SetBlackList:
	{
		return GmSetBlackList(id, num, content, target);
	}
	case GT_UnSetBlackList:
	{
		return GmUnSetBlackList(id, num, content, target);
	}
	default:
	{
		return false;
	}
	}
	//return true;
}