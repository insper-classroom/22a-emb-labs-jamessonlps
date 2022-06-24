/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

#define LED_PIO	          PIOC				 // periferico que controla o LED
#define LED_PIO_ID		  ID_PIOC			 // ID do periferico PIOC (controla LED)
#define LED_PIO_IDX		  8					 // ID do LED no PIO
#define LED_PIO_IDX_MASK  (1 << LED_PIO_IDX) // mascara para controlarmos o LED

//LED 1 da placa OLED
#define LED1_PIO          PIOA
#define LED1_PIO_ID       ID_PIOA
#define LED1_PIO_IDX      0
#define LED1_PIO_ID_MASK  (1 << LED1_PIO_IDX)

// LED 2 da placa OLED
#define LED2_PIO          PIOC
#define LED2_PIO_ID       ID_PIOC
#define LED2_PIO_IDX      30
#define LED2_PIO_ID_MASK  (1 << LED2_PIO_IDX)

// LED 3 da placa OLED
#define LED3_PIO          PIOB
#define LED3_PIO_ID       ID_PIOB
#define LED3_PIO_IDX      2
#define LED3_PIO_ID_MASK  (1 << LED3_PIO_IDX)

// BOTAO
#define BUT_PIO           PIOA
#define BUT_PIO_ID        ID_PIOA
#define BUT_PIO_IDX       11
#define BUT_PIO_ID_MASK   (1u << BUT_PIO_IDX)  // mascara para controle do botao

// BOTAO LED1
#define BUT1_PIO          PIOD
#define BUT1_PIO_ID       ID_PIOD
#define BUT1_PIO_IDX      28
#define BUT1_PIO_IDX_MASK (1 << BUT1_PIO_IDX)

// BOTÃO LED2
#define BUT2_PIO          PIOC
#define BUT2_PIO_ID       ID_PIOC
#define BUT2_PIO_IDX      31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)

// BOTÃO LED3
#define BUT3_PIO          PIOA
#define BUT3_PIO_ID       ID_PIOA
#define BUT3_PIO_IDX      19
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX)

/************************************************************************/
/* constants                                                            */
/************************************************************************/

#define TIME_LEDS_DEFAULT 50   // tempo de piscar padrão (50ms)
#define TIME_LEDS_PRESSED 500  // tempo de piscar ao pressionar botão
#define TIMES_FLASHING    5    // numero de vezes para piscar

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

void init(void);
void piscar(Pio* pio, uint32_t ul_mask, int interval_time);

/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void piscar(Pio* pio, uint32_t ul_mask, int interval_time) {
	pio_clear(pio, ul_mask);
	delay_ms(interval_time);
	pio_set(pio, ul_mask);
	delay_ms(interval_time);
}

// Função de inicialização do uC
void init(void)
{
	// Inicializa o board clock
	sysclk_init();
	
	// Desativa WatchDog Timer
	WDT -> WDT_MR = WDT_MR_WDDIS;
	
	// Ativa o PIOs
	pmc_enable_periph_clk(LED_PIO_ID); // controle do LED
	pmc_enable_periph_clk(LED1_PIO_ID);
	pmc_enable_periph_clk(LED3_PIO_ID);
	pmc_enable_periph_clk(BUT_PIO_ID); // controle do botao da placa
	pmc_enable_periph_clk(BUT1_PIO_ID);
	
	// configura pino ligado ao botao como entrada com um pull-up
	pio_set_input(BUT_PIO, BUT_PIO_ID_MASK, PIO_DEFAULT);
	pio_set_input(BUT1_PIO, BUT1_PIO_IDX_MASK, PIO_DEFAULT);
	pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, PIO_DEFAULT);
	pio_set_input(BUT3_PIO, BUT3_PIO_IDX_MASK, PIO_DEFAULT);
	
	// ativa pull-up para que o valor padrao do pino seja o energizado ???????
	pio_pull_up(BUT_PIO, BUT_PIO_ID_MASK, 1);
	pio_pull_up(BUT1_PIO, BUT1_PIO_IDX_MASK, 1);
	pio_pull_up(BUT2_PIO, BUT2_PIO_IDX_MASK, 1);
	pio_pull_up(BUT3_PIO, BUT3_PIO_IDX_MASK, 1);
	
	// Inicializa PIOs (valor 0, sem multidrive e sem resist. pull-up)
	pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
	pio_set_output(LED1_PIO, LED1_PIO_ID_MASK, 0, 0, 0);
	pio_set_output(LED2_PIO, LED2_PIO_ID_MASK, 0, 0, 0);
	pio_set_output(LED3_PIO, LED3_PIO_ID_MASK, 0, 0, 0);
}

/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void)
{
  init();

  // super loop
  // aplicacoes embarcadas não devem sair do while(1).
  while (1)
  {
	  piscar(LED1_PIO, LED1_PIO_ID_MASK, TIME_LEDS_DEFAULT);
	  piscar(LED2_PIO, LED2_PIO_ID_MASK, TIME_LEDS_DEFAULT);
	  piscar(LED3_PIO, LED3_PIO_ID_MASK, TIME_LEDS_DEFAULT);
	  piscar(LED_PIO, LED_PIO_IDX_MASK, TIME_LEDS_DEFAULT);
	  
	  if (!pio_get(BUT_PIO, PIO_INPUT, BUT_PIO_ID_MASK)) {
		  for (int i = 0; i < TIMES_FLASHING; i++) {
			  piscar(LED_PIO, LED_PIO_IDX_MASK, TIME_LEDS_PRESSED);
		  }
	  }
	  
	  if (!pio_get(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK)) {
		  // LED 1 pisca mais lento quando o botao 1 e pressionado
		  for (int i = 0; i < TIMES_FLASHING; i++) {
			  piscar(LED1_PIO, LED1_PIO_ID_MASK, TIME_LEDS_PRESSED);
		  }
	  }
	  
	  if (!pio_get(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK)) {
		  // LED 2 pisca mais lento quando o botao 2 e pressionado
		  for (int i = 0; i < TIMES_FLASHING; i++) {
			  piscar(LED2_PIO, LED2_PIO_ID_MASK, TIME_LEDS_PRESSED);
		  }
	  }
	  	  
	  if (!pio_get(BUT3_PIO, PIO_INPUT, BUT3_PIO_IDX_MASK)) {
		  // LED 3 pisca mais lento quando o botao 3 e pressionado
		  for (int i = 0; i < TIMES_FLASHING; i++) {
			  piscar(LED3_PIO, LED3_PIO_ID_MASK, TIME_LEDS_PRESSED);
		  }
	  }
  }
  return 0;
}
