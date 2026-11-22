# 🚀 Base Espacial Sustentable — OpenGL

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL_3.3-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![UNAM](https://img.shields.io/badge/FI_UNAM-002B7A?style=for-the-badge&logo=unam&logoColor=white)

Entorno 3D interactivo en **C++ y OpenGL 3.3 (Core Profile)**: una estación espacial modular con 5 zonas conectadas, 4 minijuegos, HUD propio y física de colisiones. Proyecto final individual de *Computación Gráfica e Interacción Humano-Computadora* (FI, UNAM, 2026-2), alineado con los ODS 9 y 12 de la Agenda 2030.

[![Video de demostración](https://img.youtube.com/vi/2CeQihvWECY/maxresdefault.jpg)](https://youtu.be/2CeQihvWECY)

▶️ **[Ver la demo en YouTube](https://youtu.be/2CeQihvWECY)** · 💾 **[Descargar el instalador para Windows](../../releases/latest)**

**Autora:** Karla Beatriz Rojas Chimal · **Profesor:** Dr. Sergio Teodoro Vite

---

## ✨ Lo técnico

| Área | Implementación |
|---|---|
| **Iluminación** | Shader Phong multifuente (hasta 20 luces reconfigurables por zona) y Fresnel para cristal y fluidos; pase aditivo *rim* para superficies metálicas. |
| **Texturas** | Texturas PBR y *skybox* con *cubemap* para reflejos del entorno. |
| **Animación** | Esquelética (personaje con *rigging*), por *keyframes*, procedural con **L-Systems** (helecho fractal) y **sistema de partículas** (estela de un cometa). |
| **Física** | Colisiones **AABB** generadas automáticamente por mesh al cargar cada zona (cilindro del jugador contra cajas), gravedad y salto, y puertas de teletransporte con detección por flanco. |
| **Cámara** | 1ª, 2ª y 3ª persona y orbital libre; bloqueo automático en minijuegos con vista fija. |
| **HUD 2D** | Texto con `stb_truetype` y shader propio: menú de pausa clickeable, minimapa en tiempo real, instrucciones según la dificultad y narrativa guiada. |
| **Escena** | Más de 30 modelos `.fbx` (modelo maestro de Blender de unos 32k vértices) en 5 zonas: HUB, Invernadero, Reciclaje, Energía y Cápsulas. |

## 🕹️ Minijuegos

Cada zona tiene un minijuego que se inicia con `B`, con tres niveles de dificultad (`5` Fácil · `6` Medio · `7` Experto).

| Zona | Minijuego | Cómo se juega |
|---|---|---|
| 🌱 Invernadero | **Ritmo** | Riega cada maceta cuando llega la nota (`H` `J` `K` `L`), en vista cenital. |
| ⚡ Energía | **Simon** | Repite la secuencia de colores con clic o con `Z` `X` `C` `V`. |
| ♻️ Reciclaje | **Clasificación** | Mueve, rota y escala cajas y separa los residuos en metal, vidrio y cartón. |
| 🛌 Cápsulas | **Buscar al intruso** | Revisa las cápsulas (`F`) antes de que se acabe el tiempo. |

## 🎮 Controles

| Acción | Tecla |
|---|---|
| Moverse / correr / saltar | `W` `A` `S` `D` · `Shift` · `Espacio` |
| Mirar | Mouse |
| Interactuar | `F` |
| Iniciar / salir del minijuego | `B` / `TAB` |
| Cámaras | `F1` `F2` `F3` (1ª, 2ª y 3ª persona) · `F8` (libre) |
| Ir a una zona | `0` HUB · `1` Invernadero · `2` Reciclaje · `3` Energía · `4` Cápsulas |
| Pausa / minimapa | `ESC` / `TAB` |

Las puertas doradas del HUB llevan a cada zona y cada zona tiene una puerta de regreso.

## 💾 Ejecutar

**Opción 1 — Instalador (Windows):** descarga `setup.exe` desde [Releases](../../releases/latest) e instálalo.

**Opción 2 — Compilar desde el código:**

1. Instala **Visual Studio 2022** (toolset v143).
2. Consigue las dependencias **GLAD, GLFW, GLM, Assimp e irrKlang** (versiones MSVC 2022, x64) y colócalas en `C:\src proyecto final\deps\`, que es la ruta que espera `ProjectoFinal.vcxproj`. Si las pones en otra carpeta, cambia las rutas en *Propiedades → C/C++ → Directorios de inclusión adicionales* y *Vinculador → Directorios de bibliotecas adicionales*.
3. Abre `Proyecto Final Codigo/ProjectoFinal/ProjectoFinal.sln` y compila en **x64 Debug**.
4. Copia las DLLs de Assimp, GLFW e irrKlang junto al ejecutable y ejecútalo **desde `bin/`**, que es donde están `models/`, `shaders/`, `textures/` y `sonidos/`.

```
Proyecto Final Codigo/   código fuente (ProyectoFinal.cpp, include/, solución de VS)
bin/                     recursos en tiempo de ejecución: modelos, shaders, texturas y sonidos
```

## 📚 Créditos

- **Bibliografía:** Angel & Shreiner (2011), *Interactive Computer Graphics*; Akenine-Möller et al. (2018), *Real-Time Rendering*.
- **Recursos:** texturas PBR de AmbientCG, animaciones de Mixamo y efectos de sonido de Pixabay (sus autores aparecen en el nombre de cada archivo de `bin/sonidos/`).
- **Librerías:** `stb_truetype` y `stb_image` de Sean Barrett (dominio público), GLFW, GLAD, GLM, Assimp e irrKlang.
- La base del proyecto parte del template del curso.
