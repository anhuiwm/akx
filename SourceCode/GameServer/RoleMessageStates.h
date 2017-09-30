//��ҵ� ���״̬ �ɷ������˿���
#pragma once
class CRoleEx;
enum RoleMessageType
{
	//RMT_Mail		= 1,//δ�����ʼ�״̬
	//RMT_WeekRank    = 2,//δ��ȡ�����а���
	//RMT_Giff		= 4,//δ��ȡ������ 
	//RMT_Task		= 8,//����
	//RMT_Achievement = 16,//�ɾ�
	//RMT_Action		= 32,//�
	//RMT_Check		= 64,//�ж���ҽ����Ƿ����ǩ��
	//RMT_Char		= 128,//��Ϣ
	//RMT_Relation	= 256,//����
	//RMT_Online      = 512,//�������߽���

	RMT_Mail = 1,//δ�����ʼ�״̬
	RMT_WeekRank = 2,//δ��ȡ�����а�������ʱ��Ҫ��
	RMT_Giff = 4,//δ��ȡ������ 
	RMT_Task = 8,//�ճ�����
	RMT_Achievement = 16,//�ɾ�
	RMT_Action = 32,//�
	RMT_Check = 64,//ǩ��
	RMT_Char = 128,//����
	RMT_Relation = 256,//��������
	RMT_Online = 512,//�������߽���
	RMT_WeekTask = 1024,//�ܳ�����
	RMT_Forge = 2048,//����  һ����
	//RMT_OnlineReward = 2048,//���߳齱
};
class RoleMessageStates
{
public:
	RoleMessageStates();
	virtual ~RoleMessageStates();

	void	OnInit(CRoleEx* pRole);

	void	OnChangeRoleMessageStates(RoleMessageType Type,bool IsSendToClient = true, bool Once = false);
private:
	CRoleEx*				m_pRole;
	bool					m_IsInit;
	DWORD					m_StatesValue;
};