#include "Stdafx.h"
#include "LotteryManager.h"
#include "FishServer.h"
#include "..\CommonFile\DBLogManager.h"
extern void SendLogDB(NetCmd* pCmd);
LotteryManager::LotteryManager()
{

}
LotteryManager::~LotteryManager()
{

}
void LotteryManager::OnRoleGetLotteryReward(DWORD dwUserID, BYTE LotteryID)
{
	//��һ�ý���
	CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(dwUserID);
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	HashMap<BYTE, tagLotteryOnce>::iterator Iter = g_FishServer.GetFishConfig().GetLotteryConfig().LotteryMap.find(LotteryID);
	if (Iter == g_FishServer.GetFishConfig().GetLotteryConfig().LotteryMap.end())
	{
		ASSERT(false);
		return;
	}
	if (pRole->GetRoleInfo().LotteryFishSum < g_FishServer.GetFishConfig().GetLotteryConfig().MaxLotteryFishSum)
	{
		//ASSERT(false);
		LC_Cmd_GetLotteryReward msg;
		SetMsgInfo(msg, GetMsgType(Main_Lottery, LC_GetLotteryReward), sizeof(LC_Cmd_GetLotteryReward));
		msg.LotteryID = LotteryID;
		msg.RewardID = 0;
		msg.Result = false;
		pRole->SendDataToClient(&msg);
		return;
	}
	if (pRole->GetRoleInfo().LotteryScore < Iter->second.NeedUseScore)
	{
		LC_Cmd_GetLotteryReward msg;
		SetMsgInfo(msg, GetMsgType(Main_Lottery, LC_GetLotteryReward), sizeof(LC_Cmd_GetLotteryReward));
		msg.LotteryID = LotteryID;
		msg.RewardID = 0;
		msg.Result = false;
		pRole->SendDataToClient(&msg);
		return;
	}
	//����������� ���ǽ��д���
	WORD RewardID = 0;
	DWORD RankValue = (RandUInt() % Iter->second.TotalRate);
	RankValue = min((DWORD)pRole->GetRoleMonthCard().AddLotteryRate() * RankValue * RankValue, Iter->second.TotalRate - 1);
	vector<tagLotteryReward>::iterator IterReward = Iter->second.RewardVec.begin();
	for (; IterReward != Iter->second.RewardVec.end(); ++IterReward)
	{
		if (RankValue <= IterReward->Rate)
		{
			//��ǰ������
			RewardID = IterReward->RewardID;
			break;
		}
	}
	if (RewardID == 0)
	{
		ASSERT(false);
		LC_Cmd_GetLotteryReward msg;
		SetMsgInfo(msg, GetMsgType(Main_Lottery, LC_GetLotteryReward), sizeof(LC_Cmd_GetLotteryReward));
		msg.LotteryID = LotteryID;
		msg.RewardID = RewardID;
		msg.Result = false;
		pRole->SendDataToClient(&msg);
	}
	else
	{
		//pRole->GetRoleMessageStates().OnChangeRoleMessageStates(RMT_OnlineReward);
		//�۳���һ���
		pRole->OnClearRoleLotteryInfo();//�����ҳ齱������
		//�����ҽ���
		pRole->OnAddRoleRewardByRewardID(RewardID, TEXT("�齱��ý���"));

		g_DBLogManager.LogToDB(dwUserID, LT_Lottery, LotteryID, RewardID, TEXT("��ҽ��г齱"), SendLogDB);

		g_DBLogManager.LogLotteryInfoToDB(dwUserID, LotteryID, RewardID, SendLogDB);

		//��������ͻ���
		LC_Cmd_GetLotteryReward msg;
		SetMsgInfo(msg, GetMsgType(Main_Lottery, LC_GetLotteryReward), sizeof(LC_Cmd_GetLotteryReward));
		msg.LotteryID = LotteryID;
		msg.RewardID = RewardID;
		msg.Result = true;
		pRole->SendDataToClient(&msg);
	}
}
