# Proyecto-Grafica-Base_Espacial
Proyecto final de Computación Gráfica: Base Espacial interactiva en OpenGL
# 🚀 Base Espacial Sustentable - OpenGL

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![UNAM](https://img.shields.io/badge/FI_UNAM-002B7A?style=for-the-badge&logo=unam&logoColor=white)

## 📖 Acerca del Proyecto
Este proyecto final fue desarrollado para la asignatura de **Computación Gráfica e Interacción Humano-Computadora (Semestre 2026-2)** en la Facultad de Ingeniería de la Universidad Nacional Autónoma de México (UNAM).

Consiste en un entorno virtual interactivo tridimensional programado en **C++ y OpenGL 3.3 (Core Profile)**. El proyecto simula una estación espacial modular orientada a la gestión de recursos, alineada con los Objetivos de Desarrollo Sostenible (ODS) 9 (Industria, Innovación e Infraestructura) y 12 (Producción y Consumo Responsables) de la Agenda 2030 de la ONU.

A partir de la versión actual el proyecto incluye **4 minijuegos integrados** (uno por zona habitable), sistema de **menú de pausa estilo Genshin Impact**, **minimapa en tiempo real**, **puertas con teletransporte por hitbox**, y un panel de **instrucciones adaptativo a la dificultad** seleccionada por el jugador.

**Autora:** Karla Beatriz Rojas Chimal
**Profesor:** Dr. Sergio Teodoro Vite

## ✨ Características Técnicas (Pipeline Gráfico)
El desarrollo integra un *pipeline* gráfico completo haciendo uso intensivo de la librería GLM:

* **Modelado y Entorno:** Más de 30 modelos en formato `.fbx` distribuidos en 5 zonas modulares interconectadas (HUB, Invernadero, Reciclaje, Energía y Cápsulas). El modelo maestro de Blender (`maestro.obj`, ~32k vértices) integra todos los grupos `Hub_/Inv_/Rec_/En_/Cap_/Pasillo_`.
* **Iluminación Avanzada:** Implementación de un Shader Phong v2 con iluminación multifuente (hasta 20 luces) y cálculos de *Fresnel* para simular materiales translúcidos y reflectantes (como el cristal del domo y los fluidos). Las luces se reconfiguran por zona en tiempo real (intensidad, color, posición).
* **Texturizado:** Aplicación de texturas PBR (Physically Based Rendering) y reflejos del entorno sideral mediante el uso de *Cubemaps* (Skybox).
* **Animación Multiparadigma:**
    * Animación esquelética (personaje y *rigging*) con `AnimatedModel`.
    * Animación por *keyframes* y transformaciones directas en el ciclo *Update*.
    * Generación procedural mediante *L-Systems* (helecho fractal en el invernadero, controlado con `P` / `O`).
    * Sistema de partículas para la **estela del cometa** que orbita el invernadero (con color seleccionable: cian / magenta / amarillo).
* **Física e Interacción:** Motor de colisiones AABB (*Axis-Aligned Bounding Box*) para delimitar el espacio navegable, hitboxes circulares para las puertas de teletransporte, e integración vertical básica con gravedad para el salto.
* **Cámara Dinámica:** Sistema inmersivo con 4 modos de visualización (`F1`=1ra persona, `F2`=2da, `F3`=3ra, `F8`=libre orbital) más vista cenital especial en el invernadero (`G`).
* **HUD 2D:** Renderizado de texto con `stb_truetype` (header-only, sin dependencias externas extra) + shader propio para *quads* 2D. El HUD incluye menú de pausa, overlay de controles, minimapa, banner de zona e instrucciones contextuales.

## 🕹️ Minijuegos por Zona

Cada zona habitable tiene un minijuego propio, accesible apenas el jugador entra. La dificultad (`5`=Fácil, `6`=Medio, `7`=Experto) se ajusta por zona; a menor dificultad **el panel de instrucciones muestra más pistas**.

| Zona | Minijuego | Mecánica | Teclas |
|---|---|---|---|
| 🌱 Invernadero | **Ritmo (OSU)** | Riega cada maceta cuando la nota llegue. Dos filas paralelas: notas de la fila izquierda vienen del muro izquierdo (`+x`), notas de la fila derecha vienen del muro derecho (`-x`) | `H` / `J` (izq atrás/adelante) · `K` / `L` (der atrás/adelante) · `B` reiniciar · `G` vista cenital |
| ⚡ Energía | **Simon (memoria)** | Memoriza la secuencia de colores que parpadean en los 4 botones de las pilas. El brillo se logra con 4 *point lights* sobre el modelo (sin geometría extra) | `Z` · `X` · `C` · `V` · `B` reiniciar |
| ♻️ Reciclaje | **Cajas interactivas** | 5 cajas con un control distinto cada una: rotar, mover en Y / X / Z, escalar. Reset con `R` | `O` / `L` rotar · `U` / `J` Y · `Y` / `H` X · `I` / `K` Z · `T` / `G` escalar · `R` reset |
| 🛌 Cápsulas | **Buscar al intruso** | 4 cápsulas alineadas; una de ellas oculta al intruso. Revisa antes de que se acabe el tiempo. En dificultades altas el intruso cambia de cápsula. | `F` revisar la más cercana · `B` reiniciar |

## 🎮 Controles de Usuario

El esquema se inspira (similar, no idéntico) en *Genshin Impact* — el personaje siempre se controla con WASD y la cámara orbital es opcional.

| Función | Tecla |
|---|---|
| Avanzar / Retroceder | `W` / `S` |
| Girar izquierda / derecha | `A` / `D` |
| Cámara (cuando libre) | Mouse |
| Correr (sprint) | `Shift` |
| Saltar | `Espacio` |
| Interactuar / examinar | `F` |
| 1ra / 2da / 3ra persona | `F1` / `F2` / `F3` |
| Cámara libre orbital | `F8` |
| Menú de pausa | `ESC` |
| Toggle rápido del minimapa | `TAB` |
| Ir a zona (atajo) | `0` HUB · `1` Invernadero · `2` Reciclaje · `3` Energía · `4` Cápsulas |
| Dificultad de la zona actual | `5` Fácil · `6` Medio · `7` Experto |

**Puertas con teletransporte:** Las 4 paredes cardinales del HUB tienen una puerta dorada (`puerta.fbx`). Al caminar dentro de su hitbox circular (radio ≈ 0.9), te teletransportas a la zona correspondiente: Sur → Invernadero, Este → Reciclaje, Norte → Energía, Oeste → Cápsulas. Cada zona tiene además una puerta de retorno al HUB. La detección es por **flanco** (afuera → adentro) para que tras el teleport no se re-active.

## 🖥️ HUD y Menús

* **Menú de pausa (`ESC`):** Continuar / Controles / Mapa (toggle) / Salir. Navegación con flechas o `W`/`S`, confirmar con Enter o Espacio.
* **Overlay de controles:** Tabla completa de teclas (acceso desde el menú).
* **Banner de zona:** Al teletransportarse aparece el nombre de la habitación grande en pantalla durante 2.5 s con fade-in/out.
* **Minimapa (esquina inferior derecha):** Cada zona pintada con su color, posición del jugador en tiempo real (punto rojo + blanco), marcadores amarillos de las puertas activas y nombre de la zona actual.
* **Panel de instrucciones (esquina inferior izquierda):** Lista de teclas y pistas que **crece con dificultad Fácil** y **se reduce a lo esencial en Experto**. Siempre indica la dificultad activa y cómo cambiarla.

## 🚀 Instalación y Ejecución

**Dependencias** (todas en `C:\src proyecto final\deps\` por la configuración del `vcxproj`):
- GLAD, GLFW, GLM, Assimp, irrKlang (heredadas del template original del curso)
- `stb_truetype.h` y `stb_image.h` viven en `Proyecto Final Codigo/include/` (header-only, no requieren link)

Compilar con **Visual Studio 2022** (PlatformToolset v143), configuración **x64 Debug**. Hacer clic en `ProjectoFinal.sln` o invocar `MSBuild` directamente. El ejecutable se genera en `bin/ProjectoFinal.exe` y debe ejecutarse desde la carpeta `bin/` para que encuentre las rutas relativas `models/`, `shaders/`, `textures/`.

Para instrucciones detalladas, consulta el archivo `manual.pdf` incluido en los entregables del curso.

*Nota: Por cuestiones de peso y lineamientos universitarios, este repositorio puede no incluir los binarios compilados ni todos los recursos pesados. El modelo maestro `maestro.obj` (`*.obj` está en `.gitignore`) sirve como referencia visual de la estación pero no es cargado en tiempo de ejecución; se usan los `.fbx` individuales por zona.*

## 📚 Referencias y Agradecimientos
* **Bibliografía base:** Angel, E., & Shreiner, D. (2011). *Interactive Computer Graphics*; Akenine-Möller et al. (2018). *Real-Time Rendering*.
* **Modelos y Texturas:** Recursos PBR obtenidos de plataformas como AmbientCG y Mixamo, debidamente referenciados en el reporte técnico en formato APA.
* **stb_truetype** y **stb_image** por Sean Barrett (dominio público).
