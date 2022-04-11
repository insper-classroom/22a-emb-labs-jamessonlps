#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/* Botao 1 da placa OLED */
#define BUT1_PIO          PIOD
#define BUT1_PIO_ID       ID_PIOD
#define BUT1_PIO_IDX      28
#define BUT1_PIO_IDX_MASK (1 << BUT1_PIO_IDX)

/* Botao 2 da placa OLED */
#define BUT2_PIO          PIOC
#define BUT2_PIO_ID       ID_PIOC
#define BUT2_PIO_IDX      31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)

/* Pino Y (verde) => Trigger */
#define PINY_TRIGGER_PIO          PIOD
#define PINY_TRIGGER_PIO_ID       ID_PIOD
#define PINY_TRIGGER_PIO_IDX      30
#define PINY_TRIGGER_PIO_IDX_MASK (1u << PINY_TRIGGER_PIO_IDX)

/* Pino X () => Echo */
#define PINX_ECHO_PIO             PIOC
#define PINX_ECHO_PIO_ID          ID_PIOC
#define PINX_ECHO_PIO_IDX         13
#define PINX_ECHO_PIO_IDX_MASK    (1u << PINX_ECHO_PIO_IDX)


/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);



/************************************************************************/
/* CONSTANTS                                                            */
/************************************************************************/

// Frequencia correspondente ao tempo para determinar a menor distancia
// v = x / t => t = (2 * 0.02) / 340 => t = 1/8500
volatile float freqPrescale = 8500;

// Tempo devolvido pelo RTT
volatile float time_counter = 0;

// Flag que sinaliza um novo valor de distancia calculada
volatile char draw_flag = 0;



/************************************************************************/
/* INTERRUPTION FLAGS                                                   */
/************************************************************************/
volatile char but1_flag;
volatile char but2_flag;
volatile char echo_flag = 0;
volatile char rtt_alarm_flag;



/************************************************************************/
/* CALLBACK FUNCTIONS                                                   */
/************************************************************************/
void but1_callback(void) {
	but1_flag = 1;
}


void but2_callback(void) {
	but2_flag = 1;
}


void echo_callback(void) {
	if (echo_flag) {
		// Quando echo_flag = 1, o botao foi pressionado, entao
		// inicia-se a contagem do tempo para o echo
		echo_flag = 1;
		RTT_init(freqPrescale, (int) freqPrescale * (8/340), RTT_MR_ALMIEN);
	}
	else {
		// Caso contrário, recebe o valor atual do contador
		// para calcular a distancia
		echo_flag = 0;
		time_counter = rtt_read_timer_value(RTT);
	}
}



/************************************************************************/
/* GENERAL PURPOSE FUNCTIONS                                            */
/************************************************************************/

uint32_t get_echo_pin() {
	return pio_get(PINX_ECHO_PIO, PIO_INPUT, PINX_ECHO_PIO_IDX_MASK);
}


void calculate_distance(float delta_t) {
	float dist = (float) 340 * delta_t * 100.0 / (2 * freqPrescale);
	char dist_string[10];
	
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
	
	if (rtt_alarm_flag || (dist >= 400)) {
		gfx_mono_draw_string("ERROOOU", 0,0, &sysfont);
		rtt_alarm_flag = 0;
		delay_ms(100);
		gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
	}
	else {
		sprintf(dist_string, "%2.2f cm", time_counter);
		gfx_mono_draw_string(dist_string, 0, 0, &sysfont);	
	}
}



/************************************************************************/
/* REAL TIME HANDLERS                                                   */
/************************************************************************/
void RTT_Handler(void) {
	uint32_t ul_status;

	/* RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* Interrupcao por alarme */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		rtt_alarm_flag = 1;
	}

	/* Interrupcao por contagem => */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		// por enquanto nao faz nada
	}
}



/************************************************************************/
/* REAL TIME INITS                                                      */
/************************************************************************/
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {
	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);

	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);

	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* Configura NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Ativa interrupcao do RTT */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN)) {
		rtt_enable_interrupt(RTT, rttIRQSource);
	}
	else {
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	}
}



/************************************************************************/
/* INIT FUNCTIONS                                                       */
/************************************************************************/
void pio_init(
Pio *p_pio, 
pio_type_t ul_type, 
uint32_t ul_id, 
uint32_t ul_mask, 
uint32_t ul_attribute, 
uint32_t ul_attr_irq, 
void(*p_handler)(uint32_t, uint32_t),
uint32_t priority) 
{
	pmc_enable_periph_clk(ul_id);
	pio_configure(p_pio, ul_type, ul_mask, ul_attribute);
	pio_set_debounce_filter(p_pio, ul_mask, 60);
	
	pio_handler_set(
		p_pio,
		ul_id,
		ul_mask,
		ul_attr_irq,
		p_handler
	);
	
	pio_enable_interrupt(p_pio, ul_mask);
	pio_get_interrupt_status(p_pio);
	NVIC_EnableIRQ(ul_id);
	NVIC_SetPriority(ul_id, priority);
}



void io_init(void) {
	// Inicializa Echo
	pio_init(PINX_ECHO_PIO, PIO_INPUT, PINX_ECHO_PIO_ID, PINX_ECHO_PIO_IDX_MASK, PIO_DEBOUNCE, PIO_IT_EDGE, echo_callback, 1);

	// Inicializa Trigger
	pmc_enable_periph_clk(PINY_TRIGGER_PIO_ID);
	pio_set_output(PINY_TRIGGER_PIO, PINY_TRIGGER_PIO_IDX_MASK, 0, 0, 0);
	
	// Inicializa botao 1
	pio_init(BUT1_PIO, PIO_INPUT, BUT1_PIO_ID, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE, PIO_IT_FALL_EDGE, but1_callback, 4);
}



/************************************************************************/
/* LOGICAL FUNCTIONS                                                    */
/************************************************************************/
void pulse(void) {
	pio_set(PINY_TRIGGER_PIO, PINY_TRIGGER_PIO_IDX_MASK);
	delay_us(10);
	pio_clear(PINY_TRIGGER_PIO, PINY_TRIGGER_PIO_IDX_MASK);
}





/************************************************************************/
/* MAIN LOOPING                                                         */
/************************************************************************/
int main (void)
{
	board_init();
	io_init();
	sysclk_init();
	delay_init();
	
	WDT->WDT_MR = WDT_MR_WDDIS;
	

	// Inicializa o OLED
	gfx_mono_ssd1306_init();
  
  
	gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_QUADRANT0);
	gfx_mono_draw_string("james", 50,16, &sysfont);

	while(1) {
		if (but1_flag) {
			// Gera um pulso para medir distancia
			pulse();
			
			calculate_distance(time_counter);
			but1_flag = 0;
		}
		
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
