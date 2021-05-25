/*
 * (C) Copyright 2008-2015 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifdef DRIVERS_SDMMC

#ifndef _SDM_H_
#define _SDM_H_

#define SDM_CMD_RESENT_COUNT  (3) /* ����д����ʱ���ظ����͵Ĵ��������ڶ�д������ */
#define SDM_CMD_ERROR_RETRY_COUNT   (1) /* ����ظ�����ʱ���ظ����͵Ĵ��� */

#define SDM_INVALID_CARDID    (-1)           /* ��Ч��cardId */
#define SDM_WIDE_BUS_MODE     (1 << 0)       /* for debug */
#define SDM_HIGH_SPEED_MODE   (1 << 1)       /* for debug */
#define SDM_DDR_SPEED_MODE    (1 << 2)       /* for debug */
#define SDM_HS200_SPEED_MODE  (1 << 3)       /* for debug */

#define SDM_MAX_MANAGER_PORT  (3) /* SDM��������ٸ��˿� */

/* Card Operation Function */
struct SDM_OPERATION_FUN_T {
	int32 (*read)(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf);
	int32 (*write)(int32 cardId, uint32 dataAddr, uint32 blockCount, void *pBuf);
};

/* Card Information */
typedef struct TagSDM_CARD_INFO {
	int32            cardId;           /* cardId��-1����Ϊ����Чid */
	uint32           type;             /* Card type */
	uint16           rca;              /* Relative Card Address */
	uint32           workMode;         /* for debug, record card work mode */
	uint32           tran_speed;       /* ����������ݴ����ٶȣ�Ҳ���ǿ��������Ƶ�ʣ���λKHz */
	/*************************************************************
			SD/MMC information
	*************************************************************/
	uint32           cid[4];
	u64              capacity_rpmb;
	uint32           WriteProt;      /* Mechanical write Protect switch state,
						TRUE:write protected, FALSE: no write protected, can read and write */
	uint32           bPassword;      /* ����ָʾ���Ƿ���������Ŀ�, TRUE:have password, FALSE:no password */
	/* Card internal detail */
	uint16           year;             /* Card manufacture year */
	uint8            month;            /* Card manufacture month */
	uint32           psn;              /* Product serial number */
	uint8            prv;              /* Product revision, ��4λ��ʾ��汾�ţ���4λ��ʾС�汾�ţ���:0110 0010b��ʾ6.2�汾 */
	uint8            pnm[7];           /* Product name,ASCII string */
	uint8            oid[3];           /* OEM/Application ID,ASCII string */
	uint8            mid;              /* Manufacturer ID */
	uint32           capability;       /* Card capability,��λblock����ÿ��blockΪ512�ֽ� */
	uint16           ccc;              /* Card Command Class */
	uint32           taac;
	uint32           nsac;
	uint32           timeout;          /* ���timeout����д��timeout������ģ����ܶ�д�������timeoutֵ */
	uint32           r2w_factor;
	uint32           dsr_imp;
	CARD_SPEC_VER_E  specVer;          /* SD Specification Version */
	/* operation function */
	struct SDM_OPERATION_FUN_T fun;    /* SDM operation functions */
	/*************************************************************
			SDIO information
	*************************************************************/
	uint8            nf;               /* number of functions in the card */
	uint32           smb;              /* is card support multi-block, TRUE:support, FALSE:not support, and fnBS[] is useless */
	uint16           fnBS[8];          /* each function block size, (0 <= function <= 7), (1 <= block size <= 2048) */
	/*************************************************************
			eMMC Boot information
	*************************************************************/
	uint32           bootSize;         /* boot partition size,��λsector(512B) */
} SDM_CARD_INFO_T, *pSDM_CARD_INFO_T;

/* SDM Port Information */
typedef struct TagSDM_PORT_INFO {
	pMUTEX           mutex;        /* �˿ڻ�����,��Ϊ���һ����д����������Ȳ�������Ҫ��ͬһ���˿����������ͼ�������
					  ���������м䲻�ܴ��������������ִ�е�Ч���Ͳ���������Ҫ�Ľ�������Ϊ�˱�֤
					  ����֮�����������Ҫһ�������� */
	uint32           bOpen;        /* Is port opened, TRUE:opened, FALSE:closed */
	SDM_CARD_INFO_T  cardInfo;     /* CardInfo */
#ifdef SDM_PROT_INFO_DEBUG
	uint32           step;         /* for debug, ��ʶ���ߵ���һ������ */
	int32            error;        /* for debug��������Ϣ��������� */
#endif
} SDM_PORT_INFO_T, *pSDM_PORT_INFO_T;

#undef EXT
#ifdef SDM_DRIVER
#define EXT
#else
#define EXT extern
#endif

/* ����SDM������ȫ�ֱ��� */
EXT SDM_PORT_INFO_T gSDMDriver[SDM_MAX_MANAGER_PORT];
EXT uint32 gEmmcBootPart;

/****************************************************************
			��SDM�ڲ�ʹ�õĽӿ�
****************************************************************/
uint16 _GenerateRCA(void);
uint32 _IsRCAUsable(uint16 rca);
int32 _Identify_SendCmd(int32 cardId,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf);
int32 _Identify_SendAppCmd(int32 cardId,
			uint16 rca,
			uint32 cmd,
			uint32 cmdArg,
			uint32 *responseBuf,
			uint32  blockSize,
			uint32  dataLen,
			void   *pDataBuf);

#endif /* end of #ifndef _SDM_H_ */

#endif /* end of #ifdef DRIVERS_SDMMC */
