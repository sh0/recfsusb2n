
#ifndef _B25_DECODER_H_
#define _B25_DECODER_H_

#ifndef _REAL_B25_
	typedef void ARIB_STD_B25;
	typedef void B_CAS_CARD;
#endif

class B25Decoder {
public:
	B25Decoder() : round(4), strip(false), emm(false), b25(NULL), bcas(NULL) {
	};
	virtual ~B25Decoder() {
		close();
	};

	void setRound(int32_t paramRound) {
		round = paramRound;
	};

	void setStrip(bool paramStrip) {
		strip = paramStrip;
	};

	void setEmmProcess(bool paramEmm) {
		emm = paramEmm;
	};


	void close();
	int open(void *pCardDev);
	void flush();
	void put(const uint8_t *buf, int32_t len);
	int32_t get(const uint8_t **bufp);
	
protected:
	/** Multi2のround数(default = 4) */
	int32_t round;
	/** nullパケットを削除するか? */
	bool strip;
	/** EMMの処理を行なうか? */
	bool emm;
	
	/** B25 */
	ARIB_STD_B25 *b25;
	/** BCAS */
	B_CAS_CARD   *bcas;
};

#endif /* !defined(_B25_H_) */
