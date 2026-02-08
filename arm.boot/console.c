#include "console.h"
#include "main.h"
#include "uart.h"



static void (*callback)(char*) = 0;

// État pour détecter les séquences ANSI
static int escape_state = 0;  // 0=normal, 1=ESC reçu, 2=ESC[ reçu

/*
 * Cursor movements - basic ANSI sequences
 */
void cursor_left() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'D');
}

void cursor_right() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'C');
}

void cursor_down() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'B');
}

void cursor_up() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'A');
}

void cursor_at(int row, int col) {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'H');
}

void cursor_position(int* row, int* col) {
    *row = 0;
    *col = 0;
}

void cursor_hide() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, '?');
    uart_send(UART0, '2');
    uart_send(UART0, '5');
    uart_send(UART0, 'l');
}

void cursor_show() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, '?');
    uart_send(UART0, '2');
    uart_send(UART0, '5');
    uart_send(UART0, 'h');
}

/*
 * Clear screen
 */
void console_clear() {
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, '2');
    uart_send(UART0, 'J');
    uart_send(UART0, 27);
    uart_send(UART0, '[');
    uart_send(UART0, 'H');
}

/*
 * Init console
 */
void console_init(void (*cb)(char*)) {
    callback = cb;
}


void console_echo(unsigned char byte) {
    switch (escape_state) {
        case 0:  // État normal
            if (byte == 27) {  // ESC
                escape_state = 1;
            } else if (byte >= 32 && byte <= 126) {
                uart_send(UART0, byte);  // Caractere normal
            } else if (byte == '\r' || byte == '\n') {
                uart_send(UART0, '\r');
                uart_send(UART0, '\n');
            }
            break;
            
        case 1:  // ESC recu
            if (byte == '[') {
                escape_state = 2;
            } else {
                escape_state = 0;  // Reset si pas [
            }
            break;
            
        case 2:  // ESC[ recu
            escape_state = 0;  // Reset après traitement
            switch (byte) {
                case 'A':  // Fleche haut
                    cursor_up();
                    break;
                case 'B':  // Fleche bas
                    cursor_down();
                    break;
                case 'C':  // Fleche droite
                    cursor_right();
                    break;
                case 'D':  // Fleche gauche
                    cursor_left();
                    break;
            }
            break;
    }
}