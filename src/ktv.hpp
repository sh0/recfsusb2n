/* FSUSB2N
	Tuner	NXP TDA18211HD/C2
	復調	富士通 MB86A20S

	Tuner	MaxLinear MXL135RF
	復調	富士通 MB86A21S
*/

#ifndef _KTV_FSUSB2_HPP_
#define _KTV_FSUSB2_HPP_

#include "em2874-core.hpp"

class KtvDevice {
public:
	KtvDevice (EM2874Device *pDev);
	~KtvDevice();

	virtual void InitTuner () = 0;
	virtual void SetFrequency (unsigned int freq_khz) = 0;
	virtual void InitDeMod () = 0;
	virtual void ResetDeMod () = 0;
	uint8_t DeMod_GetSequenceState();
	unsigned int DeMod_GetQuality();

	EM2874Device *usbDev;

protected:
	bool DeMod_Write (const uint8_t idx, const uint8_t val);
	uint8_t DeMod_Read (const uint8_t idx);
	bool Tuner_I2C_Write (uint8_t* buf, const int len);
	bool Tuner_I2C_Read (uint8_t* wbuf, const int wlen, uint8_t* rbuf, const int rlen);

	void CN_Counter_Reset();
	bool CN_Flag_chk ();
	uint16_t CN_Value_Get ();

	bool Tuner_InitDone;
};

class Ktv1Device : public KtvDevice
{
public:
	Ktv1Device (EM2874Device *pDev);
	~Ktv1Device();

	void InitTuner ();
	void SetFrequency (unsigned int freq_khz);
	void InitDeMod ();
	void ResetDeMod ();

private:
	void Tuner_RegWrite (const uint8_t offset, const int len);
	void Tuner_RegRead (const uint8_t offset, const int len);
	void calc_cal_pll (const unsigned int freq_khz);
	void calc_main_pll (const unsigned int freq_khz);

	uint8_t tuner_r[39];
	unsigned int Tuner_TempRFCal;
};

class Ktv2Device : public KtvDevice
{
public:
	Ktv2Device (EM2874Device *pDev);
	~Ktv2Device();

	void InitTuner ();
	void SetFrequency (unsigned int freq_khz);
	void InitDeMod ();
	void ResetDeMod ();
};

#endif

