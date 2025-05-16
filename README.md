# 📦 STM32 Motion Sensor Project - NUCLEO-L476RG / L152RE + IKS01A3 (LIS2DW12)

## 📋 Description

Ce projet embarqué utilise des cartes **STM32 NUCLEO-L476RG** et **NUCLEO-L152RE** avec le **shield IKS01A3**, intégrant notamment le capteur **LIS2DW12** (accéléromètre). L'objectif est de récupérer des données de mouvement (accélérations/gyroscope) à des fins de traitement ou de transmission.

> **⚠️ Note :** Le **LIS2DW12** est un accéléromètre 3 axes ultra-faible consommation. Pour exploiter des données de **gyroscope**, un autre capteur comme le **LSM6DSO** (présent sur le shield) devra être utilisé.

---

## 🛠 Matériel utilisé

| Composant         | Référence         | Rôle                       |
|-------------------|-------------------|----------------------------|
| Microcontrôleur   | NUCLEO-L476RG     | Carte STM32 principale     |
| Microcontrôleur   | NUCLEO-L152RE     | Carte STM32 secondaire     |
| Capteur           | IKS01A3           | Shield multi-capteurs      |
| Accéléromètre     | LIS2DW12          | Accéléromètre 3 axes       |
| Gyroscope         | LSM6DSO (si besoin) | Gyroscope + Accéléromètre |

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
