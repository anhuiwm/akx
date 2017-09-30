#include "Stdafx.h"
#include "FishPackageManager.h"
#include "FishServer.h"
FishPackageManager::FishPackageManager()
{

}
FishPackageManager::~FishPackageManager()
{

}
bool FishPackageManager::IsPackageAndAutoUser(DWORD ItemID)
{
	HashMap<DWORD, tagFishPackage>::iterator Iter = g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.find(ItemID);
	if (Iter == g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.end())
		return false;
	return Iter->second.IsAutoOpen;
}
void FishPackageManager::OnOpenFishPackage(CRoleEx* pRole, DWORD ItemOnlyID, DWORD PackageItemID)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_OpenPackage msg;
	SetMsgInfo(msg, GetMsgType(Main_Package, LC_OpenPackage), sizeof(LC_Cmd_OpenPackage));
	msg.Result = false;
	if (!g_FishServer.GetFishConfig().ItemIsExists(PackageItemID))
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (!pRole->GetItemManager().OnDelUserItem(ItemOnlyID,PackageItemID, 1))//�ӱ����Ƴ� ������Ҹջ�õ�ʱ�� ������д���
	{
		ASSERT(false);
		pRole->SendDataToClient(&msg);
		return;
	}
	//���ݱ������� �򿪱���
	HashMap<DWORD, tagFishPackage>::iterator Iter= g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.find(PackageItemID);
	if (Iter == g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.end())
	{
		ASSERT(false);
		pRole->SendDataToClient(&msg);
		return;
	}
	if (Iter->second.ItemVec.empty())
	{
		ASSERT(false);
		pRole->SendDataToClient(&msg);
		return;
	}
	DWORD PageSize = sizeof(LC_Cmd_OpenPackage)+sizeof(WORD)* (Iter->second.ItemVec.size() - 1);
	CheckMsgSize(PageSize);
	LC_Cmd_OpenPackage* msgOpen = (LC_Cmd_OpenPackage*)malloc(PageSize);
	if (!msgOpen)
	{
		ASSERT(false);
		return;
	}
	msgOpen->SetCmdSize(ConvertDWORDToWORD(PageSize));
	msgOpen->SetCmdType(GetMsgType(Main_Package, LC_OpenPackage));
	msgOpen->Result = true;
	msgOpen->RewardSum = ConvertDWORDToBYTE(Iter->second.ItemVec.size());

	vector<tagFishPackageGroup>::iterator IterGroup = Iter->second.ItemVec.begin();
	for (int i =0; IterGroup != Iter->second.ItemVec.end(); ++IterGroup)
	{
		if (IterGroup->PackageItemVec.empty())
			continue;
		//�������һ��ֵ
		DWORD RankValue = RandUInt() % IterGroup->MaxRankValue;
		vector<tagFishPackageItem>::iterator IterItem = IterGroup->PackageItemVec.begin();
		for (; IterItem != IterGroup->PackageItemVec.end(); ++IterItem)
		{
			if (RankValue <= IterItem->RankValue)
			{
				//�����Ʒ��
				pRole->OnAddRoleRewardByRewardID(IterItem->RewardID,TEXT("�򿪰�����Ʒ ��ý���"));
				msgOpen->RewardID[i] = IterItem->RewardID;
				break;
			}
		}

		++i;
	}
	pRole->SendDataToClient(msgOpen);
	free(msgOpen);
}
void FishPackageManager::OnAutoOpenFishPackage(CRoleEx* pRole, DWORD PackageItemID)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	if (!g_FishServer.GetFishConfig().ItemIsExists(PackageItemID))
	{
		ASSERT(false);
		return;
	}
	//���ݱ������� �򿪱���
	HashMap<DWORD, tagFishPackage>::iterator Iter = g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.find(PackageItemID);
	if (Iter == g_FishServer.GetFishConfig().GetFishPackageConfig().m_PackageMap.end())
	{
		ASSERT(false);
		return;
	}
	if (Iter->second.ItemVec.empty())
	{
		ASSERT(false);
		return;
	}
	vector<tagFishPackageGroup>::iterator IterGroup = Iter->second.ItemVec.begin();
	for (; IterGroup != Iter->second.ItemVec.end(); ++IterGroup)
	{
		if (IterGroup->PackageItemVec.empty())
			continue;
		//�������һ��ֵ
		DWORD RankValue = RandUInt() % IterGroup->MaxRankValue;
		vector<tagFishPackageItem>::iterator IterItem = IterGroup->PackageItemVec.begin();
		for (; IterItem != IterGroup->PackageItemVec.end(); ++IterItem)
		{
			if (RankValue <= IterItem->RankValue)
			{
				//�����Ʒ��
				pRole->OnAddRoleRewardByRewardID(IterItem->RewardID, TEXT("�򿪰�����Ʒ ��ý���"));
				break;
			}
		}
	}
}