//�һ��������
#pragma once
class CRoleEx;
class ExChangeManager
{
public:
	ExChangeManager();
	virtual ~ExChangeManager();

	void OnUseExChangeCode(CRoleEx* pRole, CL_Cmd_RoleUseExChangeCode* pMsg);//ʹ�öһ���
	void OnUseExChangeCodeDBResult(DBO_Cmd_QueryExChange* pMsg);
};