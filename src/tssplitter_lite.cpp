/* -*- tab-width: 4; indent-tabs-mode: t -*- */
/* tssplitter_lite.c -- split TS stream.

   Copyright 2009 querulous
   Copyright 2010 Naoya OYAMA <naoya.oyama@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/stat.h>
#include "decoder.h"
#include "tssplitter_lite.h"

/* prototypes */
static int ReadTs(splitter *sp, ARIB_STD_B25_BUFFER *sbuf);
static int AnalyzePat(splitter *sp, unsigned char *buf);
static int RecreatePat(splitter *sp, unsigned char *buf, int *pos);
static char** AnalyzeSid(char *sid);
static int AnalyzePmt(splitter *sp, unsigned char *buf, unsigned char mark);
static unsigned int GetCrc32(unsigned char *data, int len);
static int GetPid(unsigned char *data);

/**
 * �����ӥ�ID����
 */
static char** AnalyzeSid(
	char* sid)						// [in]		�����ӥ�ID(����޶��ڤ�ƥ�����)
{
	int i = 0;
	char** sid_list = NULL;
	char* p;
	int CommaNum = 0;

	/* sid �ϼ��η����ΰ�������Ƥ��� */
	/* ����̵�� */
	/* SID[0] */
	/* SID[0],SID[1],...,SID[N-1],SID[N] */

	/*����ޤο��������*/
	p = sid;
	while(*p != '\0')
	{
		if( *p == C_CHAR_COMMA ){
			CommaNum++;
		}
		p++;
	}

	/* sid_list�ο��ϥ���ޤο�+2(NULL�ߤ᤹�뤫��) */
	sid_list = (char**)malloc(sizeof(char*)*(CommaNum+2));
	if ( sid_list == NULL )
	{
		fprintf(stderr, "AnalyzeSid() malloc error.\n");
		return NULL;
	}

	/* sid�����Ǥ����� */
	p = sid;
	if ( strlen(p) == 0 )
	{
		sid_list[0] = NULL;
		return sid_list;
	}

	/* �����̵�� */
	if ( CommaNum == 0 )
	{
		sid_list[0] = sid;
		sid_list[1] = NULL;
		return sid_list;
	}

	/* ����޶��ڤ��ʣ������� */
	i=0;
	p = sid;
	/* ʸ����ü����ã���뤫������޿�������������ã�����齪λ */
	while((*p != '\0') || i < CommaNum)
	{
		/* ���ߤν������֤�sid_list[i]�˥��å� */
		/* ���Υ����ߥ󥰤� p ��
		 * ��sid��Ƭ
		 * ��[,]�μ���ʸ��
		 * �����줫�Ǥ���Τ� p �� sid_list[i] ���������Ƥ褤
		 */
		sid_list[i] = p;
		i++;

		/* �ǽ�˸����[,]��NULLʸ�����ִ����� */
		p = strchr(p, C_CHAR_COMMA);
		if ( p == NULL )
		{
			/* ����ޤ����Ĥ���ʤ����ϺǸ�ν����оݤʤΤǽ�λ */
			break;
		}
		*p = '\0';
		/* �������֤�NULL���ִ�����ʸ���μ��ΰ��֤����ꤹ�� */
		p++;
	}

	/* �Ǹ��sid_list[n]��NULL�ݥ��󥿤ǻߤ�� */
	sid_list[i] = NULL;

	i=0;
	while( sid_list[i] != NULL )
	{
		i++;
	}
#if 0
	for(i=0; sid_list[i] != NULL; i++)
	{
		printf("sid_list[%d]=[%s].\n",i, sid_list[i]);
	}
#endif
	return sid_list;
}

/**
 * ���������
 */
splitter* split_startup(
	char *sid		// [in]		�����ӥ�ID(�����ǻ��ꤷ��ʸ����)
)
{
	splitter* sp;
	sp = (splitter*)malloc(sizeof(splitter));
	if ( sp == NULL )
	{
		fprintf(stderr, "split_startup malloc error.\n");
		return NULL;
	}
	memset(sp->pids, 0, sizeof(sp->pids));
	memset(sp->pmt_pids, 0, sizeof(sp->pmt_pids));

	sp->sid_list	= NULL;
	sp->pat			= NULL;
	sp->sid_list	= AnalyzeSid(sid);
	if ( sp->sid_list == NULL )
	{
		free(sp);
		return NULL;
	}
	sp->pat_count	= 0xFFU;
	sp->pmt_retain = -1;
	sp->pmt_counter = 0;

	memset(sp->section_remain, 0U, sizeof(sp->section_remain));
	memset(sp->packet_seq, 0U, sizeof(sp->packet_seq));

	return sp;
}

/**
 * ��Ȥ�PID����ꤵ����
 */
int split_select(
	splitter *sp,						// [in/out]		splitter��¤��
	ARIB_STD_B25_BUFFER *sbuf			// [in]			����TS
)
{
	int result;
	// TS����
	result = ReadTs(sp, sbuf);

	return result;
}

/**
 * ��λ����
 */
void split_shutdown(splitter* sp)
{
	if ( sp != NULL ) {
		if ( sp->pat != NULL )
		{
			free(sp->pat);
			sp->pat = NULL;
		}
		if ( sp->sid_list != NULL )
		{
			free(sp->sid_list);
			sp->sid_list = NULL;
		}
		free(sp);
		sp = NULL;
	}
}

/**
 * TS ���Ͻ���
 *
 * �оݤΥ����ͥ��ֹ�Τߤ� PAT �κƹ��ۤȽ����о� PID ����Ф�Ԥ�
 */
static int ReadTs(splitter *sp, ARIB_STD_B25_BUFFER *sbuf)
{
#if 0
	unsigned char **pat,				// [out]	PAT ����ʺƹ��۸��
	unsigned char* pids,				// [out]	�����о� PID ����
	char** sid_list,					// [in]		�����оݥ����ӥ� ID �Υꥹ��
	unsigned char* pmt_pids,			// [in]		�����о�PID��PMT PID
	,			// [in]		pt1_drv������TS
	int* pmt_retain,						// [in]		�Ĥ��٤�PMT�ο�
	int* pmt_counter					// [out]	�Ĥ���PMT�ο�
#endif

	int length = sbuf->size;
	int pid;
	int result = TSS_ERROR;
	int index;
	int analyze_result = 0;

	index = 0;
	while(length - index - LENGTH_PACKET > 0) {
		pid = GetPid((unsigned char *)sbuf->data + index + 1);
		// PAT
		if(0x0000 == pid) {
			result = AnalyzePat(sp, (unsigned char *)sbuf->data + index);
			if(TSS_SUCCESS != result) {
				/* ���̤δؿ�������malloc errorȯ�� */
				return result;
			}
		}

		// PMT
		/* �Ĥ�pmt_pid�Ǥ�����ˤϡ�pmt�˽񤫤�Ƥ���
		 * �Ĥ��٤�PCR/AUDIO/VIDEO PID��������� */
		if(sp->pmt_pids[pid] == 1) {
			/* ������ˤ�PMT��˰��٤�������ʤ��褦�ˤ��Ƥ��� */
		    analyze_result = AnalyzePmt(sp, (unsigned char *)sbuf->data + index, 1);
		    if (TSS_SUCCESS == analyze_result ) {
			sp->pmt_pids[pid]++;
			sp->pmt_counter += 1;
			    *(sbuf->data + index + 1) = 0xffU;
			    *(sbuf->data + index + 2) = 0xffU;
			}
		}
		/* Ͽ�褹�����Ƥ�PMT�ˤĤ��ơ���ˤ���PCR/AUDIO/VIDEO��PID��
		 * ���� */
		/* pmt_counter �� pmt_retain �����פ�����˾������������ */
		if(sp->pmt_counter == sp->pmt_retain) {
			result = TSS_SUCCESS;
			break;
		}
		else {
			result = TSS_ERROR;
		}
		index += LENGTH_PACKET;
	}

	return(result);
}

static int RescanPID(splitter *splitter, unsigned char *buf)
{
	int result = TSS_NULL;
	int i;

	// clear
	if (splitter->pmt_counter == splitter->pmt_retain) {
	    memcpy(splitter->pids, splitter->pmt_pids, sizeof(splitter->pids));
	    splitter->pmt_counter = 0;
		memset(splitter->section_remain, 0U, sizeof(splitter->section_remain));
		memset(splitter->packet_seq, 0U, sizeof(splitter->packet_seq));

		fprintf(stderr, "Rescan PID \n");
	}

	if (TSS_SUCCESS == AnalyzePmt(splitter, buf, 2)) {
	    splitter->pmt_counter += 1;
	}

 	if (splitter->pmt_retain == splitter->pmt_counter) {
	    result = TSS_SUCCESS;
		for (i = 0; MAX_PID > i; i++) {
		    if (splitter->pids[i] > 0) {
			    splitter->pids[i] -= 1;
		    } 
		}
		fprintf(stderr, "Rescan PID End\n");
	}

	return result;
}
/**
 * TS ʬΥ����
 */
int split_ts(
	splitter *splitter,					// [in]		splitter�ѥ�᡼��
	ARIB_STD_B25_BUFFER *sbuf,			// [in]		����TS
	splitbuf_t *dbuf							// [out]	����TS
)
{
	int pid;
	unsigned char *sptr, *dptr;
	int s_offset = 0;
	int d_offset = 0;
	int result = TSS_SUCCESS;
	int pmts = 0;
	int version = 0;

	/* ����� */
	dbuf->size = 0;
	if (sbuf->size < 0) {
		return TSS_ERROR;
	}

	sptr = (unsigned char *)sbuf->data;
	dptr = dbuf->buffer;

	while(sbuf->size > s_offset) {
		pid = GetPid(sptr + s_offset + 1);
		switch(pid) {

		// PAT
		case 0x0000:
			// ��󥫥��󥿥�����ȥ��å�
			if(0xFFU == splitter->pat_count) {
				splitter->pat_count = splitter->pat[3];
			}
			else {
				splitter->pat_count += 1;
				if(0 == (splitter->pat_count & 0x0fU)) {
					splitter->pat_count -= 0x10;
				}
			}
			splitter->pat[3] = splitter->pat_count;

			memcpy(dptr + d_offset, splitter->pat, LENGTH_PACKET);
			d_offset += LENGTH_PACKET;
			dbuf->size += LENGTH_PACKET;
			break;
		default:
		    if(0 != splitter->pmt_pids[pid]) {
			    //PMT
			    if ((sptr + s_offset)[1] & 0x40) {		// PES���ϥ��󥸥�����
				    // �С����������å�
				    for(pmts = 0; splitter->pmt_retain > pmts; pmts++) { 
					    if (splitter->pmt_version[pmts].pid == pid) {
						  version = splitter->pmt_version[pmts].version;
						  break;
						}
					}
					if((version != ((sptr + s_offset)[10] & 0x3e))
					   ||(splitter->pmt_retain != splitter->pmt_counter)) {
					    // �ƥ����å�
					    result = RescanPID(splitter, sptr + s_offset);
					} 
				}
				else {
				    if (splitter->pmt_retain != splitter->pmt_counter) {
					    // �ƥ����å�
					    result = RescanPID(splitter, sptr + s_offset);
					}
				}
			}
			/* pids[pid] �� 1 �ϻĤ��ѥ��åȤʤΤǽ񤭹��� */
			if(0 != splitter->pids[pid]) {
				memcpy(dptr + d_offset, sptr + s_offset, LENGTH_PACKET);
				d_offset += LENGTH_PACKET;
				dbuf->size += LENGTH_PACKET;
			}
			break;
		} /* switch */

		s_offset += LENGTH_PACKET;
	}

	return result;
}

/**
 * PAT ���Ͻ���
 *
 * PAT ����Ϥ��������оݥ����ͥ뤬�ޤޤ�Ƥ��뤫�����å���Ԥ���PAT ��ƹ��ۤ���
 */
static int AnalyzePat(splitter *sp, unsigned char *buf)
#if 0
	unsigned char* buf,					// [in]		�ɤ߹�����Хåե�
	unsigned char** pat,				// [out]	PAT ����ʺƹ��۸��
	unsigned char* pids,				// [out]	�����о� PID ����
	char** sid_list,					// [in]		�����оݥ����ӥ� ID �Υꥹ��
	unsigned char* pmt_pids,			// [out]	�����ӥ� ID ���б����� PMT �� PID
	int* pmt_retain						// [out]	�Ĥ�PMT�ο�

#endif
{
	int pos[MAX_PID];
	int service_id;
	int i, j, k;
	int size = 0;
	int pid;
	int result = TSS_SUCCESS;
	char **p;
	int sid_found = FALSE;
	int avail_sids[MAX_SERVICES];

	unsigned char *pat = sp->pat;
	unsigned char *pids = sp->pids;
	char **sid_list = sp->sid_list;
	unsigned char *pmt_pids = sp->pmt_pids;

	char chosen_sid[512];
	chosen_sid[0] = '\0';

	if(pat == NULL) {
		/* ����� */
		sp->pmt_retain = 0;
		memset(pos, 0, sizeof(pos));
		size = buf[7];

		/* prescan SID/PMT */
		for(i = 13, j = 0; i < (size + 8) - 4; i = i + 4, j++) {
			avail_sids[j] = ((int)buf[i] << 8) + buf[i+1];
			sp->avail_pmts[j] = GetPid(&buf[i+2]);
			/* NIT (PID 0x0010) ��̵�� */
			if(sp->avail_pmts[j] == 0x0010)
				j--;
		}
		sp->num_pmts = j;

		// �оݥ����ͥ�Ƚ��
		/* size + 8 = �ѥ��å���Ĺ */
		/* �ǽ� 4 �Х��Ȥ�CRC�ʤΤ����Ф� */
		for(i = 13; i < (size + 8) - 4; i = i + 4) {

			service_id = ((int)buf[i] << 8) + buf[i+1];
			p = sid_list;

			while(*p) {
				if( sp->pmt_retain >= MAX_SERVICES )
					return TSS_ERROR;
				if(service_id == atoi(*p)) {
					/* Ͽ���оݤ� pmt_pids �� 1 �Ȥ��� */
					/* Ͽ���оݤ� pmt �� pids �� 1 �Ȥ��� */
					pid = GetPid(&buf[i + 2]);
					if(pid != 0x0010) {
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					p++;
					continue;
				}
				else if(!strcasecmp(*p, "hd") || !strcasecmp(*p, "sd1")) {
					/* hd/sd1 ������ˤ�1���ܤΥ����ӥ�����¸���� */
					if(service_id == avail_sids[0]) {
						pid = GetPid(&buf[i + 2]);
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					p++;
					continue;
				}
				else if(!strcasecmp(*p, "sd2")) {
					/* sd2 ������ˤ�2���ܤΥ����ӥ�����¸���� */
					if(service_id == avail_sids[1]) {
						pid = GetPid(&buf[i + 2]);
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					p++;
					continue;
				}
				else if(!strcasecmp(*p, "sd3")) {
					/* sd3 ������ˤ�3���ܤΥ����ӥ�����¸���� */
					if(service_id == avail_sids[2]) {
						pid = GetPid(&buf[i + 2]);
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					p++;
					continue;
				}
				else if(!strcasecmp(*p, "1seg")) {
					/* 1seg ������ˤ� PMTPID=0x1FC8 �Υ����ӥ�����¸���� */
					pid = GetPid(&buf[i + 2]);
					if(pid == 0x1FC8) {
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					p++;
					continue;
				}
				else if(!strcasecmp(*p, "all")) {
					/* all������ˤ�����¸���� */
					pid = GetPid(&buf[i + 2]);
					if(pid != 0x0010) {
						*(pmt_pids+pid) = 1;
						*(pids+pid) = 1;
						pos[pid] = i;
						sid_found = TRUE;
						sp->pmt_version[sp->pmt_retain].pid = pid;
						sp->pmt_retain += 1;
						sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					}
					break;
				}
				else if(!strcasecmp(*p, "epg")) {
					/* all������ˤ�����¸���� */
					sid_found    = TRUE;
					*(pids+0x11) = 1;
					*(pids+0x12) = 1;
					*(pids+0x23) = 1;
					*(pids+0x29) = 1;
					break;
				}

				p++;
			} /* while */
			if( sp->pmt_retain >= MAX_SERVICES )
				return TSS_ERROR;
		}

		/* if sid has been specified but no sid found, fall back to all */
		if(*sid_list && !sid_found) {
			for(i = 13; i < (size + 8) - 4; i = i + 4) {
				service_id = ((int)buf[i] << 8) + buf[i+1];
				pid = GetPid(&buf[i + 2]);
				if(pid != 0x0010U) {
					*(pmt_pids+pid) = 1;
					*(pids+pid) = 1;
					pos[pid] = i;
					sid_found = TRUE;
					sp->pmt_version[sp->pmt_retain].pid = pid;
					sp->pmt_retain += 1;
					sprintf(chosen_sid, "%s %d", *chosen_sid ? chosen_sid : "", service_id);
					if( sp->pmt_retain >= MAX_SERVICES )
						return TSS_ERROR;
				}
			}
		}

		/* print SIDs */
		fprintf(stderr, "Available sid = ");
		for(k=0; k < sp->num_pmts; k++)
			fprintf(stderr, "%d ", avail_sids[k]);
		fprintf(stderr, "\n");
		fprintf(stderr, "Chosen sid    =%s\n", chosen_sid);

#if 0
		/* print PMTs */
		fprintf(stderr, "Available PMT = ");
		for(k=0; k < sp->num_pmts; k++)
			fprintf(stderr, "%d ", sp->avail_pmts[k].pid);
		fprintf(stderr, "\n");
#endif

		// PAT �ƹ���
		result = RecreatePat(sp, buf, pos);
#if 0
		int tc;
		for(tc=0; tc<188; tc++)
			fprintf(stderr, "%02x ", *(pat+tc));
#endif
	}

	return(result);
}

/**
 * PAT �ƹ��۽���
 *
 * PMT ��������оݥ����ͥ�ʳ��Υ����ͥ�����������PAT ��ƹ��ۤ���
 */
static int RecreatePat(splitter *sp, unsigned char *buf, int *pos)
#if 0
	unsigned char* buf,					// [in]		�ɤ߹�����Хåե�
	unsigned char** pat,				// [out]	PAT ����ʺƹ��۸��
	unsigned char* pids,				// [out]	�����о� PID ����
	int *pos							// [in]		�����о� PMT �ΥХåե���ΰ���
#endif
{
	unsigned char y[LENGTH_CRC_DATA];
	unsigned int crc;
	int i;
	int j;
	int pos_i;
	int pid_num = 0;

	// CRC �׻��Τ���Υǡ���
	{
		// �����ͥ�ˤ�ä��Ѥ��ʤ���ʬ
		for (i = 0; i < LENGTH_PAT_HEADER-4; i++)
		{
			y[i] = buf[i + 5];
		}
		// ��Ƭ�� PMT �������Ū�� NIT ��
		y[LENGTH_PAT_HEADER-4] = 0x00U;
		y[LENGTH_PAT_HEADER-3] = 0x00U;
		y[LENGTH_PAT_HEADER-2] = 0xe0U;
		y[LENGTH_PAT_HEADER-1] = 0x10U;
		// �����ͥ�ˤ�ä��Ѥ����ʬ
		for (i = 0; i < MAX_PID; i++)
		{
			if(pos[i] != 0)
			{
				/* buf[pos_i] �� y �˥��ԡ�(��Ф���PID�ο�) */
				pos_i = pos[i];
				for (j = 0; j < 4; j++)
				{
					y[LENGTH_PAT_HEADER + ((4*pid_num) + j)] = buf[pos_i + j];
				}
				pid_num++;
			}
		}
	}
	/* �ѥ��åȥ������׻� */
	y[2] = (unsigned char)(pid_num * 4 + 0x0d);
	// CRC �׻�
	crc = GetCrc32(y, LENGTH_PAT_HEADER + pid_num*4);

	// PAT �ƹ���
	sp->pat = (unsigned char*)malloc(LENGTH_PACKET);
	if(sp->pat == NULL)
	{
		fprintf(stderr, "RecreatePat() malloc error.\n");
		return(TSS_NULL);
	}
	memset(sp->pat, 0xFFU, LENGTH_PACKET);
	for (i = 0; i < 5; i++)
	{
		sp->pat[i] = buf[i];
	}
	for (i = 0; i < LENGTH_PAT_HEADER + pid_num*4; i++)
	{
		sp->pat[i + 5] = y[i];
	}
	sp->pat[5 + LENGTH_PAT_HEADER + pid_num*4] = (unsigned char)((crc >> 24) & 0xFFU);
	sp->pat[6 + LENGTH_PAT_HEADER + pid_num*4] = (unsigned char)((crc >> 16) & 0xFFU);
	sp->pat[7 + LENGTH_PAT_HEADER + pid_num*4] = (unsigned char)((crc >>  8) & 0xFFU);
	sp->pat[8 + LENGTH_PAT_HEADER + pid_num*4] = (unsigned char)((crc      ) & 0xFFU);

	return(TSS_SUCCESS);
}

/**
 * PMT ���Ͻ���
 *
 * PMT ����Ϥ�����¸�оݤ� PID �����ꤹ��
 */
static int AnalyzePmt(splitter *sp, unsigned char *buf, unsigned char mark)
#if 0
	unsigned char* buf,					// [in]		�ɤ߹�����Хåե�
	unsigned char* pids				// [out]	�����о� PID ����
#endif
{
	uint32_t Nall;
	uint32_t N;
	int pcr;
	int epid;
    int count;
    int pid;
	int payload_offset;		// �ڥ����ɳ��ϥ��ե��å�

    pid = GetPid(&buf[1]);
	if (buf[1] & 0x40U) {		// PES���ϥ��󥸥�����
		sp->section_remain[pid] = ((uint16_t)(buf[6] & 0x0FU) << 8) + buf[7] + 3;	// ��������󥵥�������(�إå���)
		payload_offset = 5;

		for (count = 0; sp->pmt_retain > count; count++) {
		    if (sp->pmt_version[count].pid  == pid) {
                sp->pmt_version[count].version = (int)(buf[10] & 0x3eU);
			}
		}
		// PCR, ���Ⱦ�����Ƭ����Ϥ߽Ф뤳�ȤϤʤ�����
	// PCR
		pcr = GetPid(&buf[payload_offset + 8]);
		sp->pids[pcr] = mark;

	// ECM
		N = ((uint32_t)(buf[payload_offset + 10] & 0x0F) << 8) + buf[payload_offset + 11] + payload_offset + 12;	// ES���󳫻���

		uint32_t p = payload_offset + 12;
	while(p < N) {
		uint32_t ca_pid;
		uint32_t tag;
		uint32_t len;

		tag = buf[p];
		len = buf[p+1];
		p += 2;

		if(tag == 0x09 && len >= 4 && p+len <= N) {
			ca_pid = (((int)buf[p+2] << 8) | buf[p+3]) & 0x1fffU;
			sp->pids[ca_pid] = mark;
		}
		p += len;
	}
	}
	else {
	    if (sp->section_remain[pid] == 0) return TSS_ERROR;								// �����������Ƭ������Ǥ���
		if ((buf[3] & 0x0F) != ((sp->packet_seq[pid] + 1) & 0x0F)) return TSS_ERROR;	// �ѥ��åȥ����󥿤������
		payload_offset = 4;
		N = payload_offset;
	}
	sp->packet_seq[pid] = (uint8_t)buf[3] & 0x0F;				// ��󥫥���
	
	Nall = sp->section_remain[pid];
	if(Nall > LENGTH_PACKET - payload_offset)
		Nall = LENGTH_PACKET - payload_offset;

	// ES PID
	while (N <= Nall + payload_offset - 5)
	{
		// ���ȥ꡼����̤� 0x0D��type D�ˤϽ����оݳ�
		if (0x0D != buf[N])
		{
			epid = GetPid(&buf[N + 1]);

			sp->pids[epid] = mark;
		}
		N += 5 + (((int)(buf[N + 3]) & 0x0F) << 8) + buf[N + 4];
	}

	sp->section_remain[pid] -= Nall;

	if (sp->section_remain[pid] > 0)
		return SECTION_CONTINUE;
	else
	return TSS_SUCCESS;
}

/**
 * CRC �׻�
 */
static unsigned int GetCrc32(
	unsigned char* data,				// [in]		CRC �׻��оݥǡ���
	int len)							// [in]		CRC �׻��оݥǡ���Ĺ
{
	unsigned int crc;
	int i, j;
	unsigned int c;
	unsigned int bit;

	crc = 0xFFFFFFFFU;
	for (i = 0; i < len; i++)
	{
		unsigned char x;
		x = data[i];

		for (j = 0; j < 8; j++)
		{

			bit = (x >> (7 - j)) & 0x1U;

			c = 0;
			if (crc & 0x80000000U)
			{
				c = 1;
			}

			crc = crc << 1;

			if (c ^ bit)
			{
				crc ^= 0x04C11DB7U;
			}

			crc &= 0xFFFFFFFFU;
		}
	}

	return crc;
}

/**
 * PID ����
 */
static int GetPid(
	unsigned char* data)				// [in]		�����оݥǡ����Υݥ���
{
	return ((int)(data[0] & 0x1FU) << 8) + data[1];
}
