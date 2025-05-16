# 📦 STM32 Motion Sensor Project - NUCLEO-L476RG / L152RE + IKS01A3 (LIS2DW12)

## 📋 Description

Ce projet embarqué utilise des cartes **STM32 NUCLEO-L476RG** et **NUCLEO-L152RE** avec le **shield IKS01A3**, intégrant notamment le capteur **LSM6DSO** (accéléromètre). L'objectif est de récupérer des données de mouvement (accélérations/gyroscope) à des fins de traitement ou de transmission.

---

## 🛠 Matériel utilisé

| Composant         | Référence         | Rôle                       |
|-------------------|-------------------|----------------------------|
| Microcontrôleur   | NUCLEO-L476RG     | Carte STM32 principale     |
| Microcontrôleur   | NUCLEO-L152RE     | Carte STM32 secondaire     |
| Capteur           | IKS01A3           | Shield multi-capteurs      |
| Accéléromètre     | LIS2DW12          | Accéléromètre 3 axes       |
| Gyroscope         | LSM6DSO           | Gyroscope + Accéléromètre  |

---

## 🧰 Stack logiciel

- STM32CubeIDE `v1.18.1`
- HAL (STM32Cube HAL Drivers)
- [X-CUBE-MEMS1](https://www.st.com/en/embedded-software/x-cube-mems1.html)

---

## 🚀 Démarrage rapide

1. **Cloner le dépôt**
   ```bash
   git clone https://github.com/clementfvrl/STM32_Projet.git
   cd STM32_Projet

2. **Ouvrir avec STM32CubeIDE**
    Lancer STM32CubeIDE.
    Importer le projet avec File > Open Projects from File System....

3. **Configurer la carte**
    Vérifie que la bonne carte cible est sélectionnée (L476RG ou L152RE).
    Vérifie les pins I²C / SPI selon ta configuration matérielle.

4. **Compiler et flasher**
    Connecter la carte via USB.
    Compiler avec Project > Build.
    Flasher avec Run > Debug.
