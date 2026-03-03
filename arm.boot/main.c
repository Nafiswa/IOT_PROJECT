#include "main.h"
#include "uart.h"
#include "console.h"
#include "isr.h"

// Variables globales système
volatile uint32_t system_ticks = 0;
volatile uint32_t total_events = 0;
event_t event_buffer[EVENT_BUFFER_SIZE];
volatile uint8_t event_head = 0, event_tail = 0;

static uint32_t last_status_update = 0;
static uint8_t cursor_visible = 1;
static uint32_t last_cursor_blink = 0; 
static char input_line[80];
static uint8_t input_pos = 0;
static char history[3][80]; // Historique simple 3 commandes
static uint8_t history_count = 0, history_idx = 0;
static void (*line_callback)(char*) = 0;
static uint8_t escape_state = 0; // Pour traiter séquences échappement 
// Handler timer 1ms pour system tick
void system_tick_handler(uint32_t irq, void* cookie) {
    system_ticks++;
    mmio_write32((void*)TIMER01_BASE, TIMER_INTCLR, 1); // Clear timer interrupt
}

// Handler UART avec traitement événements
void uart_handler(uint32_t irq, void *cookie) {
    unsigned char c;
    while (uart_receive(UART0, &c)) {
        event_push(1, c); // Type 1 = UART RX
        total_events++;
    }
}

// Système d'événements minimal
void event_push(uint8_t type, uint8_t data) {
    event_buffer[event_head].type = type;
    event_buffer[event_head].data = data;
    event_head = (event_head + 1) % EVENT_BUFFER_SIZE;
    if (event_head == event_tail) event_tail = (event_tail + 1) % EVENT_BUFFER_SIZE;
}

uint8_t event_pop(event_t* event) {
    if (event_head == event_tail) return 0;
    *event = event_buffer[event_tail];
    event_tail = (event_tail + 1) % EVENT_BUFFER_SIZE;
    return 1;
}

// Timer 1ms init
void timer_init_1ms(void) {
    mmio_write32((void*)TIMER01_BASE, TIMER_LOAD, 1000); // 1ms @ 1MHz
    mmio_write32((void*)TIMER01_BASE, TIMER_CONTROL, 
                TIMER_CTRL_32BIT | TIMER_CTRL_PERIODIC | TIMER_CTRL_INTENABLE | TIMER_CTRL_ENABLE);
}

void panic() {
  while (1)
	  ;
}

// Callback pour les lignes saisies
void line_entered(char* line) {
    // Ajouter à l'historique
    if (history_count < 3) {
        for (int i = 0; i < 79 && line[i]; i++) history[history_count][i] = line[i];
        history[history_count][79] = '\0';
        history_count++;
    }
    kprintf("\r\nCmd: %s\r\n$ ", line);
    input_pos = 0;
    input_line[0] = '\0';
}

// Console update status line
void console_update_status(void) {
    if (system_ticks - last_status_update < 1000) return;
    last_status_update = system_ticks;
    
    uart_send(UART0, 27); uart_send(UART0, '['); uart_send(UART0, 's'); // Save cursor
    uart_send(UART0, 27); uart_send(UART0, '['); uart_send(UART0, '1'); uart_send(UART0, ';'); 
    uart_send(UART0, '1'); uart_send(UART0, 'H'); // Go to (1,1)
    
    kprintf("Time:%us Events:%u CPU:idle                                      ", 
            system_ticks/1000, total_events);
            
    uart_send(UART0, 27); uart_send(UART0, '['); uart_send(UART0, 'u'); // Restore cursor
}

void console_blink_cursor(void) {
    if (system_ticks - last_cursor_blink < 500) return;
    last_cursor_blink = system_ticks;
    cursor_visible = !cursor_visible;
    if (cursor_visible) cursor_show(); else cursor_hide();
}

void console_process_input(unsigned char c) {
    // Gestion séquences d'échappement (flèches)
    if (escape_state == 0 && c == 27) { // ESC
        escape_state = 1;
        return;
    }
    if (escape_state == 1 && c == '[') { // ESC[
        escape_state = 2; 
        return;
    }
    if (escape_state == 2) { // ESC[X
        escape_state = 0;
        switch(c) {
            case 'A': case 'B': // Flèches haut/bas - ignore pour l'instant
            case 'C': case 'D': // Flèches droite/gauche - ignore
                return;
        }
    }
    escape_state = 0; // Reset si séquence non reconnue
    
    if (c == 3) { // Ctrl+C
        console_clear();
        uart_send_string(UART0, "$ ");
        input_pos = 0;
    } else if (c == '\r' || c == '\n') {
        input_line[input_pos] = '\0';
        if (input_pos > 0) line_entered(input_line);
    } else if (c == 8 || c == 127) { // Backspace/Delete
        if (input_pos > 0) {
            input_pos--;
            uart_send(UART0, 8); uart_send(UART0, ' '); uart_send(UART0, 8);
        }
    } else if (c >= 32 && c <= 126 && input_pos < 78) {
        input_line[input_pos++] = c;
        uart_send(UART0, c);
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
  uart_send_string(UART0, "\nAdvanced Console Ready!\n");
 
  // ===== INITIALISATION SYSTEMES =====
  kprintf("Initializing enhanced system...\n");
  _irqs_setup();           
  irqs_setup();
  
  // Timer 1ms pour system ticks
  timer_init_1ms();
  irq_enable(TIMER0_IRQ, system_tick_handler, NULL);
  
  // UART avec événements
  uart_enable_rx_interrupt(UART0);  
  irq_enable(UART0_IRQ, uart_handler, NULL);
  irqs_enable();        

  // Console avec status line
  console_init(line_entered);
  line_callback = line_entered;
  console_clear();
  kprintf("Time:0s Events:0 CPU:idle\n\n$ ");

  // Boucle principale avec traitement événements
  event_t event;
  while (1) {
    // Traiter queue événements
    while (event_pop(&event)) {
      if (event.type == 1) { // UART RX
        console_process_input(event.data);
      }
    }
    
    // Updates périodiques
    console_update_status();
    console_blink_cursor();
    
    wfi(); // Low power wait
  }
}