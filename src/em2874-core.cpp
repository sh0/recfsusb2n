
#include <unistd.h>
#include <string.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "usbops.hpp"
#include "em2874-core.hpp"

#define USBREQ_TIMEOUT	100
#define I2C_TIMEOUT		200

#define EM28XX_REG_I2C_RET		0x05
#define EM28XX_REG_CHIPID		0x0A

#define EM2874_REG_CAS_STATUS	0x70
#define EM2874_REG_CAS_DATALEN	0x71
#define EM2874_REG_CAS_MODE1	0x72
#define EM2874_REG_CAS_RESET	0x73
#define EM2874_REG_CAS_MODE2	0x75

#define TARGET_ID_VENDOR	0x0511
#define TARGET_ID_PRODUCT	0x0029
#define TARGET_ID_PRODUCT2	0x003b

const char BASE_DIR_UDEV[]	= "/dev/bus/usb"; // udev_USB
const char BASE_DIR_USBFS[]	= "/proc/bus/usb"; // usbfs

inline uint8_t ICC_checkSum (const uint8_t* data, int len)
{
	uint8_t sum = 0;
	for ( ; len > 0; len-- ) {
		sum ^= *data++;
	}
	return sum;
}

inline void miliWait( int s )	{ usleep(s*1000); }
std::ostream * EM2874Device::log = NULL;

EM2874Device::EM2874Device ()
: isCardReady(false), fd(-1), isStream(false)
{
}

EM2874Device::~EM2874Device ()
{
	if(fd >= 0)
	{
		writeReg(EM2874_REG_TS_ENABLE, 0);
		writeReg(EM28XX_REG_GPIO, 0xFF);
		writeReg(EM2874_REG_CAS_MODE1, 0x0);
		writeReg(0x0C, 0x0);
		usb_release(fd, 0);
		close(fd);
	}
}

bool EM2874Device::openDevice (const char *devfile)
{
	usb_device_descriptor usb_desc;
	usb_getdesc(devfile, &usb_desc);
	if(usb_desc.idVendor != TARGET_ID_VENDOR ||
	(usb_desc.idProduct != TARGET_ID_PRODUCT && usb_desc.idProduct != TARGET_ID_PRODUCT2))
		return false;
	fd = usb_open(devfile);
	if(fd == -1)
		return false;
	usb_claim(fd, 0);

	uint8_t val;

	if(readReg(0x0A, &val) && readReg(0x0C, &val) && readReg(0x0B, &val)
	){

		writeReg(EM28XX_REG_GPIO, 0xFF);
	}else{
		return false;
	}
	if(writeReg(0x0C, 0x10)
	&& writeReg(0x12, 0x27)
	&& writeReg(EM28XX_REG_GPIO, 0xFE)
	&& writeReg(EM2874_REG_CAS_MODE1, 0x0))
	{
		writeReg(0x13, 0x10);
		writeReg(0x10, 0);
		writeReg(0x11, 0x11);
		writeReg(0x28, 0x01);
		writeReg(0x29, 0xff);
		writeReg(0x2a, 0x01);
		writeReg(0x2b, 0xff);
		writeReg(0x1c, 0);
		writeReg(0x1d, 0);
		writeReg(0x1e, 0);
		writeReg(0x1f, 0);
		writeReg(0x1b, 0);
		writeReg(0x5e, 128);
		writeReg( EM2874_REG_TS_ENABLE, 0 );
		return true;
	}

	return false;
}

EM2874Device* EM2874Device::AllocDevice()
{
	EM2874Device *pDev = new EM2874Device();

	boost::filesystem::path base_dir = boost::filesystem::path(BASE_DIR_UDEV);
	if (!boost::filesystem::exists(base_dir) || !boost::filesystem::is_directory(base_dir)) {
		base_dir = boost::filesystem::path(BASE_DIR_USBFS);
	}

	bool isFound (false);
	// base_dirからデバイスファイルを探す
	boost::filesystem::directory_iterator end;
	for (boost::filesystem::directory_iterator bus_iter(base_dir); bus_iter != end && !isFound; ++bus_iter) {
		// バスでループ
		if (boost::filesystem::is_directory(*bus_iter)) {
			// ディレクトリのみ
			for (boost::filesystem::directory_iterator dev_iter(*bus_iter); dev_iter != end && !isFound; ++dev_iter) {
				// バスに繋がっているデバイスでループ
				if (!boost::filesystem::is_directory(*dev_iter)) {
					// 開く
					if (pDev->openDevice(dev_iter->path().c_str())) {
						isFound = true;
						if(log) *log << "device: " << dev_iter->path() << std::endl;
						break;
					}
				}
			}
		}
	}
	if(!isFound) {
		delete pDev;
		if (log) *log << "no devices can be used." << std::endl;
		return NULL;
	}
	return pDev;
}

bool EM2874Device::initDevice2 ()
{
	bool ICCflg;
	writeReg( EM28XX_REG_GPIO, 0x3e );
	ICCflg = resetICC_1();

	miliWait(100);
	writeReg( EM28XX_REG_GPIO, 0x7e );
	miliWait(50);

	if(ICCflg) {
		if(waitICC() < 0) {
			goto CAS_off;
		}
		ICCflg = resetICC_2();
	}else{
		goto CAS_off;
	}
	return true;
CAS_off:
	miliWait(60);
	writeReg(EM2874_REG_CAS_MODE1, 0x0);
	return false;
}

uint8_t EM2874Device::readReg (const uint8_t idx)
{
	uint8_t val;
	if(!readReg (idx, &val)) {
#ifdef DEBUG
	*log << "readReg(" << (unsigned)idx << ") failed.";
#endif
	}
	return val;
}

int EM2874Device::readReg (const uint8_t idx, uint8_t *val)
{
	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0, 0, idx, 1, USBREQ_TIMEOUT, val};
	return usb_ctrl(fd, &ctrl1);
}

int EM2874Device::writeReg (const uint8_t idx, const uint8_t val)
{
	uint8_t sbuf[1] = {val};
	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0, 0, idx, 1, USBREQ_TIMEOUT, sbuf};
	return usb_ctrl(fd, &ctrl1);
}

bool EM2874Device::readI2C (const uint8_t addr, const uint16_t size, uint8_t *data, const bool isStop)
{
	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE, isStop ? 2 : 3, 0, addr, size, USBREQ_TIMEOUT, data};
	if(usb_ctrl(fd, &ctrl1) < 0)	return false;

	uint8_t ret = readReg(EM28XX_REG_I2C_RET);
	return ret == 0 ? true : false;
}

bool EM2874Device::writeI2C (const uint8_t addr, const uint16_t size, uint8_t *data, const bool isStop)
{
	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, isStop ? 2 : 3, 0, addr, size, USBREQ_TIMEOUT, data};
		if(usb_ctrl(fd, &ctrl1) < 0)	return false;

	uint8_t ret = readReg(EM28XX_REG_I2C_RET);
	return ret == 0 ? true : false;
}

bool EM2874Device::resetICC_1 ()
{
	if(getCardStatus() == 0x5 &&
	writeReg( EM2874_REG_CAS_MODE1, 0x1 ) &&
	writeReg( EM2874_REG_CAS_RESET, 0x1 )
	) {
		return true;
	}
	return false;
}

bool EM2874Device::resetICC_2 ()
{
	uint8_t rbuff[32];
	size_t rlen = sizeof(rbuff);
	readICC( &rlen, rbuff );
	miliWait(1);

	static uint8_t cmd[] = { 0x00, 0xc1, 0x01, 0xfe, 0x3e };
	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x14, 0, 0x200, sizeof(cmd), USBREQ_TIMEOUT, cmd};
	if(usb_ctrl(fd, &ctrl1) < 0) return false;

	writeReg( EM2874_REG_CAS_MODE2, 0);
	writeReg( EM2874_REG_CAS_STATUS, 0x80 );

	miliWait(30);
	writeReg( EM2874_REG_CAS_DATALEN, 5 );
	if(waitICC() < 0) return false;

	ctrl1.bRequestType = USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE;
	ctrl1.wIndex = 0;
	ctrl1.wLength = 4;
	ctrl1.data = rbuff;
	if( usb_ctrl(fd, &ctrl1) < 0 || rbuff[1] != 0xe1 || rbuff[3] != 0xfe )
		return false;
	cardPCB = 0;
	isCardReady = true;
	return true;
}

bool EM2874Device::resetICC ()
{
	if(!resetICC_1())
		return false;

	if(waitICC() < 0) {
		writeReg(EM2874_REG_CAS_MODE1, 0x0);
		return false;
	}
	return resetICC_2();
}

bool EM2874Device::writeICC ( const size_t size, const void *data )
{
	uint8_t val;
	if( getCardStatus() != 0x5 )
		return false;
	writeReg( EM2874_REG_CAS_MODE1, 0x01 );

	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x14, 0, 0, 0, USBREQ_TIMEOUT, NULL};

	uint8_t cardBuf[256];
	cardBuf[0] = 0;
	cardBuf[1] = cardPCB;
	cardBuf[2] = size;
	memcpy( cardBuf+3, data, size );
	cardBuf[size+3] = ICC_checkSum( cardBuf+1, size+2 );
	val = size + 4;

	for(int i = 0; i < val; i+=64 ) {
		ctrl1.wIndex = 0x200 + i;
		ctrl1.wLength = (val - i) > 64 ? 64 : (val - i);
		ctrl1.data = cardBuf+i;
		if(usb_ctrl(fd, &ctrl1) < 0) {
			return false;
		}
	}

	cardPCB ^= 0x40;
	writeReg( EM2874_REG_CAS_MODE2, 0);
	readReg( EM2874_REG_CAS_STATUS, &val );

	writeReg( EM2874_REG_CAS_DATALEN, size + 4 );
	miliWait(1);
	return true;
}

bool EM2874Device::readICC ( size_t *size, void *data )
{
	uint8_t val;
	val = readReg( EM2874_REG_CAS_DATALEN );
	if( val > *size + 4 || val < 5 )
		return false;
	*size = val - 4;

	usbdevfs_ctrltransfer ctrl1 = {USB_DIR_IN |USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0x14, 0, 0, 0, USBREQ_TIMEOUT, NULL};

	uint8_t cardBuf[256];
	for(int i = 0; i < val; i+=64 ) {
		ctrl1.wIndex = i;
		ctrl1.wLength = (val - i) > 64 ? 64 : (val - i);
		ctrl1.data = cardBuf+i;
		if(usb_ctrl(fd, &ctrl1) < 0) {
			return false;
		}
	}
	memcpy( data, cardBuf+3, val-4 );
	return true;
}

int EM2874Device::waitICC ()
{
	uint8_t val;
	int i;
	for(i = 0; i < 40; i++) {
		miliWait(8);
		if(!readReg(EM2874_REG_CAS_STATUS, &val))
			continue;
		if( val == 5 ) {
			return i;
		}
		if( val == 0 ) return -1;	// card error
	}
	return -2;	// timeout
}

int EM2874Device::getCardStatus()
{
	uint8_t val;
	if(readReg(EM2874_REG_CAS_STATUS, &val)) {
		return val;
	}
	return -1;
}

int EM2874Device::getDeviceID()
{
	uint8_t buf[2], tuner_reg;
	// ROMで判断
	writeReg(EM28XX_REG_I2C_CLK, 0x42);
	buf[0] = 0; buf[1] = 0x6a;	writeI2C(EEPROM_ADDR, 2, buf, false);
	if(!readI2C (EEPROM_ADDR, 2, buf, true))
		return -1;

	if(buf[0] == 0x3b && buf[1] == 0x00)
		return 2;

	// Tuner Regで判断
	writeReg(EM28XX_REG_I2C_CLK, 0x44);
	buf[0] = 0xfe; buf[1] = 0;	writeI2C(DEMOD_ADDR, 2, buf, true);
	tuner_reg = 0x0;
	writeI2C(TUNER_ADDR, 1, &tuner_reg, false);
	readI2C (TUNER_ADDR, 1, &tuner_reg, true);

	buf[0] = 0xfe; buf[1] = 1;	writeI2C(DEMOD_ADDR, 2, buf, true);

	if(tuner_reg == 0x84)	// TDA18271HD
		return 1;

	return 2;
}

/* */

void EM2874Device::startStream()
{
	if (isStream) return;

	writeReg(EM2874_REG_TS_ENABLE, EM2874_TS1_CAPTURE_ENABLE | EM2874_TS1_NULL_DISCARD);
	usb_setinterface(fd, 0, 1);
	ts_func = new TsIoThread (fd, EM2874_EP_TS1);
	ts_thread = new boost::thread(boost::ref(*ts_func));

	isStream = true;
}

void EM2874Device::stopStream()
{
	if (!isStream) return;

	writeReg(EM2874_REG_TS_ENABLE, 0);
	ts_func->cancel();
	ts_thread->join();
	delete ts_thread;
	delete ts_func;

	isStream = false;
}

int EM2874Device::getStream(const void **ptr)
{
	if (!isStream) return 0;
	return ts_func->readBuffer(ptr);
}

/* */

