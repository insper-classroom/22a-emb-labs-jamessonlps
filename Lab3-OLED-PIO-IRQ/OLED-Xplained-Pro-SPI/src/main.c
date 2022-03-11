#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

/* LED da placa principal */
#define LED_PIO           PIOC				 // Periférico que controla o LED
#define LED_PIO_ID        ID_PIOC			 // ID do periférico PIO
#define LED_PIO_IDX       8					 // ID do led no PIO
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX) // Máscara para controle do LED

/* LED 1 da placa OLED */
#define LED1_PIO          PIOA
#define LED1_PIO_ID       ID_PIOA
#define LED1_PIO_IDX      0
#define LED1_PIO_ID_MASK  (1 << LED1_PIO_IDX)

/* Botão 1 da placa OLED */
#define BUT1_PIO          PIOD
#define BUT1_PIO_ID       ID_PIOD
#define BUT1_PIO_IDX      28
#define BUT1_PIO_IDX_MASK (1 << BUT1_PIO_IDX)

/* Botão 2 da placa OLED */
#define BUT2_PIO          PIOC
#define BUT2_PIO_ID       ID_PIOC
#define BUT2_PIO_IDX      31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)

/* Botão 3 da placa OLED */
#define BUT3_PIO          PIOA
#define BUT3_PIO_ID       ID_PIOA
#define BUT3_PIO_IDX      19
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX)

volatile char but1_flag;
volatile char but2_flag;
volatile char but3_flag;
int freq = 100;
volatile char str[128];

void but1_callback(void);
void but2_callback(void);
void but3_callback(void);
void pisca_led1(int n, int i);
void io_init(void);
void display_oled(freq);
void desliga_led(void);

void but1_callback(void) {
	but1_flag = 1;
}

void but2_callback(void) {
	but2_flag = 1;
}

void but3_callback(void) {
	but3_flag = 1;
}


/**
 * \brief Função responsável por piscar o LED na frequência pedida
 * e atualizar a barra de progresso no display
 * 
 * \param n Número de vezes que o LED piscará
 * \param f Frequência que o LED piscará
 * 
 * \return void
 */
void pisca_led1(int n, int f) {
	// Calcula variáveis de tempo e frequência (em Hz)
	double time_pisca_s = (double)n / f;
	int time_pisca_ms = time_pisca_s*1000;
	display_oled(freq);
	// Inicializa barra de progresso
	gfx_mono_draw_horizontal_line(0, 10, 4*(n-1), GFX_PIXEL_SET);
	for (int i = 0; i < n; i++) {
		// Atualiza barra de progresso
		gfx_mono_draw_horizontal_line(0, 10, 4*i, GFX_PIXEL_CLR);
		pio_clear(LED1_PIO, LED1_PIO_ID_MASK);
		delay_ms(time_pisca_ms);
		pio_set(LED1_PIO, LED1_PIO_ID_MASK);
		delay_ms(time_pisca_ms);
		if (but2_flag) {
			pio_set(LED1_PIO, LED1_PIO_ID_MASK);
			gfx_mono_draw_horizontal_line(0, 10, 4*(n-1), GFX_PIXEL_CLR);
			delay_ms(30);
			break;
		}
	}
}

void desliga_led(void) {
	pio_set(LED1_PIO, LED1_PIO_ID_MASK);
	delay_ms(30);
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

	// Clock do periférico PIO que controla o botão do OLED
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);
	
	// PIO lida com pino do botão como entrada com pull-up e debounce
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_configure(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT1_PIO, BUT1_PIO_IDX_MASK, 60);
	pio_set_debounce_filter(BUT2_PIO, BUT2_PIO_IDX_MASK, 60);
	pio_set_debounce_filter(BUT3_PIO, BUT3_PIO_IDX_MASK, 60);
	
	// Configura interrupção no pino do botão e associa a ele o callback
	pio_handler_set(
		BUT1_PIO,
		BUT1_PIO_ID,
		BUT1_PIO_IDX_MASK,
		PIO_IT_FALL_EDGE,
		but1_callback
	);
	pio_handler_set(
		BUT2_PIO,
		BUT2_PIO_ID,
		BUT2_PIO_IDX_MASK,
		PIO_IT_FALL_EDGE,
		but2_callback
	);
	pio_handler_set(
		BUT3_PIO,
		BUT3_PIO_ID,
		BUT3_PIO_IDX_MASK,
		PIO_IT_FALL_EDGE,
		but3_callback
	);
					
	// Ativa a interrupção e limpa a primeira IRQ gerada na ativação
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	pio_enable_interrupt(BUT3_PIO, BUT3_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	pio_get_interrupt_status(BUT2_PIO);
	pio_get_interrupt_status(BUT3_PIO);
	
	// Configura NVIC para receber interrupções do PIO do botão com prioridade 4
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4);
	NVIC_SetPriority(BUT2_PIO_ID, 4);
	NVIC_SetPriority(BUT3_PIO_ID, 4);
}

int main (void)
{
	board_init();
	
	// Inicia o clock
	sysclk_init();

	// Desativa watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;

	// Configura botão com interrupção
	io_init();

	delay_init();

  // Init OLED
	gfx_mono_ssd1306_init();
  
	//gfx_mono_draw_filled_circle(20, 16, 16, GFX_PIXEL_SET, GFX_WHOLE);
	//display_oled(freq);

  /* Insert application code here, after the board has been initialized. */
	while(1) {
		if (but1_flag || but2_flag || but3_flag) {
			for (int i = 0; i < 99999999; i++) {
				// Se o botão 2 não está pressionado, trata dos casos dos outros botões
				if (pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK)) {
					// O looping fica verificando o status da saída do pino do botão 1
					// Se o pino ainda estiver pressionado e o contador i tiver ultrapassado
					// certo limite, então ele diminui a frequência.
					if ((!pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK) && i >= 3000000) || !pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK)) {
						freq -= 50;
						if (freq <= 0) {
							freq = 50;
						}
						pisca_led1(30, freq);
						break;
					}
					// Quando o botão sinaliza que não está mais pressionado dentro do limite
					// determinado, ele aumenta a frequência.
					else if ((pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK) && i < 3000000)) {
						freq += 50;
						pisca_led1(30, freq);
						but1_flag = 0;
						break;
					}
				}
				// Se o botão 2 foi pressionado, desliga o pisca pisca do LED.
				else {
					desliga_led();
					break;
				}
			}
			but1_flag = 0;
			but2_flag = 0;
			but3_flag = 0;
		}
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}
