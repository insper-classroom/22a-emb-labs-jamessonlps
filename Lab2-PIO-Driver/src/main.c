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

#define _PIO_DEFAULT             (0u << 0) /*  Default pin configuration (no attribute). */
#define _PIO_PULLUP              (1u << 0) /*  The internal pin pull-up is active. */
#define _PIO_DEGLITCH            (1u << 1) /*  The internal glitch filter is active. */
#define _PIO_DEBOUNCE            (1u << 3) /*  The internal debouncing filter is active. */


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
void _pio_set(Pio *p_pio, const uint32_t ul_mask);
void _pio_clear(Pio *p_pio, const uint32_t ul_mask);
void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable);
void _pio_set_input(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_attribute);
void _pio_set_output(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_default_level, const uint32_t ul_multidrive_enable, const uint32_t ul_pull_up_enable);

/************************************************************************/
/* interruptions                                                         */
/************************************************************************/

/************************************************************************/
/* functions                                                              */
/************************************************************************/

void piscar(Pio* pio, uint32_t ul_mask, int interval_time) {
	_pio_clear(pio, ul_mask);
	delay_ms(interval_time);
	_pio_set(pio, ul_mask);
	delay_ms(interval_time);
}

/**
 * \brief Set a high output level on all the PIOs defined in ul_mask.
 * This has no immediate effects on PIOs that are not output, but the PIO
 * controller will save the value if they are changed to outputs.
 * 
 * \param p_pio
 * \param ul_mask
 * 
 * \return void
 */
void _pio_set(Pio *p_pio, const uint32_t ul_mask) {
	p_pio -> PIO_SODR = ul_mask;
}

/**
 * \brief Set a low output level on all the PIOs defined in ul_mask.
 * This has no immediate effects on PIOs that are not output, but the PIO
 * controller will save the value if they are changed to outputs.
 * 
 * \param p_pio Pointer to a PIO instance
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 * 
 * \return void
 */
void _pio_clear(Pio *p_pio, const uint32_t ul_mask) {
	p_pio -> PIO_CODR = ul_mask;
}

/**
 * \brief Configure PIO internal pull-up.
 * 
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask bit mask of one or more pin(s) to configure.
 * \param ul_pull_up_enable Indicates if the pin(s) internal pull-up shall be configured.
 * 
 * \return void
 */
void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_pull_up_enable) {
	if (ul_pull_up_enable) {
		p_pio -> PIO_PUER = ul_mask;	
	}
	else {
		p_pio -> PIO_PUDR = ul_mask;
	}
}

/**
 * \brief Configure one or more pin(s) or a PIO controller as inputs.
 * Optionally, the corresponding internal pull-up(s) and glitch filter(s) can
 * be enabled.
 * 
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask bit mask indicating witch pin(s) to configure as input(s).
 * \param ul_attribute PIO attribute(s).
 * 
 * \return void
 */
void _pio_set_input(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_attribute) {
	// Qual a diferença entre configurar PIO_ODR e PIO_CODR ??
	p_pio -> PIO_CODR = ul_mask;
	
	// Ativa pull_up para o(s) pino(s) solicitado(s)
	if (ul_attribute & (1u << 0)) {
		_pio_pull_up(p_pio, ul_mask, ul_attribute);
	}

	// Ativa debounce para o(s) pino(s) solicitado(s)
	if (ul_attribute & (1u << 3)) {
		p_pio -> PIO_IFSCER = ul_mask;
	}
}

/**
 * \brief Configure one or more pin(s) of a PIO controller as outputs, with
 * the given default value. Optionally, the multi-drive feature can be enabled
 * on the pin(s).
 * 
 * \param p_pio Pionter to a PIO instance
 * \param ul_mask Bit mask indicating which pin(s) to configure.
 * \param ul_default_level Default level on the pin(s).
 * \param ul_multidrive_enable Indicates if the pin(s) shall be configured open-drain.
 * \param ul_pull_up_enable Indicates if the pin shall have its pull-up activated.
 * 
 * \return void
 */
void _pio_set_output(Pio *p_pio, const uint32_t ul_mask, const uint32_t ul_default_level, const uint32_t ul_multidrive_enable, const uint32_t ul_pull_up_enable) {
	// Configura PIO para controlar o pino
	p_pio -> PIO_PER = ul_mask;
	
	// Configura pino para ser uma saída
	p_pio -> PIO_OER = ul_mask;
	
	// Configura saída inicial do pino
	_pio_set(p_pio, ul_mask & ul_default_level);
	
	// Configura multidrive dos pinos
	p_pio -> PIO_MDER = ul_mask & ul_multidrive_enable;
	
	// Ativa pull-up para o(s) pino(s) selecionado(s)
	if (ul_pull_up_enable) {
		_pio_pull_up(p_pio, ul_mask, ul_pull_up_enable);
	}
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
	_pio_set_input(BUT_PIO, BUT_PIO_ID_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
	_pio_set_input(BUT1_PIO, BUT1_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
	_pio_set_input(BUT2_PIO, BUT2_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
	_pio_set_input(BUT3_PIO, BUT3_PIO_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE);
	
	// ativa pull-up para que o valor padrao do pino seja o energizado ???????
	//_pio_pull_up(BUT_PIO, BUT_PIO_ID_MASK, 1);
	//_pio_pull_up(BUT1_PIO, BUT1_PIO_IDX_MASK, 1);
	//_pio_pull_up(BUT2_PIO, BUT2_PIO_IDX_MASK, 1);
	//_pio_pull_up(BUT3_PIO, BUT3_PIO_IDX_MASK, 1);
	
	// Inicializa PIOs (valor 0, sem multidrive e sem resist. pull-up)
	_pio_set_output(LED_PIO, LED_PIO_IDX_MASK, 0, 0, 0);
	_pio_set_output(LED1_PIO, LED1_PIO_ID_MASK, 0, 0, 0);
	_pio_set_output(LED2_PIO, LED2_PIO_ID_MASK, 0, 0, 0);
	_pio_set_output(LED3_PIO, LED3_PIO_ID_MASK, 0, 0, 0);
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
