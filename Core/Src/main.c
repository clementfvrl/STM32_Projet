/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "lsm6dso_reg.h"
#include "cycle_dwt.h"
#include "NanoEdgeAI.h"
#include "knowledge.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/************************************************************ NEAI algorithm defines begin ************************************************************/
/************************************************************ Global settings part ************************************************************/
#ifndef AXIS
#define AXIS                          3                       /* Axis should be defined between 1 and 3 */
#endif
#ifndef SAMPLES
#define SAMPLES                       128                     /* Should be between 16 & 4096 */
#endif
#define MAX_FIFO_SIZE                   256                     /* The maximum number of data we can get in the FIFO is 512 but here we define max to 256 for our need */
#define FIFO_FULL                       512                     /* FIFO full size */
#define FIFO_WORD                       7                       /* FIFO word size composed of 1 byte which is identification tag & 6 bytes of fixed data */
/************************************************************ Sensor type part ************************************************************/
#define ACCELEROMETER                                           /* Could be either ACCELEROMETER or GYROSCOPE */
/************************************************************ Sensors configuration part ************************************************************/
#ifdef ACCELEROMETER
#ifndef ACCELEROMETER_ODR
#define ACCELEROMETER_ODR           LSM6DSO_XL_ODR_1667Hz   /* Should be between LSM6DSO_XL_ODR_12Hz5 and LSM6DSO_XL_ODR_6667Hz */
#endif
#ifndef ACCELEROMETER_FS
#define ACCELEROMETER_FS            LSM6DSO_2g             /* Should be between LSM6DSO_2g and LSM6DSO_16g */
#endif
#else
#ifndef GYROSCOPE_ODR
#define GYROSCOPE_ODR               LSM6DSO_GY_ODR_1667Hz   /* Should be between LSM6DSO_GY_ODR_12Hz5 and LSM6DSO_GY_ODR_6667Hz */
#endif
#ifndef GYROSCOPE_FS
#define GYROSCOPE_FS                LSM6DSO_2000dps         /* Should be between LSM6DSO_125dps and LSM6DSO_2000dps */
#endif
#endif
/************************************************************ Datalogger / NEAI mode part ************************************************************/
#ifndef NEAI_MODE
#define NEAI_MODE                     1                       /* 0: Datalogger mode, 1: NEAI functions mode */
#endif
/************************************************************ NEAI algorithm defines end ************************************************************/

// DETECTION DE CHUTE - PARAMETRES
#define FALL_DETECTION_THRESHOLD_IMPACT      95    // Probabilité minimale pour "Tomber"
#define MIN_CONSECUTIVE_FALLS                2     // Besoin de 2 détections consécutives
#define FALL_RECOVERY_THRESHOLD              60    // Si Tomber < 60%, reset compteur
#define MAX_FALL_SEQUENCE_TIME_MS           3000   // 3 secondes max pour séquence de chute
#define POST_FALL_DURATION_MS               10000  // 10 secondes post-chute
#define DEBUG_FALL_DETECTION				0	   // Mode débug 0 pour désactivé, 1 pour activé
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static uint8_t whoamI, rst;
uint16_t neai_id_class = 0;
uint8_t neai_state = 0;
volatile uint8_t drdy = 0;
uint16_t data_left = (uint16_t) SAMPLES, number_read = 0, neai_buffer_ptr = 0;
float neai_time = 0.0;
static float neai_buffer[AXIS * SAMPLES] = {0.0};
static float neai_output_buffer[CLASS_NUMBER] = {0.0};

const char *id2class[CLASS_NUMBER + 1] = {
		"unknown",
		"Tomber",
		"Pas_bouger",
		"Marche",
};

// DETECTION DE CHUTE - VARIABLES
uint8_t fall_detected = 0;
uint32_t last_fall_time = 0;
#define FALL_COOLDOWN_MS 5000  // 5 secondes entre détections
static uint8_t consecutive_fall_count = 0;
static uint32_t first_fall_time = 0;

// Variables mode post-chute (déplacées ici depuis la fonction)
static uint8_t post_fall_mode = 0;
static uint32_t post_fall_start_time = 0;

static uint32_t total_classifications = 0;
static uint32_t false_positives_avoided = 0;

stmdev_ctx_t dev_ctx;

int alarme=0;
char tempo;
int debounce_flag =0, sequence =0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static void lsm6dso_initialize(void);
static void lsm6dso_initialize_basics(void);
static void lsm6dso_initialize_fifo(void);
static void lsm6dso_get_buffer_from_fifo(uint16_t nb);
static float lsm6dso_convert_gyro_data_to_mdps(int16_t gyro_raw_data);
static float lsm6dso_convert_accel_data_to_mg(int16_t accel_raw_data);
static void iks01a3_i2c_stuck_quirk(void);

static void check_fall_detection(void);
static void trigger_fall_alert(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void buzzer (uint8_t buzzerState)
{
	if(buzzerState)
	{
		HAL_TIM_PWM_Start_IT(&htim3, TIM_CHANNEL_2);
		printf("Buzzer ON\r\n");
	}
	else
	{
		HAL_TIM_PWM_Stop_IT(&htim3, TIM_CHANNEL_2);
		printf("Buzzer OFF\r\n");
	}
}

void procedure_alarme (void)
{
	tempo = 0x38;
	while (alarme ==1 && tempo>0x30)
	{
		MAX7219_DisplayChar(1, tempo);
		MAX7219_DisplayChar(2, 'S');
		MAX7219_DisplayChar(3, 'E');
		MAX7219_DisplayChar(4, 'C');
		TIM3->ARR = 1032;
		TIM3->CCR2 = TIM3->ARR /2;
		buzzer(1);
		HAL_Delay(500);
		buzzer(0);
		HAL_Delay(500);
		tempo--;
	}

	if (alarme ==1) alarme =2;

	while (alarme ==2)
	{
		TIM3->ARR = 206;
		TIM3->CCR2 = TIM3->ARR/2;
		buzzer(1);
		MAX7219_DisplayChar(1, 'A');
		MAX7219_DisplayChar(2, 'L');
		MAX7219_DisplayChar(3, 'R');
		MAX7219_DisplayChar(4, 'T');
	}
	buzzer(0);
	MAX7219_Clear();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	/* Initialize mems driver interface */
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &hi2c1;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	iks01a3_i2c_stuck_quirk();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_SPI1_Init();
  MAX7219_Init();
  /* USER CODE BEGIN 2 */
	KIN1_InitCycleCounter();
	KIN1_EnableCycleCounter();
	lsm6dso_initialize();
	if (NEAI_MODE) {
		neai_state = neai_classification_init(knowledge);
		printf("Initialize NEAI Classification library. NEAI init return: %d.\r\n", neai_state);

		if (neai_state != NEAI_OK) {
			printf("ERREUR: Échec d'initialisation NanoEdgeAI!\r\n");
			// Gestion d'erreur appropriée
		}

		printf("SafeGuard - Système de détection de chute activé\r\n");
		printf("Classes disponibles: %s, %s, %s\r\n",
				id2class[1], id2class[2], id2class[3]);
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		uint8_t wtm_flag = 0, status2 = 0;
		uint16_t num = 0;
		if (drdy) {
			/* Reset data ready condition */
			drdy = 0;
			lsm6dso_read_reg(&dev_ctx, LSM6DSO_FIFO_STATUS2, &status2, 1);
			wtm_flag = status2 >> 7;
			if (wtm_flag) {
				lsm6dso_fifo_data_level_get(&dev_ctx, &num);
				if (data_left < num) {
					num = data_left;
				}
				lsm6dso_get_buffer_from_fifo(num);
				data_left -= num;
				number_read += num;
				if (data_left == 0) {
					lsm6dso_fifo_mode_set(&dev_ctx, LSM6DSO_BYPASS_MODE);
#if NEAI_MODE
					uint32_t cycles_cnt = 0;

					// CHANGÉ: Classification au lieu de learn/detect
					KIN1_ResetCycleCounter();
					neai_state = neai_classification(neai_buffer, neai_output_buffer, &neai_id_class);
					cycles_cnt = KIN1_GetCycleCounter();
					neai_time = (cycles_cnt * 1000000.0) / HAL_RCC_GetSysClockFreq();

					// Affichage des résultats
					printf("Classe détectée: %s (ID: %d, Confiance: %.1f%%).\r\n",
							id2class[neai_id_class], neai_id_class, neai_output_buffer[neai_id_class] * 100.0f);
					printf("NEAI classification return: %d. Cycles: %ld = %.1f µs.\r\n",
							neai_state, cycles_cnt, neai_time);

					// Affichage détaillé des probabilités
					printf("Probabilités: \r\n");
					for(int i = 1; i <= CLASS_NUMBER; i++) {
						printf("%s=%.1f%% ", id2class[i], neai_output_buffer[i-1] * 100.0f);
					}
					printf("\r\n");

					// AJOUTÉ: Vérification de détection de chute
					check_fall_detection();
#else
					// Mode datalogger - affichage des données brutes
					for (uint16_t i = 0; i < AXIS * SAMPLES; i++) {
						printf("%.3f ", neai_buffer[i]);
					}
					printf("\r\n");
#endif
					// Reset pour le prochain cycle
					data_left = (uint16_t) SAMPLES;
					number_read = 0;
					memset(neai_buffer, 0x00, AXIS * SAMPLES * sizeof(float));

					if (SAMPLES <= MAX_FIFO_SIZE) {
						lsm6dso_fifo_watermark_set(&dev_ctx, (uint16_t) SAMPLES);
					}
					else {
						lsm6dso_fifo_watermark_set(&dev_ctx, (uint16_t) MAX_FIFO_SIZE);
					}
					lsm6dso_fifo_mode_set(&dev_ctx, LSM6DSO_FIFO_MODE);
				}
				else if (data_left < MAX_FIFO_SIZE) {
					lsm6dso_fifo_watermark_set(&dev_ctx, data_left);
				}
			}
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 31;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1032;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 516;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 31999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 200;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(L0_GPIO_Port, L0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : B1_Pin BTN3_Pin */
  GPIO_InitStruct.Pin = B1_Pin|BTN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : L0_Pin */
  GPIO_InitStruct.Pin = L0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(L0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_CS_Pin */
  GPIO_InitStruct.Pin = SPI_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN1_Pin BTN2_Pin */
  GPIO_InitStruct.Pin = BTN1_Pin|BTN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : GYRO_ACC_INT_Pin */
  GPIO_InitStruct.Pin = GYRO_ACC_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GYRO_ACC_INT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief  Vérifie si une chute a été détectée avec filtres anti-faux-positifs et anti-cascade
 * @retval None
 */
static void check_fall_detection(void)
{
	uint32_t current_time = HAL_GetTick();

	// === FILTRE COOLDOWN GLOBAL ===
	// Éviter les détections multiples rapprochées
	if (current_time - last_fall_time < FALL_COOLDOWN_MS) {
		return;
	}

	// === MODE POST-CHUTE ===
	// Ignorer toute détection pendant 10s après une chute confirmée
	if (post_fall_mode) {
		if (current_time - post_fall_start_time > POST_FALL_DURATION_MS) {
			post_fall_mode = 0;
			printf("🟢 Fin mode post-chute, surveillance normale reprise\r\n");
		} else {
			printf("⏳ Mode post-chute actif (%lu/%d ms)\r\n",
					current_time - post_fall_start_time, POST_FALL_DURATION_MS);
			return;
		}
	}

	float tomber_prob = neai_output_buffer[0] * 100.0f;
	float pas_bouger_prob = neai_output_buffer[1] * 100.0f;
	float marche_prob = neai_output_buffer[2] * 100.0f;

#ifdef DEBUG_FALL_DETECTION
	printf("DEBUG - Classe: %d (%s) | Tomber: %.1f%% | Pas_bouger: %.1f%% | Marche: %.1f%% | Count: %d\r\n",
			neai_id_class, id2class[neai_id_class], tomber_prob, pas_bouger_prob, marche_prob, consecutive_fall_count);
#endif

	// === FILTRE 1: SEUIL PLUS STRICT ===
	if (neai_id_class == 1 && tomber_prob > FALL_DETECTION_THRESHOLD_IMPACT) {

		// Première détection de chute potentielle
		if (consecutive_fall_count == 0) {
			first_fall_time = current_time;
		}

		consecutive_fall_count++;
		printf("🟡 Chute potentielle %d/%d (%.1f%% > %.1f%%)\r\n",
				consecutive_fall_count, MIN_CONSECUTIVE_FALLS, tomber_prob, (float)FALL_DETECTION_THRESHOLD_IMPACT);

		// === FILTRE 2: DÉTECTIONS CONSÉCUTIVES ===
		if (consecutive_fall_count >= MIN_CONSECUTIVE_FALLS) {

			// === FILTRE 3: DURÉE RAISONNABLE DE SÉQUENCE ===
			uint32_t sequence_duration = current_time - first_fall_time;
			if (sequence_duration <= MAX_FALL_SEQUENCE_TIME_MS) {

				// === VRAIE CHUTE DÉTECTÉE ===
				fall_detected = 1;
				last_fall_time = current_time;
				consecutive_fall_count = 0; // Reset pour prochaine fois

				printf("\n🚨 === ALERTE CHUTE CONFIRMÉE === 🚨\r\n");
				printf("Probabilité finale: %.1f%%\r\n", tomber_prob);
				printf("Détections consécutives: %d\r\n", MIN_CONSECUTIVE_FALLS);
				printf("Durée séquence: %lu ms\r\n", sequence_duration);

				trigger_fall_alert();

				// === ACTIVATION MODE POST-CHUTE ===
				post_fall_mode = 1;
				post_fall_start_time = current_time;
				printf("⏳ Mode post-chute activé pour %d secondes\r\n", POST_FALL_DURATION_MS/1000);

				return; // Sortir immédiatement pour éviter cascade

			} else {
				// Séquence trop longue = probablement faux positif
				printf("❌ Séquence trop longue (%lu ms), reset\r\n", sequence_duration);
				consecutive_fall_count = 0;
			}
		}
	}
	// === RESET DU COMPTEUR ===
	else if (neai_id_class == 1 && tomber_prob < FALL_RECOVERY_THRESHOLD) {
		// Tomber détecté mais confiance trop faible = reset
		if (consecutive_fall_count > 0) {
			printf("🔄 Reset compteur chute (%.1f%% < %.1f%%)\r\n", tomber_prob, (float)FALL_RECOVERY_THRESHOLD);
			false_positives_avoided++;
		}
		consecutive_fall_count = 0;
	}
	else if (neai_id_class != 1) {
		// Autre classe détectée = reset total
		if (consecutive_fall_count > 0) {
			printf("🔄 Reset compteur chute (classe: %s)\r\n", id2class[neai_id_class]);
			false_positives_avoided++;
		}
		consecutive_fall_count = 0;

		// Affichage status normal (seulement si pas en mode post-chute)
		switch(neai_id_class) {
		case 2: // Pas bouger
			printf("🟢 Statut: Immobile (%.1f%%)\r\n", pas_bouger_prob);
			break;
		case 3: // Marche
			printf("🔵 Statut: En mouvement/Marche (%.1f%%)\r\n", marche_prob);
			break;
		default:
			printf("❓ Classe inconnue: %d\r\n", neai_id_class);
			break;
		}
	}

	// === TIMEOUT DE SÉQUENCE ===
	if (consecutive_fall_count > 0 && (current_time - first_fall_time) > MAX_FALL_SEQUENCE_TIME_MS) {
		printf("⏰ Timeout séquence chute, reset\r\n");
		false_positives_avoided++;
		consecutive_fall_count = 0;
	}

	total_classifications++;
}

/**
 * @brief  Déclenche l'alerte de chute (LED, buzzer, transmission, etc.)
 * @retval None
 */
static void trigger_fall_alert(void)
{
	// Allumer la LED d'alerte
	HAL_GPIO_WritePin(L0_GPIO_Port, L0_Pin, GPIO_PIN_SET);

	// TODO: Ajouter ici:
	// - Activation buzzer/sirène
	alarme = 1;
	procedure_alarme();
	// - Envoi SMS/notification
	// - Transmission radio/LoRa/WiFi
	// - Logs dans mémoire

	printf("🚨 Procédures d'urgence activées!\r\n");
	printf("Timestamp: %lu ms\r\n", HAL_GetTick());

	// Simulation temporisation d'alerte (à remplacer par vraie logique)
	HAL_Delay(1000);
	HAL_GPIO_WritePin(L0_GPIO_Port, L0_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  Redirecting stdout to USART2 which is connected on the STLINK port
 * @retval
 * @param
 */
int __io_putchar(int ch)
{
	uint8_t c[1];
	c[0] = ch & 0x00FF;
	HAL_UART_Transmit(&huart2, &*c, 1, 10);
	return ch;
}

/**
 * @brief  EXTI line rising detection callback.
 * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (!debounce_flag)
		{
			__HAL_TIM_SET_COUNTER(&htim6, 0);
			HAL_TIM_Base_Start_IT(&htim6);

			switch (GPIO_Pin)
			{
			case (B1_Pin):
					if (alarme ==1) alarme =0;
					break;

			case (BTN1_Pin):
					if (sequence ==0 && alarme ==2) sequence =1;
					else sequence =0;
					break;

			case (BTN2_Pin):
					if (sequence ==0 && alarme ==2) sequence =2;
					else sequence =0;
					break;

			case (BTN3_Pin):
					if (sequence ==0 && alarme ==2)
						{
							sequence =3;
							alarme = 0;
						}
					else sequence =0;
					break;
			}
		}

	switch(GPIO_Pin) {
		case GYRO_ACC_INT_Pin:
			drdy = 1;
			break;


	}
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
		uint16_t len)
{
	HAL_I2C_Mem_Write(handle, LSM6DSO_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);
	return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
		uint16_t len)
{
	HAL_I2C_Mem_Read(handle, LSM6DSO_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
	return 0;
}

/*
 * @brief  Initialize LSM6DSO sensor interface
 *
 * @param  No
 *
 * @return No
 *
 */
static void lsm6dso_initialize()
{
	lsm6dso_initialize_basics();
#ifdef ACCELEROMETER
	/* Accelelerometer configuration */
	lsm6dso_xl_data_rate_set(&dev_ctx, ACCELEROMETER_ODR);
	lsm6dso_xl_full_scale_set(&dev_ctx, ACCELEROMETER_FS);
#else
	/* Gyroscope configuration */
	lsm6dso_gy_data_rate_set(&dev_ctx, GYROSCOPE_ODR);
	lsm6dso_gy_full_scale_set(&dev_ctx, GYROSCOPE_FS);
#endif
	lsm6dso_initialize_fifo();
}

/*
 * @brief  Initialize LSM6DSO basics
 *
 * @param  No
 *
 * @return No
 *
 */
static void lsm6dso_initialize_basics()
{
	/* Check device ID */
	whoamI = 0;

	do {
		HAL_Delay(20);
		lsm6dso_device_id_get(&dev_ctx, &whoamI);
	} while(whoamI != LSM6DSO_ID);

	/* Restore default configuration */
	lsm6dso_reset_set(&dev_ctx, PROPERTY_ENABLE);

	do {
		lsm6dso_reset_get(&dev_ctx, &rst);
	} while (rst);

	/* Disable I3C interface */
	lsm6dso_i3c_disable_set(&dev_ctx, LSM6DSO_I3C_DISABLE);
}

/*
 * @brief  Initialize LSM6DSO internal FIFO
 *
 * @param  No
 *
 * @return No
 *
 */
static void lsm6dso_initialize_fifo()
{
#ifdef ACCELEROMETER
	/* Batch odr config */
	lsm6dso_fifo_xl_batch_set(&dev_ctx, ACCELEROMETER_ODR);
	lsm6dso_fifo_gy_batch_set(&dev_ctx, 0);
#else
	/* Batch odr config */
	lsm6dso_fifo_xl_batch_set(&dev_ctx, 0);
	lsm6dso_fifo_gy_batch_set(&dev_ctx, GYROSCOPE_ODR);
#endif
	/* FIFO MODE */
	lsm6dso_fifo_mode_set(&dev_ctx, LSM6DSO_FIFO_MODE);
	/* Watermark config */
	if (SAMPLES <= MAX_FIFO_SIZE) {
		lsm6dso_fifo_watermark_set(&dev_ctx, (uint16_t) SAMPLES);
	}
	else {
		lsm6dso_fifo_watermark_set(&dev_ctx, (uint16_t) MAX_FIFO_SIZE);
	}
	/* Need to enable interrupt pin when wtm is reached */
	uint8_t ctrl = 0x08;
	lsm6dso_write_reg(&dev_ctx, LSM6DSO_INT1_CTRL, (uint8_t *) &ctrl, 1);
}

/*
 * @brief  Get accelerometer or gyroscope data from
 *         LSM6DSO using the internal FIFO buffer
 *
 * @param  No
 *
 * @return No
 *
 */
static void lsm6dso_get_buffer_from_fifo(uint16_t nb)
{
	uint8_t reg_tag = 0;
	uint8_t buff_tmp[nb * FIFO_WORD];
	/*
	 * The data stored in FIFO are accessible from dedicated registers and each FIFO word is composed of 7
	 * bytes: one tag byte (FIFO_DATA_OUT_TAG (78h)), in order to identify the sensor, and 6 bytes of fixed data
	 * (FIFO_DATA_OUT registers from (79h) to (7Eh))
	 * So, here we read the fifo in only one transaction in order to save time
	 */
	lsm6dso_read_reg(&dev_ctx, LSM6DSO_FIFO_DATA_OUT_TAG, buff_tmp, nb * FIFO_WORD);
	for (uint16_t i = 0; i < nb; i++) {
		/* According to the datasheet, the TAG_SENSOR is the 5 MSB of the FIFO_DATA_OUT_TAG register, so we shift 3 bits to the right */
		reg_tag = buff_tmp[FIFO_WORD * i] >> 3;
		if(reg_tag == LSM6DSO_XL_NC_TAG) {
			for(uint8_t j = 0; j < AXIS; j++) {
				neai_buffer[(AXIS * neai_buffer_ptr) + (AXIS * i) + j] = lsm6dso_convert_accel_data_to_mg((uint16_t) buff_tmp[(FIFO_WORD * i) + (2 * j) + 2] << 8 | buff_tmp[(FIFO_WORD * i) + (2 * j) + 1]);
			}
		}
		else if(reg_tag == LSM6DSO_GYRO_NC_TAG) {
			for(uint8_t j = 0; j < AXIS; j++) {
				neai_buffer[(AXIS * neai_buffer_ptr) + (AXIS * i) + j] = lsm6dso_convert_gyro_data_to_mdps((uint16_t) buff_tmp[(FIFO_WORD * i) + (2 * j) + 2] << 8 | buff_tmp[(FIFO_WORD * i) + (2 * j) + 1]);
			}
		}
	}
	neai_buffer_ptr += nb;
	if (neai_buffer_ptr == SAMPLES) {
		neai_buffer_ptr = 0;
	}
}

/*
 * @brief  Convert gyroscope raw data to milli degrees per second (mdps)
 *
 * @param  gyro_raw_data: which is gyroscope raw data
 *                        depending on the full scale selected
 *
 * @return The converted value in milli degrees per second (mdps)
 *
 */
static float lsm6dso_convert_gyro_data_to_mdps(int16_t gyro_raw_data)
{
	float gyro_data_mdps = 0.0;
#ifdef GYROSCOPE
	switch (GYROSCOPE_FS)
	{
	case LSM6DSO_125dps:
		gyro_data_mdps = lsm6dso_from_fs125_to_mdps(gyro_raw_data);
		break;
	case LSM6DSO_250dps:
		gyro_data_mdps = lsm6dso_from_fs250_to_mdps(gyro_raw_data);
		break;
	case LSM6DSO_500dps:
		gyro_data_mdps = lsm6dso_from_fs500_to_mdps(gyro_raw_data);
		break;
	case LSM6DSO_1000dps:
		gyro_data_mdps = lsm6dso_from_fs1000_to_mdps(gyro_raw_data);
		break;
	case LSM6DSO_2000dps:
		gyro_data_mdps = lsm6dso_from_fs2000_to_mdps(gyro_raw_data);
		break;
	default:
		gyro_data_mdps = 0.0;
		break;
	}
#endif
	return gyro_data_mdps;
}

/*
 * @brief  Convert accelerometer raw data to milli-G' (mg)
 *
 * @param  accel_raw_data: which is accelerometer raw data
 *                        depending on the full scale selected
 *
 * @return The converted value in milli-G' (mg)
 *
 */
static float lsm6dso_convert_accel_data_to_mg(int16_t accel_raw_data)
{
	float accel_data_mg = 0.0;
#ifdef ACCELEROMETER
	switch (ACCELEROMETER_FS)
	{
	case LSM6DSO_2g:
		accel_data_mg = lsm6dso_from_fs2_to_mg(accel_raw_data);
		break;
	case LSM6DSO_4g:
		accel_data_mg = lsm6dso_from_fs4_to_mg(accel_raw_data);
		break;
	case LSM6DSO_8g:
		accel_data_mg = lsm6dso_from_fs8_to_mg(accel_raw_data);
		break;
	case LSM6DSO_16g:
		accel_data_mg = lsm6dso_from_fs16_to_mg(accel_raw_data);
		break;
	default:
		accel_data_mg = 0.0;
		break;
	}
#endif
	return accel_data_mg;
}

/*
 * Pressing the reset button while the sensor is answering to a read request
 * might lead to disaster.
 * In this case the device is stuck, waiting for pulses on SCL to finish the
 * previous transfer.
 * While stuck the sensor keep the SDA low.
 *
 * As a workaround we simply configure the SCL pin as a GPIO and send a burst
 * of pulses to bring the sensor back to an idle state.
 */
static void iks01a3_i2c_stuck_quirk(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Configure SCL as a GPIO */
	GPIO_InitStruct.Pin = SCL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
	HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

	/* Send a burst of pulses on SCL */
	int pulses = 20;
	do {
		HAL_Delay(1);
		HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);
	} while (pulses--);

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_DISABLE();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
