#pragma once
#include "Stdafx.h"
class CRoleEx;

//��ҽ���ǩ��
//1.���ÿ���ǩ��
//2.��ҿ�����ȡ��ǰ��ǩ������ Ҳ���� ������ȡ ����ǩ������


//���ǩ�����ݵ�RoleInfo���� ����Ϊ ǩ�����ⲿ������



class RoleCheck //���ǩ�� ��ѯ ��Ҽ���Ĳ�ѯ
{
public:
	RoleCheck();
	virtual ~RoleCheck();

	bool  OnInit(CRoleEx* pRole);//ǩ�����ݽ���ǩ���ĳ�ʼ��

	bool  RoleChecking();//��ҽ���ǩ��
	//bool  RoleCheckeOnther(BYTE DaySum);//��ҽ��в�ǩ

	//bool  LoadRoleCheckInfo();//������ҵ�ǩ������
	//void  LoadRoleCheckInfoResult(DBO_Cmd_LoadRoleCheckInfo* pDB);//���ݿⷵ��ֵ
	//bool  GetRoleCheckInfo();//�ͻ��˼���ǩ������
	//void OnDayChange();
	//bool IsLoadDB(){ return m_IsLoadDB; }

	bool GetCheckMessageStates();
private:
	//bool HandleCheckCheckInfo();

	//�Ե�ǰ�������д���
	bool IsCanCheckNowDay();
	DWORD GetWeekCheckNum();
	BYTE GetMonthCheckSum();

private:
	//tagRoleCheckInfo		m_RoleCheckInfo;
	CRoleEx*				m_Role;
	//bool					m_IsLoadDB;
	const static  DWORD  CheckSign = 16;
};