/*
 * FreeRTOS Kernel V10.3.1
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM3 port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "cm3_mcu.h"
#include "uart_drv.h"
#include "uart_stdio.h"

#ifndef configKERNEL_INTERRUPT_PRIORITY
#define configKERNEL_INTERRUPT_PRIORITY 255
#endif

#if configMAX_SYSCALL_INTERRUPT_PRIORITY == 0
#error configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.  See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html
#endif

#ifndef configSYSTICK_CLOCK_HZ
#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
/* Ensure the SysTick is clocked at the same frequency as the core. */
#define portNVIC_SYSTICK_CLK_BIT    ( 1UL << 2UL )
#else
/* The way the SysTick is clocked is not modified in case it is not the same
as the core. */
#define portNVIC_SYSTICK_CLK_BIT    ( 0 )
#endif

/* Legacy macro for backward compatibility only.  This macro used to be used to
replace the function that configures the clock used to generate the tick
interrupt (prvSetupTimerInterrupt()), but now the function is declared weak so
the application writer can override it by simply defining a function of the
same name (vApplicationSetupTickInterrupt()). */
#ifndef configOVERRIDE_DEFAULT_TICK_CONFIGURATION
#define configOVERRIDE_DEFAULT_TICK_CONFIGURATION 1    /* 0: SysTick; 1: Timer */
#endif

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG           ( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG           ( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG  ( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG                ( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT            ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT         ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT     ( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT            ( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT     ( 1UL << 25UL )

#define portNVIC_PENDSV_PRI                 ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI                ( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER     ( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16     ( 0xE000E3F0 )
#define portAIRCR_REG                       ( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE                 ( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE                 ( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS               ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK             ( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT                  ( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK                 ( 0xFFUL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR            ( 0x01000000 )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER               ( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR            ( 45UL )

/* For strict compliance with the Cortex-M spec the task start address should
have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK              ( ( StackType_t ) 0xfffffffeUL )

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Exception handlers.
 */
void xPortPendSVHandler( void );
void xPortSysTickHandler( void );
void vPortSVCHandler( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );

//void vApplicationIdleHook( void );

/*-----------------------------------------------------------*/

/* Each task maintains its own interrupt status in the critical nesting
variable. */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/*
 * The number of SysTick increments that make up one tick period.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
//static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
//static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
//static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Used by the portASSERT_IF_INTERRUPT_PRIORITY_INVALID() macro to ensure
 * FreeRTOS API functions are not called from interrupts that have been assigned
 * a priority above configMAX_SYSCALL_INTERRUPT_PRIORITY.
 */
#if ( configASSERT_DEFINED == 1 )
static uint8_t ucMaxSysCallPriority = 0;
static uint32_t ulMaxPRIGROUPValue = 0;
static const volatile uint8_t *const pcInterruptPriorityRegisters = ( uint8_t * ) portNVIC_IP_REGISTERS_OFFSET_16;
#endif /* configASSERT_DEFINED */

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
    /* Simulate the stack frame as it would be created by a context switch
    interrupt. */
    pxTopOfStack--; /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
    *pxTopOfStack = portINITIAL_XPSR;   /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( ( StackType_t ) pxCode ) & portSTART_ADDRESS_MASK;    /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError;   /* LR */

    pxTopOfStack -= 5;  /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( StackType_t ) pvParameters;   /* R0 */
    pxTopOfStack -= 8;  /* R11, R10, R9, R8, R7, R6, R5 and R4. */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

static void prvTaskExitError( void )
{
    /* A function that implements a task must not exit or attempt to return to
    its caller as there is nothing to return to.  If a task wants to exit it
    should instead call vTaskDelete( NULL ).

    Artificially force an assert() to be triggered if configASSERT() is
    defined, then stop here so application writers can catch the error. */
    configASSERT( uxCriticalNesting == ~0UL );
    portDISABLE_INTERRUPTS();
    for ( ;; );
}
/*-----------------------------------------------------------*/

void vPortSVCHandler( void )
{
    __asm volatile (
        "	ldr	r3, pxCurrentTCBConst2		\n" /* Restore the context. */
        "	ldr r1, [r3]					\n" /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
        "	ldr r0, [r1]					\n" /* The first item in pxCurrentTCB is the task top of stack. */
        "	ldmia r0!, {r4-r11}				\n" /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
        "	msr psp, r0						\n" /* Restore the task stack pointer. */
        "	isb								\n"
        "	mov r0, #0 						\n"
        "	msr	basepri, r0					\n"
        "	orr r14, #0xd					\n"
        "	bx r14							\n"
        "									\n"
        "	.align 4						\n"
        "pxCurrentTCBConst2: .word pxCurrentTCB				\n"
    );
}
/*-----------------------------------------------------------*/

static void prvPortStartFirstTask( void )
{
    __asm volatile(
        " ldr r0, =0xE000ED08 	\n" /* Use the NVIC offset register to locate the stack. */
        " ldr r0, [r0] 			\n"
        " ldr r0, [r0] 			\n"
        " msr msp, r0			\n" /* Set the msp back to the start of the stack. */
        " cpsie i				\n" /* Globally enable interrupts. */
        " cpsie f				\n"
        " dsb					\n"
        " isb					\n"
        " svc 0					\n" /* System call to start first task. */
        " nop					\n"
    );
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
BaseType_t xPortStartScheduler( void )
{
    /* configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to 0.
    See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html */
    configASSERT( configMAX_SYSCALL_INTERRUPT_PRIORITY );

#if( configASSERT_DEFINED == 1 )
    {
        volatile uint32_t ulOriginalPriority;
        volatile uint8_t *const pucFirstUserPriorityRegister = ( volatile uint8_t *const ) ( portNVIC_IP_REGISTERS_OFFSET_16 + portFIRST_USER_INTERRUPT_NUMBER );
        volatile uint8_t ucMaxPriorityValue;

        /* Determine the maximum priority from which ISR safe FreeRTOS API
        functions can be called.  ISR safe functions are those that end in
        "FromISR".  FreeRTOS maintains separate thread and ISR API functions to
        ensure interrupt entry is as fast and simple as possible.

        Save the interrupt priority value that is about to be clobbered. */
        ulOriginalPriority = *pucFirstUserPriorityRegister;

        /* Determine the number of priority bits available.  First write to all
        possible bits. */
        *pucFirstUserPriorityRegister = portMAX_8_BIT_VALUE;

        /* Read the value back to see how many bits stuck. */
        ucMaxPriorityValue = *pucFirstUserPriorityRegister;

        /* The kernel interrupt priority should be set to the lowest
        priority. */
        configASSERT( ucMaxPriorityValue == ( configKERNEL_INTERRUPT_PRIORITY & ucMaxPriorityValue ) );

        /* Use the same mask on the maximum system call priority. */
        ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY & ucMaxPriorityValue;

        /* Calculate the maximum acceptable priority group value for the number
        of bits read back. */
        ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;
        while ( ( ucMaxPriorityValue & portTOP_BIT_OF_BYTE ) == portTOP_BIT_OF_BYTE )
        {
            ulMaxPRIGROUPValue--;
            ucMaxPriorityValue <<= ( uint8_t ) 0x01;
        }

#ifdef __NVIC_PRIO_BITS
        {
            /* Check the CMSIS configuration that defines the number of
            priority bits matches the number of priority bits actually queried
            from the hardware. */
            configASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == __NVIC_PRIO_BITS );
        }
#endif

#ifdef configPRIO_BITS
        {
            /* Check the FreeRTOS configuration that defines the number of
            priority bits matches the number of priority bits actually queried
            from the hardware. */
            configASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == configPRIO_BITS );
        }
#endif

        /* Shift the priority group value back to its position within the AIRCR
        register. */
        ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
        ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;

        /* Restore the clobbered interrupt priority register to its original
        value. */
        *pucFirstUserPriorityRegister = ulOriginalPriority;
    }
#endif /* conifgASSERT_DEFINED */

    /* Make PendSV and SysTick the lowest priority interrupts. */
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

    /* Start the timer that generates the tick ISR.  Interrupts are disabled
    here already. */
    vPortSetupTimerInterrupt();

    /* Initialise the critical nesting count ready for the first task. */
    uxCriticalNesting = 0;

    /* Start the first task. */
    prvPortStartFirstTask();

    /* Should never get here as the tasks will now be executing!  Call the task
    exit error function to prevent compiler warnings about a static function
    not being called in the case that the application writer overrides this
    functionality by defining configTASK_RETURN_ADDRESS.  Call
    vTaskSwitchContext() so link time optimisation does not remove the
    symbol. */
    vTaskSwitchContext();
    prvTaskExitError();

    /* Should not get here! */
    return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* Not implemented in ports where there is nothing to return to.
    Artificially force an assert. */
    configASSERT( uxCriticalNesting == 1000UL );
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
    uxCriticalNesting++;

    /* This is not the interrupt safe version of the enter critical function so
    assert() if it is being called from an interrupt context.  Only API
    functions that end in "FromISR" can be used in an interrupt.  Only assert if
    the critical nesting count is 1 to protect against recursive calls if the
    assert function also uses a critical section. */
    if ( uxCriticalNesting == 1 )
    {
        configASSERT( ( portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK ) == 0 );
    }
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
    configASSERT( uxCriticalNesting );
    uxCriticalNesting--;
    if ( uxCriticalNesting == 0 )
    {
        portENABLE_INTERRUPTS();
    }
}
/*-----------------------------------------------------------*/

void xPortPendSVHandler( void )
{
    /* This is a naked function. */

    __asm volatile
    (
        "	mrs r0, psp							\n"
        "	isb									\n"
        "										\n"
        "	ldr	r3, pxCurrentTCBConst			\n" /* Get the location of the current TCB. */
        "	ldr	r2, [r3]						\n"
        "										\n"
        "	stmdb r0!, {r4-r11}					\n" /* Save the remaining registers. */
        "	str r0, [r2]						\n" /* Save the new top of stack into the first member of the TCB. */
        "										\n"
        "	stmdb sp!, {r3, r14}				\n"
        "	mov r0, %0							\n"
        "	msr basepri, r0						\n"
        "	bl vTaskSwitchContext				\n"
        "	mov r0, #0							\n"
        "	msr basepri, r0						\n"
        "	ldmia sp!, {r3, r14}				\n"
        "										\n" /* Restore the context, including the critical nesting count. */
        "	ldr r1, [r3]						\n"
        "	ldr r0, [r1]						\n" /* The first item in pxCurrentTCB is the task top of stack. */
        "	ldmia r0!, {r4-r11}					\n" /* Pop the registers. */
        "	msr psp, r0							\n"
        "	isb									\n"
        "	bx r14								\n"
        "										\n"
        "	.align 4							\n"
        "pxCurrentTCBConst: .word pxCurrentTCB	\n"
        ::"i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
    );
}
/*-----------------------------------------------------------*/

void xPortSysTickHandler( void )
{
    Pll_Unlock_Check();
    /* The SysTick runs at the lowest interrupt priority, so when this interrupt
    executes all interrupts must be unmasked.  There is therefore no need to
    save and then restore the interrupt mask value as its value is already
    known. */

#if (SUPPORT_FREERTOS_NEST_CRITICAL == 1)
    vPortEnterCritical();
#else
    portDISABLE_INTERRUPTS();
#endif
    {
        /* Increment the RTOS tick. */
        if ( xTaskIncrementTick() != pdFALSE )
        {
            /* A context switch is required.  Context switching is performed in
            the PendSV interrupt.  Pend the PendSV interrupt. */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }
#if (SUPPORT_FREERTOS_NEST_CRITICAL == 1)
    vPortExitCritical();
#else
    portENABLE_INTERRUPTS();
#endif
}
/*-----------------------------------------------------------*/

#if( configUSE_TICKLESS_IDLE == 1 )
void RTOS_PreSleepProcessing(uint32_t *v)
{
    timern_t *TIMER;
    uint32_t now_v;
    if (v)
    {
#if ( FREERTOS_SLEEP_WAKEUP_TIMER_ID == 3)
        TIMER = TIMER3;
#endif

#if ( FREERTOS_SLEEP_WAKEUP_TIMER_ID == 4)
        TIMER = TIMER4;
#endif

        TIMER->LOAD = (*v * 40) - 1;

        TIMER->CLEAR = 1;
        TIMER->CONTROL.bit.INT_ENABLE = 1;
        TIMER->CONTROL.bit.EN = 1;
    }
    Lpm_Enter_Low_Power_Mode();
    if (v)
    {
        TIMER->CONTROL.bit.EN = 0;
        TIMER->CONTROL.bit.INT_ENABLE = 0;
        TIMER->CLEAR = 1;    // clear interrupt REQ?
        Delay_us(250);    /*Delay for time value sync.*/
        now_v = (TIMER->VALUE / 40);

        if (now_v > *v)
        {
            now_v = 0;
        }
        *v -= now_v;
    }
}

__attribute__((weak)) void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
    TickType_t xModifiableIdleTime;

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __disable_irq();

    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if ( eTaskConfirmSleepModeStatus() == eAbortSleep )
    {
        /* Re-enable interrupts - see comments above the cpsid instruction()
        above. */
        __enable_irq();
    }
    else
    {
        /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
        set its parameter to 0 to indicate that its implementation contains
        its own wait for interrupt or wait for event instruction, and so wfi
        should not be executed again.  However, the original expected idle
        time variable must remain unmodified, so a copy is taken. */
        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING( &xModifiableIdleTime );

        configPOST_SLEEP_PROCESSING( &xExpectedIdleTime );

        vTaskStepTick( xModifiableIdleTime );

        /* Exit with interrupts enabled. */
        __enable_irq();
    }
}
#endif
/*-----------------------------------------------------------*/
/*
 * Setup the SysTick timer to generate the tick interrupts at the required
 * frequency.
 */
void vPortSetupTimerInterrupt( void )
{
    /* Stop and clear the SysTick. */
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

    /* Configure SysTick to interrupt at the requested rate. */
    portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );

#if ( configUSE_TICKLESS_IDLE == 1)
    timern_t *TIMER;

#if ( FREERTOS_SLEEP_WAKEUP_TIMER_ID == 3)
    TIMER = TIMER3;
    NVIC_DisableIRQ((IRQn_Type)(Timer3_IRQn));
    NVIC_SetPriority((IRQn_Type)(Timer3_IRQn), 1);
#endif

#if ( FREERTOS_SLEEP_WAKEUP_TIMER_ID == 4)
    TIMER = TIMER4;
    NVIC_DisableIRQ((IRQn_Type)(Timer4_IRQn));
    NVIC_SetPriority((IRQn_Type)(Timer4_IRQn), 1);
#endif
    TIMER->LOAD = 0;
    TIMER->CLEAR = 1;
    TIMER->CONTROL.reg = 0;

    TIMER->CONTROL.bit.PRESCALE = 0;
    TIMER->CONTROL.bit.MODE = 0;
    TIMER->CONTROL.bit.EN = 0;
#endif
}


/*-----------------------------------------------------------*/

#if( configASSERT_DEFINED == 1 )

void vPortValidateInterruptPriority( void )
{
    uint32_t ulCurrentInterrupt;
    uint8_t ucCurrentPriority;

    /* Obtain the number of the currently executing interrupt. */
    __asm volatile( "mrs %0, ipsr" : "=r"( ulCurrentInterrupt ) :: "memory" );

    /* Is the interrupt number a user defined interrupt? */
    if ( ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER )
    {
        /* Look up the interrupt's priority. */
        ucCurrentPriority = pcInterruptPriorityRegisters[ ulCurrentInterrupt ];

        /* The following assertion will fail if a service routine (ISR) for
        an interrupt that has been assigned a priority above
        configMAX_SYSCALL_INTERRUPT_PRIORITY calls an ISR safe FreeRTOS API
        function.  ISR safe FreeRTOS API functions must *only* be called
        from interrupts that have been assigned a priority at or below
        configMAX_SYSCALL_INTERRUPT_PRIORITY.

        Numerically low interrupt priority numbers represent logically high
        interrupt priorities, therefore the priority of the interrupt must
        be set to a value equal to or numerically *higher* than
        configMAX_SYSCALL_INTERRUPT_PRIORITY.

        Interrupts that use the FreeRTOS API must not be left at their
        default priority of zero as that is the highest possible priority,
        which is guaranteed to be above configMAX_SYSCALL_INTERRUPT_PRIORITY,
        and therefore also guaranteed to be invalid.

        FreeRTOS maintains separate thread and ISR API functions to ensure
        interrupt entry is as fast and simple as possible.

        The following links provide detailed information:
        http://www.freertos.org/RTOS-Cortex-M3-M4.html
        http://www.freertos.org/FAQHelp.html */
        configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
    }

    /* Priority grouping:  The interrupt controller (NVIC) allows the bits
    that define each interrupt's priority to be split between bits that
    define the interrupt's pre-emption priority bits and bits that define
    the interrupt's sub-priority.  For simplicity all bits must be defined
    to be pre-emption priority bits.  The following assertion will fail if
    this is not the case (if some bits represent a sub-priority).

    If the application only uses CMSIS libraries for interrupt
    configuration then the correct setting can be achieved on all Cortex-M
    devices by calling NVIC_SetPriorityGrouping( 0 ); before starting the
    scheduler.  Note however that some vendor specific peripheral libraries
    assume a non-zero priority group setting, in which cases using a value
    of zero will result in unpredictable behaviour. */
    configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
}

#endif /* configASSERT_DEFINED */

void vApplicationIdleHook( void )
{
    //system clock change
    if (SystemCoreClock != SystemFrequency)
    {
        SystemCoreClock = SystemFrequency;

        vPortSetupTimerInterrupt();
    }
}