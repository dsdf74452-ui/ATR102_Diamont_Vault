# Estado del modelo y calibracion

Salida de `tools/test_engine.cpp` (48 kHz) con el codigo actual:

```
Latencia reportada: 103 muestras
Impulso: pico en 203 (esperado 203)

Respuesta en frecuencia (15 ips, 456, NAB, -40 dBFS)  [dB re 1 kHz]
 bump OFF: 30:+0.02 50:+0.01 60:+0.01 100:+0.00 200:+0.00 1000:+0.00 5000:-0.04 10000:-0.19 16000:-1.11 20000:-5.61 
 bump ON : 30:+0.85 50:+1.93 60:+1.96 100:+0.88 200:+0.20 1000:+0.00 5000:-0.05 10000:-0.20 16000:-1.12 20000:-5.62 

Ganancia pequena senal a 1 kHz (-40 dBFS): -0.002 dB

THD 1 kHz (H2+H3+H4+H5) vs nivel de entrada, 15 ips:
 cinta 456: -18dBFS:1.10%  -12dBFS:3.82%  -6dBFS:10.15%  -3dBFS:14.14%  +0dBFS:17.65%  
 cinta 499: -18dBFS:0.71%  -12dBFS:2.57%  -6dBFS:7.65%  -3dBFS:11.48%  +0dBFS:15.40%  
 cinta GP9: -18dBFS:0.36%  -12dBFS:1.37%  -6dBFS:4.61%  -3dBFS:7.65%  +0dBFS:11.49%  

Alias de 9 kHz @0dBFS: 21k:-35.0 dBc  3k:-19.2 dBc

Suelo de ruido (silencio, rms dBFS, de-enfasis incluido):
 ancho 0: -84.2 dBFS   ancho 1: -87.2 dBFS   ancho 2: -90.2 dBFS   ancho 3: -93.2 dBFS  

Wow/flutter 100%: desviacion pico de pitch = 0.076% (3 kHz -> +-2.27 Hz)
```

## Que esta verificado
- Latencia reportada = latencia real (impulso).
- Ganancia de pequeña señal = 0 dB a 1 kHz.
- Wow/flutter al 100 % = 0,076 % pico de desviacion de pitch (a 15 ips).
- Sobremuestreo 2x (FIR halfband Kaiser, 79 taps) en la saturacion.

## Que NO esta validado contra una ATR-102 real
Los siguientes valores son **estimaciones iniciales**, no mediciones: margen de saturacion por cinta, ruido por
cinta/ancho/velocidad, frecuencias de corte de la perdida de HF, head bump (3,7 x ips, +1,2 a +2,8 dB),
profundidad del pre/de-enfasis (9 dB) y los factores de wow/flutter. No hay modelo de histeresis
Jiles-Atherton, ni calibracion en nWb/m, ni crosstalk, azimuth, ni ruido de modulacion.

## Procedimiento para llevarlo a nivel de referencia
1. Medir una maquina real (o un plugin de referencia con licencia) con barridos y tonos a 1 kHz y 10 kHz a varios niveles,
   15 ips y 30 ips, cinta 456 y GP9: respuesta, THD H2/H3 vs nivel, ruido ponderado, wow/flutter ponderado.
2. Ajustar la tabla `kTapes` y las constantes de `TapeEngine::rebuild()` hasta que las curvas coincidan.
3. Sustituir `tanh` por un modelo de histeresis (Jiles-Atherton) y repetir el paso 2.
4. Comparar a ciegas (nivel igualado) con la maquina.

## Direccion de giro de las bobinas
Ambas giran en sentido antihorario en PLAY (`kPlayDirection` en `PluginEditor.cpp`). Cambia el signo si tu referencia
es la contraria.
