# Hybrid Kick EQ

Plugin de EQ para kick, formato **VST3 / AU**, construido con **JUCE**.

## El problema que resuelve

En un EQ tradicional (analógico o digital que imita uno analógico), las
bandas están implementadas como filtros físicos en cascada. Esto significa
que cuando subes o bajas una banda, su "cola" matemática se extiende y
altera —aunque sea un poco— frecuencias que no querías tocar. Es un
comportamiento inherente al diseño de filtros de mínima fase (IIR), no un
error de un plugin en particular.

## La solución: núcleo híbrido

- **Núcleo de fase lineal (FFT / STFT)**: en vez de cascada de filtros,
  se calcula UNA curva de ganancia sumando la contribución de cada banda en
  cada frecuencia, y se aplica UNA sola vez sobre el espectro real de la
  señal. Una banda centrada en 3.5kHz que no tiene contribución matemática
  en 60Hz deja esa frecuencia exactamente en 0dB, sin importar qué le hagas
  a las demás bandas. Esto es aislamiento real entre bandas.
- **Etapa de saturación analógica** aplicada DESPUÉS del núcleo de EQ: le
  da el calor/carácter analógico sin comprometer el aislamiento logrado por
  el núcleo de fase lineal.

## Bandas incluidas (preset de kick)

1. **High Pass** (~30Hz) — quita rumble sub-grave
2. **Sub / Peso** (~60Hz) — refuerza el "boom" fundamental
3. **Cuerpo** (~300Hz) — reduce el "boxiness"/mud típico del kick
4. **Golpe** (~3.5kHz) — realza el click/ataque del beater
5. **Aire** (~9kHz) — brillo sutil arriba

Todas las frecuencias, ganancias y anchos (Q) son ajustables — el preset es
solo el punto de partida.

## Sobre la latencia

Un EQ de fase lineal SIEMPRE introduce latencia — es una consecuencia física
inevitable de la técnica, no una limitación de esta implementación en
particular (los EQs profesionales de fase lineal, en plugin o hardware,
tienen el mismo trade-off). Con la configuración actual (FFT de 1024
puntos), la latencia es de aproximadamente **23ms a 44.1kHz**. Studio One
compensa esta latencia automáticamente (PDC - Plugin Delay Compensation),
así que no genera desincronización con el resto de la mezcla — solo importa
si monitoreas en vivo a través de este plugin mientras grabas.

Si quieres menos latencia a cambio de menos resolución en graves, puedes
bajar `fftOrder` de `10` a `9` (512 puntos, ~11.6ms) en
`Source/DSP/STFTProcessor.h`.

## Compilar

Igual que el proyecto anterior (Analog API EQ):

```bash
cd HybridKickEQ
cmake -B build -G "Visual Studio 17 2022" -A x64   # Windows
cmake --build build --config Release
```

O sube el proyecto a GitHub — ya incluye `.github/workflows/build.yml`
configurado para compilar automáticamente en la nube (Windows) y darte el
`.vst3` como descarga en la pestaña Actions → Artifacts.

## Instalación en Studio One

1. Copia `Hybrid Kick EQ.vst3` a `C:\Program Files\Common Files\VST3\`
2. Reinicia Studio One (o Options → Locations → VST Plug-Ins → Reset)
3. Busca "Hybrid Kick EQ" en la categoría EQ del navegador de efectos

## Estructura del proyecto

```
HybridKickEQ/
├── CMakeLists.txt
├── .github/workflows/build.yml
└── Source/
    ├── PluginProcessor.h/.cpp     # Motor STFT + parámetros + saturación
    ├── PluginEditor.h/.cpp
    ├── DSP/
    │   ├── STFTProcessor.h        # Núcleo de fase lineal (overlap-add FFT)
    │   ├── SpectralEQBand.h       # Curvas matemáticas de cada banda
    │   └── AnalogSaturator.h      # Saturación analógica (post-EQ)
    └── GUI/
        ├── LookAndFeel.h/.cpp     # Tema visual (acento rojo-naranja)
        ├── EQCurveComponent.h/.cpp
        ├── SpectrumAnalyzer.h/.cpp
        └── BandPanel.h/.cpp
```

## Nota importante

Este proyecto usa una técnica de procesamiento espectral (STFT/overlap-add)
que es más compleja que un EQ IIR tradicional. Escribí la implementación
siguiendo el algoritmo estándar de overlap-add con ventana Hann al 75% de
solape, pero **no pude probarlo con audio real** en este entorno (sin DAW
disponible). Si al probarlo notas cambios de volumen inesperados al mover
las bandas, es probable que necesite un ajuste fino en la constante
`olaCorrection` de `STFTProcessor.h` — avísame el comportamiento exacto que
escuchas y lo ajustamos juntos.
