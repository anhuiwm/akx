#include "stdafx.h"
#include "FishMap.h"
#include "Crc32.h"
#include "tinyxml2.h"
#include "Random.h"
#include <algorithm>
#include "FishResManager.h"
FISHMAP FishMap::ms_AllMapList;
#include <vector>
#include <string>
using namespace tinyxml2;
#define LAST_COUNT 5
struct FindXMLData
{
	wstring fileName;
	wstring path;
};
typedef vector<FindXMLData> StrMapList;
bool IsEqualStr(const char *pc, const char *pc2, bool ignor)
{
	if (ignor)
		return _stricmp(pc, pc2) == 0;
	else
		return strcmp(pc, pc2) == 0;
}
void FindFile(wstring &dir, StrMapList &fileList)
{
	wstring root = dir;
	if (*root.rbegin() != L'\\' && *root.rbegin() != L'/')
		root += L"/";
	wstring fidstr = root + L"*.*";
	WIN32_FIND_DATA fd;
	ZeroMemory(&fd, sizeof(WIN32_FIND_DATA));
	HANDLE hFile = FindFirstFile(fidstr.c_str(), &fd);
	while (hFile != INVALID_HANDLE_VALUE)
	{
		if (fd.cFileName[0] == L'.')
		{

		}
		else if(fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			wstring subdir = root + fd.cFileName;
			FindFile(subdir, fileList);
		}
		else 
		{
			wstring file = fd.cFileName;
			UINT size = file.size();
			if (file.c_str()[size - 4] == L'.' &&
				::towlower(file.c_str()[size - 3]) == L'x' &&
				::towlower(file.c_str()[size - 2]) == L'm' &&
				::towlower(file.c_str()[size - 1]) == L'l')
			{
				FindXMLData dl;
				dl.path = root + file;
				dl.fileName = file.erase(file.size() - 4, 4);
				fileList.push_back(dl);
			}
		}
		if (!FindNextFile(hFile, &fd))
			break;
	}
}

FishMap::FishMap()
{
	m_pMap = NULL;
}
FishMap::~FishMap()
{
	m_pMap = NULL;
	ReleaseMap();
}
bool SortPackageData(const FishPackageData &f1, const FishPackageData &f2)
{
	if (f1.StartSeconds < f2.StartSeconds)
		return true;
	if (f1.StartSeconds == f2.StartSeconds)
		return f1.EndSeconds < f2.EndSeconds;
	return false;
}
bool LoadRandFlow(XMLElement *pFlow, FishMapData *pFishMapData, wstring &file)
{
	XMLElement *pEle = pFlow->FirstChildElement();
	while (pEle)
	{
		if (pEle->FirstChild() == NULL)
		{
			InnerFishLog(L"�����ļ�δ����FLow����:%s", file.c_str());
			return false;
		}
		int idx = atoi(pEle->FirstChild()->Value());
		pFishMapData->FLowList.push_back(ConvertIntToWORD(idx));
		pEle = pEle->NextSiblingElement();
	}
	return true;
}

bool LoadFishBoss(XMLElement *pFlow, FishMapData *pFishMapData, wstring &file)
{
	XMLElement *pEle = pFlow->FirstChildElement();
	while (pEle)
	{
		if (pEle->FirstChild() == NULL)
		{
			InnerFishLog(L"�����ļ�δ����FishBoss����:%s", file.c_str());
			return false;
		}
		int idx = atoi(pEle->FirstChild()->Value());
		GroupData* pgroup = FishResManager::Inst()->GetGroup(idx);
		ASSERT(pgroup != NULL);
		//{
		//	LogInfoToFile("WmFileErrorLog.txt", "group=%d", idx);
		//	//continue;
		//}
		pFishMapData->m_fishBoss.push_back( ConvertIntToWORD(idx) );
		pEle = pEle->NextSiblingElement();
	}
	return true;
}

bool LoadFixedFlow(XMLElement *pFixedFlow, FishMapData *pFishMapData, wstring &file)
{
	XMLElement *pOrder = pFixedFlow->FirstChildElement();
	XMLElement *pFlow = pFixedFlow->LastChildElement();
	if (pOrder == NULL || pFlow == NULL)
	{
		InnerFishLog(L"�����ļ�ȱ�ٹ̶���˳��ڵ�:%s", file.c_str());
		return false;
	}
	if (IsEqualStr(pOrder->Name(), "flows", true))
	{
		XMLElement *temp = pOrder;
		pOrder = pFlow;
		pFlow = pOrder;
	}
	XMLElement *pEle = pOrder->FirstChildElement();
	while (pEle)
	{
		int idx = atoi(pEle->FirstChild()->Value());
		pFishMapData->FlowOrderData.OrderIndex.push_back(ConvertIntToWORD(idx));
		pEle = pEle->NextSiblingElement();
	}
	pEle = pFlow->FirstChildElement();
	while (pEle)
	{
		int idx = atoi(pEle->FirstChild()->Value());
		pFishMapData->FlowOrderData.FlowIndex.push_back(ConvertIntToWORD(idx));
		pEle = pEle->NextSiblingElement();
	}
	return true;
}
bool LoadPackage(XMLElement *pPackage, FishMapData *pFishMapData, wstring &file)
{
	XMLElement *pEle = pPackage->FirstChildElement();
	const char *pcAtt;
	while (pEle)
	{
		FishPackageData pd;
		pcAtt = pEle->Attribute("StartSeconds");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����StartSeconds����:%s", file.c_str());
			return false;
		}
		pd.StartSeconds = (float)atoi(pcAtt);

		pcAtt = pEle->Attribute("EndSeconds");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����EndSeconds����:%s", file.c_str());
			return false;
		}
		pd.EndSeconds = (float)atoi(pcAtt);

		pcAtt = pEle->Attribute("IntervalSeconds");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����IntervalSeconds����:%s", file.c_str());
			return false;
		}
		pd.IntervalSeconds = (float)atoi(pcAtt);

		pcAtt = pEle->Attribute("Chance");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����Chance����:%s", file.c_str());
			return false;
		}
		pd.Chance = ConvertIntToWORD(atoi(pcAtt));

		pcAtt = pEle->Attribute("MaxCount");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����MaxCount����:%s", file.c_str());
			return false;
		}
		pd.MaxCount = ConvertIntToWORD(atoi(pcAtt));

		pcAtt = pEle->Attribute("PackageType");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����PackageType����:%s", file.c_str());
			return false;
		}
		XMLElement *pc = pEle->FirstChildElement();
		while (pc)
		{
			pd.FishGroup.push_back(ConvertIntToShort(atoi(pc->FirstChild()->Value())));
			pc = pc->NextSiblingElement();
		}
		pd.PackageType = ConvertIntToBYTE(atoi(pcAtt));
		pFishMapData->PackageList.push_back(pd);
		pEle = pEle->NextSiblingElement();
	}
	return true;
}
bool FishMap::LoadAllMap(const WCHAR *pcDir)
{
	wstring dir = pcDir;
	StrMapList fileList;
	FindFile(dir, fileList);
	
	tinyxml2::XMLDocument doc;

	char path[MAX_PATH];
	for (UINT i = 0; i < fileList.size(); ++i)
	{
		wstring &file = fileList[i].path;
		int namecount = fileList[i].fileName.size();
		if (namecount > MAX_MAP_NAME)
		{
			InnerFishLog(L"��ͼ���Ƴ��ȳ���ָ������(%d):%s", fileList[i].fileName.size(), file.c_str());
			return false;
		}
		::WideCharToMultiByte(CP_ACP, 0, file.c_str(), -1, path, sizeof(path), NULL, 0);
		if (doc.LoadFile(path) != XML_NO_ERROR)
		{
			InnerFishLog(L"���ص�ͼ�ļ�����:%s", file.c_str());
			return false;
		}
		XMLElement *pFirstElement = doc.FirstChildElement();
		if (IsEqualStr(pFirstElement->Name(), "map", true) == false)
		{
			InnerFishLog(L"�����ļ�����ȷ:%s", file.c_str());
			return false;
		}
		FishMapData *pFishMapData = new FishMapData;
		const char *pcAtt = pFirstElement->Attribute("BackgroundImage");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����BackgroundImage����:%s", file.c_str());
			return false;
		}
		pFishMapData->BackgroudImage = ConvertIntToBYTE(atoi(pcAtt));
		pcAtt = pFirstElement->Attribute("Repeat");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����Repeat����:%s", file.c_str());
			return false;
		}
		pFishMapData->Repeat = IsEqualStr(pcAtt, "true", true);

		pcAtt = pFirstElement->Attribute("Name");
		if (pcAtt == NULL)
		{
			InnerFishLog(L"�����ļ�δ����Name����:%s", file.c_str());
			return false;
		}
		::MultiByteToWideChar(CP_UTF8, 0, pcAtt, -1, pFishMapData->MapName, MAX_MAP_NAME);
		XMLElement *pEle = pFirstElement->FirstChildElement();
		bool bHasFixedFlow = false;
		bool bHasPackage = false;
		bool bHasRandFlow = false;
		bool bHasFishBoss = false;
		while (pEle)
		{
			if (IsEqualStr(pEle->Name(), "packages", true))
			{
				if (!LoadPackage(pEle, pFishMapData, file))
					return false;
				bHasPackage = true;
			}
			else if (IsEqualStr(pEle->Name(), "random_flows", true))
			{
				if (!LoadRandFlow(pEle, pFishMapData, file))
					return false;
				bHasRandFlow = true;
			}
			else if (IsEqualStr(pEle->Name(), "fixed_flows", true))
			{
				if (!LoadFixedFlow(pEle, pFishMapData, file))
					return false;
				bHasFixedFlow = true;
			}
			else if (IsEqualStr(pEle->Name(), "fishboss", true))
			{
				if (!LoadFishBoss(pEle, pFishMapData, file))
					return false;
				bHasFishBoss = true;
			}
			pEle = pEle->NextSiblingElement();
		}
		if (!bHasFixedFlow || !bHasPackage || !bHasRandFlow)
		{
			InnerFishLog(L"�����ļ�����ȷ��ȱ�ٹ̶���������ߺ���ڵ�:%s", file.c_str());
			return false;
		}
		
		if (!bHasFishBoss)
		{
			InnerFishLog(L"�����ļ�����ȷ��ȱ���ٻ�boss�ڵ�:%s", file.c_str());
			return false;
		}

		if (pFishMapData->FLowList.size() == 0)
		{
			InnerFishLog(L"�����ļ���������Ϊ0:%s", file.c_str());
			return false;
		}
		UINT crc = AECrc32(fileList[i].fileName.c_str(), fileList[i].fileName.size() * sizeof(WCHAR), 0);
		if (ms_AllMapList.find(crc) != ms_AllMapList.end())
		{
			InnerFishLog(L"������ͬ�ĵ�ͼ����:%s", file.c_str());
			return false;
		}
		//��֤
		for (UINT j = 0; j < pFishMapData->FlowOrderData.FlowIndex.size(); ++j)
		{
			int idx = pFishMapData->FlowOrderData.FlowIndex[j];
			FlowData *pFlow = FishResManager::Inst()->GetFlow(idx);
			if (pFlow == NULL)
			{
				InnerFishLog(L"��ͼ�еĹ̶�����ΪNUMM:%s ��%d", file.c_str(), j);
				return false;
			}
		}
		for (UINT j = 0; j < pFishMapData->FlowOrderData.OrderIndex.size(); ++j)
		{
			USHORT idx = pFishMapData->FlowOrderData.OrderIndex[j];
			if (idx < 0 || idx > pFishMapData->FLowList.size())
			{
				InnerFishLog(L"��ͼ�еĹ̶�����˳����ȷ:%s ��%d", file.c_str(), idx);
				return false;
			}
		}
		if (pFishMapData->FlowOrderData.OrderIndex.size() > 0 &&
			pFishMapData->FlowOrderData.FlowIndex.size() == 0)
		{
			InnerFishLog(L"��ͼ�еĹ̶�����Ϊ0������˳������:%s ��%d", file.c_str(), pFishMapData->FlowOrderData.OrderIndex.size());
			return false;
		}
		::sort(pFishMapData->FlowOrderData.OrderIndex.begin(), pFishMapData->FlowOrderData.OrderIndex.end());
		::sort(pFishMapData->PackageList.begin(), pFishMapData->PackageList.end(), SortPackageData);
		for (UINT j = 1; j < pFishMapData->PackageList.size(); ++j)
		{
			if (pFishMapData->PackageList[j].StartSeconds < pFishMapData->PackageList[j - 1].EndSeconds)
			{
				InnerFishLog(L"�������ʱ���ص�:%s", file.c_str());
				return false;
			}
			if (pFishMapData->PackageList[j].EndSeconds < pFishMapData->PackageList[j].StartSeconds)
			{
				InnerFishLog(L"����������ʱ��С����ʼʱ��:%s", file.c_str());
				return false;
			}
		}
		float maxtime = 0;
		for (UINT j = 0; j < pFishMapData->FLowList.size(); ++j)
		{
			int idx = pFishMapData->FLowList[j];
			FlowData *pFlow = FishResManager::Inst()->GetFlow(idx);
			if (pFlow == NULL)
			{
				InnerFishLog(L"��ͼ�е�����ΪNUMM:%s ��%d", file.c_str(), j);
				return false;
			}
			maxtime += pFlow->GetMaxTimeSeconds();
		}
		pFishMapData->MaxTime = maxtime;
		wcscpy_s(pFishMapData->FileName, MAX_MAP_NAME, fileList[i].fileName.c_str());
		ms_AllMapList[crc] = pFishMapData;
		doc.Clear();
	}
	InnerFishLog(L"���ص�ͼ:%d", ms_AllMapList.size());
	return true;
}
void FishMap::ReleaseAllMap()
{
	for (FISHMAP::iterator it = ms_AllMapList.begin(); it != ms_AllMapList.end(); ++it)
	{
		delete(it->second);
	}
	ms_AllMapList.clear();
}

bool FishMap::LoadMap(const WCHAR *mapName)
{
	UINT crc = AECrc32(mapName, wcslen(mapName) * sizeof(WCHAR), 0);
	FISHMAP::iterator it = ms_AllMapList.find(crc);
	if (it == ms_AllMapList.end())
	{
		InnerFishLog(L"�������ص�ͼʧ��:%s", mapName);
		return false;
		
	}
	m_Interval = FLT_MAX;
	m_pMap = it->second;
	ReleaseMap();
	for (UINT i = 0; i < m_pMap->FLowList.size(); ++i)
	{
		FlowIndexData *pfd = new FlowIndexData;
		pfd->FlowIdx = m_pMap->FLowList[i];
		pfd->LastIdx = false;
		m_FlowData.push_back(pfd);
	}
	Reset();
	return true;
}
void FishMap::Reset()
{
	m_LastPackageGroupID = 255;
	m_nIndex = 0;
	m_Time = 0;
	//m_Interval = FLT_MAX;
	m_FlowList.clear();
	m_PackageCount.clear();
	FlowDataList flowList;
	FlowDataList lastFlowList;

	for (UINT i = 0; i < m_pMap->PackageList.size(); ++i)
	{
		m_PackageCount.push_back(m_pMap->PackageList[i].MaxCount);
	}
	for (UINT i = 0; i < m_FlowData.size(); ++i)
	{
		if (m_FlowData[i]->LastIdx)
			lastFlowList.push_back(m_FlowData[i]);
		else
			flowList.push_back(m_FlowData[i]);
	}

	while(flowList.size() > 0)
	{
		UINT idx = RandRange(0, flowList.size());
		m_FlowList.push_back(flowList[idx]);
		flowList[idx] = flowList.back();
		flowList.pop_back();
	}
	
	int count = (int)m_FlowList.size();

	for (UINT i = 0; i < lastFlowList.size(); ++i)
	{
		UINT idx = RandRange(0, count);
		m_FlowList.insert(m_FlowList.begin() + idx, lastFlowList[i]);
		lastFlowList[i]->LastIdx = false;
	}

	UINT lastCount = min(LAST_COUNT, m_FlowList.size() / 2);
	UINT startIdx = m_FlowList.size() - lastCount;

	for (UINT i = startIdx; i < m_FlowList.size(); ++i)
		m_FlowList[i]->LastIdx = true;
}
void FishMap::ReleaseMap()
{
	for (UINT i = 0; i < m_FlowData.size(); ++i)
	{
		delete(m_FlowData[i]);
	}
	m_FlowData.clear();
	m_FlowList.clear();
}
USHORT FishMap::GetFlowIndex(bool &bFixedGroup)
{
	bFixedGroup = false;
	int idx = -1;
	for (UINT i = 0; i < m_pMap->FlowOrderData.OrderIndex.size(); ++i)
	{
		int orderidx = m_pMap->FlowOrderData.OrderIndex[i];
		if (orderidx == m_nIndex)
		{
			//����̶�������
			idx = RandRange(0, m_pMap->FlowOrderData.FlowIndex.size());
			idx = m_pMap->FlowOrderData.FlowIndex[idx];
			bFixedGroup = true;
		}
		else if (orderidx > m_nIndex)
			break;
	}
	if (idx == -1)
	{
		idx = m_FlowList.back()->FlowIdx;
		m_FlowList.pop_back();
	}

	++m_nIndex;
	return (USHORT)idx;
}
USHORT FishMap::GetPackageGroupID(FishPackageData &fpd)
{
	//��֤���ظ����γ�����ͬ��·��
	if (fpd.FishGroup.size() == 1)
		return fpd.FishGroup[0];
	USHORT index = ConvertDWORDToShort(RandRange(0, fpd.FishGroup.size()));
	USHORT groupid = fpd.FishGroup[index];
	//while (groupid == m_LastPackageGroupID)
	//	groupid = ConvertDWORDToShort(RandRange(0, fpd.FishGroup.size()));
	m_LastPackageGroupID = groupid;
	return groupid;
}
//����-1��ʾ�������
UINT FishMap::Update(float deltaTime)
{
	if (m_pMap == NULL)
		return 0xffffffff;
	UINT ret = 0xffffffff;
	//ʱ�����ɵķ�ʽ
	for (UINT i = 0; i < m_pMap->PackageList.size(); ++i)
	{
		FishPackageData &fpd = m_pMap->PackageList[i];
		USHORT &maxCount = m_PackageCount[i];
		if (m_Time <= fpd.StartSeconds && m_Time + deltaTime > fpd.EndSeconds)
		{
			if (maxCount <= 0 || m_Interval < fpd.IntervalSeconds || fpd.FishGroup.size() == 0) continue;
			--maxCount;
			int rnd = RandRange(0, 100);
			if (rnd < fpd.Chance)
			{
				//�����
				m_Interval = 0;
				ret = fpd.PackageType;
				ret = (ret << 16) | GetPackageGroupID(fpd);
				break;
			}
		}
		else if (m_Time >= fpd.StartSeconds)
		{
			if (maxCount <= 0 || m_Interval < fpd.IntervalSeconds || m_Time > fpd.EndSeconds || fpd.FishGroup.size() == 0) continue;

			int rnd = RandRange(0, 100);
			--maxCount;
			if (rnd < fpd.Chance)
			{
				//������Ⱥ
				m_Interval = 0;
				ret = fpd.PackageType;
				ret = (ret << 16) | GetPackageGroupID(fpd);
				break;
			}
		}
		else
			break;
	}
	m_Time += deltaTime;
	if (ret == -1)
		m_Interval += deltaTime;
	return ret;
}