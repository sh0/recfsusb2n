/* -*- tab-width: 4; indent-tabs-mode: nil -*- */
#ifndef _DECODER_H_
#define _DECODER_H_

#include "./arib25/arib_std_b25.h"
#include "./arib25/b_cas_card.h"

#define TRUE	1
#define FALSE	0

typedef struct Decoder {
    ARIB_STD_B25 *b25;
    B_CAS_CARD *bcas;
} decoder;

typedef struct Decoder_options {
    int round;
    int strip;
    int emm;
} decoder_options;

/* prototypes */
extern decoder *b25_startup(decoder_options *);
extern int b25_shutdown(decoder *);
extern int b25_decode(decoder *,
               ARIB_STD_B25_BUFFER *,
               ARIB_STD_B25_BUFFER *);
extern int b25_finish(decoder *,
               ARIB_STD_B25_BUFFER *,
               ARIB_STD_B25_BUFFER *);


#endif
