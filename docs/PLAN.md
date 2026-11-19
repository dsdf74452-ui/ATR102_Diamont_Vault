# Plan de proyecto

| Fase | Contenido | Estado |
|---|---|---|
| 1. Motor | Cadena de cinta en C++ puro + test offline | Hecho |
| 2. Plugin | Parametros, transporte, presets, VU, latencia | Hecho |
| 3. Interfaz | Panel real, bobinas, agujas, botones iluminados, perillas | Hecho (coordenadas medidas) |
| 4. Build | CMake + GitHub Actions (x64 y Win32) | Hecho; falta ejecutarlo en tu repo |
| 5. Prueba en DAW | Cargar en FL Studio, revisar automatizacion y estado | Pendiente (requiere Windows) |
| 6. Calibracion | Medir contra referencia real y ajustar `kTapes` / constantes | Pendiente |
| 7. Instalador y firma | Inno Setup + firma de codigo | Pendiente |

## Arquitectura
- `Source/TapeEngine.*`: DSP sin JUCE (testeable).
- `Source/PluginProcessor.*`: parametros (APVTS), transporte, camino seco con latencia igual, medidores VU/pico, presets.
- `Source/PluginEditor.*`: dibujo sobre `bg_off/bg_on`, bobinas rotadas, agujas con balistica, perillas, botones.
- `Source/Layout.h`: todas las coordenadas (espacio de referencia 2044 x 1798).
- `assets/`: imagenes reducidas a 2044 x 1798 (los originales de 6249 x 5498 no se incluyen).
