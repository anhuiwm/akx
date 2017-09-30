//�¿�
#pragma once
class CRoleEx;
class RoleMonthCard
{
public:
	RoleMonthCard();
	virtual ~RoleMonthCard();

	bool OnInit(CRoleEx* pRole);

	bool	IsInMonthCard();

	bool	SetRoleMonthCardInfo(BYTE MonthCardID);//���������¿����ұ��浽���ݿ�
	bool	GetRoleMonthCardReward();

	void	UpdateMonthCard();

	bool IsCanAutoFire();
	float AddLotteryRate();
private:
	CRoleEx*			m_pRole;
};