/* B25 Decoder */

#include <iostream>
#include <err.h>

#include "./arib25/arib_std_b25.h"
#include "./arib25/b_cas_card.h"

#define _REAL_B25_
#include "B25Decoder.hpp"

void
B25Decoder::close()
{
	if (b25 != NULL) {
		b25->release(b25);
		b25 = NULL;
	}

	if (bcas != NULL) {
		bcas->release(bcas);
		bcas = NULL;
	}
}

int
B25Decoder::open(void *pCardDev)
{
	int stat = 0;
	
	// 初期化済の場合リソースを開放
	close();
	
	// b25作成
	b25 = create_arib_std_b25();
	if (b25 == NULL) err(2, "failed to create arib_std_b25.");
	
	// b25->set_multi2_round
	stat = b25->set_multi2_round(b25, round);
	if (stat < 0) {
		std::cerr << "b25->set_multi2_round(" << round << ") failed. code=" << stat;
		return 1;
	}
	
	// b25->set_strip
	stat = b25->set_strip(b25, strip ? 1 : 0);
	if (stat < 0) {
		std::cerr << "b25->set_strip(" << strip << ") failed. code=" << stat;
		return 1;
	}
	
	// b25->set_emm_proc
	stat = b25->set_emm_proc(b25, emm ? 1 : 0);
	if (stat < 0) {
		std::cerr << "b25->set_emm_proc(" << emm << ") failed. code=" << stat;
		return 1;
	}

	// bcas作成
	bcas = create_b_cas_card();
	if (bcas == NULL) {
		std::cerr << "failed to create b_cas_card.";
		return 2;
	}
	
	//
	*(void**)(bcas->private_data) = pCardDev;
	// bcas->init
	stat = bcas->init(bcas);
	if (stat < 0) {
		std::cerr << "bcas->init failed. code=" << stat;
		return 2;
	}

	// b25->set_b_cas_card
	stat = b25->set_b_cas_card(b25, bcas);
	if (stat < 0) {
		std::cerr << "b25->set_b_cas_card failed. code=" << stat;
		return 2;
	}
	return 0;
}

void
B25Decoder::flush()
{
	int stat = b25->flush(b25);
	if (stat < 0) {
		std::cerr << "b25->flush failed. code=" << stat;
		return;
	}
}

void
B25Decoder::put(const uint8_t *buf, int32_t len)
{
	ARIB_STD_B25_BUFFER sbuf = { (uint8_t *)buf, len };
	
	int stat = b25->put(b25, &sbuf);
	if (stat < 0) {
		std::cerr << "b25->put failed. code=" << stat;
		return;
	}
}

int32_t
B25Decoder::get(const uint8_t **bufp)
{
	ARIB_STD_B25_BUFFER dbuf = { NULL, 0 };
	int stat = b25->get(b25, &dbuf);
	if (stat < 0) {
		std::cerr << "b25->get failed. code=" << stat;
		return 0;
	}
	
	*bufp = dbuf.data;
	return dbuf.size;
}

