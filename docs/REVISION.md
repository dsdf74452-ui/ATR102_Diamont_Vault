# Revision de la segunda opinion (contrastada con el codigo)

| # | Critica | Veredicto | Accion |
|---|---|---|---|
| 1 | El "gap" no esta modelado; la latencia no es la de una cinta | **Cierto, pero es decision de diseño.** El motor solo tiene un retardo de 64 muestras (centro de la modulacion wow/flutter) + 39 del sobremuestreo = 103. Un retardo fisico de 0.5"/15 ips (~33 ms) no se emula porque obligaria a reportar ~1600 muestras de latencia en un plugin de mezcla. | Documentado aqui. La latencia reportada = retardo real (test automatico). No afirmamos que sea la del equipo. |
| 2 | No hay histeresis | **Cierto.** `tanh` asimetrico sin memoria, normalizado a ganancia 1. | Ya constaba en `CALIBRACION.md`. Siguiente paso: Jiles-Atherton. |
| 3 | Las bobinas deben cambiar de tamaño al dibujarlas | **No aplica.** La bobina fisica (platos metalicos) no cambia de diametro; lo que cambia es la cantidad de cinta visible por las ventanas. Escalar la imagen del plato seria incorrecto. El radio solo gobierna la velocidad angular. | Sin cambio. Mejora posible: superponer un anillo de cinta oscuro en las ventanas con radio variable. |
| 4 | La cinta salta al llegar al extremo | **Cierto** (afectaba a la velocidad angular). Parar el transporte automaticamente no es adecuado en un plugin: dejaria pasar el audio sin procesar a mitad de mezcla. | Onda triangular continua: sin saltos (`tapePhase`). |
| 5 | Verificar el sentido de giro | **Razonable.** No lo he verificado contra fotos de una ATR-102 real. | `kPlayDirection` en `PluginEditor.cpp`; comprueba con tu referencia. |
| 6 | El ruido es blanco | **Parcial.** Se inyecta antes del de-enfasis (−9 dB en altas) y del filtro de perdida de HF, asi que a la salida no es blanco. Aun asi no esta calibrado ni tiene componente 1/f ni ruido de modulacion. | Pendiente de medir. |
| 7 | Las cifras no tienen metodo | **Cierto.** | Metodo abajo; las medidas son SIN ponderar. |
| – | "Diamont Vault®" | Es tu marca personal (confirmado por ti). Se mantiene en `LICENSE.txt`, `CMakeLists.txt` y el codigo. El simbolo ® solo es valido si esta registrada en tu jurisdiccion. | Sin cambio. |
| – | 0 VU = −18 dBFS arbitrario | **Cierto.** Es convencion de mezcla (EBU −18, SMPTE −20), no un nivel de flujo de cinta. | Pendiente: parametro de nivel de referencia. |

## Metodo de cada medida (`tools/test_engine.cpp`, 48 kHz)
- **Latencia:** impulso de 0.001 con el motor limpio; indice del pico de salida − indice de entrada.
- **Respuesta:** senos a −40 dBFS (lineal), DFT de 0.5 s tras 0.5 s de asentamiento, re 1 kHz.
- **THD:** seno de 1 kHz, 15 ips, cal +6 dB; sqrt(H2²+…+H5²)/H1, sin ponderar. Cuidado: 0 dBFS con +6 dB satura mucho.
- **Wow/flutter:** tono de 3 kHz a −20 dBFS, demodulacion I/Q con media movil de 10 ms, desviacion de pico de frecuencia, **sin ponderar** (no es DIN 45507 ni IEC 386, no comparable con la ficha del fabricante).

## Debilidad nueva detectada al revisar
Con 9 kHz a 0 dBFS y cal +6 dB (saturacion extrema), el aliasing del sobremuestreo 2x llega a −19 dBc en 3 kHz y −35 dBc en 21 kHz. A niveles normales es despreciable, pero para un uso "de referencia" conviene 4x u 8x en la saturacion.

## Fiabilidad de las dos opiniones
Ambas se presentan como respuestas de modelos que no ven el codigo real (una revisa "el texto"). Varias criticas se basan en la version anterior del proyecto (gap de 0.5") o en suposiciones, como la 3. Cualquier cifra o cita bibliografica debe verificarse por tu cuenta.
