// (c) 2026 Diamont Vault(R). All rights reserved.
//
// Coordenadas MEDIDAS sobre assets/bg_off.png / bg_on.png (2044 x 1798 px).
// Se obtuvieron por deteccion de las zonas que cambian entre el panel apagado y el
// encendido y por correlacion de las bobinas contra el panel.
// Todo el editor trabaja en este "espacio de referencia" y se escala al tamano de ventana.
// Si cambias las imagenes, usa el modo debug (tecla D, o Ctrl+clic en la pantalla inferior).
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Layout
{
using R = juce::Rectangle<float>;

constexpr float W = 2044.f, H = 1798.f;

// ---------------------------------------------------------------- bobinas
constexpr float reelLx = 596.f,  reelLy = 744.f, reelLd = 820.f;   // diametro en px de referencia
constexpr float reelRx = 1466.f, reelRy = 750.f, reelRd = 816.f;

// ------------------------------------------------------------- VU meters
inline const R vuWindowL { 233.f, 141.f, 296.f, 122.f };
inline const R vuWindowR { 1099.f, 141.f, 295.f, 122.f };
inline const juce::Point<float> vuPivotL { 381.f, 262.f }, vuPivotR { 1246.f, 262.f };
constexpr float vuNeedleLen = 150.f;
constexpr float vuRestDeg = -62.f;

// VU -> angulo de la aguja (grados desde la vertical), medido contra la escala impresa.
struct VuPoint { float vu, deg; };
inline constexpr VuPoint vuTable[] {
    { -30.f, -62.f }, { -20.f, -56.f }, { -10.f, -36.f }, { -7.f, -23.f }, { -5.f, -12.f },
    { -3.f, 0.f }, { 0.f, 28.f }, { 1.f, 40.f }, { 2.f, 49.f }, { 3.f, 58.f }, { 6.f, 64.f } };

// Zonas clicables de los selectores de medidor
inline const R meterVuZoneL   { 612.f, 266.f, 42.f, 32.f },  meterPeakZoneL { 680.f, 266.f, 68.f, 32.f };
inline const R meterVuZoneR   { 1476.f, 266.f, 42.f, 32.f }, meterPeakZoneR { 1544.f, 266.f, 68.f, 32.f };
inline const R meterInZone    { 828.f, 266.f, 56.f, 32.f },  meterOutZone   { 888.f, 266.f, 80.f, 32.f };

// ------------------------------------------------------------ encendido
inline const R powerZone { 1738.f, 20.f, 160.f, 90.f };

// ---------------------------------------------------------------- knobs
struct KnobPos { float x, y, d, deg; };     // centro, diametro de la tapa, recorrido (+-grados)
inline constexpr KnobPos kInGain { 302.f, 1435.f, 92.f, 135.f };
inline constexpr KnobPos kRecL   { 893.f, 187.f,  66.f, 130.f };
inline constexpr KnobPos kRepL   { 680.f, 185.f,  66.f, 130.f };
inline constexpr KnobPos kRecR   { 1750.f, 187.f, 66.f, 130.f };
inline constexpr KnobPos kRepR   { 1537.f, 185.f, 66.f, 130.f };
inline constexpr KnobPos kBias   { 824.f, 1532.f, 82.f, 135.f };
inline constexpr KnobPos kCal    { 1016.f, 1532.f, 82.f, 135.f };
inline constexpr KnobPos kLevel  { 1220.f, 1532.f, 82.f, 135.f };
inline constexpr KnobPos kWow    { 1355.f, 1290.f, 82.f, 135.f };

// --------------------------------------------------------------- botones
// (x, y, w, h) medidos. Fila superior de transporte
inline const R tr[6] { { 640.f, 1254.f, 92.f, 40.f }, { 748.f, 1254.f, 90.f, 40.f }, { 856.f, 1253.f, 90.f, 41.f },
                       { 965.f, 1252.f, 87.f, 41.f }, { 1069.f, 1254.f, 90.f, 40.f }, { 1178.f, 1254.f, 92.f, 40.f } };

inline const R eqBtn[2]     { { 193.f, 1258.f, 94.f, 41.f }, { 315.f, 1258.f, 95.f, 42.f } };
inline const R reelBtn[2]   { { 195.f, 1556.f, 93.f, 40.f }, { 318.f, 1557.f, 92.f, 41.f } };
inline const R linkBtn[2]   { { 199.f, 1654.f, 93.f, 46.f }, { 320.f, 1658.f, 90.f, 42.f } };

// Tipo de cinta: 2 columnas x 4 filas
inline R tapeBtn (int i)
{
    const int col = i / 4, row = i % 4;
    return { col == 0 ? 1637.f : 1763.f, 1273.f + 32.f * float (row), col == 0 ? 99.f : 98.f, 29.f };
}
// Ancho de cinta: 2 x 2
inline R widthBtn (int i)
{
    return { (i % 2) == 0 ? 1665.f : 1745.f, 1438.f + 34.f * float (i / 2), 78.f, 30.f };
}
inline R calBtn (int i) { return { 1636.f + 77.f * float (i), 1549.f, 70.f, 44.f }; }
inline R routeBtn (int i) { return { 1629.f + 79.f * float (i % 3), i < 3 ? 1639.f : 1685.f, 73.f, 38.f }; }

inline const R speedBtn[3] { { 1419.f, 1528.f, 78.f, 41.f }, { 1419.f, 1580.f, 78.f, 41.f }, { 1419.f, 1634.f, 77.f, 40.f } };
inline const R lowBtn[4]   { { 819.f, 1692.f, 80.f, 36.f }, { 929.f, 1693.f, 79.f, 35.f },
                             { 1038.f, 1693.f, 78.f, 35.f }, { 1145.f, 1693.f, 79.f, 35.f } };

// Pantalla inferior (placa dorada)
inline const R strip { 780.f, 1757.f, 482.f, 26.f };
}
