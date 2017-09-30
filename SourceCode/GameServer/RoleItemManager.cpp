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
			g_FishServer.GetPackageManager().OnOpenFishPackage(m_pUser, ItemOnlyID, ItemID);//�������Ʒ
	}
	else if (pItemConfig->ItemType == EItemType::IT_MonthCard)
	{
		//���ʹ���¿���Ʒ ���ǽ��д���
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
		//����Ʒ ����Ƿ����������Ʒ
		if (OnDelUserItem(ItemOnlyID, ItemID, ItemSum))
		{
			//��Ʒɾ���ɹ��� ���ǿ�ʼ�������ӽ�Ǯ
			WORD RewardID = static_cast<WORD>(pItemConfig->ItemParam>>16);
			m_pUser->OnAddRoleRewardByRewardID(RewardID, TEXT("��ȡ�۱��轱��"), ItemSum);

			g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(ItemSum),0, TEXT("ʹ�þ۱���"), SendLogDB);

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
	//�����ݿ������ҵ���Ʒ���� 
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
		//���е���Ʒ�Ѿ����������
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

	//�����ʼ������
	tagRoleMail	MailInfo;
	MailInfo.bIsRead = false;
	//������������Ҫ����Ĵ��� ������Ҫһ�� �����ת���ַ��� �ͻ��� �� ������ͨ�õ� 
	_sntprintf_s(MailInfo.Context, CountArray(MailInfo.Context), TEXT("%s �������� %u�� {ItemName:ItemID=%u}"), m_pUser->GetRoleInfo().NickName,ItemSum,ItemID);
	//��ItemID ת��Ϊ RewardID �����ʼ�Я�� ��Ʒ���д���? 
	MailInfo.RewardID = ItemID;// static_cast<WORD>(pItemConfig->ItemParam);//�۱���Ľ���ID 
	MailInfo.RewardSum = ItemSum;
	MailInfo.MailID = 0;
	MailInfo.SendTimeLog = time(NULL);
	MailInfo.SrcFaceID = 0;
	TCHARCopy(MailInfo.SrcNickName, CountArray(MailInfo.SrcNickName), m_pUser->GetRoleInfo().NickName, _tcslen(m_pUser->GetRoleInfo().NickName));
	MailInfo.SrcUserID = m_pUser->GetUserID();                                      //ϵͳ����
	MailInfo.bIsExistsReward = (MailInfo.RewardID != 0 && MailInfo.RewardSum != 0);
	//DBR_Cmd_AddUserMail msg;
	//SetMsgInfo(msg, DBR_AddUserMail, sizeof(DBR_Cmd_AddUserMail));
	//msg.MailType = Mail_SEND;
	//msg.dwDestUserID = DestUserID;
	//msg.MailInfo = MailInfo;
	//g_FishServer.SendNetCmdToDB(&msg);
	if (m_pUser->GetMailManager().OnAddUserMail(&MailInfo, DestUserID))
	{
		if (!OnDelUserItem(ItemOnlyID, ItemID, ItemSum))//�۳���Ʒʧ��
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

	g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(ItemSum), DestUserID, TEXT("������Ʒ"), SendLogDB);

	return true;
}
bool RoleItemManger::OnTryUseItem(DWORD ItemOnlyID, DWORD ItemID, DWORD ItemSum)
{
	//ʹ�ñ��������Ʒ
	HashMap<DWORD, tagItemType*>::iterator Iter = m_ItemMap.find(ItemID);
	if (Iter == m_ItemMap.end())
	{
		ASSERT(false);
		return false;
	}
	if (!OnUseItem(ItemOnlyID, ItemID, ItemSum)) //ʹ����Ʒ�����Ѿ��۳��� �����ٿ۳�
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
	//����ұ�������Ʒ���͵��ͻ���ȥ
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
	//�����Ʒ
	if (!m_pUser)
	{
		ASSERT(false);
		return false;
	}
	if (!IsExistsItem(pItem.ItemID))
	{
		return false;
	}
	//����Ӧ�ý��д��� �ж���Ʒ����
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
			//�����ҽ��
			m_pUser->ChangeRoleGlobe(pItem.ItemSum, true);
			return true;
		}
		break;
	case IT_Medal:
		{
			m_pUser->ChangeRoleMedal(pItem.ItemSum, TEXT("�����������Ʒ ����Ϊ�������� ��ӽ���"));
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
			m_pUser->GetRoleTitleManager().AddRoleTitle(ConvertDWORDToBYTE(pItem.ItemSum));//��Ʒ����Ϊ�ƺ�ID
			return true;
		}
		break;
	case IT_Currey:
		{
			m_pUser->ChangeRoleCurrency(pItem.ItemSum,TEXT("�����������Ʒ ����Ϊ��ʯ���� �����ʯ"));
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

					Res = m_pUser->ChangeRoleCashpoint(AddMoney, TEXT("ʵ�ʳ�ֵͨ���ʼ���ȯ"));
					if (Res && m_pUser->IsFirstPayCashpoint(Index))
					{
						m_pUser->ChangeRoleIsFirstPayCashpoint(Index);
						RealCharge = true;
					}
					if (Res)
					{
					//	m_pUser->OnHandleEvent(false, true, false, ET_Recharge_First, 0, Iter->second.dDisCountPrice);//�׳�
						RealCharge = true;
					}
				}
				else if (Iter->second.IsAddCashOne())
				{
					bool Res = m_pUser->ChangeRoleCashpoint(AddMoney, TEXT("��ֵһԪ�����ȯ"));
					if (Res)
					{
						//m_pUser->OnHandleEvent(false, true, false, ET_Recharge_One, 0, Iter->second.dDisCountPrice);//һԪ����
						RealCharge = true;
					}
				}
				else if (Iter->second.IsAddReward())
				{
					m_pUser->OnAddRoleRewardByRewardID(Iter->second.RewardID, TEXT("��ֵ��ý���"));
					RealCharge = true;
				}

				if (RealCharge)
				{
					m_pUser->ChangeRoleTotalRechargeSum(Iter->second.dDisCountPrice);//�������ܳ�ֵ��
					m_pUser->OnHandleEvent(false, true, false, ET_Recharge_One, 0, Iter->second.dDisCountPrice);//һԪ����
					m_pUser->OnHandleEvent(true, true, true, ET_Recharge, 0, /*m_pUser->GetRoleInfo().TotalRechargeSum*/ Iter->second.dDisCountPrice);//��ֵ��¼
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
	case IT_Charm://������Ʒ������ӵ���������ȥ
	case IT_GlobelBag:
	case IT_Horn:
		{
			//if (ItemType == IT_GlobelBag)
			{
				g_DBLogManager.LogToDB(m_pUser->GetUserID(), LT_GlobelBag, static_cast<int>(pItem.ItemSum), 0, TEXT("�����������Ʒ ��ƷΪ�۱���"), SendLogDB);
			}
			//��ͨ��Ʒ��ӵ���������ȥ
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
			//������������¿���ʱ�� ֱ��ʹ�õ� ���뱣��
			tagItemConfig* pItemConfig = g_FishServer.GetFishConfig().GetItemInfo(pItem.ItemID);
			if (!pItemConfig)
			{
				ASSERT(false);
				return false;
			}
			BYTE MonthCardID = static_cast<BYTE>(pItemConfig->ItemParam);
			if (!m_pUser->GetRoleMonthCard().SetRoleMonthCardInfo(MonthCardID))
			{
				//ʹ��ʧ�������Ʒ
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
		    //������Ʒ����ӵ���������ȥ  
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
	//ɾ�������Ʒ
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
		//��Ʒ��������
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

		if (!m_pUser->LostUserMoney(Iter->second.Globel*ConvertSum, Iter->second.Medal*ConvertSum, Iter->second.Currey*ConvertSum,TEXT("�����۳���Ʒ���� ת��Ϊ���Ҵ���")))
			return false;
		if (IterFind->second->DelItem(ItemID, IterFind->second->AllItemSum()))//�Ƴ�ȫ������Ʒ
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
	//��Ϊ�漰����Ʒ�����֮�����ֱ��ת���Ĺ��� ������Ҫ����һ���Ե�ɾ�� ����������Ĳ���
	if (pVec.empty())
		return true;
	//������Ҫ��ǰͳ�Ƴ���Ҫ�Ļ������� �Ѿ� ��Ʒ�����Ƿ��㹻
	//DWORD Globel = 0, Medal = 0, Currey = 0;

	if (bQuery)
	{
		vector<tagItemOnce>::iterator Iter = pVec.begin();
		for (; Iter != pVec.end(); ++Iter)
		{
			DWORD ItemSum = QueryItemCount(Iter->ItemID);
			if (ItemSum < Iter->ItemSum) return false;
			//continue;
			////��Ʒ���� ���Ҳ�
			//HashMap<DWORD, tagItemConvert>::iterator IterConvert = g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.find(Iter->ItemID);
			//if (IterConvert == g_FishServer.GetFishConfig().GetItemConvertConfig().m_ConvertMap.end())
			//	return false;
			//Globel += (Iter->ItemSum - ItemSum) * IterConvert->second.Globel;
			//Medal += (Iter->ItemSum - ItemSum) * IterConvert->second.Medal;
			//Currey += (Iter->ItemSum - ItemSum) * IterConvert->second.Currey;
		}
		//�ж���һ����Ƿ��㹻
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
		//��Ǯ
		if (Currey > 0)
		{
			m_pUser->ChangeRoleCurrency(Currey*-1, TEXT("�����µı���"));
		}

		//�۳���Ʒ
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
	HashMap<DWORD, tagItemType*>::iterator  Iter = m_ItemMap.find(ItemID);//��ȡָ����Ʒ�ļ���
	if (Iter == m_ItemMap.end())
	{
		return false;
	}
	//���� 
	if (Iter->second->AllItemSum() < ItemSum)
	{
		return false;
	}
	else
	{
		//��Ʒ�㹻 �ж�
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
	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//ʱ�����ֻ��һ��
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
	
	//�����һ����Ʒ��������ʱ�� �����ж��Ƿ�Ϊ��� �����Ƿ���Ҫ������
	if (g_FishServer.GetPackageManager().IsPackageAndAutoUser(pItem.ItemID)&& pItem.ItemSum > 0)
	{
		for (size_t i = 0; i < pItem.ItemSum; ++i)
		{
			//ֱ�Ӵ򿪱���
			g_FishServer.GetPackageManager().OnAutoOpenFishPackage(m_pRole, pItem.ItemID);//Ŀǰ��ұ������޵�ǰ��Ʒ ��Ҫ����ֱ��ת��
		}
		return true;
	}
	DWORD LastMin = 0;//pItem.LastMin;
	if (g_FishServer.GetFishConfig().IsTimeItem(pItem.ItemID))//ʱ�����
	{
		if (pItem.LastMin == 0)//ʱ��Ϊ0
		{
			LogInfoToFile("WmItemError.txt", "time itemid=%u, itemsum=%u ,lastmin=%u", pItem.ItemID, pItem.ItemSum, pItem.LastMin);
			return true;
		}
		LastMin = pItem.LastMin;
	}
	else
	{
		if (pItem.ItemSum == 0)//��Ʒ����Ϊ0
		{
			LogInfoToFile("WmItemError.txt", "no time itemid=%u, itemsum=%u ,lastmin=%u", pItem.ItemID, pItem.ItemSum, pItem.LastMin);
			return true;
		}
	}

	LogInfoToFile("WmItem.txt", "adduserid=%u,itemid=%u, NonTimeItem.ItemSum=%u TimeItem.ItemSum=%u  additemsum=%u ,addlastmin=%u", m_pRole->GetUserID(), pItem.ItemID, NonTimeItem.ItemSum, TimeItem.ItemSum, pItem.ItemSum, pItem.LastMin);

	if (LastMin == 0 && NonTimeItem.ItemSum != 0)//����ʱ���� �����Ѿ�����ֱ�Ӹ�����
	{
		NonTimeItem.ItemID = pItem.ItemID;
		NonTimeItem.ItemSum += pItem.ItemSum;
		//AllItemSum += pItem.ItemSum;
		//���������޸���Ʒ���� change
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

		//���͸ı������ͻ���ȥ
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

	if (LastMin != 0 && TimeItem.EndTime != 0 ) //����ʱ��Ʒ
	{
		TimeItem.ItemSum = 1;
		TimeItem.EndTime += LastMin * 60 * pItem.ItemSum;
		//���������޸���Ʒʱ�� change
		DBR_Cmd_ChangeUserItem msg;
		SetMsgInfo(msg, DBR_ChangeUserItem, sizeof(DBR_Cmd_ChangeUserItem));
		msg.ItemOnlyID = TimeItem.ItemOnlyID;
		msg.EndTime = TimeItem.EndTime;
		msg.ItemSum = TimeItem.ItemSum;
		msg.ItemID = TimeItem.ItemID;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		//���͸ı������ͻ���ȥ
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

	//��һ�μӵĵ���  ������ʱ����� ����Add  �� DBR �������ݿ�ȥ
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

	if (g_FishServer.GetFishConfig().IsTimeItem(pMsg->ItemInfo.ItemID))//ʱ�����
	{
		if (pMsg->ItemInfo.EndTime == 0)//ʱ��Ϊ0
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
	else//����
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
//	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//ʱ�����ֻ��һ�� //��ɾ����ʱ����
//	{
//		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u,ItemSum=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(), ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);
//
//		//DelItemSum = 1;// TimeItem.ItemSum;
//		//����ɾ�������� DBR_Del
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
//	else//��������
//	{
//		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(),ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);
//
//		if (NonTimeItem.ItemSum < ItemSum)
//		{
//			return false;
//		}
//		else if (NonTimeItem.ItemSum == ItemSum)
//		{
//			//����ɾ��������
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
//			//�����޸ĵ�����
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

	if (g_FishServer.GetFishConfig().IsTimeItem(TimeItem.ItemID))//ʱ�����ֻ��һ�� //��ɾ����ʱ����
	{
		//DelItemSum = 1;// TimeItem.ItemSum;
		//����ɾ�������� DBR_Del
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
	else//��������
	{
		//LogInfoToFile("WmItem.txt", "deluserid=%u,itemid=%u, NonTimeItem.ItemSum=%u  TimeItem.ItemSum=%u ", m_pRole->GetUserID(),ItemID, ItemSum,NonTimeItem.ItemSum, TimeItem.ItemSum);

		if (NonTimeItem.ItemSum < ItemSum)
		{
			return false;
		}
		else if (NonTimeItem.ItemSum == ItemSum)
		{
			//����ɾ��������
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
			//�����޸ĵ�����
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
			//ɾ����
			DBR_Cmd_DelUserItem msg;
			SetMsgInfo(msg,DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
			msg.ItemOnlyID = Iter->second.ItemOnlyID;
			g_FishServer.SendNetCmdToSaveDB(&msg);

			//��������ͻ���ȥ ɾ��ָ����Ʒ
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
	AllItemSum -= DelItemSum;//��Ʒ��������
	if (DelItemSum > 0)
	{
		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);//����Ʒ��ɾ����
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
		//ɾ����
		DBR_Cmd_DelUserItem msg;
		SetMsgInfo(msg, DBR_DelUserItem, sizeof(DBR_Cmd_DelUserItem));
		msg.ItemOnlyID = TimeItem.ItemOnlyID;
		g_FishServer.SendNetCmdToSaveDB(&msg);

		//��������ͻ���ȥ ɾ��ָ����Ʒ
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
	//AllItemSum -= DelItemSum;//��Ʒ��������
	if (DelItemSum > 0)
	{
		m_pRole->GetRoleLauncherManager().OnDelItem(ItemID);//����Ʒ��ɾ����
	}

#endif
}