#pragma once
#include "Fish.h"
#include "TArray.h"
typedef HashMap<USHORT, Fish*> FishMapList;
typedef FishMapList::iterator FishMapIt;
typedef vector<Fish*> FishVecList;
class FishManager
{
public:
	bool Init()
	{
		return true;
	}
	void Shutdown()
	{
		ClearAllFish(true);
	}
	void Update(float delta)
	{
		m_FishVecList.clear();
		for (FishMapIt it = m_FishMapList.begin(); it != m_FishMapList.end();)
		{
			Fish *pFish = it->second;
			if (!pFish->UpdateFish(delta))
			{
				delete pFish;
				it = m_FishMapList.erase(it);
			}
			else
			{
				if (pFish->IsActive && pFish->IsValidTime() && pFish->FishType != 24)
					m_FishVecList.push_back(pFish);
				//return;
				++it;
			}
		}
		//�����б����
		for (UINT i = 0; i < m_DeadFishList.size();)
		{
			Fish *pFish = m_DeadFishList[i];
			if (pFish->UpdateDeadTime(delta))
			{
				ListRemoveAt(m_DeadFishList, i);
				delete(pFish);
			}
			else
			{
				++i;
			}
		}
	}
	
	bool AddFish(Fish *pFish)
	{
		if (m_FishMapList.insert(make_pair(pFish->FishID, pFish)).second)
		{
			return true;
		}
		else
		{
			MessageBoxA(0, "�����ʧ�ܣ�������ͬ��ID!", 0,0);
			//InnerFishLog(L"�����ʧ�ܣ�������ͬ��ID:%d", pFish->FishID);
			return false;
		}
	}
	void RemoveFish(FISHID FishID, float speedScaling, float stayTime[])
	{
		FishMapIt it = m_FishMapList.find(FishID);
		if (it != m_FishMapList.end())
		{
			Fish *pFish = it->second;
			pFish->Controller.AddSkillTimeScaling(speedScaling, stayTime, DELAY_SKILL);
			pFish->SetDead(pFish->Controller.GetDelayData().AllTime);
			m_DeadFishList.push_back(pFish);
			m_FishMapList.erase(it);
		}
	}
	void RemoveFishImmediate(FISHID FishID)
	{
		FishMapIt it = m_FishMapList.find(FishID);
		if (it != m_FishMapList.end())
		{
			delete it->second;
			m_FishMapList.erase(it);
		}
	}
	void ClearAllFish(bool bReset)
	{
		for (UINT i = 0; i < m_DeadFishList.size(); ++i)
			delete(m_DeadFishList[i]);

		for (FishMapIt it = m_FishMapList.begin(); it != m_FishMapList.end(); )
		{
			delete(it->second);
			it = m_FishMapList.erase(it);
		}
		m_DeadFishList.clear();
		m_FishVecList.clear();
		m_TrunDeadFishList.clear();		
	}
	UINT GetFishCount()
	{
		return m_FishMapList.size() + m_DeadFishList.size();
	}
	Fish* GetFish(USHORT id)
	{
		FishMapIt it = m_FishMapList.find(id);
		if (it != m_FishMapList.end())
			return it->second;
		else
			return NULL;
	}
	//���λ�á�
	void RandVecList()
	{
		UINT count = m_FishVecList.size();
		for (UINT i = 0; i < count; ++i)
		{
			int idx = RandRange(0, count);
			Fish *pt = m_FishVecList[i];
			m_FishVecList[i] = m_FishVecList[idx];
			m_FishVecList[idx] = pt;
		}
	}

	//��ȡ�����о����������
	USHORT GetNearFish(Vector2 ScreenPos)
	{
		//uint gold = 0;
		USHORT fishID = 0;
		//float dist = float.MaxValue;
		//Dictionary<ushort, Fish>.ValueCollection fishlist = m_FishList.Values;
		float minlen = 1000.0f;
		for(auto pfish : m_FishVecList)
		{
			if (pfish->FishID == 24)
			{
				continue;
			}
			Vector2 dir = ScreenPos - pfish->ScreenPos;
			float dirlen = Vec2Length(dir);
			if (dirlen < minlen)
			{
				minlen = dirlen;
				fishID = pfish->FishID;
			}
			//LogInfoToFile("WmTestLog.txt", "fishID=%d, screenPos.x=%f, screenPos.y=%f ,pfish->ScreenPos.x=%f, pfish->ScreenPos.y=%f", fishID, ScreenPos.x, ScreenPos.y, pfish->ScreenPos.x, pfish->ScreenPos.y);
		}
		return fishID;
	}

	FishVecList* GetFishVecList()
	{
		return &m_FishVecList;
	}
	FishMapList* GetFishMapList()
	{
		return &m_FishMapList;
	}
	INT FishCount()const
	{
		return m_FishMapList.size();
	}
	vector<Fish*>& GetDeadFishList()
	{
		return m_DeadFishList;
	}
	vector<Fish*>& GetTrunDeadFishList()
	{
		return m_TrunDeadFishList;
	}
	void ClearTrunDeadFishList()
	{
		m_TrunDeadFishList.clear();
	}
	void AddTrunDeadFishList(Fish* pFish)
	{
		m_TrunDeadFishList.push_back(pFish);
	}
protected:
	FishMapList	m_FishMapList;
	FishVecList	m_DeadFishList;
	FishVecList	m_FishVecList;
	FishVecList	m_TrunDeadFishList;
};

