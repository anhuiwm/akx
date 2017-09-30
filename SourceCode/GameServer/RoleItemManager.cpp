#include "Stdafx.h" 
#include "RoleItemManager.h"
#include "RoleEx.h"
#include "RoleManager.h"
#include "FishServer.h"
#include "..\CommonFile\DBLogManager.h"
extern void SendLogDB(NetCmd* pCmd);
RoleItemManger::RoleItemManger()
{
	m_IsLoadToClient = false;
	m_IsLoadDB = false;
	m_RoleManager = NULL;
	m_pUser = NULL;
	m_ItemMap.clear();
}
RoleItemManger::~RoleItemManger()
{
	if (!m_ItemMap.empty())
	{
		HashMap<DWORD, tagItemType*>::iterator  Iter = m_ItemMap.begin();
		for (; Iter != m_ItemMap.end(); ++Iter)
		{
			delete Iter->second;
		}
		m_ItemMap.clear();
	}
}
bool RoleItemManger::IsExistsItem(DWORD ItemID)
{
	return g_FishServer.GetFishConfig().ItemIsExists(ItemID);
}
bool RoleItemManger::IsCanUseItem(DWORD ItemID)
{
	if (!IsExistsItem(ItemID))
		return false;
	BYTE ItemTypeID = g_FishServer.GetFishConfig().GetItemType(ItemID);
	if (ItemTypeID == 0)
		return false;
	if (ItemTypeID == EItemType::IT_Package || ItemTypeID == EItemType::IT_MonthCard || ItemTypeID == EItemType::IT_GlobelBag)
		return true;
	else
		return false;
}
bool RoleItemManger::IsCanAcceptItem(DWORD ItemID, DWORD ItemSum)
{
	if (!IsExistsItem(ItemID))
		return false;
	BYTE ItemTypeID = g_FishServer.GetFishConfig().GetItemType(ItemID);
	if (ItemTypeID == 0)
		return false;

	if (!g_FishServer.GetFishConfig().CanSendItemType(ItemTypeID))
		return false;
	//if (ItemTypeID == EItemType::IT_GlobelBag)
	//	return true;
	//else
	//	return false;

	if (QueryItemCount(ItemID) < ItemSum)
		return false;

	if (m_pUser->GetRoleVip().GetSendItemNum() == 0)
	{
		return false;
	}

	if (ItemID == GetBronzeBulletID())
	{
		if (m_pUser->GetRoleInfo().SendBronzeBulletNum + ItemSum > m_pUser->GetRoleVip().GetSendBronzeBulletNum())
		{
			return false;
		}
	}
	else if (ItemID == GetSilverBulletID())
	{
		if (m_pUser->GetRoleInfo().SendSilverBulletNum + ItemSum > m_pUser->GetRoleVip().GetSendSilverBulletNum())
		{
			return false;
		}
	}
	else if (ItemID == GetGoldBulletID())
	{
		if (m_pUser->GetRoleInfo().SendGoldBulletNum + ItemSum > m_pUser->GetRoleVip().GetSendGoldBulletNum())
		{
			return false;
		}
	}

	return true;
}
bool RoleItemManger::OnUseItem(DWORD ItemOnlyID,DWORD ItemID, DWORD ItemSum)
{
	if (!IsCanUseItem(ItemID))
		return false;
	if (!m_pUser)
		return false;
	tagItemConfig* pItemConfig = g_FishServer.GetFishConfig().GetItemInfo(ItemID);
	if (!pItemConfig)
		return false;
	if (pItemConfig->ItemType == EItemType::IT_Package)
	{
		for (size_t i = 0; i < ItemSum; ++i)
			g_FishServer.GetPackageManager().OnOpenFishPackage(m_pUser, ItemOnlyID, ItemID);//打开礼包物品
	}
	else if (pItemConfig->ItemType == EItemType::IT_MonthCard)
	{
		//玩家使用月卡物品 我们进行处理
		if (ItemSum > 1)
		{
			return false;
		}
		if (m_pUser->GetRoleInfo().MonthCardID != 0 && time(null) < m_pUser->GetRoleInfo().MonthCardEndTime)
		{
			return false;
		}
		if (OnDelUserItem(ItemOnlyID, ItemID, ItemSum))
		{
			if (!m_pUser->GetRoleMonthCard().SetRoleMonthCardInfo(ConvertDWORDToBYTE(pItemConfig->ItemParam)))
			{
				tagItemOnce pOnce;
				pOnce.ItemID = ItemID;
				pOnce.ItemSum = ItemSum;
				OnAddUserItem(pOnce);
				ASSERT(false);
				return false;
			}
			return true;
		}
		else
			return false;
	}
	else if (pItemConfig->ItemType == EItemType::IT_GlobelBag)
	{
		//对物品 添加是否可以赠送物品
		if (OnDelUserItem(ItemOnlyID, ItemID, ItemSum))
		{
			//物品删除成功后 我们开始给玩家添加金钱
			WORD RewardID = static_cast<WORD>(pItemConfig->ItemParam>>16);
			m_pUser->OnAddRoleRewardByRewardID(RewardID, TEXT("领取聚宝盆奖励"), ItemSum);

			g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(ItemSum),0, TEXT("使用聚宝盆"), SendLogDB);

			return true;
		}
		else
			return false;
	}
	else
	{
		ASSERT(false);
		return false;
	}
	return true;
}
bool RoleItemManger::OnInit(CRoleEx* pUser, RoleManager* pManager)
{
	if (!pUser || !pManager)
	{
		ASSERT(false);
		return false;
	}
	m_RoleManager = pManager;
	m_pUser = pUser;
	return OnLoadUserItem();
}
bool RoleItemManger::OnLoadUserItem()
{
	//向数据库加载玩家的物品数据 
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	m_ItemMap.clear();
	/*DBR_Cmd_LoadUserItem msg;
	SetMsgInfo(msg,DBR_LoadUserItem, sizeof(DBR_Cmd_LoadUserItem));
	msg.dwUserID = m_pUser->GetUserID();
	g_FishServer.SendNetCmdToDB(&msg);*/
	return true;
}
void RoleItemManger::OnLoadUserItemResult(DBO_Cmd_LoadUserItem* pDB)
{
	if (!pDB || !m_pUser)
	{
		ASSERT(false);
		return;
	}
	if ((pDB->States & 1) != 0)
	{
		m_ItemMap.clear();
	}
	for (size_t i = 0; i < pDB->Sum; ++i)
	{
		if (!IsExistsItem(pDB->Array[i].ItemID))
			continue;
		HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(pDB->Array[i].ItemID);
		if (IterFind == m_ItemMap.end())
		{
			tagItemType* pItem = new tagItemType;
			pItem->OnInit(m_pUser, this);
			pItem->LoadItem(pDB->Array[i]);
			m_ItemMap.insert(HashMap<DWORD, tagItemType*>::value_type(pDB->Array[i].ItemID, pItem));
		}
		else
		{
			IterFind->second->LoadItem(pDB->Array[i]);
		}
	}
	if ((pDB->States & 2) != 0)
	{
		//所有的物品已经加载完毕了
		m_IsLoadDB = true;
		m_pUser->IsLoadFinish();
	}
}
//void RoleItemManger::OnLoadUserItemFinish()
//{
//	if (!m_pUser)
//	{
//		ASSERT(false);
//		return;
//	}
//	
//}
bool RoleItemManger::OnTryAcceptItemToFriend(DWORD DestUserID,DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}

	if (!IsCanAcceptItem(ItemID, ItemSum))
	{
		ASSERT(false);
		return false;
	}

	//发送邮件给玩家
	tagRoleMail	MailInfo;
	MailInfo.bIsRead = false;
	//比赛的内容需要特殊的处理 我们想要一个 特殊的转义字符串 客户端 和 服务器通用的 
	_sntprintf_s(MailInfo.Context, CountArray(MailInfo.Context), TEXT("%s 赠送了您 %u个 {ItemName:ItemID=%u}"), m_pUser->GetRoleInfo().NickName,ItemSum,ItemID);
	//将ItemID 转化为 RewardID 或者邮件携带 物品进行处理? 
	MailInfo.RewardID = ItemID;// static_cast<WORD>(pItemConfig->ItemParam);//聚宝盆的奖励ID 
	MailInfo.RewardSum = ItemSum;
	MailInfo.MailID = 0;
	MailInfo.SendTimeLog = time(NULL);
	MailInfo.SrcFaceID = 0;
	TCHARCopy(MailInfo.SrcNickName, CountArray(MailInfo.SrcNickName), m_pUser->GetRoleInfo().NickName, _tcslen(m_pUser->GetRoleInfo().NickName));
	MailInfo.SrcUserID = m_pUser->GetUserID();                                      //系统发送
	MailInfo.bIsExistsReward = (MailInfo.RewardID != 0 && MailInfo.RewardSum != 0);
	//DBR_Cmd_AddUserMail msg;
	//SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
	//msg.MailType = Mail_SEND;
	//msg.dwDestUserID = DestUserID;
	//msg.MailInfo = MailInfo;
	//g_FishServer.SendNetCmdToDB(&msg);
	if (m_pUser->GetMailManager().OnAddUserMail(&MailInfo, DestUserID))
	{
		if (!OnDelUserItem(ItemOnlyID, ItemID, ItemSum))//扣除物品失败
			return false;

		if (ItemID == GetBronzeBulletID())
		{
			m_pUser->ChangeRoleSendBronzeBulletNum(ItemSum);
		}
		else if (ItemID == GetSilverBulletID())
		{
			m_pUser->ChangeRoleSendSilverBulletNum(ItemSum);
		}
		else if (ItemID == GetGoldBulletID())
		{
			m_pUser->ChangeRoleSendGoldBulletNum(ItemSum);
		}
	}

	g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(ItemSum), DestUserID, TEXT("赠送物品"), SendLogDB);

	return true;
}
bool RoleItemManger::OnTryUseItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum)
{
	//使用背包里的物品
	HashMap<DWORD, tagItemType*>::iterator Iter = m_ItemMap.find(ItemID);
	if (Iter == m_ItemMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (!OnUseItem(ItemOnlyID, ItemID, ItemSum)) //使用物品里面已经扣除了 无须再扣除
	{
		ASSERT(false);
		return false;
	}
	return true;
}
bool RoleItemManger::OnGetUserItem()
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//将玩家背包的物品发送到客户端去
	vector<tagItemInfo> pItemList;
	HashMap<DWORD, tagItemType*>::iterator  Iter = m_ItemMap.begin();
	for (; Iter != m_ItemMap.end(); ++Iter)
	{
		tagItemType * pType = Iter->second;
		if (pType->NonTimeItem.ItemID != 0)
		{
			pItemList.push_back(pType->NonTimeItem);
		}

#if 0
		HashMap<DWORD, tagItemInfo>::iterator IterTimeItem = pType->TimeItem.begin();
		for (; IterTimeItem != pType->TimeItem.end(); ++IterTimeItem)
		{
			pItemList.push_back(IterTimeItem->second);
		}
#else
		if (pType->TimeItem.ItemID != 0)
		{
			pItemList.push_back(pType->TimeItem);
		}
#endif
	}

	DWORD PageSize = sizeof(LC_Cmd_GetUserItem)+sizeof(tagItemInfo)* (pItemList.size() - 1);
	LC_Cmd_GetUserItem* msg = (LC_Cmd_GetUserItem*)malloc(PageSize);
	msg->SetCmdType(GetMsgType(Main_Item, LC_GetUserItem));
	vector<tagItemInfo>::iterator IterItem = pItemList.begin();
	for (WORD i = 0; IterItem != pItemList.end(); ++IterItem, ++i)
	{
		msg->Array[i] = *IterItem;
	}
	std::vector<LC_Cmd_GetUserItem*> pVec;
	SqlitMsg(msg, PageSize, pItemList.size(), true, pVec);
	free(msg);
	if (!pVec.empty())
	{
		std::vector<LC_Cmd_GetUserItem*>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			m_pUser->SendDataToClient(*Iter);
			free(*Iter);
		}
		pVec.clear();
	}
	m_IsLoadToClient = true;
	return true;
}
bool RoleItemManger::OnAddUserItem(tagItemOnce& pItem)
{
	//添加物品
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if (!IsExistsItem(pItem.ItemID))
	{
		return false;
	}
	//我们应该进行处理 判断物品类型
	BYTE ItemType = g_FishServer.GetFishConfig().GetItemType(pItem.ItemID);
	if (ItemType == 0)
	{
		ASSERT(false);
		return false;
	}
	switch (ItemType)
	{
	case IT_Globel:
		{
			//添加玩家金币
			m_pUser->ChangeRoleGlobe(pItem.ItemSum, true);
			return true;
		}
		break;
	case IT_Medal:
		{
			m_pUser->ChangeRoleMedal(pItem.ItemSum, TEXT("往背包添加物品 发现为奖牌类型 添加奖牌"));
			return true;
		}
		break;
	case IT_AchievementPoint:
		{
			m_pUser->ChangeRoleAchievementPoint(pItem.ItemSum);
			return true;
		}
		break;
	case IT_Title:
		{
			m_pUser->GetRoleTitleManager().AddRoleTitle(ConvertDWORDToBYTE(pItem.ItemSum));//物品数量为称号ID
			return true;
		}
		break;
	case IT_Currey:
		{
			m_pUser->ChangeRoleCurrency(pItem.ItemSum,TEXT("往背包添加物品 发现为钻石类型 添加钻石"));
			return true;
		}
		break;
	case IT_Cashpoint:
	{
		HashMap<DWORD, tagFishRechargeInfo>::iterator Iter = g_FishServer.GetFishConfig().GetFishRechargesConfig().m_FishRechargeMap.begin();
		for (; Iter != g_FishServer.GetFishConfig().GetFishRechargesConfig().m_FishRechargeMap.end(); ++Iter)
		{
			if (Iter->second.AddMoney == pItem.ItemSum)
			{
				BYTE Index = BYTE(Iter->first % 10);
				bool RealCharge = false;
				bool Res = true;
				DWORD AddMoney = Iter->second.AddMoney;
				if (Iter->second.IsAddCashpoint())
				{
					AddMoney = static_cast<DWORD>(m_pUser->GetRoleVip().AddReChargeRate() * AddMoney);
					if (m_pUser->IsFirstPayCashpoint(Index))
					{
						AddMoney *= 2;
					}

					Res = m_pUser->ChangeRoleCashpoint(AddMoney, TEXT("实际充值通过邮件点券"));
					if (Res && m_pUser->IsFirstPayCashpoint(Index))
					{
						m_pUser->ChangeRoleIsFirstPayCashpoint(Index);
						RealCharge = true;
					}
					if (Res)
					{
					//	m_pUser->OnHandleEvent(false, true, false, ET_Recharge_First, 0, Iter->second.dDisCountPrice);//首充活动
						RealCharge = true;
					}
				}
				else if (Iter->second.IsAddCashOne())
				{
					bool Res = m_pUser->ChangeRoleCashpoint(AddMoney, TEXT("充值一元豪礼点券"));
					if (Res)
					{
						//m_pUser->OnHandleEvent(false, true, false, ET_Recharge_One, 0, Iter->second.dDisCountPrice);//一元豪礼活动
						RealCharge = true;
					}
				}
				else if (Iter->second.IsAddReward())
				{
					m_pUser->OnAddRoleRewardByRewardID(Iter->second.RewardID, TEXT("充值获得奖励"));
					RealCharge = true;
				}

				if (RealCharge)
				{
					m_pUser->ChangeRoleTotalRechargeSum(Iter->second.dDisCountPrice);//添加玩家总充值数
					m_pUser->OnHandleEvent(false, true, false, ET_Recharge_One, 0, Iter->second.dDisCountPrice);//一元豪礼活动
					m_pUser->OnHandleEvent(true, true, true, ET_Recharge, 0, /*m_pUser->GetRoleInfo().TotalRechargeSum*/ Iter->second.dDisCountPrice);//充值记录
				}
				break;
			}
		}
		return true;
	}
	break;

	case IT_NobilityPoint:
	{
		m_pUser->ChangeRoleNobilityPoint(pItem.ItemSum);
		return true;
	}
	break;

	case IT_MonthSocre:
		{
			if (m_pUser->GetRoleMonth().IsInMonthTable())
			{
				m_pUser->GetRoleMonth().OnChangeRoleMonthPoint(pItem.ItemSum, true);
			}	  
			return true;
		}
		break;
	case IT_MonthGlobel:
		{
			if (m_pUser->GetRoleMonth().IsInMonthTable())
			{
				m_pUser->GetRoleMonth().OnChangeRoleMonthGlobel(pItem.ItemSum, true);
			}
			return true;
		}
		break;
	case IT_Normal:
	case IT_Skill:
	case IT_Package:
	case IT_Cannon:
	case IT_OsCar:
	case IT_Charm://魅力物品可以添加到背包里面去
	case IT_GlobelBag:
	case IT_Horn:
		{
			//if (ItemType == IT_GlobelBag)
			{
				g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(pItem.ItemSum), 0, TEXT("背包里添加物品 物品为聚宝盆"), SendLogDB);
			}
			//普通物品添加到背包里面去
			HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(pItem.ItemID);
			if (IterFind != m_ItemMap.end())
			{
				return IterFind->second->AddItem(pItem);
			}
			else
			{
				tagItemType* pNewItemType = new tagItemType;
				pNewItemType->OnInit(m_pUser, this);
				m_ItemMap.insert(HashMap<DWORD, tagItemType*>::value_type(pItem.ItemID, pNewItemType));
				return pNewItemType->AddItem(pItem);
			}
			return true;
		}
		break;
	case IT_MonthCard:
		{
			//往背包里添加月卡的时候 直接使用掉 无须保存
			tagItemConfig* pItemConfig = g_FishServer.GetFishConfig().GetItemInfo(pItem.ItemID);
			if (!pItemConfig)
			{
				ASSERT(false);
				return false;
			}
			BYTE MonthCardID = static_cast<BYTE>(pItemConfig->ItemParam);
			if (!m_pUser->GetRoleMonthCard().SetRoleMonthCardInfo(MonthCardID))
			{
				//使用失败添加物品
				HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(pItem.ItemID);
				if (IterFind != m_ItemMap.end())
				{
					return IterFind->second->AddItem(pItem);
				}
				else
				{
					tagItemType* pNewItemType = new tagItemType;
					pNewItemType->OnInit(m_pUser, this);
					m_ItemMap.insert(HashMap<DWORD, tagItemType*>::value_type(pItem.ItemID, pNewItemType));
					return pNewItemType->AddItem(pItem);
				}
			}
			return true;
		}
		break;
	case IT_Giff:
	case IT_Entity:
		{
		    //特殊物品不添加到背包里面去  
			return false;
		}
	}
	return false;
}
void RoleItemManger::OnAddUserItemResult(DBO_Cmd_AddUserItem* pDB)
{
	if (!pDB)
	{
		ASSERT(false);
		return;
	}
	if (!IsExistsItem(pDB->ItemInfo.ItemID))
	{
		return;
	}
	HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(pDB->ItemInfo.ItemID);
	if (IterFind == m_ItemMap.end())
	{
		//ASSERT(false);
		tagItemType* pNewItemType = new tagItemType;
		pNewItemType->OnInit(m_pUser, this);
		pNewItemType->OnAddItemResult(pDB);
		m_ItemMap.insert(HashMap<DWORD, tagItemType*>::value_type(pDB->ItemInfo.ItemID, pNewItemType));
	}
	else
	{
		IterFind->second->OnAddItemResult(pDB);
	}
}
bool RoleItemManger::OnDelUserItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if (!IsExistsItem(ItemID))
	{
		return false;
	}
	HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(ItemID);
	if (IterFind == m_ItemMap.end())
	{
		ASSERT(false);
		return false;
	}
	return IterFind->second->DelItem(/*ItemOnlyID, */ItemID, ItemSum);
}
bool RoleItemManger::OnDelUserItem(DWORD ItemID, DWORD ItemSum)
{
	//删除玩家物品
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if (!IsExistsItem(ItemID))
	{
		return false;
	}
	HashMap<DWORD, tagItemType*>::iterator  IterFind = m_ItemMap.find(ItemID);
	if (IterFind == m_ItemMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (ItemSum > IterFind->second->AllItemSum())
	{
		//物品数量不够
		HashMap<DWORD, tagItemConvert>::iterator Iter = g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.find(ItemID);
		if (Iter == g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.end())
			return false;
		DWORD ConvertSum = ItemSum - IterFind->second->AllItemSum();

		if (static_cast<UINT64>(Iter->second.Globel) * ItemSum  > MAXUINT32 ||
			static_cast<UINT64>(Iter->second.Medal) * ItemSum  > MAXUINT32 ||
			static_cast<UINT64>(Iter->second.Currey) * ItemSum  > MAXUINT32
			)
		{
			ASSERT(false);
			return false;
		}

		if (!m_pUser->LostUserMoney(Iter->second.Globel*ConvertSum, Iter->second.Medal*ConvertSum, Iter->second.Currey*ConvertSum,TEXT("背包扣除物品不够 转化为货币代扣")))
			return false;
		if (IterFind->second->DelItem(ItemID, IterFind->second->AllItemSum()))//移除全部的物品
			return true;
		else
		{
			ASSERT(false);
			return true;
		}
	}
	else
	{
		return IterFind->second->DelItem(ItemID, ItemSum);
	}
}
bool RoleItemManger::OnQueryDelUserItemList(vector<tagItemOnce>& pVec, DWORD Currey, bool bQuery)
{
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	//因为涉及到物品与货币之间进行直接转换的功能 我们想要进行一次性的删除 类似于事务的操作
	if (pVec.empty())
		return true;
	//我们想要提前统计出想要的货币数量 已经 物品数量是否足够
	//DWORD Globel = 0, Medal = 0, Currey = 0;

	if (bQuery)
	{
		vector<tagItemOnce>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			DWORD ItemSum = QueryItemCount(Iter->ItemID);
			if (ItemSum < Iter->ItemSum) return false;
			//continue;
			////物品不够 货币补
			//HashMap<DWORD, tagItemConvert>::iterator IterConvert = g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.find(Iter->ItemID);
			//if (IterConvert == g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.end())
			//	return false;
			//Globel += (Iter->ItemSum - ItemSum) * IterConvert->second.Globel;
			//Medal += (Iter->ItemSum - ItemSum) * IterConvert->second.Medal;
			//Currey += (Iter->ItemSum - ItemSum) * IterConvert->second.Currey;
		}
		//判断玩家货币是否足够
		//if (m_pUser->GetRoleInfo().dwGlobeNum < Globel || m_pUser->GetRoleInfo().dwMedalNum < Medal || m_pUser->GetRoleInfo().dwCurrencyNum < Currey)
		//{
		//	//ASSERT(false);
		//	return false;
		//}
		if (m_pUser->GetRoleInfo().dwCurrencyNum < Currey)
			return false;
	}
	else
	{
		//扣钱
		if (Currey > 0)
		{
			m_pUser->ChangeRoleCurrency(Currey*-1, TEXT("开启新的倍率"));
		}

		//扣除物品
		vector<tagItemOnce>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			if (Iter->ItemSum > 0)
			{
				if (!OnDelUserItem(Iter->ItemID, Iter->ItemSum))
				{
					ASSERT(false);
				}
			}
		}
	}

	return true;
}
DWORD RoleItemManger::QueryItemCount(DWORD ItemID)
{
	HashMap<DWORD, tagItemType*>::iterator IterFind = m_ItemMap.find(ItemID);
	if (IterFind != m_ItemMap.end())
		return IterFind->second->AllItemSum();
	else
		return 0;
}
DWORD RoleItemManger::QueryItemAllTimeCount(DWORD ItemID)
{
	HashMap<DWORD, tagItemType*>::iterator IterFind = m_ItemMap.find(ItemID);
	if (IterFind != m_ItemMap.end())
		return IterFind->second->NonTimeItem.ItemSum;
	else
		return 0;
}
void RoleItemManger::OnUpdateByMin(bool IsHourChange, bool IsDayChange, bool IsMonthChange, bool IsYearChange)
{
	HashMap<DWORD, tagItemType*>::iterator  Iter = m_ItemMap.begin();
	for (; Iter != m_ItemMap.end(); ++Iter)
	{
		Iter->second->OnUpdateByMin();
	}
}
bool RoleItemManger::GetItemIsAllExists(DWORD ItemID, DWORD ItemSum)
{
	if (ItemSum == 0)
	{
		return true;
	}
	HashMap<DWORD, tagItemType*>::iterator  Iter = m_ItemMap.find(ItemID);//获取指定物品的集合
	if (Iter == m_ItemMap.end())
	{
		return false;
	}
	//存在 
	if (Iter->second->AllItemSum() < ItemSum)
	{
		return false;
	}
	else
	{
		//物品足够 判断
		if (Iter->second->NonTimeItem.ItemSum >= ItemSum)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void tagItemType::OnInit(CRoleEx* pRole, RoleItemManger* pManager)
{
	if (!pRole || !pManager)
	{
		ASSERT(false);
		return;
	}
	m_pRole = pRole;
	m_pItemManager = pManager;
}
void tagItemType::LoadItem(tagItemInfo& pInfo)
{
	//AllItemSum += pInfo.ItemSum;
	if (pInfo.EndTime == 0)
	{
		if (NonTimeItem.ItemID != 0)
		{
			//ASSERT(false);
			NonTimeItem.ItemSum += pInfo.ItemSum;

			if (pInfo.ItemOnlyID != 0)
			{
				DBR_Cmd_DelUserItem msg;
				SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
				msg.ItemOnlyID = pInfo.ItemOnlyID;
				g_FishServer.SendNetCmdToSaveDB(&msg);
			}

			DBR_Cmd_ChangeUserItem msgChange;
			SetMsgInfo(msgChange,DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
			msgChange.ItemOnlyID = NonTimeItem.ItemOnlyID;
			msgChange.ItemSum = NonTimeItem.ItemSum;
			msgChange.ItemID = NonTimeItem.ItemID;
			msgChange.EndTime = 0;
			g_FishServer.SendNetCmdToSaveDB(&msgChange);

			if (NonTimeItem.ItemID == m_pItemManager->GetGoldBulletID())
			{
				m_pRole->SaveRoleGoldBulletNum(msgChange.ItemSum,TEXT("LOAD"));
			}
			return;
		}
		NonTimeItem = pInfo;
	}
	else
	{
#if 0
		TimeItem.insert(HashMap<DWORD, tagItemInfo>::value_type(pInfo.ItemOnlyID, pInfo));
#else 
		TimeItem = pInfo;
#endif
	}
	return;
}
void tagItemType::Destroy()
{
	ZeroMemory(&NonTimeItem, sizeof(NonTimeItem));
	//TimeItem.clear();
	ZeroMemory(&TimeItem, sizeof(TimeItem));
	//AllItemSum = 0;
}
DWORD tagItemType::AllItemSum()
{
	//if (TimeItem.ItemSum != 0)
	//{
	//	ASSERT(TimeItem.ItemSum == 1);
	//}
	//return ( NonTimeItem.ItemSum + TimeItem.ItemSum);
	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//时间道具只有一个
	{
		return 1;
	}
	else
	{
		return NonTimeItem.ItemSum;
	}
}
bool tagItemType::AddItem(tagItemOnce& pItem)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	
	//当添加一件物品到背包的时候 我们判断是否为礼包 并且是否需要主动打开
	if (g_FishServer.GetPackageManager().IsPackageAndAutoUser(pItem.ItemID)&& pItem.ItemSum > 0)
	{
		for (size_t i = 0; i < pItem.ItemSum; ++i)
		{
			//直接打开背包
			g_FishServer.GetPackageManager().OnAutoOpenFishPackage(m_pRole, pItem.ItemID);//目前玩家背包里无当前物品 需要进行直接转换
		}
		return true;
	}
	DWORD LastMin = 0;//pItem.LastMin;
	if (g_FishServer.GetFishConfig().IsTimeItem(pItem.ItemID))//时间道具
	{
		if (pItem.LastMin == 0)//时间为0
		{
			LogInfoToFile("WmItemError.txt", "time itemid=%u, itemsum=%u ,lastmin=%u", pItem.ItemID, pItem.ItemSum, pItem.LastMin);
			return true;
		}
		LastMin = pItem.LastMin;
	}
	else
	{
		if (pItem.ItemSum == 0)//物品数量为0
		{
			LogInfoToFile("WmItemError.txt", "no time itemid=%u, itemsum=%u ,lastmin=%u", pItem.ItemID, pItem.ItemSum, pItem.LastMin);
			return true;
		}
	}

	LogInfoToFile("WmItem.txt", "adduserid=%u,itemid=%u, NonTimeItem.ItemSum=%u TimeItem.ItemSum=%u  additemsum=%u ,addlastmin=%u", m_pRole->GetUserID(), pItem.ItemID, NonTimeItem.ItemSum, TimeItem.ItemSum, pItem.ItemSum, pItem.LastMin);

	if (LastMin == 0 && NonTimeItem.ItemSum != 0)//非限时道具 并且已经插入直接改数量
	{
		NonTimeItem.ItemID = pItem.ItemID;
		NonTimeItem.ItemSum += pItem.ItemSum;
		//AllItemSum += pItem.ItemSum;
		//发送命令修改物品数量 change
		DBR_Cmd_ChangeUserItem msg;
		SetMsgInfo(msg,DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
		msg.ItemOnlyID = NonTimeItem.ItemOnlyID;
		msg.ItemSum = NonTimeItem.ItemSum;
		msg.ItemID = NonTimeItem.ItemID;
		msg.EndTime = 0;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		if (NonTimeItem.ItemID == m_pItemManager->GetGoldBulletID())
		{
			m_pRole->SaveRoleGoldBulletNum(msg.ItemSum, TEXT("ADD"));
		}

		//发送改变的命令到客户端去
		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_ChangeUserItem msgClient;
			SetMsgInfo(msgClient,GetMsgType(Main_Item, LC_ChangeUserItem), sizeof(LC_Cmd_ChangeUserItem));
			msgClient.ItemOnlyID = NonTimeItem.ItemOnlyID;
			msgClient.ItemSum = NonTimeItem.ItemSum;
			msgClient.EndTime = 0;
			m_pRole->SendDataToClient(&msgClient);
		}

		m_pRole->GetRoleLauncherManager().OnAddItem(pItem.ItemID);
		LogInfoToFile("WmItem.txt", "afteradduserid=%u,itemid=%u, NonTimeItem.ItemSum=%u TimeItem.ItemSum=%u  additemsum=%u ,addlastmin=%u", m_pRole->GetUserID(), pItem.ItemID, NonTimeItem.ItemSum, TimeItem.ItemSum, pItem.ItemSum, pItem.LastMin);

		return true;
	}

	if (LastMin != 0 && TimeItem.EndTime != 0 ) //是限时物品
	{
		TimeItem.ItemSum = 1;
		TimeItem.EndTime += LastMin * 60 * pItem.ItemSum;
		//发送命令修改物品时间 change
		DBR_Cmd_ChangeUserItem msg;
		SetMsgInfo(msg, DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
		msg.ItemOnlyID = TimeItem.ItemOnlyID;
		msg.EndTime = TimeItem.EndTime;
		msg.ItemSum = TimeItem.ItemSum;
		msg.ItemID = TimeItem.ItemID;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		//发送改变的命令到客户端去
		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_ChangeUserItem msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_ChangeUserItem), sizeof(LC_Cmd_ChangeUserItem));
			msgClient.ItemOnlyID = TimeItem.ItemOnlyID;
			msgClient.ItemSum = TimeItem.ItemSum;
			msgClient.EndTime = TimeItem.EndTime;
			m_pRole->SendDataToClient(&msgClient);
		}

		//m_pRole->GetRoleLauncherManager().OnAddItem(pItem.ItemID);
		LogInfoToFile("WmItem.txt", "afteradduserid=%u,itemid=%u, NonTimeItem.ItemSum=%u TimeItem.ItemSum=%u  additemsum=%u ,addlastmin=%u", m_pRole->GetUserID(), pItem.ItemID, NonTimeItem.ItemSum, TimeItem.ItemSum, pItem.ItemSum, pItem.LastMin);

		return true;
	}

	//第一次加的道具  或者是时间道具 发送Add  的 DBR 请求到数据库去
	DBR_Cmd_AddUserItem msg;
	SetMsgInfo(msg,DBR_AddUserItem, sizeof(DBR_Cmd_AddUserItem));
	msg.dwUserID = m_pRole->GetUserID();
	msg.ItemInfo = pItem;
	g_FishServer.SendNetCmdToDB(&msg);
	return true;
}
void tagItemType::OnAddItemResult(DBO_Cmd_AddUserItem* pMsg)
{
	if (!pMsg || !m_pRole)
	{
		ASSERT(false);
		return;
	}
	LogInfoToFile("WmItem.txt", "addresult:userid=%u NontimeSum=%u  timeendtime=%u itemid=%u, itemsum=%u ,EndTime=%u", m_pRole->GetUserID(),NonTimeItem.ItemSum,TimeItem.EndTime, pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);

	if (g_FishServer.GetFishConfig().IsTimeItem(pMsg->ItemInfo.ItemID))//时间道具
	{
		if (pMsg->ItemInfo.EndTime == 0)//时间为0
		{
			LogInfoToFile("WmItemError.txt", "311 time itemid=%u, itemsum=%u ,EndTime=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
			return ;
		}

		//if (TimeItem.ItemID != 0)
		//{
		//	LogInfoToFile("WmItemError.txt", "312 time itemid=%u, itemsum=%u ,EndTime=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
		//	//NonTimeItem.ItemID = pMsg->ItemInfo.ItemID;
		//	//return;
		//}
		TimeItem = pMsg->ItemInfo;

		//if (TimeItem.ItemID == m_pItemManager->GetGoldBulletID())
		//{
		//	m_pRole->SaveRoleGoldBulletNum(TimeItem.ItemSum, TEXT("add result"));
		//}

		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_AddUserItem msg;
			SetMsgInfo(msg, GetMsgType(Main_Item, LC_AddUserItem), sizeof(LC_Cmd_AddUserItem));
			msg.ItemInfo = pMsg->ItemInfo;
			m_pRole->SendDataToClient(&msg);
		}

		m_pRole->GetRoleLauncherManager().OnAddItem(pMsg->ItemInfo.ItemID);
		LogInfoToFile("WmItem.txt", "addresult:end:userid=%u NontimeSum=%u  timeendtime=%u itemid=%u, itemsum=%u ,EndTime=%u", m_pRole->GetUserID(), NonTimeItem.ItemSum, TimeItem.EndTime, pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
	}
	else//数量
	{
		if (pMsg->ItemInfo.ItemSum == 0)
		{
			LogInfoToFile("WmItemError.txt", "321 notime itemid=%u, itemsum=%u ,EndTime=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
			return;
		}
		//if (NonTimeItem.ItemID != 0)
		//{
		//	LogInfoToFile("WmItemError.txt", "322 notime itemid=%u, itemsum=%u ,EndTime=%u", pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
		//	//return;
		//}
		DWORD TempItemSum = NonTimeItem.ItemSum;
		NonTimeItem = pMsg->ItemInfo;
		NonTimeItem.ItemSum += TempItemSum;
		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_AddUserItem msg;
			SetMsgInfo(msg, GetMsgType(Main_Item, LC_AddUserItem), sizeof(LC_Cmd_AddUserItem));
			msg.ItemInfo = pMsg->ItemInfo;
			m_pRole->SendDataToClient(&msg);
		}
		m_pRole->GetRoleLauncherManager().OnAddItem(pMsg->ItemInfo.ItemID);
		if (NonTimeItem.ItemID == m_pItemManager->GetGoldBulletID())
		{
			m_pRole->SaveRoleGoldBulletNum(NonTimeItem.ItemSum, TEXT("add result"));
		}
		LogInfoToFile("WmItem.txt", "addresult:end:userid=%u NontimeSum=%u  timeendtime=%u itemid=%u, itemsum=%u ,EndTime=%u", m_pRole->GetUserID(), NonTimeItem.ItemSum, TimeItem.EndTime, pMsg->ItemInfo.ItemID, pMsg->ItemInfo.ItemSum, pMsg->ItemInfo.EndTime);
	}
}
//bool tagItemType::DelItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum)
//{
//	if (!m_pRole)
//	{
//		ASSERT(false);
//		return false;
//	}
//	if (ItemSum == 0)
//		return true;
//	if (ItemSum > AllItemSum())
//		return false;
//	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//时间道具只有一个 //先删除限时道具
//	{
//		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u,ItemSum=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(), ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);
//
//		//DelItemSum = 1;// TimeItem.ItemSum;
//		//发送删除的命令 DBR_Del
//		DBR_Cmd_DelUserItem msg;
//		SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
//		msg.ItemOnlyID = TimeItem.ItemOnlyID;
//		g_FishServer.SendNetCmdToSaveDB(&msg);
//		if (m_pItemManager->IsSendClient())
//		{
//			LC_Cmd_DelUserItem msgClient;
//			SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
//			msgClient.ItemOnlyID = TimeItem.ItemOnlyID;
//			m_pRole->SendDataToClient(&msgClient);
//		}
//		ZeroMemory(&TimeItem, sizeof(TimeItem));
//		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
//	}
//	else//数量道具
//	{
//		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(),ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);
//
//		if (NonTimeItem.ItemSum < ItemSum)
//		{
//			return false;
//		}
//		else if (NonTimeItem.ItemSum == ItemSum)
//		{
//			//发送删除的命令
//			DBR_Cmd_DelUserItem msg;
//			SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
//			msg.ItemOnlyID = NonTimeItem.ItemOnlyID;
//			g_FishServer.SendNetCmdToSaveDB(&msg);
//
//			if (m_pItemManager->IsSendClient())
//			{
//				LC_Cmd_DelUserItem msgClient;
//				SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
//				msgClient.ItemOnlyID = NonTimeItem.ItemOnlyID;
//				m_pRole->SendDataToClient(&msgClient);
//			}
//			ZeroMemory(&NonTimeItem, sizeof(NonTimeItem));
//
//			m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
//		}
//		else
//		{
//			NonTimeItem.ItemSum -= ItemSum;
//			//发送修改的命令
//			DBR_Cmd_ChangeUserItem msg;
//			SetMsgInfo(msg, DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
//			msg.ItemOnlyID = NonTimeItem.ItemOnlyID;
//			msg.ItemSum = NonTimeItem.ItemSum;
//			msg.ItemID = NonTimeItem.ItemID;
//			msg.EndTime = 0;
//			g_FishServer.SendNetCmdToSaveDB(&msg);
//
//			if (NonTimeItem.ItemID == 910)
//			{
//				m_pRole->SaveRoleGoldBulletNum(msg.ItemSum, TEXT("DELL"));
//			}
//
//			if (m_pItemManager->IsSendClient())
//			{
//				LC_Cmd_ChangeUserItem msgClient;
//				SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_ChangeUserItem), sizeof(LC_Cmd_ChangeUserItem));
//				msgClient.ItemOnlyID = NonTimeItem.ItemOnlyID;
//				msgClient.ItemSum = NonTimeItem.ItemSum;
//				m_pRole->SendDataToClient(&msgClient);
//			}
//			m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
//		}
//	}
//	return true;
//}

bool tagItemType::DelItem(DWORD ItemID, DWORD ItemSum)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return false;
	}
	if (ItemSum == 0)
		return true;
	if (ItemSum > AllItemSum())
		return false;
	LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u,ItemSum=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(), ItemID, ItemSum, NonTimeItem.ItemSum, TimeItem.ItemSum);

	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//时间道具只有一个 //先删除限时道具
	{
		//DelItemSum = 1;// TimeItem.ItemSum;
		//发送删除的命令 DBR_Del
		DBR_Cmd_DelUserItem msg;
		SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
		msg.ItemOnlyID = TimeItem.ItemOnlyID;
		g_FishServer.SendNetCmdToSaveDB(&msg);
		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_DelUserItem msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
			msgClient.ItemOnlyID = TimeItem.ItemOnlyID;
			m_pRole->SendDataToClient(&msgClient);
		}
		ZeroMemory(&TimeItem, sizeof(TimeItem));
		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
	}
	else//数量道具
	{
		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(),ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);

		if (NonTimeItem.ItemSum < ItemSum)
		{
			return false;
		}
		else if (NonTimeItem.ItemSum == ItemSum)
		{
			//发送删除的命令
			DBR_Cmd_DelUserItem msg;
			SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
			msg.ItemOnlyID = NonTimeItem.ItemOnlyID;
			g_FishServer.SendNetCmdToSaveDB(&msg);

			if (m_pItemManager->IsSendClient())
			{
				LC_Cmd_DelUserItem msgClient;
				SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
				msgClient.ItemOnlyID = NonTimeItem.ItemOnlyID;
				m_pRole->SendDataToClient(&msgClient);
			}
			ZeroMemory(&NonTimeItem, sizeof(NonTimeItem));

			m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
		}
		else
		{
			NonTimeItem.ItemSum -= ItemSum;
			//发送修改的命令
			DBR_Cmd_ChangeUserItem msg;
			SetMsgInfo(msg, DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
			msg.ItemOnlyID = NonTimeItem.ItemOnlyID;
			msg.ItemSum = NonTimeItem.ItemSum;
			msg.ItemID = NonTimeItem.ItemID;
			msg.EndTime = 0;
			g_FishServer.SendNetCmdToSaveDB(&msg);

			if (NonTimeItem.ItemID == 910)
			{
				m_pRole->SaveRoleGoldBulletNum(msg.ItemSum, TEXT("DELL"));
			}

			if (m_pItemManager->IsSendClient())
			{
				LC_Cmd_ChangeUserItem msgClient;
				SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_ChangeUserItem), sizeof(LC_Cmd_ChangeUserItem));
				msgClient.ItemOnlyID = NonTimeItem.ItemOnlyID;
				msgClient.ItemSum = NonTimeItem.ItemSum;
				m_pRole->SendDataToClient(&msgClient);
			}
			m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);
		}
	}
	LogInfoToFile("WmItem.txt", "afterdeluserid=%u,itemid=%u,ItemSum=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(), ItemID, ItemSum, NonTimeItem.ItemSum, TimeItem.ItemSum);

	return true;
}
void tagItemType::OnUpdateByMin()
{
#if 0
	/*if (TimeItem.Size() <= 0)
	return;*/
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (TimeItem.empty())
		return;
	time_t NowTime = time(NULL);
	DWORD DelItemSum = 0;
	DWORD ItemID = 0;
	HashMap<DWORD, tagItemInfo>::iterator Iter = TimeItem.begin();
	for (; Iter != TimeItem.end();)
	{
		if (Iter->second.EndTime != 0 && NowTime >= Iter->second.EndTime)
		{
			ItemID = Iter->second.ItemID;
			//删除掉
			DBR_Cmd_DelUserItem msg;
			SetMsgInfo(msg,DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
			msg.ItemOnlyID = Iter->second.ItemOnlyID;
			g_FishServer.SendNetCmdToSaveDB(&msg);

			//发送命令到客户端去 删除指定物品
			if (m_pItemManager->IsSendClient())
			{
				LC_Cmd_DelUserItem msgClient;
				SetMsgInfo(msgClient,GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
				msgClient.ItemOnlyID = Iter->second.ItemOnlyID;
				m_pRole->SendDataToClient(&msgClient);
			}
			DelItemSum += Iter->second.ItemSum;
			Iter = TimeItem.erase(Iter);
		}
		else
			++Iter;
	}
	AllItemSum -= DelItemSum;//物品总数减少
	if (DelItemSum > 0)
	{
		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);//有物品被删除了
	}

#else
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (TimeItem.ItemID == 0 || TimeItem.ItemSum == 0)
		return;
	time_t NowTime = time(NULL);
	DWORD DelItemSum = 0;
	DWORD ItemID = 0;
	if (TimeItem.EndTime != 0 && NowTime >= TimeItem.EndTime)
	{
		ItemID = TimeItem.ItemID;
		//删除掉
		DBR_Cmd_DelUserItem msg;
		SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
		msg.ItemOnlyID = TimeItem.ItemOnlyID;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		//发送命令到客户端去 删除指定物品
		if (m_pItemManager->IsSendClient())
		{
			LC_Cmd_DelUserItem msgClient;
			SetMsgInfo(msgClient, GetMsgType(Main_Item, LC_DelUserItem), sizeof(LC_Cmd_DelUserItem));
			msgClient.ItemOnlyID = TimeItem.ItemOnlyID;
			m_pRole->SendDataToClient(&msgClient);
		}
		ZeroMemory(&TimeItem, sizeof(TimeItem));
		DelItemSum = 1;
	}
	//AllItemSum -= DelItemSum;//物品总数减少
	if (DelItemSum > 0)
	{
		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);//有物品被删除了
	}

#endif
}