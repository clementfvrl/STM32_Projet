# Plan de développement détaillé - Projet SafeGuard

## 1. Préparation de l'environnement de développement

### 1.1 Installation des outils
- **STM32CubeIDE** (dernière version)
  - Télécharger depuis le site de ST Microelectronics
  - Installer avec les pilotes ST-Link
  - Configurer les packages pour la famille STM32L4
  
- **NanoEdgeAI Studio**
  - S'inscrire sur le portail ST
  - Télécharger et installer NanoEdgeAI Studio
  - Créer un compte développeur ST si nécessaire

- **Git/GitHub**
  - Créer un dépôt pour le projet
  - Configurer les collaborateurs
  - Établir une stratégie de branches (main, develop, features)

### 1.2 Documentation et ressources
- Télécharger et organiser les documentations nécessaires:
  - Manuel de référence de la carte Nucleo-L476RG
  - Datasheet du capteur LSM6DSO
  - Documentation STM32CubeMX
  - Guide d'utilisation NanoEdgeAI Studio
  - Exemples de projets HAL pour STM32L4

## 2. Configuration initiale du projet

### 2.1 Création du projet dans STM32CubeIDE
1. Lancer STM32CubeIDE
2. Créer un nouveau projet STM32:
   - Sélectionner la carte NUCLEO-L476RG
   - Nommer le projet "SafeGuard"
   - Configurer le projet en mode C/C++
   - Utiliser le framework STM32Cube HAL

### 2.2 Configuration matérielle avec CubeMX
1. **Configurer les horloges système**:
   - Régler la fréquence principale à 80 MHz
   - Configurer les horloges périphériques

2. **Configuration des périphériques**:
   
   a. **GPIO** (en mode interruption):
   - PA0: Bouton d'annulation d'alerte (EXTI, pull-up, falling edge)
   - PA1: Bouton d'alerte manuelle (EXTI, pull-up, falling edge)
   - PB0-PB2: LEDs d'état (output push-pull)
   
   b. **TIMER**:
   - TIM2: Timer d'échantillonnage (100 Hz, interruption activée)
   - TIM3: Timer de délai avant alerte (configurable, interruption activée)
   - TIM4: Timer de clignotement LED (variable, interruption activée)
   
   c. **ADC**:
   - ADC1 (canal IN1): Niveau de batterie simulé
   - Mode scan continu
   - Résolution 12 bits
   
   d. **UART**:
   - USART2: Communication série (115200 baud, 8N1)
   - Interruptions Rx/Tx activées
   
   e. **SPI** pour LSM6DSO:
   - SPI1: Communication avec LSM6DSO
   - Mode maître, full-duplex
   - Vitesse: 10 MHz
   - CPOL=0, CPHA=0 (Mode 0)
   - NSS (CS): Logiciel (GPIO PA4)

3. **Configurer les interruptions NVIC**:
   - Priorités pour les interruptions EXTI (boutons)
   - Priorités pour les Timers
   - Priorités pour SPI et UART

4. **Générer le code initial**:
   - Générer le code HAL et les fichiers d'initialisation
   - Organiser le projet en structure modulaire

## 3. Développement du driver LSM6DSO

### 3.1 Structure du driver
Créer un module pour le LSM6DSO avec les fichiers:
- `lsm6dso.h`: Déclarations et définitions
- `lsm6dso.c`: Implémentations

### 3.2 Fonctions principales du driver
```c
// Initialisation
HAL_StatusTypeDef LSM6DSO_Init(SPI_HandleTypeDef *hspi);

// Configuration
HAL_StatusTypeDef LSM6DSO_ConfigAccelerometer(uint8_t odr, uint8_t scale);
HAL_StatusTypeDef LSM6DSO_ConfigGyroscope(uint8_t odr, uint8_t scale);
HAL_StatusTypeDef LSM6DSO_EnableFreeFallDetection(uint8_t threshold, uint8_t duration);

// Lecture des données
HAL_StatusTypeDef LSM6DSO_ReadAcceleration(int16_t *x, int16_t *y, int16_t *z);
HAL_StatusTypeDef LSM6DSO_ReadGyroscope(int16_t *x, int16_t *y, int16_t *z);
HAL_StatusTypeDef LSM6DSO_ReadTemperature(int16_t *temp);

// Gestion des interruptions
HAL_StatusTypeDef LSM6DSO_ConfigInterrupts(uint8_t int_type, uint8_t int_pin);
uint8_t LSM6DSO_GetInterruptSource(void);

// Fonctions utilitaires
HAL_StatusTypeDef LSM6DSO_WriteRegister(uint8_t reg, uint8_t value);
HAL_StatusTypeDef LSM6DSO_ReadRegister(uint8_t reg, uint8_t *value);
```

### 3.3 Implémentation du driver
- Développer les fonctions de lecture/écriture SPI
- Implémenter les fonctions d'initialisation et de configuration
- Ajouter la gestion des interruptions du capteur
- Créer des fonctions de conversion des données brutes

## 4. Algorithme de détection de chute

### 4.1 Implémentation de l'algorithme de base
Créer un module pour la détection de chute avec les fichiers:
- `fall_detection.h`: Déclarations et paramètres
- `fall_detection.c`: Implémentation de l'algorithme

Algorithme simple basé sur les critères suivants:
1. Détection d'accélération importante (> seuil_impact)
2. Suivi d'une période de faible mouvement (proche de 1g vertical)
3. Analyse de l'orientation finale

```c
typedef struct {
    float impact_threshold;
    float static_threshold;
    uint32_t static_time_ms;
} FallDetection_Config;

void FallDetection_Init(FallDetection_Config config);
bool FallDetection_ProcessData(int16_t acc_x, int16_t acc_y, int16_t acc_z);
void FallDetection_Reset(void);
```

### 4.2 Machine à états de la détection
Implémenter une machine à états avec les états suivants:
1. `MONITORING`: Surveillance continue
2. `IMPACT_DETECTED`: Impact potentiel détecté
3. `ANALYZING`: Analyse du mouvement post-impact
4. `FALL_DETECTED`: Chute confirmée
5. `ALERTING`: Envoi des alertes

## 5. Collecte de données pour NanoEdgeAI

### 5.1 Développement du data logger
Créer un module pour l'enregistrement des données:
- `data_logger.h`: Déclarations et paramètres
- `data_logger.c`: Implémentation du logger

```c
void DataLogger_Init(UART_HandleTypeDef *huart);
void DataLogger_StartCapture(uint32_t duration_ms, uint8_t scenario_id);
void DataLogger_StopCapture(void);
void DataLogger_ProcessSample(int16_t acc_x, int16_t acc_y, int16_t acc_z, int16_t gyro_x, int16_t gyro_y, int16_t gyro_z);
```

### 5.2 Protocole de collecte des données
1. Définir différents scénarios:
   - Chutes normales (avant, arrière, côté)
   - Mouvements quotidiens (marche, s'asseoir, se lever)
   - Mouvements brusques non dangereux
   
2. Pour chaque scénario:
   - Capturer 10-15 échantillons
   - Étiqueter correctement les données
   - Organiser les données par catégorie

### 5.3 Utilisation de NanoEdgeAI Studio
1. Importer les données collectées
2. Sélectionner les signaux pertinents
3. Configurer les paramètres:
   - Taille de la fenêtre: 500ms à 1000ms
   - Chevauchement: 50%
   - Type de classification: binaire (chute/non-chute)
4. Entraîner le modèle
5. Évaluer les performances
6. Générer la bibliothèque

## 6. Intégration de NanoEdgeAI dans le projet

### 6.1 Intégration de la bibliothèque générée
- Ajouter les fichiers de la bibliothèque NanoEdgeAI au projet
- Configurer les paramètres d'inclusion et de compilation

### 6.2 Création du module d'intelligence artificielle
Créer un module pour l'IA avec les fichiers:
- `ai_fall_detection.h`: Interface avec NanoEdgeAI
- `ai_fall_detection.c`: Implémentation

```c
void AI_Init(void);
void AI_Reset(void);
uint8_t AI_ProcessBuffer(float *data_buffer, uint16_t size);
uint8_t AI_GetConfidence(void);
```

### 6.3 Intégration avec l'algorithme traditionnel
- Combiner l'algorithme traditionnel et le modèle NanoEdgeAI
- Implémenter un système de score pondéré ou de validation croisée
- Optimiser les seuils de détection

## 7. Système d'alerte et interface utilisateur

### 7.1 Module d'alerte
Créer un module pour la gestion des alertes:
- `alert_system.h`: Déclarations
- `alert_system.c`: Implémentation

```c
typedef enum {
    ALERT_NONE,
    ALERT_PREALERT,
    ALERT_EMERGENCY,
    ALERT_MANUAL
} AlertLevel;

void AlertSystem_Init(UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim);
void AlertSystem_SetLevel(AlertLevel level);
void AlertSystem_Cancel(void);
void AlertSystem_Process(void);
```

### 7.2 Interface utilisateur
Créer un module pour l'interface:
- `user_interface.h`: Déclarations
- `user_interface.c`: Implémentation

```c
void UI_Init(void);
void UI_UpdateLEDs(AlertLevel level);
void UI_ProcessButtonPress(uint16_t GPIO_Pin);
```

### 7.3 Interface de configuration série
Implémenter une interface de commande via UART:
- Menu de configuration
- Diagnostics système
- Calibration des capteurs
- Test manuel des fonctionnalités

## 8. Application principale

### 8.1 Structure de l'application
```c
// main.c
int main(void) {
    // Initialisation matérielle
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_ADC1_Init();
    
    // Initialisation des modules
    LSM6DSO_Init(&hspi1);
    FallDetection_Init(default_config);
    AlertSystem_Init(&huart2, &htim3);
    UI_Init();
    AI_Init();
    
    // Démarrage des timers
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim4);
    
    // Boucle principale
    while (1) {
        // Traitement des données en file d'attente
        ProcessDataQueue();
        
        // Gestion des alertes
        AlertSystem_Process();
        
        // Mode basse consommation si possible
        if (IsSystemIdle()) {
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        }
    }
}
```

### 8.2 Gestionnaire d'interruptions
Implémenter les handlers d'interruption:
- EXTI pour les boutons
- TIM2 pour l'échantillonnage du capteur
- TIM3 pour le délai d'alerte
- TIM4 pour le clignotement des LEDs
- SPI et UART pour la communication

### 8.3 Gestion de l'énergie
- Implémenter des stratégies d'économie d'énergie
- Utiliser les modes basse consommation du microcontrôleur
- Adapter la fréquence d'échantillonnage selon le contexte

## 9. Tests et validation

### 9.1 Tests unitaires
- Tester individuellement chaque module
- Vérifier les fonctionnalités de base
- Validation des interfaces

### 9.2 Tests d'intégration
- Tester les interactions entre modules
- Vérifier le fonctionnement du système complet
- Valider les scénarios d'utilisation

### 9.3 Tests de performance
- Mesurer les temps de réponse
- Évaluer la précision de la détection
- Analyser la consommation d'énergie

## 10. Finalisation et documentation

### 10.1 Optimisation du code
- Optimiser les performances
- Réduire la consommation d'énergie
- Nettoyer le code et éliminer les sections inutilisées

### 10.2 Documentation technique
- Documenter le code source
- Créer des diagrammes d'architecture
- Rédiger un guide d'utilisation

### 10.3 Préparation de la présentation
- Créer les slides de présentation
- Préparer la démonstration
- Anticiper les questions potentielles

## 11. Planning de réalisation

| Étape | Tâche | Durée estimée |
|-------|-------|---------------|
| 1 | Configuration initiale du projet | 2 jours |
| 2 | Développement du driver LSM6DSO | 3 jours |
| 3 | Algorithme de détection de chute | 3 jours |
| 4 | Collecte de données pour NanoEdgeAI | 4 jours |
| 5 | Intégration de NanoEdgeAI | 3 jours |
| 6 | Système d'alerte et interface | 2 jours |
| 7 | Application principale | 3 jours |
| 8 | Tests et validation | 4 jours |
| 9 | Finalisation et documentation | 3 jours |
