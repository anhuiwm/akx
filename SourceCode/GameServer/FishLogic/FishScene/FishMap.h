#pragma once
#include "FishResData.h"
#include <map>
#include <vector>
#include "TArray.h"
using namespace std;

struct FishPackageData
{
	float	StartSeconds;		//��ʼʱ��
	float	EndSeconds;			//����ʱ��
	float	IntervalSeconds;	//ÿ�γ��ֵļ��
	USHORT	Chance;				//����
	USHORT	MaxCount;			//���������
	BYTE	PackageType;		//�������
	vector<USHORT> FishGroup;
};
struct FlowIndexData
{
	USHORT	FlowIdx;
	bool	LastIdx;
};
struct FlowOrderData
{
	vector<USHORT>	OrderIndex;
	vector<USHORT>	FlowIndex;
};
struct FishMapData
{
	WCHAR			FileName[MAX_MAP_NAME];		//��ͼ�ļ�����
	WCHAR			MapName[MAX_MAP_NAME];		//��ͼ����
	bool			Repeat;						//�Ƿ������������֮�����¿�ʼ
	BYTE			BackgroudImage;				//����ͼƬ
	float			MaxTime;					//�������������ʱ��
	vector<USHORT>  m_fishBoss;                 //�ٻ�boss ��
	vector<USHORT>	FLowList;					//����
	vector<FishPackageData> PackageList;		//���
	FlowOrderData	FlowOrderData;
};
typedef HashMap<UINT, FishMapData*> FISHMAP;
typedef vector<FlowIndexData*> FlowDataList;
class FishMap
{
public:
	FishMap();
	~FishMap();
	
	static bool LoadAllMap(const WCHAR *pcDir);
	static void ReleaseAllMap();
	bool IsLoadMap()const
	{
		return m_pMap != NULL;
	}
	bool LoadMap(const WCHAR *mapName);
	void ReleaseMap();
	USHORT GetFlowIndex(bool &bFixedGroup);
	bool HasFlowIndex()
	{
		return m_FlowList.size() > 0;
	}
	void Reset();
	bool IsRepeat()
	{
		return m_pMap->Repeat;
	}
	//����-1��ʾ�������
	UINT Update(float deltaTime);

	float MaxTime()const
	{
		return m_pMap->MaxTime;
	}
	byte BackgroundType()const
	{
		return m_pMap->BackgroudImage;
	}
	USHORT GetMapFishBoss()
	{
		if (m_pMap->m_fishBoss.size() == 0)
			return 200;

		return m_pMap->m_fishBoss[RandUInt() % m_pMap->m_fishBoss.size()];
		//return m_pMap->m_fishBoss;
	}
	const WCHAR *GetMapName()const
	{
		return m_pMap->MapName;
	}
	static FISHMAP& GetAllMapList()
	{
		return ms_AllMapList;
	}
protected:
	USHORT GetPackageGroupID(FishPackageData &fpd);
	int				m_nIndex;
	vector<USHORT>	m_PackageCount;
	USHORT			m_LastPackageGroupID;
	float			m_Time;
	float			m_Interval;
	FishMapData		*m_pMap;
	FlowDataList	m_FlowList;
	FlowDataList	m_FlowData;
	static FISHMAP	ms_AllMapList;
};