/************************************************** 
compile with the command: gcc lab4_t_hum_prog4.c rs232.c -Wall -Wextra \-o2 -o prog4 
**************************************************/ 
#include <stdlib.h> 
#include <stdio.h> 
#include <sys/time.h> 
#include <time.h> 
#ifdef _WIN32 
#include <Windows.h> 
#else 
#include <unistd.h> 
#endif 
#include "rs232.h"
 
double decode_temperature(unsigned int rbuf); 
double decode_humidity(unsigned int rbuf, double temperature_ref);
unsigned long long time_musec(); 
 
int main() 
{ 
	//verificare che tutto funzioni anche con sleep_ms=600 e 1200
	struct tm *gmp; 
	struct tm gm; 
	time_t t0, t; //t0 tempo in cui inizia il programma, t è il tempo di ricezione dei bytes (secondi)
 	unsigned long long t_current, t_prec_data, delta_t, t_flags[20]; /*tempo dell'ultima volta in cui interroghiamo l'fpga, tempo della volta precedente, differenza calcolata 
 	tra le flag, tempi attribuiti alle flag, tempo di avvio del programma (in miscrosecondi)*/
	int ty, tmon, tday, thour, tmin, tsec, time_acq_h_MAX; 
 	float time_acq_s; 
 	int sleep_ms, i, j, l_resto, n_dati, n, cport_nr=17, bdrate=115200; //n è il numero di byte ricevuti, cnt è il numero di cicli che sono stati effettuati 
 	
	int cnt=0; 
 	FILE *file; 
 	FILE *currN; //file that stores current run name to be used by external programs 
 	double val_temp[20], val_hum[20]; //valori decodificati con senso fisico
 	unsigned char buf[4096], resto[6]; //resto custodisce i bytes che sono avanzati dal blocco precedente/dalla decodifica
 	unsigned int val_temp_int[20], val_hum_int[20]; //valori grezzi interi ottenuti da due byte 
 	char NameF[100]; //questa stringa viene usata per salvare il nome del file su cui verranno salvati i dati 
 	char NameF_bytes[100]; //questra stringa verrà usata per salvare il nome del file su cui verranno salvati i bytes ottenuti
	char mode[]={'8','N','1',0}; //configurazione della porta seriale, 8 bit di dati, nessun bit di parità, 1 bit di stop tra ogni blocco inviato 
 	
 	//il programma è come cenerentola, a mezzanotte deve fermarsi
	// t0 per lo start del run, t0= numero di secondi trascorsi tra il 1 gennaio 1970 e il momento in cui il programma è partito 
 	t0 = time(NULL); 
 	t_current=time_musec();
 	gmp = gmtime(&t0); 
 	if (gmp == NULL) 
 	{
 		printf("error on gmp");
 	} 
 	gm = *gmp; 
 	ty=gm.tm_year+1900; 
 	tmon=gm.tm_mon+1; 
 	tday=gm.tm_mday; 
 	thour=gm.tm_hour; 
 	tmin=gm.tm_min; 
 	tsec=gm.tm_sec; 
 	//printf("%d", t0);
 	printf("Inserire sleep time (ms):"); 
  	scanf(" %d", &sleep_ms);
 	printf("Inserire numero di ore di acquisizione:"); 
  	scanf(" %d", &time_acq_h_MAX);
	
	sprintf(NameF_bytes, "bytes_ricevuti_%d-%d-%d.txt", tday, tmon, ty); //stringa contenente il nome del file 
	sprintf(NameF,"sht75_nblab02__Hum_Temp_RUN_%04d%02d%02d%02d%02d%02d_%d_h.txt",ty,tmon,tday,thour,tmin,tsec,time_acq_h_MAX); //stringa contenente il nome del file
 	printf("file_open %s --> durata in ore %d\n",NameF,time_acq_h_MAX); 
 	file=fopen(NameF, "a");
 	fprintf(file, "sleep time (ms):%d\n", sleep_ms);
 	file=fopen(NameF_bytes, "a");
 	fprintf(file, "sleep time (ms):%d\n", sleep_ms);
 	//writing the file name to the current run name file to be read and used by external programs 
 	//---------------------------------------------------------------
 	currN = fopen("currN.txt", "w"); 
 	fprintf(currN, NameF); //questo file contiene il nome dell'ultimo file sht75... 
 	fclose (currN); 
 	//---------------------------------------------------------------

 	//opencomport apre la seriale, ritorna 1 se fallisce
 	if(RS232_OpenComport(cport_nr, bdrate, mode)) 
 	{ 
 		printf("Can not open comport\n"); 
 		return(0); 
 	} 
//POLLCOMPORT  
	while(1) //il ciclo si esegue all'infinito 
 	{ 
		//tempo vecchio
		t_prec_data = t_current;
 		n = RS232_PollComport(cport_nr, buf, 4095); 
 
 		// tempo evento 
 		t_current = time_musec();
		t=time(NULL);  
 		gmp = gmtime(&t); 
 		if (gmp == NULL) 
		{
			printf("error on gmp");
		}
 		printf("cnt= %d  n= %d ", cnt, n); 
 		gm = *gmp; 
 		time_acq_s=(t-t0); 
 		printf("time diff: %f (sec) \n", time_acq_s); 
 		if (time_acq_s> time_acq_h_MAX*3600) 
 		{ 
 			printf(" time_duration RUN in hours > %d \n",time_acq_h_MAX); 
 			break; 
 		} 
 		else 
 		{ 
 			if (cnt%100==0) 
			{
 			printf(" time current in hour %f \n",(float)time_acq_s/3600);
			}
 		} 
 		if(n > 0) 
 		{ 
			cnt++;  		
 			printf("cnt %d received %i bytes \n", cnt, n); 			
 			file=fopen(NameF_bytes, "a"); //l'apertura con a permette di aggiungere nuovi elementi al file se già esiste o di crearlo se non esiste
 			fprintf(file, "tempo ricezione: %ld s\n", t);
 			fprintf(file, "cnt= %d", cnt);
 			for(i=1; i<=n; i++) 
			{
				fprintf(file, "%X ", buf[i-1]);
				printf("%X ", buf[i-1]);
				if(i%6==0)
				{
					fprintf(file, "\n");
					printf("\n");	
				} 	
			}
			fprintf(file, "\n");
			printf("\n");
			fclose(file);
			n_dati=0;
			i=0;
			if(l_resto!=0) //c'è stato resto nel ciclo precedente
			{
				for(j=0; j<6-l_resto; j++) 
				{
					resto[l_resto+j]=buf[j]; //aggiunge al resto i primi byte del nuovo ciclo
				}
				if(resto[0]==0xaa && resto[1]==0xaa) 
				{
					val_hum_int[0]=(unsigned int)resto[2]<<8|resto[3];
					val_temp_int[0]=(unsigned int)resto[4]<<8|resto[5];
					val_temp[0]=decode_temperature(val_temp_int[0]);
					val_hum[0]=decode_humidity(val_hum_int[0], val_temp[0]);
					n_dati=1;
					i=6-l_resto; //se ho ricostruito un dato usando i primi bytes del nuovo buf, non devo ripassare su quei dati nella successiva decodifica
				}
			}
			l_resto=0; //azzero resto perché nel prossimo ciclo si costruisce quello nuovo
 			while(i<n)
			{
				if(n_dati>=20) //protezione dalla segmentazione
				{
					printf("dati maggiori della memoria allocata!\n");
					break;
				}
				if(i<n-6) //perché un frame è composto da 6 byte
				{
					if(buf[i]==0xaa && buf[i+1]==0xaa)
					{
						val_hum_int[n_dati]=(unsigned int)buf[i+2]<<8|buf[i+3];
						val_temp_int[n_dati]=(unsigned int)buf[i+4]<<8|buf[i+5];
						val_temp[n_dati]=decode_temperature(val_temp_int[n_dati]);
						val_hum[n_dati]=decode_humidity(val_hum_int[n_dati], val_temp[n_dati]);
						n_dati++;
						i+=5; //questo serve per non passare di nuovo sui dati già decodificati
					} 
				}
				else //se abbiamo meno di sei bytes allora questi fanno parte del resto
				{
					resto[l_resto]=buf[i]; //questo è il resto che puo essere solo negli ultimi 5
					l_resto++;
				}
				i++;
			}
			printf("Resto:\n");
				for(j=0; j<l_resto; j++)
				{
					printf("%X ", resto[j]);
				}
				printf("\n");
			if(n_dati>0)
			{
				delta_t=(t_current-t_prec_data)/n_dati; //questo ci serve per conoscere il passo vero dell'acquisizione, delta t ora è in microsecondi
				printf("Delta_t=%lld (us)\n", delta_t);
				for(i=0; i<n_dati; i++)
				{
					t_flags[i]=t_prec_data+i*delta_t;
				} 
				file=fopen(NameF, "a");
				fprintf(file, "cnt=%d\n", cnt);
				for(i=0; i<n_dati; i++)
				{
					fprintf(file, "%d  %d  %d  %d  %.3f  %d  %.4f  %d  %.4f\n", cnt, ty, tmon, tday, (float)t_flags[i]/1000000, val_hum_int[i], val_hum[i], val_temp_int[i], val_temp[i]); //t_flags /mille se vogliamo i millisecondi 
					printf("%d  %d  %d  %d  %.3f  %d  %.4f  %d  %.4f\n", cnt, ty, tmon, tday, (float)t_flags[i]/1000000, val_hum_int[i], val_hum[i], val_temp_int[i], val_temp[i]); //t_flags /mille se vogliamo i millisecondi
				} 
		 		fclose(file);
			}
		} //chiude if (n>0) 


		#ifdef _WIN32 //il tempo per windows millisecondi 
 		Sleep(sleep_ms);  
		#else  //il tempo per linux in microsecondi
 		usleep(sleep_ms*1000); 
		#endif 
		printf("\n\n");
 	} //chiude while
	return(0); 
} //chiude main



//FUNZIONE PER DECODIFICARE LA TEMPERATURA
double decode_temperature(unsigned int rbuf) 
{
  	//rbuf valore dei byte di temperatura in intero 
  	double d1=-39.7;
  	double d2=0.01;
  	return(d1+d2*rbuf);
}

//FUNZIONE PER DECODIFICARE L'UMIDITA' DOPO AVER DECODIFICATO LA TEMPERATURA
double decode_humidity(unsigned int rbuf, double temperature_ref)
{
	//rbuf valore dei byte di umidità in intero, temperature_ref è la temperatura decodificata
  	double c1=-2.0468;
 	double c2=0.0367;
  	double c3=-1.5955E-6;
  	double t1=0.01;
  	double t2=0.00008;
  	double UR_linear;
  	UR_linear=c1+c2*rbuf+c3*rbuf*rbuf;
  	return((temperature_ref-25)*(t1+t2*rbuf)+UR_linear);
}

//FUNZIONE PER AVERE IL TEMPO IN MICROSECONDI
//questa funzione restituisce il tempo di microsecondi trascorsi dalla mezzanotte 
unsigned long long time_musec() //unsigned long long altrimenti dopo un'ora va in crisi
{
	unsigned long long sec_00;
	time_t t_t;
	struct tm gm;
	struct timeval t; //la struttura timeval contiene due interi a 32 bit senza segno uno per i secondi e uno per i microsecondi
	gettimeofday(&t, NULL); //acquisisce il tempo nel momento in cui viene chiamata
	t_t = (time_t)t.tv_sec;
	gm = *gmtime(&t_t);
	sec_00=gm.tm_hour*3600+gm.tm_min*60+gm.tm_sec;
	return sec_00*(unsigned long long)1000000+t.tv_usec; 
}
