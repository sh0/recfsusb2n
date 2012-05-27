// USB操作

#ifndef _USB_OPS_HPP_
#define _USB_OPS_HPP_

#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
//#include <string>

void	usb_getdesc(const char *devfile, usb_device_descriptor* desc);
int	usb_open(const char *devfile);
void	usb_claim(int fd, unsigned int interface);
void	usb_release(int fd, unsigned int interface);
int	usb_setinterface(int fd, const unsigned int interface, const unsigned int altsetting);
int	usb_ctrl(int fd, usbdevfs_ctrltransfer *ctrl);
int	usb_submiturb(int fd, usbdevfs_urb* urbp);
int	usb_reapurb_ndelay(int fd, usbdevfs_urb** urbpp);
int	usb_discardurb(int fd, usbdevfs_urb* urbp);

#endif

