#include "main.h"
#include "uart.h"
#include "console.h"
#include "isr.h"  

/*
 * Define ECHO_ZZZ to have a periodic reminder that this code is polling
 * the UART, actively. This means the processor is running continuously.
 * Polling is of course not the way to go, the processor should halt in
 * a low-power state and wake-up only to handle an interrupt from the UART.
 * But this would require setting up interrupts...
 */
#define ECHO_ZZZ

static int test_interruptions = 0; 
// Handler simple pour tester
void test_handler(uint32_t irq, void *cookie) {
    test_interruptions++;
    kprintf("*** IRQ DETECTEE ! Total: %d ***\n", test_interruptions);
    unsigned char c;
    uart_receive(UART0, &c);
}

void panic() {
  while (1)
	  ;
}

// Callback pour les lignes saisies - ne fait rien pour l'instant
void line_entered(char* line) {
    // Pour l'instant, on fait rien , on interperète pas et on traite pas ce qui est tapé 
    // on les affiches seulement dans la console
}

// faire une boucle de 1sec
void wait(){
	for (int i=0; i<1000000; i++){
	}
}

/**
 * This is the C entry point, upcalled once the hardware has been setup properly
 * in assembly language, see the startup.s file.
 */
void _start() {

  uart_send_string(UART0, "\nFor information:\n");
  uart_send_string(UART0, "  - Quit with \"C-a c\" to get to the QEMU console.\n");
  uart_send_string(UART0, "  - Then type in \"quit\" to stop QEMU.\n");

  uart_send_string(UART0, "\nHello world!\n");
 
  // ===== TEST INTERRUPTION SIMPLE =====
  kprintf("Test interruption: setup...\n");
  _irqs_setup();           
  irqs_setup();          
  uart_enable_rx_interrupt(UART0);  
  irq_enable(UART0_IRQ, test_handler, NULL);  // Register handler
  irqs_enable();        

 
  // Initialiser la console 
  console_init(line_entered);
  uart_send_string(UART0, "Console ready!\n");

  // Boucle principale avec la console + test interruption
  int compteur_affichage = 0;
  while (1) {
    wfi();  // Attendre une interruption (ex: UART) 
  }
}