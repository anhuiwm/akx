#include "Stdafx.h"
#include "ShopManager.h"
#include "FishServer.h"
#include "..\CommonFile\DBLogManager.h"
extern void SendLogDB(NetCmd* pCmd);
ShopManager::ShopManager()
{
}
ShopManager::~ShopManager()
{
	Destroy();
}
void ShopManager::OnInit()
{
	HandleShopItem();
}
void ShopManager::UpdateByMin()
{
	//��ȷ���� ֻ����0���ʱ����и��´���  ���߳�ʼ����ʱ����и��� 
	SYSTEMTIME time;
	GetLocalTime(&time);
	static BYTE LogUpdateMin = 0xff;//�ϴθ��¼�¼�ķ�����
	if (LogUpdateMin != 0xff && time.wSecond != 0 && time.wMinute == LogUpdateMin)
		return;//�����и���
	LogUpdateMin = (BYTE)time.wMinute;//��¼�ϴθ���
	//���������̵���Ʒ��״̬
	HandleShopItem();
}
void ShopManager::HandleShopItem()
{
	m_ShopItemStates.clear();
	HashMap<BYTE, tagShopConfig>::iterator Iter = g_FishServer.GetFishConfig().GetShopConfig().ShopMap.begin();
	for (; Iter != g_FishServer.GetFishConfig().GetShopConfig().ShopMap.end(); ++Iter)
	{
		HashMap<BYTE, tagShopItemConfig>::iterator IterItem = Iter->second.ShopItemMap.begin();
		for (; IterItem != Iter->second.ShopItemMap.end(); ++IterItem)
		{
			tagShopItemConfig* pItemConfig = &IterItem->second;
			if (ShopItemIsInTime(pItemConfig))
			{
				m_ShopItemStates.insert(HashMap<WORD, bool>::value_type((Iter->first << 8) + IterItem->first, true));
			}
			else
			{
				m_ShopItemStates.insert(HashMap<WORD, bool>::value_type((Iter->first << 8) + IterItem->first, false));
			}
		}
	}
}
bool ShopManager::ShopItemIsInTime(tagShopItemConfig* pItemConfig)
{
	if (!pItemConfig)
	{
		ASSERT(false);
		return false;
	}
	if (pItemConfig->TimeVec.empty())
		return true;
	time_t Now = time(NULL);
	std::vector<tagShopItemTimeConfig>::iterator Iter = pItemConfig->TimeVec.begin();
	for (; Iter != pItemConfig->TimeVec.end(); ++Iter)
	{
		if (Now >= Iter->BeginTime && Now <= (Iter->BeginTime + Iter->LastHour * 3600))
			return true;
	}
	return false;
}
void ShopManager::OnShellShopItem(CRoleEx* pRole, BYTE ShopID, BYTE ItemIndex, DWORD ItemSum)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	if (ItemSum > 10000 || ItemSum == 0)
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_ShopItemResult msg;
	SetMsgInfo(msg,GetMsgType(Main_Shop, LC_ShopItemResult), sizeof(LC_Cmd_ShopItemResult));
	msg.Result = false;
	msg.ShopID = ShopID;
	//������Ʒ �ж������ļ�
	HashMap<BYTE, tagShopConfig>::iterator Iter = g_FishServer.GetFishConfig().GetShopConfig().ShopMap.find(ShopID);
	if (Iter == g_FishServer.GetFishConfig().GetShopConfig().ShopMap.end())
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}

	if (Iter->first != ShopID)
	{
		ASSERT(false);
		return;
	}
	HashMap<BYTE, tagShopItemConfig>::iterator IterItem = Iter->second.ShopItemMap.find(ItemIndex);
	if (IterItem == Iter->second.ShopItemMap.end())
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (IterItem->second.ShopItemIndex != ItemIndex)
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	//�ж���Ʒ�Ƿ���ʱ����
	WORD Key = (Iter->first << 8) + IterItem->first;
	HashMap<WORD, bool>::iterator IterTime = m_ShopItemStates.find(Key);
	if (IterTime == m_ShopItemStates.end() || !IterTime->second)
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (ItemSum > 1 && !IterItem->second.IsCanPile)//�����Զѵ�����Ʒ һ��ֻ���Թ���һ��
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (IterItem->second.ShopType == SIT_Entity && !pRole->GetRoleEntity().IsCanUserEntity())//Ϊʵ����Ʒ ����δ���ú���ҵ���ʵ��ַ �޷�������ȡ��Ʒ
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (IterItem->second.ShopType == SIT_PhonePay && !PhoneIsCanUse(pRole->GetRoleEntity().GetEntityInfo().Phone))//Ϊʵ����Ʒ ����δ���ú���ҵ���ʵ��ַ �޷�������ȡ��Ʒ
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	if (IterItem->second.ShopType == SIT_MonthCard && pRole->GetRoleInfo().MonthCardID != 0 && time(null) < pRole->GetRoleInfo().MonthCardEndTime)
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	//���Ϊ�һ����� 
	if ((IterItem->second.ShopType == SIT_Entity || IterItem->second.ShopType == SIT_PhonePay) && pRole->GetRoleInfo().CashSum >= pRole->GetRoleVip().GetUseMedalSum())//���ÿ����Զһ��Ĵ���
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	
	if (static_cast<UINT64>(IterItem->second.PriceGlobel) * ItemSum  >  g_FishServer.GetFishConfig().GetSystemConfig().MaxGobelSum ||
		static_cast<UINT64>(IterItem->second.PriceMabel) * ItemSum  > MAXUINT32 ||
		static_cast<UINT64>(IterItem->second.PriceCurrey) * ItemSum  > MAXUINT32
		)
	{
		pRole->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}

	if (IterItem->second.ShopType == SIT_Sell)//����
	{
		if (pRole->GetItemManager().QueryItemCount(IterItem->second.ItemInfo.ItemID) >= ItemSum)
		{
			if (pRole->GetItemManager().OnDelUserItem(IterItem->second.ItemInfo.ItemID, ItemSum))
			{
				pRole->ChangeRoleGlobe(IterItem->second.PriceGlobel*ItemSum, true);
				pRole->ChangeRoleMedal(IterItem->second.PriceMabel*ItemSum, TEXT("�̵������Ʒ"));
				pRole->ChangeRoleCurrency(IterItem->second.PriceCurrey*ItemSum, TEXT("�̵������Ʒ"));
				msg.Result = true;
			}
		}
	}
	else
	{
		//�ж���ҵĽ�Ǯ �� ��Ʒ�ļ۸�
		if (!pRole->LostUserMoney(IterItem->second.PriceGlobel*ItemSum, IterItem->second.PriceMabel*ItemSum, IterItem->second.PriceCurrey*ItemSum, TEXT("�̵깺����Ʒ�۳���һ���")))
		{
			pRole->SendDataToClient(&msg);
			ASSERT(false);
			return;
		}
		if (IterItem->second.ShopType == SIT_Entity)
		{
			//����ʵ����Ʒ ��������
			//GO_Cmd_BuyEntityItem msg;
			//SetMsgInfo(msg, GetMsgType(Main_Operate, GO_BuyEntityItem), sizeof(GO_Cmd_BuyEntityItem));
			//TCHARCopy(msg.Addres, CountArray(msg.Addres), pRole->GetRoleEntity().GetEntityInfo().EntityItemUseAddres, _tcslen(pRole->GetRoleEntity().GetEntityInfo().EntityItemUseAddres));
			//TCHARCopy(msg.Name, CountArray(msg.Name), pRole->GetRoleEntity().GetEntityInfo().EntityItemUseName, _tcslen(pRole->GetRoleEntity().GetEntityInfo().EntityItemUseName));
			//msg.Phone = pRole->GetRoleEntity().GetEntityInfo().EntityItemUsePhone;
			//msg.ShopID = Iter->second.ShopID;
			//msg.ShopOnlyID = IterItem->second.ShopItemIndex;
			//msg.ItemID = IterItem->second.ItemInfo.ItemID;
			//msg.ItemSum = ItemSum;//���������
			//msg.dwUserID = pRole->GetUserID();
			//g_FishServer.SendNetCmdToOperate(&msg);

			//����ʵ����Ʒ ��������Ӫ������ ֱ�ӷ���������ݿ�ȥ
			DBR_Cmd_AddRoleEntityItem msgDB;//���͵����ݿ�ȥ
			SetMsgInfo(msgDB, DBR_AddRoleEntityItem, sizeof(DBR_Cmd_AddRoleEntityItem));
			msgDB.dwUserID = pRole->GetUserID();
			msgDB.ItemID = IterItem->second.ItemInfo.ItemID;
			msgDB.ItemSum = IterItem->second.ItemInfo.ItemSum * ItemSum;
			msgDB.UseMedal = IterItem->second.PriceMabel * ItemSum;//���ѵĽ�������
			msgDB.NowMedalSum = pRole->GetRoleInfo().dwMedalNum;
			//������ʵ�ĵ�ַ����
			tagRoleAddressInfo& pInfo = pRole->GetRoleEntity().GetEntityInfo();
			TCHARCopy(msgDB.Name, CountArray(msgDB.Name), pInfo.EntityItemUseName, _tcslen(pInfo.EntityItemUseName));
			TCHARCopy(msgDB.IDEntity, CountArray(msgDB.IDEntity), pInfo.IdentityID, _tcslen(pInfo.IdentityID));
			msgDB.Phone = pInfo.EntityItemUsePhone;
			TCHARCopy(msgDB.Addres, CountArray(msgDB.Addres), pInfo.EntityItemUseAddres, _tcslen(pInfo.EntityItemUseAddres));
			g_FishServer.SendNetCmdToSaveDB(&msgDB);
			//g_FishServer.GetAnnouncementManager().OnAddNewAnnouncementOnce(pRole->GetRoleInfo().NickName, Iter->second.ShopID, IterItem->second.ShopItemIndex);//���һ������
			g_FishServer.SendBroadCast(pRole, NoticeType::NT_Exchange, IterItem->second.ShopItemName, msgDB.ItemID, msgDB.UseMedal);
			pRole->ChangeRoleCashSum(1);//��Ӵ���

			pRole->ChangeRoleShareStates(false);
		}
		else if (IterItem->second.ShopType == SIT_PhonePay)
		{
			ASSERT(false);
#if 0
			//���ѳ�ֵ
			GO_Cmd_PhonePay msg;
			SetMsgInfo(msg, GetMsgType(Main_Operate, GO_PhonePay), sizeof(GO_Cmd_PhonePay));
			msg.dwUserID = pRole->GetUserID();
			msg.ShopID = Iter->second.ShopID;
			msg.ShopOnlyID = IterItem->second.ShopItemIndex;
			msg.ShopSum = ItemSum;//��Ʒ������
			msg.Phone = pRole->GetRoleEntity().GetEntityInfo().Phone;
			g_FishServer.SendNetCmdToOperate(&msg);

			pRole->ChangeRoleCashSum(1);//��Ӵ���
#endif
			return;
		}
		else if (IterItem->second.ShopType == SIT_MonthCard)
		{
			//��ҹ����¿���Ʒ
			DWORD ItemParam = g_FishServer.GetFishConfig().GetItemParam(IterItem->second.ItemInfo.ItemID);
			if (ItemParam == 0 || !pRole->GetRoleMonthCard().SetRoleMonthCardInfo(ConvertDWORDToBYTE(ItemParam)))
			{
				pRole->ChangeRoleGlobe(IterItem->second.PriceGlobel*ItemSum, true);
				pRole->ChangeRoleMedal(IterItem->second.PriceMabel*ItemSum, TEXT("�̵깺����Ʒ �¿������� �黹��ҽ���"));
				pRole->ChangeRoleCurrency(IterItem->second.PriceCurrey*ItemSum, TEXT("�̵깺����Ʒ �¿������� �黹�����ʯ"));
				pRole->SendDataToClient(&msg);
				ASSERT(false);
				pRole->SendDataToClient(&msg);
				return;
			}
		}
		else
		{
			//����������Ʒ ��Ʒ����
			if (IterItem->second.ShopType == SIT_GlobelBag)
			{
				g_DBLogManager.LogToDB(pRole->GetUserID(), LT_GlobelBag, static_cast<int>(ItemSum), 0, TEXT("����۱���"), SendLogDB);
			}

			tagItemOnce pOnce = IterItem->second.ItemInfo;
			pOnce.ItemSum *= ItemSum;
			if (!pRole->GetItemManager().OnAddUserItem(pOnce))
			{
				pRole->ChangeRoleGlobe(IterItem->second.PriceGlobel*ItemSum, true);
				pRole->ChangeRoleMedal(IterItem->second.PriceMabel*ItemSum, TEXT("�̵깺����Ʒ �����Ʒʧ�� �黹��ҽ���"));
				pRole->ChangeRoleCurrency(IterItem->second.PriceCurrey*ItemSum, TEXT("�̵깺����Ʒ �����Ʒʧ�� �黹�����ʯ"));
				pRole->SendDataToClient(&msg);
				ASSERT(false);
				pRole->SendDataToClient(&msg);
				return;
			}

			if (ShopID == Shop_Exchange)
			{
				//��������� �������
				DBR_Cmd_AddRoleShopNormalItem msgDB;//���͵����ݿ�ȥ
				SetMsgInfo(msgDB, DBR_AddRoleShopNormalItem, sizeof(DBR_Cmd_AddRoleShopNormalItem));
				msgDB.dwUserID = pRole->GetUserID();
				msgDB.ItemID = IterItem->second.ItemInfo.ItemID;
				msgDB.ItemSum = IterItem->second.ItemInfo.ItemSum * ItemSum;
				msgDB.UseMedal = IterItem->second.PriceMabel * ItemSum;//���ѵĽ�������
				msgDB.NowMedalSum = pRole->GetRoleInfo().dwMedalNum;
				//������ʵ�ĵ�ַ����
				tagRoleAddressInfo& pInfo = pRole->GetRoleEntity().GetEntityInfo();
				g_FishServer.SendNetCmdToSaveDB(&msgDB);
				g_FishServer.SendBroadCast(pRole, NoticeType::NT_Exchange, IterItem->second.ShopItemName, msgDB.ItemID, msgDB.UseMedal);
				//g_FishServer.GetAnnouncementManager().OnAddNewAnnouncementOnce(pRole->GetRoleInfo().NickName, Iter->second.ShopID, IterItem->second.ShopItemIndex);//���һ������

			}

		}
	}
	//��ҹ���ɹ���
	//��������ͻ���ȥ
	msg.Result = true;
	pRole->SendDataToClient(&msg);
	return;
}
void ShopManager::Destroy()
{
}