#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
	int status;
  	//Recogemos el argumento entero de la pila
	if(argint(0,&status) < 0) return -1;
	
	//Para cumplir con WEXITSTATUS se desplaza 8 bits, así los 8 bits bajos quedan a 0
	//indicando salida normal
	exit(status << 8);
  	return 0;  // not reached
}

int
sys_wait(void)
{
	int *p;
	//Recogemos el puntero, usando sizeof para verificar que cabe
	if(argptr(0, (void**)&p, sizeof(int)) < 0) return -1;
  
	return wait(p);
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0) return -1;
  addr = myproc()->sz;
  if(n > 0){	//Aumentamos la memoria del proceso
	if(myproc()->sz + n >= KERNBASE) return -1;	//Si la mem pedida llega hasta el kernel: Falla
	myproc()->sz +=n;
  }
  else if(growproc(n) < 0) return -1; //Si no es positiva, llamamos a growproc
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
int
sys_date(void)
{
	struct rtcdate *d;

	if(argptr(0,(void**)&d, sizeof(struct rtcdate)) < 0) return -1;		//Recogemos el param de la primera pos de la pila
	
	cmostime(d);	//Llamamos a cmostime con el puntero

	return 0;
}
