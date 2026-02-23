

#include "main.h"
#include "isr.h"
#include "isr-mmio.h"


/*
 * Structure pour gerer les handlers d'interruption avec cookies
 */
struct irq_handler {
    void (*callback)(uint32_t irq, void *cookie);
    void *cookie;
    uint8_t enabled;
};

/*
 * Table des handlers d'interruption possibles
 */
static struct irq_handler irq_handlers[NIRQS];

/*
 * Fonctions utilitaires pour accéder au VIC
 */
static inline void vic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)(VIC_BASE_ADDR + offset) = value;
}

static inline uint32_t vic_read(uint32_t offset) {
    return *(volatile uint32_t*)(VIC_BASE_ADDR + offset);
}

/*
 * Initialisation du système d'interruption
 */
void irqs_setup() {
    // Initialiser tous les handlers à NULL
    for (int i = 0; i < NIRQS; i++) {
        irq_handlers[i].callback = 0;
        irq_handlers[i].cookie = 0;
        irq_handlers[i].enabled = 0;
    }
    
    // Désactiver toutes les interruptions du VIC
    vic_write(VICINTENABLE, 0x00000000);
    // Configurer toutes les interruptions comme IRQ (pas FIQ)
    vic_write(VICINTSELECT, 0x00000000);
    
    kprintf("IRQ system initialized\n");
}

/*
 * Activer les interruptions globalement (niveau processeur)
 */
void irqs_enable() {
    __asm__ volatile (
        "mrs r0, cpsr\n\t"
        "bic r0, r0, #0x80\n\t"  // Clear IRQ bit (bit 7)
        "msr cpsr_c, r0"
        :
        :
        : "r0"
    );
}

/*
 * Désactiver les interruptions globalement (niveau processeur)
 */
void irqs_disable() {
    __asm__ volatile (
        "mrs r0, cpsr\n\t"
        "orr r0, r0, #0x80\n\t"  // Set IRQ bit (bit 7)
        "msr cpsr_c, r0"
        :
        :
        : "r0"
    );
}

/*
 * Activer une interruption spécifique avec callback et cookie
 */
void irq_enable(uint32_t irq, void(*callback)(uint32_t, void*), void* cookie) {
    if (irq >= NIRQS) {
        kprintf("IRQ %u out of range\n", irq);
        return;
    }
    
    // Sauvegarder le handler et le cookie
    irq_handlers[irq].callback = callback;
    irq_handlers[irq].cookie = cookie;
    irq_handlers[irq].enabled = 1;
    
    // Activer cette IRQ dans le VIC
    uint32_t mask = (1 << irq);
    vic_write(VICINTENABLE, mask);
    
    kprintf("IRQ %u enabled\n", irq);
}

/*
 * Désactiver une interruption spécifique
 */
void irq_disable(uint32_t irq) {
    if (irq >= NIRQS) {
        kprintf("IRQ %u out of range\n", irq);
        return;
    }
    
    // Désactiver cette IRQ dans le VIC
    uint32_t current = vic_read(VICINTENABLE);
    uint32_t mask = ~(1 << irq);
    vic_write(VICINTENABLE, current & mask);
    
    // Nettoyer le handler
    irq_handlers[irq].callback = 0;
    irq_handlers[irq].cookie = 0;
    irq_handlers[irq].enabled = 0;
    
    kprintf("IRQ %u disabled\n", irq);
}

/*
 * Handler principal d'interruption IRQ
 * Cette fonction est appelée depuis irq.S
 */
void irq_handler() {
    // Lire le statut des interruptions actives
    uint32_t irq_status = vic_read(VICIRQSTATUS);
    
    if (irq_status == 0) {
        kprintf("Spurious IRQ!\n");
        return;
    }
    
    // Trouver quelle IRQ est active (priorité du bit le plus bas)
    for (uint32_t irq = 0; irq < NIRQS; irq++) {
        uint32_t mask = (1 << irq);
        if (irq_status & mask) {
            // Cette IRQ est active
            if (irq_handlers[irq].enabled && irq_handlers[irq].callback) {
                // Appeler le handler avec son cookie
                irq_handlers[irq].callback(irq, irq_handlers[irq].cookie);
            } else {
                kprintf("Unhandled IRQ %u\n", irq);
            }
            // Une seule IRQ à la fois pour simplifier
            break;
        }
    }
}

void wfi(void) {
    _wfi();
}