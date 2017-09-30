#include "stdafx.h"
#include "RoleCache.h"
#include "FishServer.h"
RoleCache::RoleCache()
{

}
RoleCache::~RoleCache()
{

}
void RoleCache::OnAddRoleCache(DWORD dwUserID)
{
	HashMap<DWORD, DWORD>::iterator Iter = m_CacheRoleMap.find(dwUserID);
	if (Iter != m_CacheRoleMap.end())
		return;
	m_CacheRoleMap.insert(HashMap<DWORD, DWORD>::value_type(dwUserID, timeGetTime()));
}
void RoleCache::OnDleRoleCache(DWORD dwUserID)
{
	HashMap<DWORD, DWORD>::iterator Iter = m_CacheRoleMap.find(dwUserID);
	if (Iter == m_CacheRoleMap.end())
		return;
	m_CacheRoleMap.erase(Iter);
}
void RoleCache::UpdateByMin()
{
	if (m_CacheRoleMap.empty())
		return;
	DWORD Timer = timeGetTime();
	HashMap<DWORD, DWORD>::iterator Iter = m_CacheRoleMap.begin();
	for (; Iter != m_CacheRoleMap.end();)
	{
		if (Timer - Iter->second >= g_FishServer.GetFishConfig().GetSystemConfig().CacheUserMin*60000)//���ͨ���������� ��������������� ��Ϊ����Ѿ�������
		{
			CRoleEx* pRole = g_FishServer.GetRoleManager()->QueryUser(Iter->first);
			if (pRole)
			{
				//pRole->ChangeRoleIsOnline(false);//�ȸ���������� ������
				pRole->SendUserLeaveToCenter();//��������������������
				g_FishServer.GetTableManager()->OnPlayerLeaveTable(pRole->GetUserID());
				g_FishServer.GetRoleManager()->OnDelUser(pRole->GetUserID(),true,true);//���ڴ����Ƴ���
			}
			Iter = m_CacheRoleMap.erase(Iter);
		}
		else
			++Iter;
	}
}
bool RoleCache::IsOpenRoleCache()//�ж�GameServer�Ƿ����˻���
{
	return (g_FishServer.GetFishConfig().GetSystemConfig().CacheUserMin != 0);
}