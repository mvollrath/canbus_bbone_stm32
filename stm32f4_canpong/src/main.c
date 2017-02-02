#include "setup.h"
#include "stm32f4xx_can.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_tim.h"

// counter for outgoing messages
static uint8_t led_counter = 0;

static void tx(uint8_t data) {
    CanTxMsg message;
    uint8_t mailbox;
    uint8_t status;

    message.StdId = 0x23;
    message.ExtId = 0x0;
    message.RTR = CAN_RTR_DATA;
    message.IDE = CAN_ID_STD;
    message.DLC = 1;
    message.Data[0] = data;

    mailbox = CAN_Transmit(CAN1, &message);
    status = CAN_TransmitStatus(CAN1, mailbox);
    if (status == CAN_TxStatus_Failed) {
        GPIO_SetBits(GPIOD, GPIO_Pin_14); // red on
    }
}

static void rx() {
    CanRxMsg message;
    uint8_t counter;

    while (CAN_MessagePending(CAN1, CAN_FIFO0) > 0) {
        CAN_Receive(CAN1, CAN_FIFO0, &message);

        if (message.DLC < 1) {
            GPIO_SetBits(GPIOD, GPIO_Pin_14); // red on
            return;
        }

        counter = message.Data[0];
        if (counter % 2 == 0) {
            GPIO_ResetBits(GPIOD, GPIO_Pin_15); // blue
        } else {
            GPIO_SetBits(GPIOD, GPIO_Pin_15); // blue
        }
    }
}

// timer handler
void TIM2_IRQHandler() {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        GPIO_ToggleBits(GPIOD, GPIO_Pin_13); // orange
        tx(led_counter++);
    }
}

// can message handler
void CAN1_RX0_IRQHandler() {
    if (CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET) {
        rx();
    }
}

// user button handler
void EXTI0_IRQHandler() {
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        EXTI_ClearITPendingBit(EXTI_Line0);
        GPIO_ResetBits(GPIOD, GPIO_Pin_14); // clear red
    }
}

int main() {
    if (setup() != 0) {
        GPIO_SetBits(GPIOD, GPIO_Pin_14); // red on
        return -1;
    }
    GPIO_SetBits(GPIOD, GPIO_Pin_12); // green on

    while (1) asm("wfi");

    return 0;
}
