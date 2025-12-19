#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

extern int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
extern pde_t* walkpgdir(pde_t *pgdir, const void *va, int alloc);

struct gatedesc idt[256];
extern uint vectors[];
struct spinlock tickslock;
uint ticks;

void tvinit(void) {
  int i;
  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  initlock(&tickslock, "time");
}

void idtinit(void) {
  lidt(idt, sizeof(idt));
}

void trap(struct trapframe *tf) {
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed) exit(0);
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed) exit(0);
    return;
  }

  switch(tf->trapno){
  case T_PGFLT:

	uint fltaddr = rcr2();

    	//La direcc de fallo de pag está por encima del tamaño de la memoria: Matamos.
    	if (fltaddr >= myproc()->sz){
      		if (fltaddr >= KERNBASE) cprintf("Fallo de pag: 0x%x fuera del espacio de mem de usuario\n", fltaddr);
      		else cprintf("Fallo de pag: 0x%x fuera de la memoria del proceso\n", fltaddr);
      		myproc()->killed = 1;
      		break;
    	}

    	//Consultamos su entrada en la tabla de pags
    	uint fltpage = PGROUNDDOWN(fltaddr);
    	pde_t * pgfltpde = walkpgdir(myproc()->pgdir, (void *)fltpage, 0);
 
    	//Comprobamos si la pag estaba reservada y estaba presente
    	if (pgfltpde && *pgfltpde & PTE_P){
      		//Si el fallo pasó en modo kernel sobre una pag. presente: Panic
      		if (!(tf->err & PTE_U)) panic("El kernel tuvo el fallo de pag");

      		//Si no es una pag de usuario falla
      		if (!(*pgfltpde & PTE_U)) cprintf("Fallo de pag: 0x%x. Acceso a memoria protegida\n", fltaddr);
        
      		//Si es pag de usuario el fallo es de lectura o escritura
      		else{
        		if (tf->err & PTE_W) cprintf("Fallo de pag: error de escritura en 0x%x", fltaddr);
        		else cprintf("Fallo de pag: error de lectura en 0x%x", fltaddr);
      		}
      		myproc()->killed = 1;
      		break;
    	}

    	//Reservamos memoria para la pag y la inicializamos a 0
    	char *mem;
    	if ((mem = kalloc()) ==0){
      		cprintf("error de kalloc()\n");
      		myproc()->killed = 1;
      		break;
    	}
    	memset(mem, 0, PGSIZE);

    	//Mapeamos la memoria reservada a la pag con mappages
    	if (mappages(myproc()->pgdir, (char*)fltpage, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      		cprintf("error de mappages()\n");
      		kfree(mem);
      		myproc()->killed = 1;
    	}

    	break;

  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    lapiceoi();
    break;

  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    myproc()->killed = 1;
  }

  // Solo salimos si estamos en modo USUARIO.
  // Si el kernel causó el trap, terminamos la función y dejamos que el kernel siga.
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit(0);

  if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit(0);
}
