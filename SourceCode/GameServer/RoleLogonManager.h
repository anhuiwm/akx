//����ظ���½������
#pragma once
#include "Stdafx.h"
class RoleLogonManager
{
public:
	RoleLogonManager();
	virtual ~RoleLogonManager();

	DWORD OnAddRoleOnlyInfo(DWORD UserID);//�������ظ���½��ƾ֤
	bool CheckRoleOnlyID(DWORD UserID, DWORD RandID);
	void OnDleRoleOnlyInfo(DWORD UserID);
private:
	HashMap<DWORD, DWORD>  m_RoleOnlyMap;
};