# include "types.h"
# include "user.h"
# include "date.h"

int
main (int argc, char *argv [])
{
  struct rtcdate r;

  if (date(&r))
  {
    printf(2, "date failed\n");
    exit(0);
  }

  // Pon aquí tu código para imprimir la fecha en el formato que desees
  // Usamos el formato "Año-Mes-Dia, Hora:Minuto:Segundo"
  printf(1,"%d-%d-%d, %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);
  exit(0);
}
