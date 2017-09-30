//�������Ϸ�е� һЩ���ݵ�ͳ��
//��Ҳμӱ��� ������εĴ���
//��Ҳμӱ��� ���ǰ�����Ĵ��� (�ֱ�123)
//��Ҳ��������� �Ĵ��� (������)
//��һ�ý�ҵ�������
//��ҿ������Ľ�ҵ�����������ͳ��
#pragma once
#include "Stdafx.h"
class CRoleEx;
struct tagRoleGameData;
class RoleGameData
{
public:
	RoleGameData();
	virtual ~RoleGameData();

	bool OnInit(CRoleEx* pRole);

	void OnLoadRoleGameDataByDB();//�����ݿ������ҵ�����
	void OnLoadRoleGameDataResult(DBO_Cmd_LoadGameData* pMsg);//�����ݿⷵ�ص������Ϸ���ݵ�����
	void SendRoleGameDataToClient();//��Ҽ�¼���ݷ��͵��ͻ���ȥ
	//void SaveRoleGameData();//����Ҽ�¼���ݱ��浽���ݿ�ȥ
	bool IsLoadDB(){ return m_IsLoadDB; }
	//������Ҽ�¼���ݵľ��崦���� ���ݹ���д�� �������⴦��
	void OnHandleCatchFish(BYTE FishID);
	void OnHandleRoleGetGlobel(int AddGlobel);//��һ�ý��
	void OnHandleRoleMonthReward(int RewardIndex);//��һ�ñ���������

	void OnHandleRoleJoinTable(bool IsMonth);
	void OnHandleRoleLeaveTable();
	void OnHandleRoleSignUpMonth();

	void OnHandleRoleComb();

	tagRoleGameData& GetGameData(){ return m_RoleGameData; }
private:
	bool					m_IsLoadDB;
	CRoleEx*				m_pRole;
	//����Ľṹ ��Ҫ���ݲ߻���������� ��������� һ����һ������ ���ҽ�����ֱ��д�� ����������
	tagRoleGameData			m_RoleGameData;

	//��ʱ��¼����
	bool					m_IsInMonth;
	DWORD					m_LogJoinTableTime;
};