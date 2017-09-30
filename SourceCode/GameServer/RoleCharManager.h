#pragma once
struct tagRoleCharArray
{
	DWORD		SrcUserID;
	vector<tagRoleCharInfo> Array;
};
class CRoleEx;
class RoleCharManager  //�û����������  
{
public:
	RoleCharManager();
	virtual ~RoleCharManager();

	bool	OnInit(CRoleEx* pRole);//��ʼ��
	void	OnLoadAllCharInfoByDB(DBO_Cmd_LoadCharInfo* pMsg);//����ȫ����������Ϣ����
	//��Ҳ���
	void    OnLoadCharMapList();//��ȡȫ���������ݵļ�������
	void    OnLoadAllCharInfoByUserID(DWORD dwSrcUserID);//���ص�����ҵ�ȫ������������
	void	OnAddCharInfo(tagRoleCharInfo& pInfo);//����յ�һ����Ϣ
	void	OnSendCharInfo(tagRoleCharInfo& pInfo);//��ҷ���һ����Ϣ 

	bool IsLoadDB(){ return m_IsLoadDB; }

	void	OnDelRelation(DWORD dwDestUserID);

	bool GetCharMessageStates();
private:
	CRoleEx*							m_pRole;//���
	bool								m_IsLoadDB;
	HashMap<DWORD, tagRoleCharArray>	m_CharMap;//��Ϣ�б�
};