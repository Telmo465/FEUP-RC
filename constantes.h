#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E ////flag de inicio e fim

#define A_ER 0x03 // Campo de Endereço (A) de commandos do Emissor, resposta do Receptor
#define A_RE 0x01 // Campo de Endereço (A) de commandos do Receptor, resposta do Emissor

#define C_I0 0x00
#define C_I1 0x40
#define C_UA 0x07 //Campo de Controlo - UA (Unnumbered Acknowledgement)
#define C_SET 0x03 //Campo de Controlo - SET (set up)
#define C_RR_0 0x05
#define C_RR_1 0x85
#define C_REJ_0 0x01
#define C_REJ_1 0x81
#define C_DISC 0x0B //Campo de Controlo - DISC (disconnect)


#define BCC1_SET A_ER ^ C_SET
#define BCC1_UA A_ER ^ C_UA
#define BCC1_I0 A_ER ^ C_I0
#define BCC1_I1 A_ER ^ C_I1
#define BCC1_RR_0 A_RE ^ C_RR_0
#define BCC1_RR_1 A_RE ^ C_RR_1
#define BCC1_REJ_0 A_RE ^ C_REJ_0
#define BCC1_REJ_1 A_RE ^ C_REJ_1
#define BCC_DISC A_ER ^ C_DISC

#define TRANSMITTER 0
#define RECEIVER 1

#define START_C 0x02
#define END_C 0x03

