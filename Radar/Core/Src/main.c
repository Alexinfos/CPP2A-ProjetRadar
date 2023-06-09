/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include <stdio.h>
#include "lcd16x2.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Initialisation de certaines variables
char textBuffer[60];
int textBufferLength;
int readyForCalculation = 0;

// Initialisation du timer et du compteur
uint16_t timerValue;
uint16_t newTimerValue;
uint8_t counter = 0;
int maxCounter = 10;
uint16_t timerBuffer[10];

// Fonction appelée à chaque front montant du signal
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	// On récupère la valeur actuelle du timer
	newTimerValue = __HAL_TIM_GET_COUNTER(&htim2);

	// On vérifie que l'interrupt a bien été déclenché par le bon pin
	if (GPIO_Pin == IN1_Pin) {
		// On calcule l'intervalle de temps entre les deux derniers front montants
		timerBuffer[counter] = newTimerValue - timerValue;
		timerValue = newTimerValue;

		if (counter >= maxCounter - 1) {
			// Le tableau timerBuffer est rempli, on indique à la boucle principale
			// qu'elle a assez de données pour effectuer à nouveau des calculs pour
			// l'affichage
			readyForCalculation = 1;

			// On réinitialise le compteur pour recommencer au début du tableau à la prochaine itération
			counter = 0;
		} else {
			counter++;
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  // Initialisation de certaines variables
  double measuredSpeed = 10000.0;
  int usingLastValue = 0;
  int consecutiveSkips = 10;
  int missedCalcs = 0;
  uint32_t speedIntegerPart = 0;
  uint32_t speedDecimalPart = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // Initialisation de l'afficheur LCD
  lcd16x2_init_4bits(LCD_RS_GPIO_Port, LCD_RS_Pin, LCD_E_Pin, LCD_D4_GPIO_Port, LCD_D4_Pin, LCD_D5_Pin, LCD_D6_Pin, LCD_D7_Pin);
  lcd16x2_cursorShow(0);
  lcd16x2_clear();

  // Écran de bienvenue
  lcd16x2_setCursor(0, 0);
  lcd16x2_printf("  Projet Radar  ");
  lcd16x2_setCursor(1, 0);
  lcd16x2_printf("CPP P29 - Grp D2");
  HAL_Delay(5000);
  lcd16x2_clear();

  // Initialisation du timer
  HAL_TIM_Base_Start(&htim2);
  timerValue = __HAL_TIM_GET_COUNTER(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Boucle principale
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // On vérifie qu'on a assez de valeurs pour effectuer les calculs
	if (readyForCalculation == 1) {
		missedCalcs = 0;

		// Calcul de la période moyenne (moyenne arithmétique)
		uint32_t sum = 0;
		for (uint8_t i = 0; i < maxCounter; i++) {
			sum += timerBuffer[i];
		}
		uint32_t average = sum / maxCounter;

		// Si la période moyenne est non-nulle, on peut faire les calculs suivants
		if (average != 0) {
			// Fréquence = 1 / Période moy (en s) = 1 / Période moy (en µs) * 1e-6
			double frequency = 1.0 / (average * 1e-6);

			// Si on remarque que la nouvelle vitesse mesurée est grande devant celle mesurée à
			// l'itération précédente, on continue d'afficher la vitesse précédemment mesurée.
			if (frequency / 19.49 <= 30 * measuredSpeed /*&& frequency / 19.49 <= 120*/) {
				measuredSpeed = frequency / 19.49;

				// If angle = 45°
				//measuredSpeed = (frequency / (sqrt(2.0) * (10.525e9 / 3.0e8))) * 3.6;

				// If angle = 30°
				//measuredSpeed = (frequency / (sqrt(3.0) * (10.525e9 / 3.0e8))) * 3.6;

				// If angle = 15°
				//measuredSpeed = (frequency / (((sqrt(6.0) + sqrt(2.0)) / 2.0) * (10.525e9 / 3.0e8))) * 3.6;
				usingLastValue = 1;
				consecutiveSkips = 0;
			} else {
				consecutiveSkips += 1;
				usingLastValue = 0;
			}

			// On initialise réinitialise la position du curseur
			lcd16x2_setCursor(0, 0);

			// Si on n'utilise pas la valeur mesurée lors de cette itération, on affiche
			// une petite étoile dans le coin supérieur droit.
			if (usingLastValue == 1) {
				lcd16x2_printf("Vitesse:        ");
			} else {
				lcd16x2_printf("Vitesse:       *");
			}

			// Si le bouton est pressé, on convertit la vitesse en m/s
			GPIO_PinState pinState = HAL_GPIO_ReadPin (UNIT_SWITCH_GPIO_Port, UNIT_SWITCH_Pin);
			if (pinState == 0) {
				// On convertit le nombre décimal en deux entiers pour l'affichage
				speedIntegerPart = measuredSpeed / 3.6;
				speedDecimalPart = (measuredSpeed / 3.6) * 1000 - 1000 * speedIntegerPart;
				textBufferLength = sprintf(textBuffer, "     %.3lu.%.3lu m/s", speedIntegerPart, speedDecimalPart);
			} else {
				// On convertit le nombre décimal en deux entiers pour l'affichage
				speedIntegerPart = measuredSpeed;
				speedDecimalPart = measuredSpeed * 1000 - 1000 * speedIntegerPart;
				textBufferLength = sprintf(textBuffer, "     %.2lu.%.3lu km/h", speedIntegerPart, speedDecimalPart);
			}

			// On affiche le résultat sur l'écran
			lcd16x2_setCursor(1, 0);
			lcd16x2_printf(textBuffer);
		}

		// Réinitialisation du tableau TimerBuffer
		for (uint8_t i = 0; i < maxCounter; i++) {
			timerBuffer[i] = 0;
		}

		// On indique qu'on n'est pas prêt pour la prochaine itération de l'affichage
		readyForCalculation = 0;
		counter = 0;
	} else {
		// Si la moyenne est nulle, on continue d'afficher la dernière mesure pendant 2.5 secondes
		// (sauf si celle-ci est supérieure à 100 km/h, notre radar n'étant en principe pas prévu
		// pour des utilisations avec des vitesses aussi élevées, une telle vitesse serait sans doute
		// liée à la présence de bruit dans le signal)
		if (consecutiveSkips > 2 && consecutiveSkips < 7 && measuredSpeed < 100) {
			lcd16x2_setCursor(0, 0);
			lcd16x2_printf("Derniere mesure:");

		} else if (consecutiveSkips >= 7 || measuredSpeed >= 100) {
			lcd16x2_setCursor(0, 0);
			lcd16x2_printf("                ");
			lcd16x2_setCursor(1, 0);
			lcd16x2_printf("Aucun signal !  ");
		}

		consecutiveSkips++;

		missedCalcs++;
		// Si on a loupé plus de 4 calculs (2s), on réinitialise toutes les valeurs précédemment mesurées
		if (missedCalcs > 4) {
			missedCalcs = 0;
			// Réinitialisation du tableau TimerBuffer
			for (uint8_t i = 0; i < maxCounter; i++) {
				timerBuffer[i] = 0;
			}

			if (counter > 0) {
				lcd16x2_setCursor(0, 15);
				lcd16x2_printf(">");
			}

			// On indique qu'on n'est pas prêt pour la prochaine itération de l'affichage
			readyForCalculation = 0;
			counter = 0;

			// DEBUG
			//lcd16x2_setCursor(0, 15);
			//lcd16x2_printf("R");
		}
	}

	// On itère sur cette boucle toutes les 500ms
	HAL_Delay(500);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65536-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_RS_Pin|LCD_E_Pin|LCD_D4_Pin|LCD_D5_Pin
                          |LCD_D6_Pin|LCD_D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LCD_RS_Pin LCD_E_Pin LCD_D4_Pin LCD_D5_Pin
                           LCD_D6_Pin LCD_D7_Pin */
  GPIO_InitStruct.Pin = LCD_RS_Pin|LCD_E_Pin|LCD_D4_Pin|LCD_D5_Pin
                          |LCD_D6_Pin|LCD_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : VCP_TX_Pin VCP_RX_Pin */
  GPIO_InitStruct.Pin = VCP_TX_Pin|VCP_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : UNIT_SWITCH_Pin */
  GPIO_InitStruct.Pin = UNIT_SWITCH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(UNIT_SWITCH_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IN1_Pin */
  GPIO_InitStruct.Pin = IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(IN1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */

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
