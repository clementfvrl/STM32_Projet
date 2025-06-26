# 📦 STM32_Projet - NUCLEO-L152RE + IKS01A3 (avec capteur LSM6DSO)

## 📋 Description

Ce projet embarqué utilise une carte **NUCLEO-L152RE** avec le **shield IKS01A3**, intégrant notamment le capteur **LSM6DSO** (gyroscope). L'objectif est de récupérer des données de mouvement (accélérations/inclinaisons) à des fins de traitement ou de transmission.

---

## 🛠 Matériel utilisé

| Composant         | Référence         | Rôle                       |
|-------------------|-------------------|----------------------------|
| Microcontrôleur   | NUCLEO-L152RE     | Carte STM32                |
| Capteur           | IKS01A3           | Shield multi-capteurs      |
| Gyroscope         | LSM6DSO           | Gyroscope + Accéléromètre  |

---

## 🧰 Stack logiciel

- STM32CubeIDE `v1.x18.1`
- HAL (STM32Cube HAL Drivers)
- stm32ai-datalogger (NanoEdgeAI Studio)
- Pilotes LSM6DSO

---

## 🚀 Démarrage rapide

1. **Cloner le dépôt**
   ```bash
   git clone https://github.com/clementfvrl/STM32_Projet.git
   cd STM32_Projet

2. **Ouvrir avec STM32CubeIDE**

   * Lancer STM32CubeIDE.
   * Importer le projet avec `File > Open Projects from File System...`.

3. **Configurer la carte**

   * Vérifie que la bonne carte cible est sélectionnée (L476RG ou L152RE).
   * Vérifie les pins I²C / SPI selon ta configuration matérielle.

4. **Compiler et flasher**

   * Connecter la carte via USB.
   * Compiler avec `Project > Build`.
   * Flasher avec `Run > Debug`.

5. **Ouvir un terminal série type PuTTY ou TeraTerm**
   * Déterminer le port série via Device Manager
   * Vitesse de communication : 115200 bauds

---

## 📁 Structure du projet

```
/Core/
  └── Src/            # Fichiers source C
  └── Inc/            # Fichiers d’en-tête
/Drivers/
  └── Sensors/            # Drivers du capteur
  └── STM32L4xx_HAL/  # HAL STM32
```

---

## 👨‍💻 Auteurs

Projet développé par BOMPUIS, FAVAREL et PUTZ dans le cadre du Projet STM32 de l'ISEN Méditerranée.
