/*
recfsusb2n for Fedora 12 Linux 2.6
http://tri.dw.land.to/fsusb2n/
2009-09-14 20:22
2011-04-06 23:50
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#include <iostream>

#include "ktv.hpp"

#ifdef B25
	#include "B25Decoder.hpp"
#endif

/* usageの表示 */
void usage(char *argv0)
{
	std::cerr << "usage: " << argv0
#ifdef B25
		<< " [--b25]"
#endif
		<< " [-v] channel recsec destfile" << std::endl;
	exit(1);
}
/* オプション情報 */
struct Args {
	bool b25;
	bool std_out;
	int channel;
	bool forever;
	int recsec;
	char* destfile;
	bool verbose;
};

/* オプションの解析 */
Args parseOption(int argc, char *argv[])
{
	Args args = {
		false,
		false,
		0,
		false,
		0,
		NULL,
		false
	};
	
	while (1) {
		int option_index = 0;
		static option long_options[] = {
			{ "b25",      0, NULL, 'b' },
			{ "B25",      0, NULL, 'b' },
			{ 0,     0, NULL, 0   }
		};
		
		int r = getopt_long(argc, argv,
		                    "bv",
		                    long_options, &option_index);
		if (r < 0) {
			break;
		}
		
		switch (r) {
			case 'b':
				args.b25 = true;
				break;
			case 'v':
				args.verbose = true;
				break;
			default:
				break;
		}
	}
	
	if (argc - optind != 3) {
		usage(argv[0]);
	}
	
	char* chstr    = argv[optind++];
	args.channel   = atoi(chstr);
	char *recsecstr = argv[optind++];
	if (strcmp("-", recsecstr) == 0) {
		args.forever = true;
	}
	args.recsec    = atoi(recsecstr);
	args.destfile = argv[optind++];
	if (strcmp("-", args.destfile) == 0) {
		args.std_out = true;
	}
	
	return args;
}

static bool caughtSignal = false;

void sighandler(int arg)
{
	caughtSignal = true;
}


int main(int argc, char **argv)
{
	Args args = parseOption(argc, argv);

	if (!args.forever && args.recsec <= 0) {
		std::cerr << "recsec must be (recsec > 0)." << std::endl;
		exit(1);
	}
	// ログ出力先設定
	std::ostream& log = args.std_out ? std::cerr : std::cout;
	log << "recfsusb2n ver. 0.9.2" << std::endl << "ISDB-T DTV Tuner FSUSB2N" << std::endl;
	EM2874Device::setLog(&log);

	EM2874Device *usbDev = EM2874Device::AllocDevice();
	if(usbDev == NULL)
		return 1;
	usbDev->initDevice2();

	KtvDevice *pDev;
	if(usbDev->getDeviceID() == 2) {
		pDev = new Ktv2Device(usbDev);
	}else{
		pDev = new Ktv1Device(usbDev);
	}

	pDev->InitTuner();
	// 周波数を計算 (UHF13ch = 473143 kHz)
	pDev->SetFrequency( (args.channel * 6000) + 395143 );
	pDev->InitDeMod();
	pDev->ResetDeMod();

#ifdef B25
	// B25初期化
	B25Decoder b25dec;
	if (args.b25) {
		b25dec.setRound(4);
		b25dec.setStrip(true);
		b25dec.setEmmProcess(false);
		if(b25dec.open(usbDev) == 0) {
			log << "B25Decoder initialized." << std::endl;
		}else{
			// エラー時b25を行わず処理続行。終了ステータス1
			std::cerr << "disable b25 decoding." << std::endl;
			args.b25 = false;
		}
	}
#endif /* defined(B25) */

	// 出力先ファイルオープン
	FILE *dest;
	if(!args.std_out) {
		dest = fopen(args.destfile, "w");
		if (NULL == dest) {
			std::cerr << "can't open file '" << args.destfile << "' to write." << std::endl;
			exit(1);
		}
	}else dest = stdout;

	// SIGINT, SIGTERM
	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	do {
		sleep(1);
	} while(pDev->DeMod_GetSequenceState() < 8 && !caughtSignal);

	// 録画時間の基準開始時間
	time_t time_start = time(NULL);

	usbDev->startStream();

	// Main loop
	while (!caughtSignal && (args.forever || time(NULL) <= time_start + args.recsec)) {
		usleep(750000);
		const void *buf = NULL;
		int rlen = usbDev->getStream(&buf);
		if (0 == rlen) continue;
#ifdef B25
			// B25を経由して受け取る
			if (args.b25) {
				const uint8_t *b25buf = (const uint8_t *)buf;
				b25dec.put(b25buf, rlen);
				rlen = b25dec.get(&b25buf);
				if (0 == rlen) {
					continue;
				}
				buf = b25buf;
			}
#endif /* defined(B25) */

		if(args.verbose) {
			log << "Sequence = " << (unsigned)pDev->DeMod_GetSequenceState() << ", Quality = " << 0.02*pDev->DeMod_GetQuality()
 << ", " << rlen << "bytes wrote." << std::endl;
		}
		if((unsigned)rlen > fwrite(buf, 1, rlen, dest)) {
			log << "fwrite failed." << std::endl;
		}
	}

	if (caughtSignal) {
		log << "interrupted." << std::endl;
	}
	usbDev->stopStream();

	// Default Signal Handler
	struct sigaction saDefault;
	memset(&saDefault, 0, sizeof(struct sigaction));
	saDefault.sa_handler = SIG_DFL;
	sigaction(SIGINT,  &saDefault, NULL);
	sigaction(SIGTERM, &saDefault, NULL);

#ifdef B25
	// B25Decoder flush Data
	if (args.b25) {
		b25dec.flush();
		const uint8_t *buf = NULL;
		int rlen = b25dec.get(&buf);
		if (0 < rlen) {
			fwrite(buf, 1, rlen, dest);
		}
	}
#endif /* defined(B25) */

	// 録画時間の測定
	time_t time_end = time(NULL);

	fflush(dest);
	if(!args.std_out) {
		fclose(dest);
	}

	log << "done." << std::endl;
	log << "Rec time: " << static_cast<unsigned>(time_end - time_start) << " sec." << std::endl;
	delete pDev;
	delete usbDev;
	return 0;
}

/* */

