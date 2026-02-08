
////   Console fonctionnelle avec un cursor qui bouge (haut,bas,gauche,droite) 


(trés important)
--> Il faut changer et augmenter la valeur de MEMSIZE=16 dans le Makefile.
Personnellement, je l’ai augmentée à 64, car sans ça la console ne fonctionnait pas.

--> Il faut ajouter console.o et kprintf.o dans la variable objs du Makefile.

--> Il faut également mettre à jour le main.c afin d’initialiser la console.


## Principe de fonctionnement de la console :


- **Reception clavier** : uart_receive() lit les octets du terminal, console_echo() les traite

- **uart_send(UART0, byte)** : envoie un octet via le port série vers le terminal

- **Séquences ANSI** : `ESC[D` (gauche), `ESC[C` (droite), etc. pour contrôler les mouvements du curseur 

- **Machine à états** : Pour detecter les differents séquences de flèches et les caractères saisies 



