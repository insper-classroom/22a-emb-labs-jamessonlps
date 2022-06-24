#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* DEFINES                                                              */
/************************************************************************/
#define PINX_ECHO PIOA
#define PINX_ECHO_PIO_ID ID_PIOA
#define PINX_ECHO_PIO_IDX 4
#define PINX_ECHO_PIO_IDX_MASK (1 << PINX_ECHO_PIO_IDX)

#define PINY_TRIGGER_PIO PIOC
#define PINY_TRIGGER_PIO_ID ID_PIOC
#define PINY_TRIGGER_PIO_IDX 13
#define PINY_TRIGGER_PIO_IDX_MASK (1 << PINY_TRIGGER_PIO_IDX)

// Botao 1 da placa OLED
#define BUT1_PIO PIOD
#define BUT1_PIO_ID ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_PIO_IDX_MASK (1 << BUT1_PIO_IDX)




/************************************************************************/
/* FLAGS                                                                */
/************************************************************************/
volatile char but_flag;
volatile char echo_flag;
volatile char display_flag;
volatile char timeout_flag;




/************************************************************************/
/* CONSTANTS AND VARIABLES                                              */
/************************************************************************/
volatile float freqPrescale = 1 / (0.000058 * 2);
volatile float timer_count = 0;
volatile float memory[6] = {0, 0, 0, 0, 0, 0}; // historico de medicoes




/************************************************************************/
/* PROTOTYPES                                                           */
/************************************************************************/
void io_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);




/************************************************************************/
/* CALLBACKS                                                            */
/************************************************************************/
void but_callback(void)
{
	but_flag = 1;
}

void echo_callback(void)
{
	if (!echo_flag)
	{
		// Quando echo_flag = 0, o botao foi pressionado, entao
		// inicia-se a contagem do tempo para o echo
		tc_stop(TC0, 0);
		RTT_init(freqPrescale, 0, 0);
	}
	else
	{
		// Caso contrário, recebe o valor atual do contador
		// para calcular a distancia
		timer_count = rtt_read_timer_value(RTT);
		display_flag = 1;
	}
	echo_flag = !echo_flag;
}




/************************************************************************/
/* HANDLERS                                                             */
/************************************************************************/

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);
	
	/* Interrupcao por contagem */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
	}
}

void TC0_Handler(void)
{
	volatile uint32_t status = tc_get_status(TC0, 0);
	timeout_flag = 1;
}




/************************************************************************/
/* TIMERS INITS                                                         */
/************************************************************************/

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource)
{

	uint16_t pllPreScale = (int)(((float)32768) / freqPrescale);

	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);

	if (rttIRQSource & RTT_MR_ALMIEN)
	{
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT))
			;
		rtt_write_alarm_time(RTT, IrqNPulses + ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
		rtt_enable_interrupt(RTT, rttIRQSource);
	else
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
}

void TC_init(Tc *TC, int ID_TC, int TC_CHANNEL, int freq)
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupï¿½cï¿½o no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type)ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}




/************************************************************************/
/* FUNCTIONS                                                            */
/************************************************************************/

void pulse(void) {
	pio_set(PINY_TRIGGER_PIO, PINY_TRIGGER_PIO_IDX_MASK);
	delay_us(10);
	pio_clear(PINY_TRIGGER_PIO, PINY_TRIGGER_PIO_IDX_MASK);
}

void update_memo(float measurements[], float new_value, int n)
{
	for (int i = n - 1; i > 0; i--)
	{
		measurements[i] = measurements[i - 1];
	}
	measurements[0] = new_value;
}

void clear_display(void)
{
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
}

void draw_display_distance(float distance)
{
	clear_display();
	char str[10];
	sprintf(str, "%2.2f", distance);
	gfx_mono_draw_string(str, 0, 0, &sysfont);
	gfx_mono_draw_string("cm", 8, 16, &sysfont);
}

void draw_display_error(void)
{
	clear_display();
	gfx_mono_draw_string("Erro!", 0, 8, &sysfont);
}

void draw_display_graphic(float measurements[], int n)
{
	int x = 64;
	int bar_width = 5;
	for (int i = 0; i < n; i++)
	{
		int n_pixels = measurements[i] * 32 / 400;
		if (n_pixels == 0)
		{
			gfx_mono_generic_draw_filled_rect(x, 0, bar_width, 32, GFX_PIXEL_SET);
			gfx_mono_generic_draw_filled_rect(x, 0, bar_width, 31, GFX_PIXEL_CLR);
		}
		else
		{
			gfx_mono_generic_draw_filled_rect(x, 0, bar_width, 32, GFX_PIXEL_SET);
			gfx_mono_generic_draw_filled_rect(x, 0, bar_width, 32 - n_pixels, GFX_PIXEL_CLR);
		}
		x += 8;
	}
}




/************************************************************************/
/* INITS                                                                */
/************************************************************************/

void io_init(void)
{
	// Configura Trigger
	pmc_enable_periph_clk(PINY_TRIGGER_PIO_ID);
	pio_configure(PINY_TRIGGER_PIO, PIO_OUTPUT_0, PINY_TRIGGER_PIO_IDX_MASK, PIO_DEFAULT);

	// Configura Echo
	pmc_enable_periph_clk(PINX_ECHO_PIO_ID);
	pio_set_input(PINX_ECHO, PINX_ECHO_PIO_IDX_MASK, PIO_DEFAULT);

	// Configura botao 1 da placa OLED
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(PINX_ECHO, BUT1_PIO_IDX_MASK, 60);

	pio_handler_set(PINX_ECHO,
		PINX_ECHO_PIO_ID,
		PINX_ECHO_PIO_IDX_MASK,
		PIO_IT_EDGE,
		echo_callback
	);

	pio_handler_set(BUT1_PIO,
		BUT1_PIO_ID,
		BUT1_PIO_IDX_MASK,
		PIO_IT_FALL_EDGE,
		but_callback
	);

	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);

	pio_enable_interrupt(PINX_ECHO, PINX_ECHO_PIO_IDX_MASK);
	pio_get_interrupt_status(PINX_ECHO);

	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4);

	NVIC_EnableIRQ(PINX_ECHO_PIO_ID);
	NVIC_SetPriority(PINX_ECHO_PIO_ID, 4);
}

int main(void)
{
	board_init();
	sysclk_init();
	delay_init();
	io_init();

	gfx_mono_ssd1306_init();

	int memo_size = 6;

	while (1)
	{
		if (but_flag)
		{
			pulse();
			// Um tc configurado para dar timeout se o retorno demorar
			TC_init(TC0, ID_TC0, 0, 40);
			tc_start(TC0, 0);
			but_flag = 0;
		}
		
		if (display_flag)
		{
			float dist = (float)(340 * timer_count * 100) / (2 * freqPrescale);
			if (dist > 400 || dist < 2)
			{
				draw_display_error();
			}
			else
			{
				update_memo(memory, dist, memo_size);
				draw_display_distance(dist);
				draw_display_graphic(memory, memo_size);
			}
			display_flag = 0;
		}

		if (timeout_flag)
		{
			draw_display_error();
			tc_stop(TC0, 0);
			timeout_flag = 0;
		}

		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
