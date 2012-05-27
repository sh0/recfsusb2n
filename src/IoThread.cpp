
#include <inttypes.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>	// valloc()
#include <sys/select.h>

#include "usbops.hpp"
#include "IoThread.hpp"

// usbfsのバルク転送上限サイズ 16KB
#define TSBULKSIZE 16384

// TS Buffer (8sec * 2040KB/s / 16KB/unit = 1020unit)
#define TSBUFFERSIZE 1020

#define TSSIZE_INVALID (TSBULKSIZE+1)

TsIoThread::TsIoThread (const int fd, const int endpoint)
: fd(fd), endpoint(endpoint), is_cancelled(false),
buf_pushIndex(0), buf_popIndex(0)
{
	//buf = new uint8_t[TSBULKSIZE * TSBUFFERSIZE];
	buf = (uint8_t*)valloc(TSBULKSIZE * TSBUFFERSIZE);
	actualSize = new size_t[TSBUFFERSIZE];
}

TsIoThread::~TsIoThread()
{
	//delete [] buf;
	free(buf);
	delete [] actualSize;
}

void TsIoThread::operator()()
{
	for(int i = 0; i < IOREQ_RESERVE_NUM; i++) {
		// 非同期URB要求発行
		memset(&(urb[i]), 0, sizeof(usbdevfs_urb));
		urb[i].type		= USBDEVFS_URB_TYPE_BULK;
		urb[i].endpoint	= endpoint;
		SetBufferPtrOfUrb(&(urb[i]));

		int r = usb_submiturb(fd, &(urb[i]));
		if (r < 0) {
			err(1, "can't send urb");
		}
	}
	issuedRequestNum = IOREQ_RESERVE_NUM;

	fd_set writefds;
	while(!is_cancelled || issuedRequestNum > 0) {
		FD_ZERO(&writefds);
		FD_SET(fd, &writefds);

		int r = select(fd+1, NULL, &writefds, NULL, NULL);
		if (r < 0) {
			err(1, "select failed.");
		}

		if (FD_ISSET(fd, &writefds)) {
			// usbfsから何か来た
			reapUrbs();
		}
	}
}

void TsIoThread::cancel()
{
	is_cancelled = true;
	usleep(150000);
	if(issuedRequestNum > 0) {
		for(int i = 0; i < IOREQ_RESERVE_NUM; i++) {
			if(urb[i].buffer_length == 0) continue;
			int ret = usb_discardurb(fd, &(urb[i]));
			if (ret < 0) {
				err(1, "can't discard urb");
			}
		}
	}
}

void TsIoThread::SetBufferPtrOfUrb(usbdevfs_urb* urbp)
{
	actualSize[buf_pushIndex] = TSSIZE_INVALID;
	urbp->usercontext	= &(actualSize[buf_pushIndex]);
	urbp->buffer		= buf + (buf_pushIndex * TSBULKSIZE);
	urbp->buffer_length	= TSBULKSIZE;

	if(buf_pushIndex < (TSBUFFERSIZE - 1)) buf_pushIndex++;
	else buf_pushIndex = 0;
}

void TsIoThread::reapUrbs()
{
	usbdevfs_urb *urbp;
	while (usb_reapurb_ndelay(fd, &urbp) >= 0) {
		// 読み込めた
		size_t *actSz = reinterpret_cast<size_t *>(urbp->usercontext);
		*actSz = (urbp->status == 0) ? urbp->actual_length : 0;

		if(is_cancelled) {
			// このURB構造体が発行されていないことを示す
			urbp->buffer_length = 0;
			issuedRequestNum--;
		}else{
			// 次の非同期URB要求発行
			memset(urbp, 0, sizeof(usbdevfs_urb));
			urbp->type		= USBDEVFS_URB_TYPE_BULK;
			urbp->endpoint	= endpoint;
			SetBufferPtrOfUrb(urbp);

			int r = usb_submiturb(fd, urbp);
			if (r < 0) {
				err(1, "can't send urb");
			}
		}
	}
}

int TsIoThread::readBuffer(const void **ptr)
{
	*ptr = buf + (buf_popIndex * TSBULKSIZE);
	if(buf_pushIndex == buf_popIndex)
		return 0;

	int i = buf_popIndex, remain = -1;
	for(; i < TSBUFFERSIZE && i != buf_pushIndex; i++) {
		if(actualSize[i] == TSSIZE_INVALID) {
			break;
		}else if(actualSize[i] < TSBULKSIZE) {
			remain = actualSize[i];
			break;
		}
	}
	int continuousSize = (i - buf_popIndex) * TSBULKSIZE + ((remain < 0) ? 0 : remain);
	if(remain >= 0) i++;
	buf_popIndex = (i < TSBUFFERSIZE) ? i : 0;
	return continuousSize;
}

/* */

