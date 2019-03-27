/*
  ===========================================================================

  Copyright (C) 2010 Evariste F5OEO

  This file is part of LIMESDR_TOOLBOX.

  LIMESDR_TOOLBOX is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LIMESDR_TOOLBOX is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LIMESDR_TOOLBOX.  If not, see <http://www.gnu.org/licenses/>.

  ===========================================================================
*/

//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include "./libdvbmod/libdvbmod/libdvbmod.h"
#include <getopt.h>
#include <ctype.h>
 #include <termios.h>
#define PROGRAM_VERSION "0.0.1"

#ifndef WINDOWS

/* Non-windows includes */

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h> /* IPPROTO_IP, sockaddr_in, htons(), 
htonl() */
#include <arpa/inet.h>  /* inet_addr() */
#include <netdb.h>
#else

/* Windows-specific includes */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#endif /* WINDOWS */
#include <time.h>
#include <sys/ioctl.h>

#include "limesdr_util.h"

FILE *input, *output;
enum
{
	DVBS,
	DVBS2
};
int Bitrate = 0;
int ModeDvb = 0;
#define BUFFER_SIZE (188)
int Pilot = 0;
unsigned int SymbolRate = 0;
int FEC = CR_1_2;
bool Simu=false;
float GainSimu=1.0; // Gain QSB
float TimingSimu=1.0; //Cycle QSB

//LimeSpecific
lms_device_t* device = NULL;
unsigned int freq = 0;
	double bandwidth_calibrating = 8e6;
	double sample_rate = 2e6;
	double gain = 1;
	unsigned int buffer_size = 129960;
	double postpone_emitting_sec = 0.5;
	unsigned int device_i = 0;
	unsigned int channel = 0;
	

static uint64_t _timestamp_ns(void)
{
	struct timespec tp;

	if (clock_gettime(CLOCK_REALTIME, &tp) != 0)
	{
		return (0);
	}

	return ((int64_t)tp.tv_sec * 1e9 + tp.tv_nsec);
}

unsigned int NullFiller(lms_stream_t *tx_stream,int NbPacket)
{
	unsigned char NullPacket[188] = {0x47, 0x1F, 0xFF};
	unsigned int TotalSampleWritten = 0;
	for (int i = 0; i < NbPacket; i++)
	{
		int len;
		if (ModeDvb == DVBS)
			len = DvbsAddTsPacket(NullPacket);
		if (ModeDvb == DVBS2)
			len = Dvbs2AddTsPacket(NullPacket);
		if (len != 0)
		{
			sfcmplx *Frame = NULL;
			//fprintf(stderr, "Len %d\n", len);
			if (ModeDvb == DVBS)
				Frame = Dvbs_get_IQ();
			if (ModeDvb == DVBS2)
				Frame = Dvbs2_get_IQ();

			//fwrite(Frame, sizeof(sfcmplx), len, output);
			 LMS_SendStream( tx_stream, Frame, len, NULL, 1000 ); 
			TotalSampleWritten += len;
		}
	}
	return TotalSampleWritten;
}


bool Tune(lms_stream_t *tx_stream,int Frequency)
{
	
	#define LEN_CARRIER 1000
	static sfcmplx Frame[LEN_CARRIER];
	if(Frequency==0)
	for(int i=0;i<LEN_CARRIER;i++)
	{
		Frame[i].re=0x7fff;
		Frame[i].im=0;
	}
	else
	{
		//write a Chirp or Spectrum paint something ?
	}
	int nb_samples = LMS_SendStream( tx_stream, Frame, LEN_CARRIER, NULL, 1000 ); 
	//fwrite(Frame, sizeof(sfcmplx), LEN_CARRIER, output);
	return true;
}

bool RunWithFile(lms_stream_t *tx_stream,bool live)
{

	static unsigned char BufferTS[BUFFER_SIZE];

	static uint64_t DebugReceivedpacket = 0;
	//fprintf(stderr, "Output samplerate is %u\n", CalibrateOutput());
	if(FEC == -1)
	{
		Tune(tx_stream,0);
		return true; // Bypass real modulation
	}

	if (live)
	{
		int nin, nout;
		int ret = ioctl(fileno(input), FIONREAD, &nin);
		
		
		if ((ret == 0) && (nin <BUFFER_SIZE))
		{
			//fprintf(stderr,"Pipein=%d\n",nin);
			usleep(100);
			//NullFiller(100);
			return true;
		}	
		
	}
	
	int NbRead = fread(BufferTS, 1, BUFFER_SIZE, input);
	DebugReceivedpacket += NbRead;
	if (NbRead != BUFFER_SIZE)
	{
		if (!live)
        {
            fseek(input, 0, SEEK_SET);
            fprintf(stderr,"TS Loop file\n");
            return true;
			//return false; //end of file
        }
	}

	if (NbRead % 188 != 0)
		fprintf(stderr, "TS alignment Error\n");
	if (BufferTS[0] != 0x47)
	{
		fprintf(stderr, "TS Sync Error Try to Resynch\n");
		for (int i = 0; i < NbRead; i++)
		{
			if (BufferTS[i] == 0x47)
			{
				fread(BufferTS, 1, i, input); //Try to read garbage
				fprintf(stderr, "TS Sync recovery\n");
				break;
			}
		}
		fread(BufferTS, 1, BUFFER_SIZE, input); //Read the next aligned
	}
	for (int i = 0; i < NbRead; i += 188)
	{
		int len = 0;
		/*if ((BufferTS[i + 1] == 0x1F) && (BufferTS[i + 2] == 0xFF))
			continue; // Remove Null packets*/
		if (ModeDvb == DVBS)
			len = DvbsAddTsPacket(BufferTS + i);
		if (ModeDvb == DVBS2)
			len = Dvbs2AddTsPacket(BufferTS + i);

		if (len != 0)
		{
			sfcmplx *Frame = NULL;
			//fprintf(stderr, "Len %d\n", len);
			if (ModeDvb == DVBS)
				Frame = Dvbs_get_IQ();
			if (ModeDvb == DVBS2)
				Frame = Dvbs2_get_IQ();
			if(Simu==false)
			{
				
								
				/*if(0xFFFF-nout<len*sizeof(sfcmplx))
				{
					 fprintf(stderr,"dvb2iq Blocked by output to write %d/left %d -> \n",len*sizeof(sfcmplx),nout);
					 return true;
				}*/	
				//fprintf(stderr,"Len = %d\n", len);
				int nb_samples = LMS_SendStream( tx_stream, Frame, len, NULL, 1000 ); 
				//fwrite(Frame, sizeof(sfcmplx), len, output);
			}
			/*else
			{
				sfcmplx *FrameSimu=(sfcmplx *)malloc(len*sizeof(sfcmplx));
				for(int j=0;j<len;j++)
				{
					static int ComptSample=0;
					FrameSimu[j].re=Frame[j].re*(0.5+0.5*GainSimu*sin(2*3.1415*(ComptSample/(SymbolRate*TimingSimu)))));
					FrameSimu[j].im=Frame[j].im*(0.5+0.5*GainSimu*sin(2*3.1415*(ComptSample/(SymbolRate*TimingSimu)))));
				}

			}*/

		}
	}
	int n, ret;

	return true;
}

// Global variable used by the signal handler and capture/encoding loop
static int want_quit = 0;

// Global signal handler for trapping SIGINT, SIGTERM, and SIGQUIT
static void signal_handler(int signal)
{
	want_quit = 1;
}

void print_usage()
{

	fprintf(stderr,
			"limesdr_dvb -%s\n\
Usage:\nlimesdr_dvb -s SymbolRate [-i File Input] [-f Fec]  [-m Modulation Type]  [-c Constellation Type] [-p] [-h] \n\
-i            Input Transport stream File (default stdin) \n\
-s            SymbolRate in (10000-4000000) \n\
-f            Fec : {1/2,3/4,5/6,7/8} for DVBS {1/4,1/3,2/5,1/2,3/5,2/3,3/4,5/6,7/8,8/9,9/10} for DVBS2 \n\
-m            Modulation Type {DVBS,DVBS2}\n\
-c 	      Constellation mapping (DVBS2) : {QPSK,8PSK,16APSK,32APSK}\n\
-p 	      Pilots on(DVBS2)\n\
-r 	      upsample (1,2,4) Better MER for low SR(<1M) choose 4\n\
-v 	      ShortFrame(DVBS2)\n\
-d 	      print net bitrate on stdout and exit\n\
-t 	      Tune frequency in Hertz \n\
-g 	      Gain (0..1) \n\
-q 	      {0,1} 0:Use a calibration file 1:Process calibration (!HF peak!)\n\
-h            help (print this help).\n\
Example : ./limesdr_dvb -s 1000 -f 7/8 -m DVBS2 -c 8PSK -p\n\
\n",
			PROGRAM_VERSION);

} /* end function print_usage */

int main(int argc, char **argv)
{
	input = stdin;
	output = stdout;
	
	int Constellation = M_QPSK;
	int a;
	int anyargs = 0;
	int upsample = 1;
	bool AskNetBitrate=false;
	ModeDvb = DVBS;
	bool ShortFrame=false;
	//Lime
	bool WithCalibration=false;

	while (1)
	{
		a = getopt(argc, argv, "i:s:f:c:hf:m:c:pr:dvt:g:q:");

		if (a == -1)
		{
			if (anyargs)
				break;
			else
				a = 'h'; //print usage and exit
		}
		anyargs = 1;

		switch (a)
		{
		case 'i': // InputFile
			input = fopen(optarg, "r");
			if (NULL == input)
			{
				fprintf(stderr, "Unable to open '%s': %s\n",
						optarg, strerror(errno));
				exit(EXIT_FAILURE);
			}
			break;
		case 's': // SymbolRate in KS
			SymbolRate = atoi(optarg) ;
			break;
		case 'f': // FEC
			if (strcmp("1/2", optarg) == 0)
				FEC = CR_1_2;
			if (strcmp("2/3", optarg) == 0)
				FEC = CR_2_3;
			if (strcmp("3/4", optarg) == 0)
				FEC = CR_3_4;
			if (strcmp("5/6", optarg) == 0)
				FEC = CR_5_6;
			if (strcmp("7/8", optarg) == 0)
				FEC = CR_7_8;

			//DVBS2 specific
			if (strcmp("1/4", optarg) == 0)
				FEC = CR_1_4;
			if (strcmp("1/3", optarg) == 0)
				FEC = CR_1_3;
			if (strcmp("2/5", optarg) == 0)
				FEC = CR_2_5;
			if (strcmp("3/5", optarg) == 0)
				FEC = CR_3_5;
			if (strcmp("4/5", optarg) == 0)
				FEC = CR_4_5;
			if (strcmp("8/9", optarg) == 0)
				FEC = CR_8_9;
			if (strcmp("9/10", optarg) == 0)
				FEC = CR_9_10;

			if (strcmp("carrier", optarg) == 0)
			{
				FEC = -1;
			} //CARRIER MODE
			if (strcmp("test", optarg) == 0)
				FEC = -2; //TEST MODE
			break;
		case 'h': // help
			print_usage();
			exit(0);
			break;
		case 'l': // loop mode
			break;
		case 'm': //Modulation DVBS or DVBS2
			if (strcmp("DVBS", optarg) == 0)
				ModeDvb = DVBS;
			if (strcmp("DVBS2", optarg) == 0)
				ModeDvb = DVBS2;
		case 'c': // Constellation DVB S2
			if (strcmp("QPSK", optarg) == 0)
				Constellation = M_QPSK;
			if (strcmp("8PSK", optarg) == 0)
				Constellation = M_8PSK;
			if (strcmp("16APSK", optarg) == 0)
				Constellation = M_16APSK;
			if (strcmp("32APSK", optarg) == 0)
				Constellation = M_32APSK;
			break;
		case 'p':
			Pilot = 1;
			break;
		case 'd':
			AskNetBitrate = true;
			break;	
		case 'r':
			upsample = atoi(optarg);
			if (upsample > 4)
				upsample = 4;
			break;
		case 'v':
			ShortFrame = true;
			break;	
		case 't':
			freq = atof(optarg);
			break;	
		case 'g':
			gain = atof(optarg);
			break;
		case 'q':
			WithCalibration = (atoi(optarg)==1);
			break;
		case -1:
			break;
		case '?':
			if (isprint(optopt))
			{
				fprintf(stderr, "dvb2iq `-%c'.\n", optopt);
			}
			else
			{
				fprintf(stderr, "dvb2iq: unknown option character `\\x%x'.\n", optopt);
			}
			print_usage();

			exit(1);
			break;
		default:
			print_usage();
			exit(1);
			break;
		} /* end switch a */
	}	 /* end while getopt() */

	if (SymbolRate == 0)
	{
		fprintf(stderr, "SymbolRate is mandatory !\n");
		exit(0);
	}
	if ((ModeDvb == DVBS)&&(FEC>=0))
	{
		Bitrate = DvbsInit(SymbolRate, FEC, Constellation, upsample);
	}
	if ((ModeDvb == DVBS2)&&(FEC>=0))
	{
		Bitrate = Dvbs2Init(SymbolRate, FEC, Constellation, Pilot, RO_0_35, upsample,ShortFrame);
	}

	fprintf(stderr, "Net TS bitrate input should be %d\n", Bitrate);
	if(AskNetBitrate)
	{
		fprintf(stdout, "%d\n", Bitrate);
		exit(1);
	}

		signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGQUIT, signal_handler);
        signal(SIGKILL, signal_handler);
        signal(SIGPIPE, signal_handler);

	bool isapipe = (fseek(input, 0, SEEK_CUR) < 0); //Dirty trick to see if it is a pipe or not
	if (isapipe)
	{
	
		
		fprintf(stderr, "Using live mode\n");
	}	
	else
			fprintf(stderr, "Using file mode\n");

// Init LimeSDR
char* antenna = "BAND1";
	double host_sample_rate;

if(upsample>1)
	 sample_rate=SymbolRate*upsample; // Upsampling
else 
	 	 sample_rate=SymbolRate; 
	if ( limesdr_init( sample_rate,
			   freq,
			   bandwidth_calibrating,
			   gain,
			   device_i,
			   channel,
			   antenna,
			   LMS_CH_TX,
			   &device,
			   &host_sample_rate,
			   WithCalibration) < 0 ) {
		return 1;
	}
	fprintf(stderr, "sample_rate: %f\n", host_sample_rate);

if(ModeDvb == DVBS2)
{
	buffer_size=((ShortFrame)?16200:64800)*upsample; //4Buffer of frame Max
}
else
{
	buffer_size=16200*upsample; //FixMe for DVB-S
}


	lms_stream_t tx_stream = {

		handle : 0,		
		isTx : LMS_CH_TX,
		channel : channel,
		fifoSize : buffer_size,
		throughputVsLatency : 0.0,
		dataFmt : lms_stream_t::LMS_FMT_I16
	};

	if ( LMS_SetupStream(device, &tx_stream) < 0 ) {
		fprintf(stderr, "LMS_SetupStream() : %s\n", LMS_GetLastErrorMessage());
		return 1;
	}
	if(SetGFIR(device,upsample)<0)
	{
		fprintf(stderr, "SetGFIR() : %s\n", LMS_GetLastErrorMessage());
		return -1;
	}

		if(upsample>1)
		LMS_SetGFIR(device, LMS_CH_TX, 0, LMS_GFIR3, true);
	else
			LMS_SetGFIR(device, LMS_CH_TX, 0, LMS_GFIR3, false);



if (isapipe)
	{
			static unsigned char BufferDummyTS[BUFFER_SIZE*10];	
		int nin=0xffff;
		while(nin>BUFFER_SIZE*10)
		{
			int ret = ioctl(fileno(input), FIONREAD, &nin);
			fread(BufferDummyTS,1,BUFFER_SIZE*10,input);
			fprintf(stderr,"Init Pipein=%d\n",nin);
		}	
		
	}

int DebugCount=0;
bool FirstTx=true;
	bool Transition=true;
LMS_StartStream(&tx_stream);
LMS_SetNormalizedGain( device, LMS_CH_TX, channel, gain );
while (!want_quit)
	{
		lms_stream_status_t Status;
			LMS_GetStreamStatus(&tx_stream,&Status);
		if(!FirstTx&&Status.fifoFilledCount<Status.fifoSize*0.1)
		{
			NullFiller(&tx_stream,1);
			fprintf(stderr,"Underflow %d\n",Status.fifoFilledCount);		
			
		}

		if(DebugCount%1000==0)
		{
			//fprintf(stderr,"Fifo =%d/%d\n",Status.fifoFilledCount,Status.fifoSize);
			
		}	
		DebugCount++;	

		if(FirstTx&&(Status.fifoFilledCount==Status.fifoSize))
		{
			fprintf(stderr,"Restart stream %d \n",Status.fifoFilledCount);
			//LMS_StartStream(&tx_stream);
			//LMS_SetNormalizedGain( device, LMS_CH_TX, channel, gain );
			/*while(Status.fifoFilledCount>Status.fifoSize*3/4)
			{
				LMS_GetStreamStatus(&tx_stream,&Status);
				usleep(100);
			}	*/
			FirstTx=false;
		}	
		
		

		 RunWithFile(&tx_stream,isapipe);
	}	

LMS_SetNormalizedGain( device, LMS_CH_TX, channel, 0 );
	LMS_StopStream(&tx_stream);
	LMS_DestroyStream(device, &tx_stream);
	LMS_EnableChannel( device, LMS_CH_TX, channel, false);
	LMS_Close(device);

	return 0;
}
