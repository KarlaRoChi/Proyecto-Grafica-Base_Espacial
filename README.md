# Proyecto-Grafica-Base_Espacial
Proyecto final de Computación Gráfica: Base Espacial interactiva en OpenGL
# 🚀 Base Espacial Sustentable - OpenGL

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![UNAM](https://img.shields.io/badge/FI_UNAM-002B7A?style=for-the-badge&logo=unam&logoColor=white)

## 📖 Acerca del Proyecto
Este proyecto final fue desarrollado para la asignatura de **Computación Gráfica e Interacción Humano-Computadora (Semestre 2026-2)** en la Facultad de Ingeniería de la Universidad Nacional Autónoma de México (UNAM). 

Consiste en un entorno virtual interactivo tridimensional programado en **C++ y OpenGL 3.3 (Core Profile)**. El proyecto simula una estación espacial modular orientada a la gestión de recursos, alineada con los Objetivos de Desarrollo Sostenible (ODS) 9 (Industria, Innovación e Infraestructura) y 12 (Producción y Consumo Responsables) de la Agenda 2030 de la ONU.

**Autora:** Karla Beatriz Rojas Chimal  
**Profesor:** Dr. Sergio Teodoro Vite  

## ✨ Características Técnicas (Pipeline Gráfico)
El desarrollo integra un *pipeline* gráfico completo haciendo uso intensivo de la librería GLM:

* **Modelado y Entorno:** Más de 30 modelos en formato `.fbx` distribuidos en 5 zonas modulares interconectadas (HUB, Invernadero, Reciclaje, Energía y Cápsulas).
* **Iluminación Avanzada:** Implementación de un Shader Phong v2 con iluminación multifuente y cálculos de *Fresnel* para simular materiales translúcidos y reflectantes (como el cristal del domo y los fluidos).
* **Texturizado:** Aplicación de texturas PBR (Physically Based Rendering) y reflejos del entorno sideral mediante el uso de *Cubemaps* (Skybox).
* **Animación Multiparadigma:** * Animación esquelética (personaje y rigging).
    * Animación por *keyframes* y transformaciones directas en el ciclo *Update*.
    * Generación procedural mediante *L-Systems* (crecimiento de vegetación).
* **Física e Interacción:** Motor de colisiones AABB (*Axis-Aligned Bounding Box*) para delimitar el espacio navegable y un sistema HUD 2D para despliegue de mensajes en pantalla (diseño centrado en el usuario).
* **Cámara Dinámica:** Sistema inmersivo con 4 modos de visualización:
    * `F1`: Cámara Libre Orbital
    * `F2`: Primera Persona
    * `F3`: Segunda Persona (Frontal)
    * `F4`: Tercera Persona (Detrás del avatar)

## 🎮 Controles de Usuario
El entorno fue diseñado bajo principios de Interacción Humano-Computadora (HCI) pensando en el usuario como un "Cadete Espacial":
* `W`, `A`, `S`, `D`: Desplazamiento del personaje por las zonas.
* `Ratón`: Control de la dirección de la vista.
* `Teclas de Función (F1-F4)`: Cambio de cámara.
* *(Los *Contextual Prompts* en el HUD indicarán al usuario cuándo interactuar con objetivos específicos).*

## 🚀 Instalación y Ejecución
Para instrucciones detalladas sobre cómo instalar y ejecutar la simulación en un entorno Windows local, por favor consulta el archivo `manual.pdf` incluido en los entregables del curso. 

*Nota: Por cuestiones de peso y lineamientos universitarios, este repositorio puede no incluir los binarios compilados ni los recursos pesados completos (modelos 3D y texturas de alta resolución).*

## 📚 Referencias y Agradecimientos
* **Bibliografía base:** Angel, E., & Shreiner, D. (2011). *Interactive Computer Graphics*; Akenine-Möller et al. (2018). *Real-Time Rendering*.
* **Modelos y Texturas:** Recursos PBR obtenidos de plataformas como AmbientCG y Mixamo, debidamente referenciados en el reporte técnico en formato APA.
