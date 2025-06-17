# 📦 STM32_Projet - NUCLEO-L476RG / L152RE + IKS01A3 (LSM6DSO)

## 📋 Description

Ce projet embarqué utilise des cartes **STM32 NUCLEO-L476RG** et **NUCLEO-L152RE** avec le **shield IKS01A3**, intégrant notamment le capteur **LSM6DSO** (gyroscope). L'objectif est de récupérer des données de mouvement (accélérations/gyroscope) à des fins de traitement ou de transmission.

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

- STM32CubeIDE `v1.x18.1`
- HAL (STM32Cube HAL Drivers)

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

---

## 📁 Structure du projet

```
/Core/
  └── Src/            # Fichiers source C
  └── Inc/            # Fichiers d’en-tête
/Drivers/
  └── BSP/            # Drivers de la board
  └── STM32L4xx_HAL/  # HAL STM32
```

---

## ✅ À faire

* [x] Configuration I2C/SPI
* [x] Lecture LIS2DW12
* [ ] Intégration du gyroscope LSM6DSO
* [ ] Ajout de traitement de données
* [ ] Communication UART/BLE

---

## 👨‍💻 Auteurs

Projet développé par FAVAREL, PUTZ et BOMPUIS dans le cadre du Projet STM32 de l'ISEN Méditerranée.
