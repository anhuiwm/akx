//������߽�������
#pragma once
class CRoleEx;
class RoleOnlineReward
{
public:
	RoleOnlineReward();
	virtual ~RoleOnlineReward();
	void OnGetAllOnlineReward(CRoleEx* pRole);
	void OnGetOnlineReward(CRoleEx* pRole, BYTE ID);//��ȡ���߽���
	void OnNoticeOnlineRewardComplete(CRoleEx* pRole, BYTE ID);//��ȡ���߽���
};