#pragma once
class CRoleEx;
class RoleVip
{
public:
	RoleVip();
	virtual ~RoleVip();
	bool OnInit(CRoleEx* pRole);
	void OnRechargeRMBChange();
	void SendReward(BYTE OldVipLevel, BYTE VipLevel);
	BYTE GetLauncherReBoundNum();//��ȡ�ӵ������Ĵ���
	DWORD GetSendGoldBulletNum();
	DWORD GetSendSilverBulletNum();
	DWORD GetSendBronzeBulletNum();
	DWORD GetSendItemNum();
	//bool  IsCanLauncherLocking();//�Ƿ�����ӵ�����
	BYTE AddAlmsSum();//������ȡ�ȼý����
	float AddMonthScoreRate();//����������������
	float AddReChargeRate();//��ֵ��������
	float AddAlmsRate();//�ȼý��������
	BYTE GetUseMedalSum();//�һ���������
	float AddCatchFishRate();//�����������
private:
	bool OnChangeRoleVipLevel(BYTE VipLevel);
private:
	CRoleEx*			m_pRole;
};