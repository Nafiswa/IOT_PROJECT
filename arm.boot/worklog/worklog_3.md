////   Système d'interruptions IRQ fonctionnel avec handlers et cookies


(très important)
--> Il faut ajouter irq.o et isr.o dans la variable objs du Makefile.

--> Il faut modifier exception.s pour pointer irq_handler_addr vers _irq_handler au lieu de _isr_handler.

--> Il faut inclure isr.h dans main.c pour utiliser les fonctions d'interruption.

--> Il faut activer les interruptions UART avec uart_enable_rx_interrupt() avant d'enregistrer le handler.


## Principe de fonctionnement des interruptions :


- **VIC (Vectored Interrupt Controller)** : Contrôle les  IRQs possibles du système, registres à 0x10140000

- **irq_enable(IRQ_NUM, handler, cookie)** : Enregistre un handler avec son cookie pour une IRQ spécifique

- **Handler assembly** : _irq_handler sauvegarde les registres, appelle irq_handler() en C, puis restore

- **Table de handlers** : Structure irq_handler[32] stocke callback, cookie et état enabled pour chaque IRQ

- **Flux d'interruption** : Hardware → VIC → _irq_handler → irq_handler() → callback_utilisateur()

- **UART0_IRQ = 12** : Interruption déclenchée à chaque caractère reçu sur le port série


## Test simple que j'ai effectué:

Le main.c initialise le système (_irqs_setup, irqs_setup, irqs_enable) puis enregistre un handler pour UART0. 

À chaque frappe clavier, l'interruption se déclenche et incrémente un compteur.

**Explication du Comportement**: Le processeur entre en veille (`wfi()`) et se réveille uniquement sur IRQ UART0.

**process** : Caractère tapé → UART0 IRQ 12 → `test_handler()` → affichage compteur → retour veille.

