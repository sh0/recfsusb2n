// USB操作

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

#include <iostream>

#include "usbops.hpp"

void
usb_getdesc(const char *devfile, usb_device_descriptor* desc)
{
	int f = open(devfile, O_RDONLY);
	if (-1 == f) {
		std::cerr << "can't open usbdevfile to read '" << devfile << "'" << std::endl;
		return;
	}
	
	memset(desc, 0, sizeof(usb_device_descriptor));
	ssize_t rlen = read(f, desc, sizeof(usb_device_descriptor));
	if (-1 == rlen) {
		std::cerr << "can't read usbdevfile '" << devfile << "'" << std::endl;
	}
	close(f);
}

int
usb_open(const char *devfile)
{
	// open
	int fd = open(devfile, O_RDWR);
	if (-1 == fd) {
		int errno_bak = errno;
		std::cerr << "usb open failed" << errno_bak << "!!" << std::endl;
	}else if(::flock(fd, LOCK_EX | LOCK_NB) < 0) {
		close(fd);
		fd = -1;
		std::cerr << "share violation" << std::endl;
	}
	return fd;
}
void
usb_claim(int fd, unsigned int interface)
{
	int r = ioctl(fd, USBDEVFS_CLAIMINTERFACE, &interface);
	if (r < 0) {
		int errno_bak = errno;
		if (errno_bak == EBUSY) { // BUSY?
			std::cerr << "usb interface busy." << std::endl;
			return;
		}
		
		// failed
		std::cerr << "usb claim failed: " << errno_bak << std::endl;
	}
}

void
usb_release(int fd, unsigned int interface)
{
	int r = ioctl(fd, USBDEVFS_RELEASEINTERFACE, &interface);
	if (r < 0) {
		int errno_bak = errno;
		// failed
		std::cerr << "usb release failed: " << errno_bak << std::endl;
	}
}

int
usb_setinterface(int fd, const unsigned int interface, const unsigned int altsetting)
{
	usbdevfs_setinterface setintf;
	setintf.interface = interface;
	setintf.altsetting = altsetting;
	return ioctl(fd, USBDEVFS_SETINTERFACE, &setintf);
}

int
usb_ctrl(int fd, usbdevfs_ctrltransfer *ctrl)
{
	int r = ioctl(fd, USBDEVFS_CONTROL, ctrl);
	if (r < 0) {
		int errno_bak = errno;
		// failed
		std::cerr << "usb ctrl failed: " << errno_bak << std::endl;
	}
	return r;
}

int
usb_submiturb(int fd, usbdevfs_urb* urbp)
{
	return ioctl(fd, USBDEVFS_SUBMITURB, urbp);
}

int
usb_reapurb_ndelay(int fd, usbdevfs_urb** urbpp)
{
	return ioctl(fd, USBDEVFS_REAPURBNDELAY, (void *)urbpp);
}

int
usb_discardurb(int fd, usbdevfs_urb* urbp)
{
	return ioctl(fd, USBDEVFS_DISCARDURB, urbp);
}

