#include "stack_function_wrapper.h"

StackFunctionWrapper::StackFunctionWrapper(WrapperFunction_t pvTaskCode, void * const pvParameters, 
    const uint32_t usStackDepth, UBaseType_t uxPriority):
_fun_wrapper_queue(xQueueCreate(1, sizeof(int *))),
_context_pvTaskCode(pvTaskCode),
_context_pvParameters(pvParameters),
_context_usStackDepth(usStackDepth),
_context_uxPriority(uxPriority)
{
    
}

StackFunctionWrapper::~StackFunctionWrapper()
{
    if(_fun_wrapper_queue)
        vQueueDelete(_fun_wrapper_queue);
}

int StackFunctionWrapper::run()
{
    int result = 0;
    xTaskCreate(new_stack_fun, "new_stack_fun", _context_usStackDepth, this,
                _context_uxPriority, NULL);
    xQueueReceive(_fun_wrapper_queue, &result, portMAX_DELAY);
    return result;
}

void StackFunctionWrapper::new_stack_fun(void *pvParameters)
{
    StackFunctionWrapper *_fun_wrapper = (StackFunctionWrapper *)pvParameters;
    if(_fun_wrapper){
        int result = _fun_wrapper->_context_pvTaskCode(_fun_wrapper->_context_pvParameters);
        xQueueSend(_fun_wrapper->_fun_wrapper_queue, &result, portMAX_DELAY);
    }
    
    vTaskDelete(NULL);
}
