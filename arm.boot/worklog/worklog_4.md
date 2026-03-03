////   Système événementiel avec Timer, Events et Shell console


(Element très important qui m'ont pris du temps de les comprendre et les debuger)

--> Il faut activer le timer avec timer_init_1ms() avant d'enregistrer le handler timer.

--> Il faut enregistrer le handler timer sur TIMER0_IRQ (IRQ 4) avec irq_enable().

--> Il faut activer l'interruption RX du UART avant d'enregistrer le handler UART.


## Timer 1ms :

- **Timer0** à l'adresse 0x101E2000, chargé avec 1000 cycles (1ms à 1MHz)

- Mode périodique 32-bit avec interruptions activées

- `system_tick_handler()` incrémente `system_ticks` à chaque milliseconde et clear l'interruption

- Ca nous donne base de temps pour tout le système


## Système d'événements :

- **Ring buffer** de 16 éléments, structure event_t avec type + data

- `event_push()` appelé dans le top-handler UART (en interruption), écrit dans le ring

- `event_pop()` appelé dans la boucle principale, lit les événements en dehors de l'interruption

- Le traitement applicatif (console_process_input) se fait dans la boucle principale = bottom-handler


## Console / Shell :

- Status bar en haut de l'écran, mise à jour toutes les secondes (temps + nb events)

- Curseur qui clignote toutes les 500ms grâce au timer

- Saisie ligne avec backspace, Ctrl+C pour clear, Enter pour valider


## Boucle principale :

- Dépile les événements, traite l'input console, met à jour le status, puis `wfi()`

- Le processeur dort entre les interruptions 
