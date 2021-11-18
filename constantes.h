#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


#define FLAG 0x7E ////flag de inicio e fim

#define A_ER 0x03 // Campo de Endereço (A) de commandos do Emissor, resposta do Receptor
#define A_RE 0x01 // Campo de Endereço (A) de commandos do Receptor, resposta do Emissor

#define C_UA 0x07 //Campo de Controlo - UA (Unnumbered Acknowledgement)
#define C_SET 0x03 //Campo de Controlo - SET (set up)
#define C_RR 0x05
#define C_REJ 0x01
#define C_DISC 0x0B //Campo de Controlo - DISC (disconnect)

#define BCC1_SET A_ER ^ C_SET
#define BCC1_UA A_ER ^ C_UA

#define TRANSMITTER 0
#define RECEIVER 1


