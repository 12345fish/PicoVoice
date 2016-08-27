#include <stdio.h>
#include "base.h"
#include "timebase.h"
#include "voice.h"

/*-----------------------------------*/

/*-----------------------------------*/

int main(void)
{
    TimeBase_Init();
    //LED_Init();
    
    voice_init();
    voice_adapt();
    //voice_test();
    
    while (1) {
        voice_process();
        //LED_Light(TRUE);
        //delay_ms(250);
        //LED_Light(FALSE);
        //delay_ms(250);
    }
}

/*-----------------------------------*/

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* Infinite loop */
    while (1) {
    }
}
#endif //USE_FULL_ASSERT
