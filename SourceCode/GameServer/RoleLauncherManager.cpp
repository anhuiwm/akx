#include "Stdafx.h"
#include "RoleEx.h"
#include "FishServer.h"
#include "RoleLauncherManager.h"
RoleLauncherManager::RoleLauncherManager()
{
	m_LauncherStates = 0; 
	//m_UsingLauncher = 0;
	m_pConfig = null;
	m_pRole = null;
	m_ItemToLauncherMap.clear();
}
RoleLauncherManager::~RoleLauncherManager()
{
	m_LauncherStates = 0;
	//m_UsingLauncher = 0;
	m_pConfig = null;
	m_pRole = null;
	m_ItemToLauncherMap.clear();
}
void RoleLauncherManager::OnInit(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	m_LauncherStates = 0;
	//m_UsingLauncher = 0;
	m_pConfig = g_FishServer.GetTableManager()->GetGameConfig();
	if (!m_pConfig)
	{
		ASSERT(false);
		return;
	}
	time_t pNow = time(null);
	m_pRole = pRole;
	m_ItemToLauncherMap.clear();
	for (int i = 0; i < MAX_LAUNCHER_NUM; ++i)
	{
		DWORD ItemID = 0;
		DWORD ItemSum = 0;
		m_pConfig->GoodsInfo(i, (int&)ItemID, (int&)ItemSum);
		if (ItemID == 0 || ItemSum == 0)
		{
			//��ǰ��̨������Ʒ ֱ������Ϊ ����
			m_LauncherStates |= (1 << (i + 1));//������̨��״̬����
			continue;
		}
		m_ItemToLauncherMap.insert(HashMap<DWORD, BYTE>::value_type(ItemID, i));

		DWORD AllItemSum = m_pRole->GetItemManager().QueryItemCount(ItemID);
		if (AllItemSum >= ItemSum)
		{
			m_LauncherStates |= (1 << (i + 1));//������̨��״̬����
			continue;
		}

		//�ж����VIP�ȼ�
		if (m_pRole->GetRoleInfo().VipLevel != 0)
		{
			HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(m_pRole->GetRoleInfo().VipLevel);
			if (Iter != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
			{
				if (Iter->second.CanUseLauncherMap.count(i) == 1)
				{
					//����ʹ����
					m_LauncherStates |= (1 << (i + 1));//������̨��״̬����
					continue;
				}
			}
		}

		//�ж�����¿�
		//if (m_pRole->GetRoleInfo().MonthCardID != 0 && pNow < m_pRole->GetRoleInfo().MonthCardEndTime)
		//{
		//	HashMap<BYTE, tagMonthCardOnce>::iterator Iter= g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.find(m_pRole->GetRoleInfo().MonthCardID);
		//	if (Iter != g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.end())
		//	{
		//		if (Iter->second.CanUseLauncherMap.count(i) == 1)
		//		{
		//			//����ʹ����
		//			m_LauncherStates |= (1 << (i + 1));//������̨��״̬����
		//			continue;
		//		}
		//	}
		//}
	}
	//�����ݷ��͵��ͻ���ȥ.
	if (m_pRole->IsRobot())
	{
		return;
	}
	LC_Cmd_LauncherData msg;
	SetMsgInfo(msg, GetMsgType(Main_Launcher, LC_LauncherData), sizeof(LC_Cmd_LauncherData));
	msg.LauncherData = m_LauncherStates;
	m_pRole->SendDataToClient(&msg);
}
void RoleLauncherManager::OnAddItem(DWORD ItemID)
{
	if (!m_pConfig || !m_pRole)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, BYTE>::iterator Iter = m_ItemToLauncherMap.find(ItemID);
	if (Iter == m_ItemToLauncherMap.end())
		return;
	//����Ʒ
	if (IsCanUserLauncherByID(Iter->second))//�Ѿ������õ�
		return;
	//�����ñ�Ϊ����
	int nItemID = 0;
	int nItemSum = 0;
	int Launcher = Iter->second;
	m_pConfig->GoodsInfo(Launcher, nItemID, nItemSum);
	DWORD AllItemSum = m_pRole->GetItemManager().QueryItemCount(nItemID);
	if (AllItemSum >= ConvertIntToDWORD(nItemSum))
	{
		m_LauncherStates |= (1 << (Launcher + 1));
		//��������ͻ���ȥ 
		LC_Cmd_LauncherData msg;
		SetMsgInfo(msg, GetMsgType(Main_Launcher, LC_LauncherData), sizeof(LC_Cmd_LauncherData));
		msg.LauncherData = m_LauncherStates;
		m_pRole->SendDataToClient(&msg);

		if (GetUsingLauncher() != Launcher)
		{
			SetUsingLauncher(Launcher);
			LC_Cmd_LauncherType msgtype;
			SetMsgInfo(msgtype, GetMsgType(Main_Launcher, LC_LauncherType), sizeof(LC_Cmd_LauncherType));
			msgtype.LauncherType = GetUsingLauncher();
			m_pRole->SendDataToClient(&msg);
		}

		//�ж���ҵ�ǰ���Ƿ�仯�� ������Ҫ�ж�
		CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
		if (pRole /*&& pRole->GetLauncherType() == Launcher*/)
		{
			pRole->OnResetRoleLauncher();
		}
	}
}
void RoleLauncherManager::OnDelItem(DWORD ItemID)
{
	if (!m_pConfig || !m_pRole)
	{
		ASSERT(false);
		return;
	}
	HashMap<DWORD, BYTE>::iterator Iter = m_ItemToLauncherMap.find(ItemID);
	if (Iter == m_ItemToLauncherMap.end())
		return;
	if (!IsCanUserLauncherByID(Iter->second))
		return;
	//��Ϊ����ʹ�� ������Ҫ�ж��Ƿ�ΪVIP����� ���õ�
	if (m_pRole->GetRoleInfo().VipLevel != 0)
	{
		HashMap<BYTE, tagVipOnce>::iterator IterVip = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(m_pRole->GetRoleInfo().VipLevel);
		if (IterVip != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
		{
			if (IterVip->second.CanUseLauncherMap.count(Iter->second) == 1)
			{
				//��ǰ��ΪVIP������� ��Ҫɾ��
				return;
			}
		}
	}

	int nItemID = 0;
	int nItemSum = 0;
	int Launcher = Iter->second;
	m_pConfig->GoodsInfo(Launcher, nItemID, nItemSum);
	DWORD AllItemSum = m_pRole->GetItemManager().QueryItemCount(nItemID);
	if (AllItemSum < ConvertIntToDWORD(nItemSum))
	{
		//��ǰ��̨�޷�ʹ��
		m_LauncherStates ^= (1 << (Launcher + 1));
		//��������ͻ���ȥ
		LC_Cmd_LauncherData msg;
		SetMsgInfo(msg, GetMsgType(Main_Launcher, LC_LauncherData), sizeof(LC_Cmd_LauncherData));
		msg.LauncherData = m_LauncherStates;
		m_pRole->SendDataToClient(&msg);

		if (GetUsingLauncher() == Launcher)
		{
			ResetUsingLauncher();
			LC_Cmd_LauncherType msgtype;
			SetMsgInfo(msgtype, GetMsgType(Main_Launcher, LC_LauncherType), sizeof(LC_Cmd_LauncherType));
			msgtype.LauncherType = GetUsingLauncher();
			m_pRole->SendDataToClient(&msg);
		}

		CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
		if (pRole && pRole->GetLauncherType() == Launcher)
			pRole->OnResetRoleLauncher();//��Ϊʧȥ�� ���Ǵ����������̨״̬��ǰ��
	}
}
bool RoleLauncherManager::IsCanUserLauncherByID(BYTE LauncherID)
{
	if (LauncherID >= 32 || LauncherID >= MAX_LAUNCHER_NUM)
	{
		ASSERT(false);
		return false;
	}
	return  ((m_LauncherStates & (1 << (LauncherID + 1))) != 0);
}
void RoleLauncherManager::OnVipLevelChange(BYTE OldVipLevel, BYTE NewVipLevel)
{
	//��VIP�����仯��ʱ�� �������Ƴ��ɵ�VIP��̨���� �� ����µ���̨����
	if (OldVipLevel != 0)
	{
		HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(OldVipLevel);
		if (Iter != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
		{
			set<BYTE>::iterator IterLauncher = Iter->second.CanUseLauncherMap.begin();
			for (; IterLauncher != Iter->second.CanUseLauncherMap.end(); ++IterLauncher)
			{
				m_LauncherStates ^= (1 << (*IterLauncher + 1));
			}
		}
	}
	if (NewVipLevel != 0)
	{
		HashMap<BYTE, tagVipOnce>::iterator Iter = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(NewVipLevel);
		if (Iter != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
		{
			set<BYTE>::iterator IterLauncher = Iter->second.CanUseLauncherMap.begin();
			for (; IterLauncher != Iter->second.CanUseLauncherMap.end(); ++IterLauncher)
			{
				m_LauncherStates |= (1 << (*IterLauncher + 1));//������̨��״̬����
			}
		}
	}
	LC_Cmd_LauncherData msg;
	SetMsgInfo(msg, GetMsgType(Main_Launcher, LC_LauncherData), sizeof(LC_Cmd_LauncherData));
	msg.LauncherData = m_LauncherStates;
	m_pRole->SendDataToClient(&msg);

	ResetUsingLauncher();
	LC_Cmd_LauncherType msgtype;
	SetMsgInfo(msgtype, GetMsgType(Main_Launcher, LC_LauncherType), sizeof(LC_Cmd_LauncherType));
	msgtype.LauncherType = GetUsingLauncher();
	m_pRole->SendDataToClient(&msg);

	CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
	if (pRole)
		pRole->OnResetRoleLauncher();
}
bool RoleLauncherManager::IsCanUseLauncherAllTime(BYTE LauncherID)
{
	if (!IsCanUserLauncherByID(LauncherID))
		return false;
	//ӵ�е��� �ж��Ƿ�ΪVIP��
	if (m_pRole->GetRoleInfo().VipLevel != 0)
	{
		HashMap<BYTE, tagVipOnce>::iterator IterVip = g_FishServer.GetFishConfig().GetVipConfig().VipMap.find(m_pRole->GetRoleInfo().VipLevel);
		if (IterVip != g_FishServer.GetFishConfig().GetVipConfig().VipMap.end())
		{
			if (IterVip->second.CanUseLauncherMap.count(LauncherID) == 1)
				return true;
		}
	}
	//�ڲ���VIP���� ���Ǽ����ж� �Ƿ�����ӵ����Ʒ
	int nItemID = 0;
	int nItemSum = 0;
	int Launcher = LauncherID;
	m_pConfig->GoodsInfo(Launcher, nItemID, nItemSum);
	return m_pRole->GetItemManager().GetItemIsAllExists(nItemID, nItemSum);
}

BYTE  RoleLauncherManager::GetUsingLauncher()
{
	if (m_pRole)
	{
		return m_pRole->GetRoleInfo().byUsingLauncher;
	}
	return 0;

}
void  RoleLauncherManager::SetUsingLauncher(BYTE Launcher)
{
	if (m_pRole)
	{
		if (m_pRole->GetRoleInfo().byUsingLauncher != Launcher)
		{
			m_pRole->GetRoleInfo().byUsingLauncher = Launcher;
			DBR_Cmd_SaveRoleUsingLauncher msgDB;
			SetMsgInfo(msgDB, DBR_SaveRoleUsingLauncher, sizeof(DBR_Cmd_SaveRoleUsingLauncher));
			msgDB.dwUserID = m_pRole->GetRoleInfo().dwUserID;
			msgDB.byUsingLauncher = m_pRole->GetRoleInfo().byUsingLauncher;
			g_FishServer.SendNetCmdToSaveDB(&msgDB);
		}
	}

}

void  RoleLauncherManager::ResetUsingLauncher()
{
	BYTE Launcher = GetUsingLauncher();
	//if (Launcher == 0)
	{
		for (BYTE i = MAX_LAUNCHER_NUM - 1; i >= 0; --i)
		{
			if (IsCanUserLauncherByID(i))
			{
				Launcher = i;
				return;
			}
		}
	}

	SetUsingLauncher(Launcher);
}

//void RoleLauncherManager::OnMonthCardChange(BYTE OldMonthCardID, BYTE NewMonthCardID)
//{
//	if (OldMonthCardID != 0)
//	{
//		HashMap<BYTE, tagMonthCardOnce>::iterator Iter= g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.find(OldMonthCardID);
//		if (Iter != g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.end())
//		{
//			set<BYTE>::iterator IterLauncher = Iter->second.CanUseLauncherMap.begin();
//			for (; IterLauncher != Iter->second.CanUseLauncherMap.end(); ++IterLauncher)
//			{
//				m_LauncherStates ^= (1 << (*IterLauncher + 1));
//			}
//		}
//	}
//	if (NewMonthCardID != 0)
//	{
//		HashMap<BYTE, tagMonthCardOnce>::iterator Iter = g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.find(NewMonthCardID);
//		if (Iter != g_FishServer.GetFishConfig().GetMonthCardConfig().MonthCardMap.end())
//		{
//			set<BYTE>::iterator IterLauncher = Iter->second.CanUseLauncherMap.begin();
//			for (; IterLauncher != Iter->second.CanUseLauncherMap.end(); ++IterLauncher)
//			{
//				m_LauncherStates += (1 << (*IterLauncher + 1));//������̨��״̬����
//			}
//		}
//	}
//	LC_Cmd_LauncherData msg;
//	SetMsgInfo(msg, GetMsgType(Main_Launcher, LC_LauncherData), sizeof(LC_Cmd_LauncherData));
//	msg.LauncherData = m_LauncherStates;
//	m_pRole->SendDataToClient(&msg);
//
//	CRole* pRole = g_FishServer.GetTableManager()->SearchUser(m_pRole->GetUserID());
//	if (pRole)
//		pRole->OnResetRoleLauncher();
//}