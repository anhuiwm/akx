#include "Stdafx.h"
#include "RoleVip.h"
#include "RoleEx.h"
#include "FishServer.h"
RoleVip::RoleVip()
{

}
RoleVip::~RoleVip()
{

}
bool RoleVip::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	m_pRole = pRole;
	DWORD RechargeValue = m_pRole->GetRoleInfo().TotalRechargeSum*10 + m_pRole->GetRoleInfo().NobilityPoint;//总充值的数值 我们想要继续处理
	BYTE VipLevel = 0;
	std::map<BYTE, tagVipOnce> sortVipMap;

	HashMap<BYTE, tagVipOnce>::iterator Iter= g_FishServer.GetFishConfig().GetVipConfig().VipMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end(); ++Iter)
	{
		sortVipMap.insert(std::make_pair(Iter->first, Iter->second));
		////便利循环VIP等级
		//if (RechargeValue >= Iter->second.NeedRechatgeRMBSum)
		//	VipLevel = Iter->first;
		//else
		//	break;
	}

	for (auto& itv : sortVipMap)
	{
		if (RechargeValue >= itv.second.NeedRechatgeRMBSum)
			VipLevel = itv.first;
		else
			break;
	}

	if (m_pRole->GetRoleInfo().VipLevel != VipLevel)
	{
		m_pRole->GetRoleInfo().VipLevel = VipLevel;

		DBR_Cmd_SaveRoleVipLevel msgDB;
		SetMsgInfo(msgDB, DBR_SaveRoleVipLevel, sizeof(DBR_Cmd_SaveRoleVipLevel));
		msgDB.VipLevel = m_pRole->GetRoleInfo().VipLevel;
		msgDB.UserID = m_pRole->GetUserID();
		g_FishServer.SendNetCmdToSaveDB(&msgDB);
	}
	return true;
}
void RoleVip::OnRechargeRMBChange()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	DWORD RechargeValue = m_pRole->GetRoleInfo().TotalRechargeSum*10 + m_pRole->GetRoleInfo().NobilityPoint;//总充值的数值 我们想要继续处理
	BYTE VipLevel = 0;
	std::map<BYTE, tagVipOnce> sortVipMap;
	HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end(); ++Iter)
	{
		sortVipMap.insert(std::make_pair(Iter->first, Iter->second));
		////便利循环VIP等级
		//if (RechargeValue >= Iter->second.NeedRechatgeRMBSum)
		//	VipLevel = Iter->first;
		//else
		//	break;
	}

	for (auto& itv : sortVipMap)
	{
		if (RechargeValue >= itv.second.NeedRechatgeRMBSum)
			VipLevel = itv.first;
		else
			break;
	}

	//std::cout<<"viplevel="<<VipLevel<<endl;
	//改变玩家VIP等级
	OnChangeRoleVipLevel(VipLevel);
}
bool RoleVip::OnChangeRoleVipLevel(BYTE VipLevel)
{
	if (VipLevel == 0)
	{
		if (m_pRole->GetRoleInfo().VipLevel == 0)
			return true;
	}
	else
	{
		if (VipLevel == m_pRole->GetRoleInfo().VipLevel)
			return true;
		HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(VipLevel);
		if (Iter == g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
			return false;
		if (m_pRole->GetRoleInfo().TotalRechargeSum*10 + m_pRole->GetRoleInfo().NobilityPoint < Iter->second.NeedRechatgeRMBSum)
			return false;
	}
	SendReward(m_pRole->GetRoleInfo().VipLevel, VipLevel);
	m_pRole->GetRoleLauncherManager().OnVipLevelChange(m_pRole->GetRoleInfo().VipLevel, VipLevel);//初始化的时候 无须修改
	m_pRole->GetRoleInfo().VipLevel = VipLevel;

	DBR_Cmd_SaveRoleVipLevel msgDB;
	SetMsgInfo(msgDB, DBR_SaveRoleVipLevel, sizeof(DBR_Cmd_SaveRoleVipLevel));
	msgDB.VipLevel = m_pRole->GetRoleInfo().VipLevel;
	msgDB.UserID = m_pRole->GetUserID();
	g_FishServer.SendNetCmdToSaveDB(&msgDB);

	//发送命令到中央服务器告诉玩家 VIP等级变化了
	CC_Cmd_ChangeRoleVipLevel msgCenter;
	SetMsgInfo(msgCenter, GetMsgType(Main_Role, CC_ChangeRoleVipLevel), sizeof(CC_Cmd_ChangeRoleVipLevel));
	msgCenter.dwUserID = m_pRole->GetUserID();
	msgCenter.VipLevel = m_pRole->GetRoleInfo().VipLevel;
	m_pRole->SendDataToCenter(&msgCenter);

	LC_Cmd_ChangeRoleVipLevel msg;
	SetMsgInfo(msg, GetMsgType(Main_Role, LC_ChangeRoleVipLevel), sizeof(LC_Cmd_ChangeRoleVipLevel));
	msg.VipLevel = m_pRole->GetRoleInfo().VipLevel;
	m_pRole->SendDataToClient(&msg);

	//发送同桌子上的玩家
	LC_Cmd_TableChangeRoleVipLevel msgTable;
	SetMsgInfo(msgTable, GetMsgType(Main_Table, LC_TableChangeRoleVipLevel), sizeof(LC_Cmd_TableChangeRoleVipLevel));
	msgTable.dwDestUserID = m_pRole->GetUserID();
	msgTable.VipLevel = m_pRole->GetRoleInfo().VipLevel;
	m_pRole->SendDataToTable(&msgTable);

	//GM_Cmd_RoleChangeVipLevel msgMini;
	//SetMsgInfo(msgMini, GetMsgType(Main_MiniGame, GM_RoleChangeVipLevel), sizeof(GM_Cmd_RoleChangeVipLevel));
	//msgMini.dwUserID = m_pRole->GetUserID();
	//msgMini.VipLevel = m_pRole->GetRoleInfo().VipLevel;
	//g_FishServer.SendNetCmdToMiniGame(&msgMini);

	g_FishServer.SendBroadCast(m_pRole, NoticeType::NT_ChangeVip);
	return true;
}


void RoleVip::SendReward(BYTE OldVipLevel, BYTE VipLevel)
{
	if (VipLevel == 0)
	{
		return ;
	}
	if (OldVipLevel == VipLevel )
	{
		return;
	}
	for (BYTE i = OldVipLevel+1; i <= VipLevel; i++)
	{
		HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(i);
		if (Iter == g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
			continue;
		//m_pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID, TEXT("vip升级奖励"));
		tagRoleMail	MailInfo;
		MailInfo.bIsRead = false;
		_sntprintf_s(MailInfo.Context, CountArray(MailInfo.Context), TEXT("恭喜您,获得贵族%u礼包!"), i);
		MailInfo.RewardID = Iter->second.RewardID;
		MailInfo.RewardSum = 1;
		MailInfo.MailID = 0;
		MailInfo.SendTimeLog = time(NULL);
		MailInfo.SrcFaceID = 0;
		TCHARCopy(MailInfo.SrcNickName, CountArray(MailInfo.SrcNickName), TEXT(""), 0);
		MailInfo.SrcUserID = 0;//系统发送
		MailInfo.bIsExistsReward = (MailInfo.RewardID != 0 && MailInfo.RewardSum != 0);
		DBR_Cmd_AddUserMail msg;
		SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
		msg.dwDestUserID = m_pRole->GetUserID();
		msg.MailInfo = MailInfo;
		g_FishServer.SendNetCmdToDB(&msg);
	}
}

BYTE RoleVip::GetLauncherReBoundNum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return g_FishServer.GetFishConfig().GetVipConfig().DefaultLauncherReBoundNum;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return g_FishServer.GetFishConfig().GetVipConfig().DefaultLauncherReBoundNum;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetLauncherReBoundNum(m_pRole->GetRoleInfo().VipLevel);
}

DWORD RoleVip::GetSendGoldBulletNum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 0;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetSendGoldBulletNum(m_pRole->GetRoleInfo().VipLevel);
}

DWORD RoleVip::GetSendSilverBulletNum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 0;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetSendSilverBulletNum(m_pRole->GetRoleInfo().VipLevel);
}

DWORD RoleVip::GetSendBronzeBulletNum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 0;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetSendBronzeBulletNum(m_pRole->GetRoleInfo().VipLevel);
}

DWORD RoleVip::GetSendItemNum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 0;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 0;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetSendItemNum(m_pRole->GetRoleInfo().VipLevel);
}

//bool  RoleVip::IsCanLauncherLocking()
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return false;
//	}
//	if (m_pRole->GetRoleInfo().VipLevel == 0)
//	{
//		return false;
//	}
//	return g_FishServer.GetFishConfig().GetVipConfig().IsCanLauncherLocking(m_pRole->GetRoleInfo().VipLevel);
//}
BYTE RoleVip::AddAlmsSum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return g_FishServer.GetFishConfig().GetVipConfig().DefaultAlmsSum;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return g_FishServer.GetFishConfig().GetVipConfig().DefaultAlmsSum;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().AddAlmsSum(m_pRole->GetRoleInfo().VipLevel);
}
float RoleVip::AddMonthScoreRate()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 1.0f;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 1.0f;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().AddMonthScoreRate(m_pRole->GetRoleInfo().VipLevel);
}
float RoleVip::AddReChargeRate()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 1.0f;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 1.0f;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().AddReChargeRate(m_pRole->GetRoleInfo().VipLevel);
}
float RoleVip::AddAlmsRate()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 1.0f;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 1.0f;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().AddAlmsRate(m_pRole->GetRoleInfo().VipLevel);
}
BYTE RoleVip::GetUseMedalSum()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return  g_FishServer.GetFishConfig().GetVipConfig().DefaultUseMedalSum;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return  g_FishServer.GetFishConfig().GetVipConfig().DefaultUseMedalSum;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().GetUseMedalSum(m_pRole->GetRoleInfo().VipLevel);
}
float RoleVip::AddCatchFishRate()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return 1.0f;
	}
	if (m_pRole->GetRoleInfo().VipLevel == 0)
	{
		return 1.0f;
	}
	return g_FishServer.GetFishConfig().GetVipConfig().AddCatchFishRate(m_pRole->GetRoleInfo().VipLevel);
}