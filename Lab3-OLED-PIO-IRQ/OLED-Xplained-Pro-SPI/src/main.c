#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/* LED da placa principal */
#define LED_PIO           PIOC				 // Perif�rico que controla o LED
#define LED_PIO_ID        ID_PIOC			 // ID do perif�rico PIO
#define LED_PIO_IDX       8					 // ID do led no PIO
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX) // M�scara para controle do LED

/* LED 1 da placa OLED */
#define LED1_PIO          PIOA
#define LED1_PIO_ID       ID_PIOA
#define LED1_PIO_IDX      0
#define LED1_PIO_ID_MASK  (1 << LED1_PIO_IDX)

/* Bot�o 1 da placa OLED */
#define BUT1_PIO          PIOD
#define BUT1_PIO_ID       ID_PIOD
#define BUT1_PIO_IDX      28
#define BUT1_PIO_IDX_MASK (1 << BUT1_PIO_IDX)

volatile char but1_flag;
int freq = 100;
volatile char str[128];

void but1_callback(void);
void pisca_led1(int n, int i);
void io_init(void);
void display_oled(freq);

void but1_callback(void) {
	if (pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK)) {
		// PINO == 1 --> Borda de subida
		} else {
		// PINO == 0 --> Borda de descida
	}
	but1_flag = 1;
}


void pisca_led1(int n, int t) {
	double dt = (double)n / t;
	int time_pisca = dt * 1000; // delay em ms
	display_oled(freq);
	for (int i = 0; i < n; i++) {
		pio_clear(LED1_PIO, LED1_PIO_ID_MASK);
		delay_ms(time_pisca);
		pio_set(LED1_PIO, LED1_PIO_ID_MASK);
		delay_ms(time_pisca);
	}
}

void display_oled(freq) {
	char freq_string[15];
	sprintf(freq_string, "f=%d Hz", freq);
	gfx_mono_draw_string(freq_string, 10, 16, &sysfont);
}

void io_init(void) {
	// Configura o LED
	pmc_enable_periph_clk(LED1_PIO_ID);
	pio_configure(LED1_PIO, PIO_OUTPUT_0, LED1_PIO_ID_MASK, PIO_DEFAULT);

	// Clock do perif�rico PIO que controla o bot�o do OLED
	pmc_enable_periph_clk(BUT1_PIO_ID);
	
	// PIO lida com pino do bot�o como entrada com pull-up e debounce
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO, BUT1_PIO_IDX_MASK, 60);
	
	// Configura interrup��o no pinto do bot�o e associa a ele o callback
	pio_handler_set(BUT1_PIO,
					BUT1_PIO_ID,
					BUT1_PIO_IDX_MASK,
					PIO_IT_FALL_EDGE,
					but1_callback);
					
	// Ativa a interrup��o e limpa a primeira IRQ gerada na ativa��o
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	
	// Configura NVIC para receber interrup��es do PIO do bot�o com prioridade 4
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4);
}

int main (void)
{
	board_init();
	
	// Inicia o clock
	sysclk_init();

	// Desativa watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;

	// Configura bot�o com interrup��o
	io_init();

	delay_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
	display_oled(freq);

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if (but1_flag) {
			for (int i = 0; i < 99999999; i++) {
				// O looping fica verificando o status da sa�da do pino do bot�o 1
				// Se o pino ainda estiver pressionado e o contador i tiver ultrapassado
				// certo limite, ent�o ele diminui a frequ�ncia.
				if (!pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK) && i >= 9999000) {
					freq -= 50;
					if (freq <= 0) {
						freq = 50;
					}
					pisca_led1(15, freq);
					break;
				}
				// Quando o bot�o sinaliza que n�o est� mais pressionado dentro do limite
				// determinado, ele aumenta a frequ�ncia.
				else if (pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK) && i < 9999000) {
					freq += 50;
					pisca_led1(15, freq);
					but1_flag = 0;
					break;
				}
			}
			but1_flag = 0;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
