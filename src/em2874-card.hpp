// EM2874 Card

#ifndef _EM2874CARD_HPP_
#define _EM2874CARD_HPP_

#ifdef __cplusplus
extern "C" {
#endif
extern int KtvCardReset(const void *pCardDev);
extern int KtvCardTransmit(const void *pCardDev, const void *pSend, const size_t nSendLen, void *pRecv, size_t *nRecvLen);

#ifdef __cplusplus
}
#endif

#endif

