// TS I/O発行・受信スレッド

#ifndef _IO_THREAD_HPP_
#define _IO_THREAD_HPP_

#include "usbops.hpp"

#define IOREQ_RESERVE_NUM 20

class TsIoThread
{
public:
	TsIoThread (const int fd, const int endpoint);
	~TsIoThread();
	void operator()();
	void cancel();
	int readBuffer(const void **ptr);

protected:
	int fd;
	int endpoint;
	bool volatile is_cancelled;
	usbdevfs_urb urb[IOREQ_RESERVE_NUM];
	uint8_t *buf;
	size_t *actualSize;
	int volatile buf_pushIndex;
	int volatile buf_popIndex;
	int issuedRequestNum;

	void SetBufferPtrOfUrb(usbdevfs_urb* urbp);
	void reapUrbs();
};

#endif

