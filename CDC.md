# Cahier des Charges - Projet "SafeGuard"

## 1. Présentation du projet

**Nom du projet** : SafeGuard  
**Type** : Système de détection de chutes pour personnes âgées  
**Équipe** : FAVAREL, PUTZ, BOMPUIS

### 1.1 Contexte et problématique

Chaque année, environ un tiers des personnes âgées de plus de 65 ans subissent une chute. Ces accidents représentent la principale cause d'hospitalisation pour traumatisme chez les seniors et peuvent entraîner une perte d'autonomie significative. L'enjeu majeur réside dans la rapidité d'intervention après une chute, particulièrement pour les personnes vivant seules.

### 1.2 Objectifs du projet

Développer un dispositif portable basé sur la carte Nucleo-L476RG capable de :
- Détecter automatiquement une chute en temps réel
- Différencier une chute réelle d'autres mouvements (faux positifs)
- Déclencher une alerte après un temps configurable sans mouvement post-chute
- Permettre à l'utilisateur d'annuler l'alerte (en cas de fausse détection)
- Transmettre des informations de localisation et d'état

## 2. Spécifications techniques

### 2.1 Matériel utilisé
- Carte Nucleo-L476RG avec microcontrôleur STM32
- Shield avec capteur LSM6DSO (accéléromètre/gyroscope 6 axes)
- 2 boutons poussoirs pour l'interface utilisateur
- LEDs pour l'indication visuelle des états du système
- Éventuellement, un buzzer pour signalisation sonore (optionnel)

### 2.2 Fonctionnalités logicielles
- Algorithme de détection de chute basé sur les données du LSM6DSO
- Traitement des données par NanoEdgeAI pour réduire les faux positifs
- Système d'état à plusieurs niveaux (surveillance, détection, pré-alerte, alerte)
- Interface de configuration et de diagnostic via terminal série
- Transmission d'alertes via UART (simulant une communication externe)
- Gestion d'énergie pour optimiser l'autonomie de la batterie

### 2.3 Périphériques STM32 utilisés
- **GPIO** (en interruption) : 
  - Bouton d'annulation d'alerte
  - Bouton d'alerte manuelle
  - LEDs d'état

- **TIMER** (en interruption) : 
  - Timer pour la période d'échantillonnage de l'accéléromètre
  - Timer pour le délai avant déclenchement de l'alerte après détection
  - Timer pour le clignotement des LEDs

- **ADC** : 
  - Surveillance de niveau de batterie simulé via potentiomètre

- **UART** : 
  - Communication série pour debugging et configuration
  - Simulation de l'envoi des alertes (printf)

- **SPI** : 
  - Communication avec le capteur LSM6DSO

### 2.4 Utilisation de NanoEdgeAI Studio
- Collecte de données d'entraînement pour différents scénarios :
  - Chutes réelles (simulées de façon sécurisée)
  - Mouvements quotidiens normaux
  - Mouvements brusques mais non dangereux
- Création d'un modèle de classification pour distinguer les vraies chutes
- Intégration du modèle dans le firmware

## 3. Architecture logicielle

### 3.1 Structure du code
- Module de gestion des capteurs (driver LSM6DSO)
- Module de détection d'événements (algorithmes + NanoEdgeAI)
- Module de gestion des alertes et notifications
- Module d'interface utilisateur (boutons, LEDs)
- Module de configuration et diagnostic

### 3.2 Machine à états principale
1. **État d'initialisation** : Configuration des périphériques et du capteur
2. **État de surveillance** : Monitoring continu des mouvements
3. **État de détection** : Analyse d'un événement potentiel de chute
4. **État de pré-alerte** : Délai avant alerte permettant l'annulation
5. **État d'alerte** : Envoi des signaux d'urgence
6. **État de configuration** : Paramétrage du système via interface série

## 4. Plan de développement

### 4.1 Étapes du projet
1. Portage initial du code sur la carte Nucleo-L476RG
2. Mise en place des communications avec le capteur LSM6DSO
3. Développement des algorithmes de base de détection de chute
4. Collecte de données pour NanoEdgeAI et création du modèle
5. Intégration du modèle NanoEdgeAI
6. Implémentation des systèmes d'alerte
7. Test et optimisation de la fiabilité
8. Finalisation et documentation

### 4.2 Tests et validation
- Tests unitaires des différents modules
- Tests d'intégration du système complet
- Tests réels avec différents scénarios de chute et mouvements quotidiens
- Validation de la fiabilité (minimisation des faux positifs et faux négatifs)
- Validation de la conformité avec le cahier des charges

## 5. Livrables

- Présentation (5-6 slides) expliquant le projet, sa mise en œuvre et les défis relevés
- Maquette fonctionnelle démontrant les capacités du système
- Code source complet et documenté sur GitHub
- Fichier README détaillé expliquant :
  - Installation et compilation du code
  - Utilisation du système
  - Architecture technique
  - Description des algorithmes implémentés
  - Résultats des tests de performance

## 6. Défis techniques anticipés

- Calibration fine des seuils de détection pour limiter les faux positifs
- Optimisation de l'algorithme pour fonctionner en temps réel avec des ressources limitées
- Fiabilité du système en conditions réelles (mouvements imprévisibles, variations d'orientation)
- Gestion efficace de l'énergie pour prolonger l'autonomie
- Adaptation du système à différents profils d'utilisateurs

## 7. Critères de réussite

- Taux de détection des vraies chutes > 95%
- Taux de faux positifs < 5%
- Temps de réaction du système < 500ms
- Délai configurable avant alerte de 5s à 30s
- Interface utilisateur intuitive et fiable
