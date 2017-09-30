//��ҵı��������
#pragma once
#include "Stdafx.h"
#include <set>
class CRoleEx;
struct tagChestInfo  //��������Ľ�
{
	BYTE		ChestOnlyID;
	BYTE		OpenSum;//�Ѿ������Ĵ���
	DWORD		ActionTime;//�����ʱ��
	DWORD		ActionStates;//���״̬
	DWORD		ActionOpenStates;//������״̬
	std::vector<ChestOnceStates> ChestArray;
	tagChestConfig ChestConfig;
	tagChestInfo(BYTE ChestOnlyID, BYTE ChestTypeID, tagChestConfig& pConfig)
	{
		this->ChestOnlyID = ChestOnlyID;
		ChestConfig = pConfig;
		OpenSum = 0;
		ActionTime = 0;
		ActionStates = 0;
		ActionOpenStates = 0;
		ChestArray.clear();
	}
	BYTE GetChestTypeID()
	{
		return ChestConfig.ChestID;
	}
	bool IsAction()
	{
		return (ActionTime != 0);
	}

	void OnAction(CRoleEx* pRole, BYTE ChestSum);
	bool OnEndChest(CRoleEx* pRole);
	
	BYTE GetRankTypeID(BYTE RPValue)
	{
		if (OpenSum >= ChestConfig.CostInfo.MaxCostNum)
		{
			ASSERT(false);
			return 0;
		}
		//��ȡһ����������ID �������ļ��ж�ȡ ��ȡ��ʱ��ɾ��
		if (RPValue >= ChestConfig.RewardInfo.SpecialRewardUseRp && ChestConfig.RewardInfo.RewardMap.count(ChestConfig.RewardInfo.SpecialRewardTypeID) == 1)
		{
			return ChestConfig.RewardInfo.SpecialRewardTypeID;
		}
		else
		{
			DWORD RankValue = RandUInt() % ChestConfig.RewardInfo.MaxRewardChances;//������ֵ
			DWORD AddValue = 0;

			HashMap<BYTE, tagChestsReward>::iterator Iter = ChestConfig.RewardInfo.RewardMap.begin();
			for (; Iter != ChestConfig.RewardInfo.RewardMap.end(); ++Iter)
			{
				if (RankValue <= (Iter->second.Chances + AddValue))
				{
					return Iter->second.RewardID;
				}
				else
				{
					AddValue += Iter->second.Chances;
				}
			}
			ASSERT(false);
			return 0;
		}
	}
	void OnAddOpenID(BYTE TypeID, BYTE Index, BYTE RewardOnlyID)
	{
		ActionStates |= TypeID;
		ActionOpenStates |= (1<<Index);

		ChestOnceStates pOnce;
		pOnce.Index = Index;
		pOnce.RewardID = TypeID;
		pOnce.RewardOnlyID = RewardOnlyID;
		ChestArray.push_back(pOnce);

		OpenSum += 1;

		//��Ϊ���ָ���Ľ��� ���ǽ��д��� ��������Map����ɾ����
		HashMap<BYTE, tagChestsReward>::iterator Iter = ChestConfig.RewardInfo.RewardMap.find(TypeID);
		if (Iter != ChestConfig.RewardInfo.RewardMap.end())
		{
			ChestConfig.RewardInfo.MaxRewardChances -= Iter->second.Chances;//�۳�Ȩ��
			ChestConfig.RewardInfo.RewardMap.erase(Iter);
		}
		else
		{
			ASSERT(false); 
		}
	}
	bool IsCanUseTypeID(BYTE TypeID)
	{
		return (ActionStates & TypeID) == 0;
	}
	bool IsCanUseIndex(BYTE Index)
	{
		return (ActionOpenStates & (1<<Index)) == 0;
	}
};
class CRole;
class RoleChestManager  //������ϵı�������� ���ڹ����������Ϸ�����еı���
{
public:
	RoleChestManager();
	virtual ~RoleChestManager();

	void	AddChest(BYTE ChestID);//���һ������
	void	OpenChest(BYTE ChestOnlyID,BYTE ChestIndex);//����һ������ ���µ�
	void    Update(DWORD dwTimer);
	void	CloseChest(BYTE ChestOnlyID);

	void	OnInit(CRole* pRole);
	void    Clear();

	//void	OnRoleAFK();

	void    SendAllChestToClient();
private:
	void	ActionChest();//����һ����ǰ��ı���
	void	ChestClose();//���µı����Ѿ�������
	//BYTE	GetRankTypeID();
private:
	CRole*			m_pRole;		//�������ڵ����
	BYTE			m_RpValue;		//��ҿ��������Rpֵ
	BYTE			m_BeginChestID;
	std::list<tagChestInfo>  m_ChestList;//����ļ���
};