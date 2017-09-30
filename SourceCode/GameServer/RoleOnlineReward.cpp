#include "Stdafx.h"
#include "RoleOnlineReward.h"
#include "RoleEx.h"
#include "FishServer.h"
RoleOnlineReward::RoleOnlineReward()
{

}
RoleOnlineReward::~RoleOnlineReward()
{

}
void RoleOnlineReward::OnGetOnlineReward(CRoleEx* pRole,BYTE ID)
{
	LC_Cmd_GetOnlineReward msg;
	SetMsgInfo(msg,GetMsgType(Main_OnlineReward, LC_GetOnlineReward), sizeof(LC_Cmd_GetOnlineReward));
	msg.RewardID = ID;
	if (!pRole)
	{
		ASSERT(false);
		return;
	}

	//没有找到配置
	HashMap<BYTE, tagOnceOnlienReward>::iterator Iter = g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.find(ID);
	if (Iter == g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.end())
	{
		ASSERT(false);
		msg.Result = false;
		pRole->SendDataToClient(&msg);
		return;
	}

	OnNoticeOnlineRewardComplete(pRole, ID);
	
	if ( !(pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1 + 16))) /*|| Iter->second.OnlineMin > OnLineMin*/)
	{//如果未完成
		//ASSERT(false);
		msg.Result = false;
		pRole->SendDataToClient(&msg);
		return;
	}

	//如果已经领取完成
	if (pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1)) /*|| Iter->second.OnlineMin > OnLineMin*/)
	{
		//ASSERT(false);
		msg.Result = false;
		pRole->SendDataToClient(&msg);
		return;
	}

	//需要记录玩家是否领取奖励了 配置数量不限制的情况下 我们需要循环变量玩家的奖励 32 DWORD  
	//在线时间足够  并且 未领取过奖励
	pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID,TEXT("领取在线奖励"));
	//2.奖励发送完毕后设置标记
	pRole->ChangeRoleOnlineRewardStates(pRole->GetRoleInfo().OnlineRewardStates | (1 << (ID - 1)));
	//3.发送命令到客户端去 玩家领取在线奖励成功了
	msg.Result = true;
	msg.States = pRole->GetRoleInfo().OnlineRewardStates;
	pRole->SendDataToClient(&msg);
	return;
}


void RoleOnlineReward::OnNoticeOnlineRewardComplete(CRoleEx* pRole, BYTE ID)
{

	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	//没有找到配置
	HashMap<BYTE, tagOnceOnlienReward>::iterator Iter = g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.find(ID);
	if (Iter == g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.end())
	{
		ASSERT(false);
		return;
	}

	if (Iter->second.OnlineSec == 0)
	{
		return;
	}

	
	if ((pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1 + 16))) /*|| Iter->second.OnlineMin > OnLineMin*/)
	{
		return;
	}

	//如果已经领取完成
	if (pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1)) /*|| Iter->second.OnlineMin > OnLineMin*/)
	{
		return;
	}

	if (pRole->GetRoleOnlineSec()+1 < Iter->second.OnlineSec)
	{
		return;
	}

	pRole->ChangeRoleOnlineRewardStates(pRole->GetRoleInfo().OnlineRewardStates | (1 << (ID - 1) + 16));
	return;
}



void RoleOnlineReward::OnGetAllOnlineReward(CRoleEx* pRole)
{
	HashMap<BYTE, tagOnceOnlienReward>::iterator Iter = g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.begin();
	vector<BYTE> vecID;
	DWORD tempStatus = pRole->GetRoleInfo().OnlineRewardStates;
	LogInfoToFile("WmOnline.txt", "OnGetAllOnlineReward::starttempStatus=%d", tempStatus);
 	for(;Iter != g_FishServer.GetFishConfig().GetOnlineRewardConfig().m_OnlineRewardMap.end(); ++Iter)
	{
		BYTE ID = Iter->first;
		OnNoticeOnlineRewardComplete(pRole, ID);
		//如果完成
		if (!(pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1 + 16))) /*|| Iter->second.OnlineMin > OnLineMin*/)
		{
			//ASSERT(false);
			//msg.Result = false;
			//pRole->SendDataToClient(&msg);
			continue;
		}

		//如果已经领取完成
		if (pRole->GetRoleInfo().OnlineRewardStates & (1 << (ID - 1)) /*|| Iter->second.OnlineMin > OnLineMin*/)
		{
			//ASSERT(false);
			//msg.Result = false;
			//pRole->SendDataToClient(&msg);
			continue;
		}
		//需要记录玩家是否领取奖励了 配置数量不限制的情况下 我们需要循环变量玩家的奖励 32 DWORD  
		//在线时间足够  并且 未领取过奖励
		pRole->OnAddRoleRewardByRewardID(Iter->second.RewardID, TEXT("领取在线奖励"));
		tempStatus = tempStatus | (1 << (ID - 1));
		//2.奖励发送完毕后设置标记 不多  所以每次都发
		vecID.push_back(ID);
		LogInfoToFile("WmOnline.txt", "OnGetAllOnlineReward::ID=%d", ID);
	}
	LogInfoToFile("WmOnline.txt", "OnGetAllOnlineReward::endtempStatus=%d", tempStatus);
	if (vecID.empty())
	{
		ASSERT(false);
		return;
	}
	//pRole->ChangeRoleOnlineRewardStates(pRole->GetRoleInfo().OnlineRewardStates | (1 << (ID - 1)));
	pRole->ChangeRoleOnlineRewardStates(tempStatus);

	DWORD PageSize = sizeof(LC_Cmd_GetAllOnlineReward) + (vecID.size() - 1) * sizeof(BYTE);
	LC_Cmd_GetAllOnlineReward* pMsg = (LC_Cmd_GetAllOnlineReward*)malloc(PageSize);
	if (!pMsg)
	{
		ASSERT(false);
		return;
	}
	WORD Sum = 0;
	pMsg->SetCmdType(GetMsgType(Main_OnlineReward, LC_GetAllOnlineReward));
	for (vector<BYTE>::iterator Iter=vecID.begin(); Iter != vecID.end() ; ++Iter,Sum++)
	{
		pMsg->Array[Sum] = *Iter;
	}
	PageSize = sizeof(LC_Cmd_GetAllOnlineReward) + (Sum - 1) * sizeof(BYTE);
	std::vector<LC_Cmd_GetAllOnlineReward*> pVec;
	SqlitMsg(pMsg, PageSize, Sum, true, pVec);
	free(pMsg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetAllOnlineReward*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			pRole->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}

}