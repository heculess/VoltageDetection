#ifndef STACK_FUNCTION_WRAPPER_INCLUDED
#define STACK_FUNCTION_WRAPPER_INCLUDED

#include <freertos/FreeRTOS.h> 
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef int (*WrapperFunction_t)( void * );

    class StackFunctionWrapper
    {
    public:
        StackFunctionWrapper(WrapperFunction_t pvTaskCode,
            void * const pvParameters,
			const uint32_t usStackDepth = 4096,
			UBaseType_t uxPriority = configMAX_PRIORITIES - 1);
        virtual ~StackFunctionWrapper();

        int run();

    private:
        xQueueHandle _fun_wrapper_queue;
        WrapperFunction_t  _context_pvTaskCode;
        void * const _context_pvParameters;
        const uint32_t _context_usStackDepth;
        UBaseType_t _context_uxPriority;

        static void new_stack_fun(void *pvParameters); 
    };


#ifdef __cplusplus
}
#endif

#endif // STACK_FUNCTION_WRAPPER_INCLUDED