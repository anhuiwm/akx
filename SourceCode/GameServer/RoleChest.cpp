#include "Stdafx.h"
#include "RoleChest.h"
#include "Role.h"
#include "FishServer.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
RoleChestManager::RoleChestManager()
{
	m_ChestList.clear();//�յı��伯��
	m_pRole = NULL;
	m_RpValue = 0;
	m_BeginChestID = 0;
}
RoleChestManager::~RoleChestManager()
{

}
void RoleChestManager::AddChest(BYTE ChestID)
{
	if (!m_pRole || !m_pRole->IsActionUser())
	{
		ASSERT(false);
		return;
	}
	HashMap<BYTE, tagChestConfig>::iterator Iter= g_FishServer.GetFishConfig().GetChestConfig().ChestMap.find(ChestID);
	if (Iter == g_FishServer.GetFishConfig().GetChestConfig().ChestMap.end())
	{
		ASSERT(false);
		return;
	}
	if ((*Iter).second.ImmediateRewardid != 0)
	{		
		m_pRole->GetRoleExInfo()->OnAddRoleRewardByRewardID((*Iter).second.ImmediateRewardid, TEXT("��Ƭ����"));
		return;
	}
	++m_BeginChestID;
	//��ȡ�������ID�� ���Ǽ��뵽��������ȥ
	tagChestInfo pChest(m_BeginChestID, ChestID, Iter->second);//����һ���µı��� ID �� ����
	m_ChestList.push_back(pChest);
	//�ж��Ƿ���Ҫ�����
	if (m_ChestList.size() == 1)
	{
		ActionChest();
	}
}
void RoleChestManager::ActionChest()//������ǰ���һ������
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.empty())
		return;
	if (!m_pRole->IsActionUser())
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.front().IsAction())//�Ѿ�����ı����޷��ٴμ���
	{
		ASSERT(false);
		return;
	}
	//��ʽ������ͷ�ϵı���
	m_ChestList.front().OnAction(m_pRole->GetRoleExInfo(), ConvertDWORDToBYTE(m_ChestList.size()));//�����
}
void RoleChestManager::ChestClose()
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.empty())
	{
		ASSERT(false);
		return;
	}
	if (!m_pRole->IsActionUser())
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.front().OnEndChest(m_pRole->GetRoleExInfo()))
	{
		m_ChestList.pop_front();
		if (!m_ChestList.empty())//���¼���һ������
		{
			ActionChest();
		}
	}
	else
	{
		ASSERT(false);
		return;
	}
}
void RoleChestManager::OpenChest(BYTE ChestOnlyID, BYTE ChestIndex)
{
	if (!m_pRole)
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.empty())
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.front().ChestOnlyID != ChestOnlyID)
	{
		ASSERT(false);
		return;
	}
	if (!m_ChestList.front().IsAction())
	{
		ASSERT(false);
		return;
	}
	if (!m_pRole->IsActionUser())
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_GetChestReward msg;
	SetMsgInfo(msg,GetMsgType(Main_Chest, LC_GetChestReward), sizeof(LC_Cmd_GetChestReward));
	msg.ChestOnlyID = ChestOnlyID;
	msg.ChestStates.Index = ChestIndex;
	msg.ChestStates.RewardID = 0;
	msg.ChestStates.RewardOnlyID = 0;
	msg.Result = false;
	//��ҿ�������
	if (!m_ChestList.front().IsCanUseIndex(ChestIndex))
	{
		ASSERT(false);
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		return;
	}
	if (m_ChestList.front().OpenSum >= m_ChestList.front().ChestConfig.CostInfo.MaxCostNum)
	{
		ASSERT(false);
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		return;
	}
	//1.�жϵ�ǰ�Ļ���
	HashMap<BYTE, DWORD>::iterator IterCost = m_ChestList.front().ChestConfig.CostInfo.CostMap.find(m_ChestList.front().OpenSum + 1);
	if (IterCost == m_ChestList.front().ChestConfig.CostInfo.CostMap.end())
	{
		ASSERT(false);
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		return;
	}
	DWORD UserCurrcey= IterCost->second;
	//2.�۳���ҵĽ�Ǯ
	if (!m_pRole->GetRoleExInfo()->ChangeRoleCurrency(UserCurrcey*-1,TEXT("������Ⱥ���� �����ֱ�")))
	{
		//ASSERT(false);
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		return;
	}
	////3.��Ӵ���
	//m_ChestList.front().OpenSum += 1;
	//4.���г齱 4�������������ظ�
	BYTE RewardTypeID = m_ChestList.front().GetRankTypeID(m_RpValue);//��ȡ���������
	if (RewardTypeID == 0)
	{
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	//��ȡ������ID����д���
	HashMap<BYTE, tagChestsReward>::iterator IterReward = m_ChestList.front().ChestConfig.RewardInfo.RewardMap.find(RewardTypeID);
	if (IterReward == m_ChestList.front().ChestConfig.RewardInfo.RewardMap.end())
	{
		m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
		ASSERT(false);
		return;
	}
	//������⽱���� ��ȡ����������
	bool IsSendReward = false;
	DWORD RandValue = RandUInt() % IterReward->second.MaxOnceChances;
	std::vector<tagChestsRewardOnce>::iterator IterOnce = IterReward->second.RewardMap.begin();
	for (; IterOnce != IterReward->second.RewardMap.end(); ++IterOnce)
	{
		if (RandValue <= IterOnce->Chances)
		{
			//����RPֵ
			if (RewardTypeID == m_ChestList.front().ChestConfig.RewardInfo.SpecialRewardTypeID)
			{
				if (m_RpValue >= m_ChestList.front().ChestConfig.RewardInfo.SpecialRewardUseRp)
					m_RpValue -= m_ChestList.front().ChestConfig.RewardInfo.SpecialRewardUseRp;
				else
					m_RpValue = 0;
			}
			else
			{
				m_RpValue += m_ChestList.front().ChestConfig.RewardInfo.OtherAddRp;
			}
			m_pRole->GetRoleExInfo()->OnAddRoleRewardByRewardID(IterOnce->RewardID,TEXT("��ȡ��Ⱥ���佱��"));
			//��������ͻ��� ��һ�ñ���Ľ��� ��
			msg.Result = true;
			msg.ChestStates.RewardID = RewardTypeID;
			msg.ChestStates.RewardOnlyID = IterOnce->OnceID;
			m_pRole->GetRoleExInfo()->SendDataToClient(&msg);

			m_ChestList.front().OnAddOpenID(RewardTypeID, ChestIndex, IterOnce->OnceID);//

			IsSendReward = true;
			break;
		}
	}
	if (!IsSendReward)
	{
		ASSERT(false);
		return;
	}
	//���������� ������Ҫ���м�������
	if (m_ChestList.front().OpenSum >= m_ChestList.front().ChestConfig.CostInfo.MaxCostNum)//��Ҫ������
	{
		//��ǰ�����Ѿ�ʹ������� �޷�����ʹ���� �����Զ��ر���
		ChestClose();
	}
}
void RoleChestManager::Update(DWORD dwTimer)
{
	//���ڱ���������ϵı���
	if (!m_pRole || m_ChestList.empty() || !m_pRole->IsActionUser())
		return;
	//��ҿ�������
	if ((dwTimer - m_ChestList.front().ActionTime) >= (DWORD)(m_ChestList.front().ChestConfig.ExisteSec * 1000))
	{
		//��ʾ��ǰ���������
		if (m_ChestList.front().OpenSum == 0)
		{
			//����һ��
			OpenChest(m_ChestList.front().ChestOnlyID, 0);//�ڱ���رյ�ʱ�� ������ֱ���Ϊ���� ���Զ���æ����һ��
		}
		ChestClose();//�ر���ǰ�漤��ı���
	}
}
void RoleChestManager::CloseChest(BYTE ChestOnlyID)
{
	//�رյ�ǰ����ı���
	//��Ҳ������������
	if (m_ChestList.empty())
	{
		ASSERT(false);
		return;
	}
	if (m_ChestList.front().ChestOnlyID != ChestOnlyID)
	{
		ASSERT(false);
		return;
	}
	ChestClose();
}
void RoleChestManager::OnInit(CRole* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	m_pRole = pRole;
}
void RoleChestManager::SendAllChestToClient()
{
	if (!m_pRole || !m_pRole->GetRoleExInfo())
	{
		ASSERT(false);
		return;
	}
	LC_Cmd_ResetChest msg;
	SetMsgInfo(msg, GetMsgType(Main_Chest, LC_ResetChest), sizeof(LC_Cmd_ResetChest));
	m_pRole->GetRoleExInfo()->SendDataToClient(&msg);
	if (m_ChestList.empty())
	{
		return;
	}
	HashMap<BYTE, tagChestConfig>::iterator Iter = g_FishServer.GetFishConfig().GetChestConfig().ChestMap.find(m_ChestList.front().GetChestTypeID());
	if (Iter == g_FishServer.GetFishConfig().GetChestConfig().ChestMap.end())
	{
		ASSERT(false);
		return;
	}
	//���ͼ�������
	DWORD PageSize = sizeof(LC_Cmd_ActionChest)+sizeof(ChestOnceStates)*(m_ChestList.front().ChestArray.size() - 1);
	DWORD UserTime = timeGetTime() - m_ChestList.front().ActionTime;
	if (static_cast<DWORD>(Iter->second.ExisteSec * 1000) <= PageSize)
	{
		ASSERT(false);
		return;
	}
	DWORD EndSec = Iter->second.ExisteSec * 1000 - UserTime;
	LC_Cmd_ActionChest* msgAction = (LC_Cmd_ActionChest*)malloc(PageSize);
	if (!msgAction)
	{
		ASSERT(false);
		return;
	}
	CheckMsgSize(PageSize);
	msgAction->SetCmdSize(ConvertDWORDToWORD(PageSize));
	msgAction->SetCmdType(GetMsgType(Main_Chest, LC_ActionChest));
	msgAction->ChestOnlyID = m_ChestList.front().ChestOnlyID;
	msgAction->ChestTypeID = m_ChestList.front().GetChestTypeID();
	msgAction->EndSec = EndSec;
	msgAction->IsReset = true;
	//msgAction->ActionOpenStates = m_ChestList.front().ActionOpenStates;
	//msgAction->ActionRewardStates = m_ChestList.front().ActionStates;
	msgAction->ChestSum = ConvertDWORDToBYTE(m_ChestList.size());
	std::vector<ChestOnceStates>::iterator IterVec = m_ChestList.front().ChestArray.begin();
	for (int i = 0; IterVec != m_ChestList.front().ChestArray.end(); ++i, ++IterVec)
	{
		msgAction->ChestArray[i] = *IterVec;
	}
	msgAction->OpenSum = m_ChestList.front().OpenSum;
	m_pRole->GetRoleExInfo()->SendDataToClient(msgAction);
	free(msgAction);
}
void RoleChestManager::Clear()
{
	//�������ǰ�ı��� ����뿪��ʱ�� �����������ȫ�������
	m_RpValue = 0;
	m_ChestList.clear();
}


void tagChestInfo::OnAction(CRoleEx* pRole, BYTE ChestSum)
{
	if (!pRole)
	{
		ASSERT(false);
		return;
	}
	if (IsAction())
	{
		ASSERT(false);
		return;
	}
	//���ǰ����
	OpenSum = 0;
	ActionTime = timeGetTime();//���ü����ʱ��
	//���Ѿ�����ı�������ݷ��͵��ͻ���ȥ
	DWORD PageSize = sizeof(LC_Cmd_ActionChest)+sizeof(ChestOnceStates)*(ChestArray.size() - 1);
	LC_Cmd_ActionChest* msg = (LC_Cmd_ActionChest*)malloc(PageSize);
	if (!msg)
	{
		ASSERT(false);
		return;
	}
	CheckMsgSize(PageSize);
	msg->SetCmdSize(ConvertDWORDToWORD(PageSize));
	msg->SetCmdType(GetMsgType(Main_Chest, LC_ActionChest));
	msg->IsReset = false;
	msg->ChestOnlyID = ChestOnlyID;
	msg->ChestTypeID = GetChestTypeID();
	msg->EndSec = ChestConfig.ExisteSec * 1000;
	/*msg->ActionOpenStates = m_ChestList.front().ActionOpenStates;
	msg->ActionRewardStates = m_ChestList.front().ActionStates;*/
	msg->ChestSum = ChestSum;
	std::vector<ChestOnceStates>::iterator IterVec = ChestArray.begin();
	for (int i = 0; IterVec != ChestArray.end(); ++i, ++IterVec)
	{
		msg->ChestArray[i] = *IterVec;
	}
	msg->OpenSum = OpenSum;
	pRole->SendDataToClient(msg);
	free(msg);
}
bool tagChestInfo::OnEndChest(CRoleEx* pRole)
{
	if (!pRole)
	{
		ASSERT(false);
		return false;
	}
	//�رյ���ǰ����Ѿ�����ı���
	if (!IsAction())
	{
		ASSERT(false);
		return false;//δ����ı����޷�����
	}
	LC_Cmd_ChestEnd msg;
	SetMsgInfo(msg, GetMsgType(Main_Chest, LC_ChestEnd), sizeof(LC_Cmd_ChestEnd));
	msg.ChestOnlyID = ChestOnlyID;
	pRole->SendDataToClient(&msg);
	return true;
}