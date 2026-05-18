# Proyecto-Grafica-Base_Espacial
Proyecto final de Computación Gráfica: Base Espacial interactiva en OpenGL
# 🚀 Base Espacial Sustentable - OpenGL

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![UNAM](https://img.shields.io/badge/FI_UNAM-002B7A?style=for-the-badge&logo=unam&logoColor=white)

## 📖 Acerca del Proyecto
Este proyecto final fue desarrollado para la asignatura de **Computación Gráfica e Interacción Humano-Computadora (Semestre 2026-2)** en la Facultad de Ingeniería de la Universidad Nacional Autónoma de México (UNAM).

Consiste en un entorno virtual interactivo tridimensional programado en **C++ y OpenGL 3.3 (Core Profile)**. El proyecto simula una estación espacial modular orientada a la gestión de recursos, alineada con los Objetivos de Desarrollo Sostenible (ODS) 9 (Industria, Innovación e Infraestructura) y 12 (Producción y Consumo Responsables) de la Agenda 2030 de la ONU.

A partir de la versión actual el proyecto incluye **4 minijuegos integrados** (uno por zona habitable), sistema de **menú de pausa estilo Genshin Impact**, **minimapa en tiempo real**, **puertas con teletransporte por hitbox**, un panel de **instrucciones adaptativo a la dificultad** y una **narrativa guiada** que muestra mensajes contextuales al entrar a cada módulo y al iniciar cada tarea (bienvenida, riega las plantas, organiza la basura, etc.).

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
* **Física e Interacción:** Motor de colisiones AABB (*Axis-Aligned Bounding Box*) por mesh: cada FBX por zona genera sus propios colliders al cargarse y el chequeo es por cilindro contra AABB. Mesas, atril, macetas con plantas, pilas de energía y cápsulas son ahora **sólidos navegables** (no se atraviesan). Las hitboxes circulares de las puertas siguen siendo independientes y se disparan por flanco; la integración vertical con gravedad sigue activa para el salto.
* **Cámara Dinámica:** Sistema inmersivo con 4 modos (`F1`=1ra persona, `F2`=2da, `F3`=3ra, `F8`=libre orbital). En tercera/primera/segunda persona el **mouse rota la orientación del avatar** (no solo en cámara libre). Cuando se inicia un minijuego con vista fija (invernadero cenital, panel del Simon) la cámara se **bloquea** y `TAB` sirve de tecla de interrupción para liberarla.
* **HUD 2D:** Renderizado de texto con `stb_truetype` (header-only, sin dependencias externas extra) + shader propio para *quads* 2D. El HUD incluye menú de pausa, overlay de controles, minimapa, banner de zona, instrucciones contextuales y **panel de narrativa guiada** centrado en la parte inferior. La fuente se elevó a 48 px para mejorar legibilidad.
* **Brillo Metálico:** Las superficies metálicas (`RECICLAJE_METAL`, `ENERGIA_METALICO`) se renderizan con specular alto (`spec = 20`, `alphaIndex = 4096`) y un segundo pase aditivo tipo *rim/fresnel* que solo se nota en los bordes, dando un acabado pulido/cromo sin necesidad de un shader extra.

## 🕹️ Minijuegos por Zona

Cada zona habitable tiene un minijuego propio. A diferencia de versiones anteriores, **ningún minijuego arranca automáticamente al entrar** a la zona: el jugador debe presionar `B` para iniciarlo (la narrativa guía con un mensaje del estilo *"Tarea: riega las plantas… Pulsa B para empezar"*). La dificultad (`5`=Fácil, `6`=Medio, `7`=Experto) se ajusta por zona; a menor dificultad **el panel de instrucciones muestra más pistas**.

Cuando un minijuego pide vista fija (cenital del invernadero, frente al panel del Simon en energía) la cámara se bloquea automáticamente y `TAB` libera el control para volver al desplazamiento normal.

| Zona | Minijuego | Mecánica | Teclas |
|---|---|---|---|
| 🌱 Invernadero | **Ritmo (OSU)** | Riega cada maceta cuando la nota llegue. Dos filas paralelas: notas de la fila izquierda vienen del muro izquierdo (`+x`), notas de la fila derecha vienen del muro derecho (`-x`). Al pulsar `B` la cámara se mueve a vista cenital y el desplazamiento del avatar se congela. | `B` iniciar (activa la vista cenital) · `H` / `J` (izq atrás/adelante) · `K` / `L` (der atrás/adelante) · `G` alternar vista · `TAB` interrumpir |
| ⚡ Energía | **Simon (memoria)** | Memoriza la secuencia de colores que parpadean en los 4 botones de las pilas y repítela. Al iniciar, la cámara se fija frente al panel y el cursor del mouse aparece para que **clickees los botones directamente** además del teclado. Los botones se dibujan como cubos de color sobre los modelos reales de las pilas. | `B` iniciar (fija cámara + muestra cursor) · clic izquierdo sobre el botón · `Z` `X` `C` `V` (alternativo) · `TAB` interrumpir |
| ♻️ Reciclaje | **Cajas interactivas + clasificación** | 5 cajas, cada una con un control distinto (mover en X/Y/Z, rotar, escalar). Al pulsar `B` se reparten los items por el piso y los 3 botes (metal/vidrio/cartón) aparecen marcados. | `B` iniciar · `TAB` cambiar caja activa · `I` `J` `K` `L` mover · `U` `M` Y · `O` `.` rotar · `T` `G` escalar · `R` reset |
| 🛌 Cápsulas | **Buscar al intruso** | Hasta 8 cápsulas alineadas; una de ellas oculta al intruso. Revisa antes de que se acabe el tiempo. En dificultades altas el intruso cambia de cápsula. | `B` iniciar · `F` revisar la más cercana |

## 🎮 Controles de Usuario

El esquema se inspira (similar, no idéntico) en *Genshin Impact* — el personaje siempre se controla con WASD y la cámara orbital es opcional.

| Función | Tecla |
|---|---|
| Avanzar / Retroceder | `W` / `S` |
| Girar izquierda / derecha | `A` / `D` |
| Cámara (siempre, no solo en libre) | Mouse |
| Correr (sprint) | `Shift` |
| Saltar | `Espacio` |
| Interactuar / examinar | `F` |
| 1ra / 2da / 3ra persona | `F1` / `F2` / `F3` |
| Cámara libre orbital | `F8` |
| Menú de pausa | `ESC` |
| Toggle rápido del minimapa | `TAB` (fuera de minijuegos) |
| Interrumpir minijuego con cámara fija | `TAB` (dentro del minijuego) |
| Iniciar minijuego de la zona actual | `B` |
| Ir a zona (atajo) | `0` HUB · `1` Invernadero · `2` Reciclaje · `3` Energía · `4` Cápsulas |
| Dificultad de la zona actual | `5` Fácil · `6` Medio · `7` Experto |

**Puertas con teletransporte:** Las 4 paredes cardinales del HUB tienen una puerta dorada (`puerta.fbx`). Al caminar dentro de su hitbox circular (radio ≈ 0.9), te teletransportas a la zona correspondiente: Sur → Invernadero, Este → Reciclaje, Norte → Energía, Oeste → Cápsulas. Cada zona tiene además una puerta de retorno al HUB. La detección es por **flanco** (afuera → adentro) para que tras el teleport no se re-active.

## 🖥️ HUD y Menús

* **Menú de pausa (`ESC`):** Continuar / Controles / Mapa (toggle) / Salir. Navegación con flechas o `W`/`S`, confirmar con Enter o Espacio.
* **Overlay de controles:** Tabla completa de teclas (acceso desde el menú).
* **Banner de zona:** Al teletransportarse aparece el nombre de la habitación grande en pantalla durante 2.5 s con fade-in/out.
* **Minimapa (esquina inferior derecha):** Cada zona pintada con su color, posición del jugador en tiempo real (punto rojo + blanco), marcadores amarillos de las puertas activas y nombre de la zona actual.
* **Panel de instrucciones (esquina inferior izquierda):** Lista de teclas y pistas que **crece con dificultad Fácil** y **se reduce a lo esencial en Experto**. Siempre indica la dificultad activa y cómo cambiarla.
* **Panel de narrativa guiada (centro inferior):** Una cola de mensajes contextuales se reproduce al entrar a cada zona (*"Bienvenido a la estación espacial sustentable"*, *"Tarea: riega las plantas siguiendo el ritmo. Pulsa B para empezar"*, etc.). Cada mensaje dura ~4.5 s con transiciones de fade. Funciona como recorrido guiado para que el jugador entienda qué hacer en cada módulo sin tener que leer el README.

## 🆕 Cambios de esta versión

* **Colisiones AABB rigurosas.** El filtro de `GenerarColisionesDeModelo` se afinó: ya no descarta objetos altos ni los que tocan el piso (eso bloqueaba demasiado). Solo se ignoran pisos enteros (anchos en XZ, casi sin alto), paredes externas y meshes diminutos (LEDs decorativos). El cilindro del jugador testea contra cada AABB de la zona actual, por lo que **mesas, atril, macetas con plantas, pilas de energía y cápsulas dejaron de ser atravesables**.
* **Minijuegos no autoarrancan.** Cuando entras a una zona ahora se carga la geometría y los mensajes guía, pero el minijuego permanece dormido hasta que pulses `B`. Esto evita que el bloqueo de cámara o el cursor del Simon sorprendan al jugador.
* **Bloqueo de cámara con interrupción.** Los minijuegos que requieren vista fija (invernadero cenital, panel del Simon) bloquean la cámara y el desplazamiento. `TAB` actúa como tecla de "salir del desafío" para devolver el control normal y volver a explorar.
* **Mouse para orientar al avatar.** En primera/segunda/tercera persona el mouse rota el `yaw` del jugador, no solo cuando la cámara está en modo libre.
* **Simon con clic.** Al iniciar el minijuego de energía aparece el cursor del mouse y los 4 botones de las pilas se dibujan como cubos de color sobre el modelo real; puedes hacer clic directamente sobre el botón (también funciona `Z` `X` `C` `V`).
* **Brillo metálico mejorado.** Los modelos metálicos de reciclaje y energía usan un pase aditivo extra que simula un *rim/fresnel* (specular muy concentrado solo en los bordes). El `alphaIndex` subió a 4096 para concentrar el highlight.
* **Texto HUD más legible.** La fuente del HUD se elevó a 48 px (antes 32 px) y se añadió el panel de narrativa centrado.
* **Menú clickeable.** El menú de pausa acepta clic con el mouse: al pasar por encima de una opción se resalta, al hacer clic se ejecuta. El teclado (flechas/Enter) sigue funcionando en paralelo. En el overlay de controles, cualquier clic vuelve a la pausa.
* **Mouse para mirar arriba/abajo.** El movimiento vertical del mouse ahora controla el `pitch` del jugador en 1ra/2da/3ra persona (clamp a ±85°), así puedes ver el techo del invernadero o el reactor sin tener que entrar a cámara libre.
* **Colisiones más estables.** El filtro de `GenerarColisionesDeModelo` se afinó para que solo se registren *props* (footprint ≤ 5 m, altura ≤ 4 m, mínimo 30 cm sobre el piso). Además `CheckCollision` ignora colliders en los que el jugador ya esté dentro al spawnear (evita quedar atrapado).
* **Dificultad no auto-inicia.** Pulsar `5`/`6`/`7` solo ajusta el slider de dificultad; el minijuego se inicia siempre con `B`, en cualquier zona.

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
