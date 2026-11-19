# ATR-102 MASTERING TAPE RECORDER PRO

Plugin VST3 de grabadora de cinta de masterizacion. (c) 2026 Diamont Vault(R).

Interfaz construida sobre tus imagenes (`assets/`): panel apagado/encendido, bobinas y aguja VU con canal alpha.

## Compilar (Windows 10/11)

Requisitos: Visual Studio 2022 (carga "Desarrollo de escritorio con C++") y CMake 3.22+.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

El VST3 queda en `build\ATR102_artefacts\Release\VST3\`. Copialo a `C:\Program Files\Common Files\VST3\`
(o configura `-DATR_COPY_AFTER_BUILD=ON` y ejecuta como administrador). En FL Studio:
Options > Manage plugins > Find installed plugins.

32 bits: `-A Win32`. Es experimental; FL Studio actual es de 64 bits.

## Compilar en GitHub (sin instalar nada)

Sube el proyecto a un repositorio. `.github/workflows/build.yml` compila x64 y Win32 en cada push y
publica el VST3 como artefacto en la pestana **Actions**.

## Mapa de controles

| Zona del panel | Control |
|---|---|
| POWER (arriba derecha) | Enciende/apaga. Apagado = señal limpia con la misma latencia |
| Fila superior de 6 | REW, STOP, PLAY, REC, FF, PAUSE. Solo PLAY/REC pasan por la cinta. REC exige estar en PLAY |
| Perilla a la derecha de la fila | Cantidad de wow/flutter |
| EMPHASIS EQ | NAB / IEC |
| INPUT GAIN | Ganancia de entrada ±12 dB |
| REEL CONTROL | W/F on/off, NOISE on/off |
| LINK MODE | REC y REPRO de L/R movidos juntos / independientes |
| REPRODUCE / RECORD (VU L y R) | Trim ±2 dB por canal |
| VU / PEAK, INPUT / OUTPUT | Clic en la etiqueta impresa para elegir balistica y fuente del medidor |
| BIAS | Sub/sobre-bias: distorsion, margen y ancho de banda |
| CAL (FLAT) | Ajuste de agudos de reproduccion ±3 dB |
| LEVEL | Nivel de salida ±12 dB |
| Velocidad (columna derecha) | 7.5 / 15 / 30 ips |
| TAPE TYPE | 8 formulaciones |
| TAPE WIDTH | 1/4", 1/2", 1", 2" (ruido y head bump) |
| CALIBRATION | 0 / +3 / +6 dB de nivel de operacion |
| OUTPUT ROUTING | INPUT (sin cinta), SYNC, REPRO, STEREO, MONO, FLIP |
| Fila inferior | PRESET -, PRESET +, BYPASS, BUMP |

Pantalla dorada inferior: contador de cinta, frecuencia de muestreo, ancho y velocidad; muestra el valor de cualquier
perilla mientras la mueves. Doble clic en una perilla = valor por defecto. La ventana se redimensiona.
Modo debug: tecla **D** o Ctrl+clic sobre la pantalla (muestra los rectangulos de todos los controles).

## Latencia

El plugin reporta 103 muestras al host (sobremuestreo 2x + retardo base del wow/flutter) y mantiene la
misma latencia con el plugin apagado o en bypass, asi que no hay saltos de fase al alternar.

## Pruebas del motor

`cmake --build build --target test_engine` (o ver `tools/test_engine.cpp`): latencia, respuesta en frecuencia, THD,
alias, ruido y wow/flutter. Resultados de referencia y limites en `docs/CALIBRACION.md`.
