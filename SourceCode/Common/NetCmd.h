#pragma once
#pragma pack(push)
#pragma pack(1)
#define HEAD_LENGTH 4
struct NetCmd
{
	USHORT	CmdSize;
	BYTE	SubCmdType;
	BYTE	CmdType;
	void SetCmdType(USHORT cmdType)
	{
		CmdType = static_cast<BYTE>(cmdType >> 8);
		SubCmdType = static_cast<BYTE>(cmdType);
	}
	USHORT GetCmdType()
	{
		return static_cast<USHORT>((CmdType << 8) | SubCmdType);
	}
	void SetCmdSize(USHORT size)
	{
		CmdSize = size;
	}
	USHORT GetCmdSize()
	{
		return CmdSize;
	}
	USHORT GetDataSize()
	{
		return CmdSize > HEAD_LENGTH ? (CmdSize - 4) : 0;
	}
};

inline NetCmd* CreateCmd(USHORT size)
{
	NetCmd* pCmd = (NetCmd*)malloc(size);
	pCmd->SetCmdSize(size);
	return pCmd;
}
inline NetCmd* CreateCmd(USHORT size, void *pSrc)
{
	NetCmd* pCmd = (NetCmd*)malloc(size);
	if (!pCmd)
		return NULL;
	memcpy(pCmd, pSrc, size);
	return pCmd;
}
inline NetCmd* CreateCmd(USHORT cmdType, USHORT size)
{
	NetCmd* pCmd = (NetCmd*)malloc(size);

	pCmd->SetCmdSize(size);
	pCmd->SetCmdType(cmdType);
	return pCmd;
}
inline void DeleteCmd(NetCmd *pCmd)
{
	free(pCmd);
}

struct CL_Cmd_Json : public NetCmd
{
	//USHORT	CmdSize;
	//BYTE	SubCmdType;
	//BYTE	CmdType;
	unsigned char data[0];
};

#pragma pack(pop)