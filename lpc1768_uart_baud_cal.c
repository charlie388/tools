#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for MINGW32 and MINGW64
#include <math.h>

typedef struct _record_t_ {
    uint8_t DIVADDVAL;
    uint8_t MULVAL;
    double FD;
} record_t;

record_t param[150];

uint32_t gcd(uint32_t a, uint32_t b) {
    while(b!=0) {
        int t = a%b;
        a=b;  b=t;
    }
    return a;
}

int main() {
    uint8_t index;
    uint32_t pclk, baud, divider;
    
    //Due to mintty buffered for a pipe
    //We can use below setting or append each printf() with fflush(stdout)  
    setvbuf(stdout, 0, _IONBF, 0);

    param[0].DIVADDVAL = 0;
    param[0].MULVAL = 1;
    param[0].FD = 1.0;
    index=1;
    for( uint8_t divaddval=1; divaddval<=14; divaddval++) {
        for( uint8_t mulval=2; mulval<=15; mulval++) {
            if(divaddval<mulval) {
                if( gcd(divaddval, mulval)==1 ) {
                    param[index].DIVADDVAL = divaddval;
                    param[index].MULVAL = mulval;
                    //param[index].FD = ((double)mulval)/((double)(mulval+divaddval));
                    param[index].FD = ((double)(mulval+divaddval))/((double)mulval);
                    index++;
                }
            }
        }
    }

    param[index].DIVADDVAL = 0xFF;
    param[index].MULVAL = 0xFF;

    //sorting the param table base on FD
    for(uint8_t i=0; i<index; i++) {
        for(uint8_t j=i; j<index; j++) {
            if( param[i].FD > param[j].FD ) {
                uint8_t divaddval, mulval;
                double tmpFD;
                divaddval = param[i].DIVADDVAL;
                mulval = param[i].MULVAL;
                tmpFD = param[i].FD;
                param[i].DIVADDVAL = param[j].DIVADDVAL;
                param[i].MULVAL = param[j].MULVAL;
                param[i].FD = param[j].FD;
                param[j].DIVADDVAL = divaddval;
                param[j].MULVAL = mulval;
                param[j].FD = tmpFD;
            }
        }
    }

    for(index=0; ; index++) {
        if(param[index].DIVADDVAL == 0xFF) break;
        printf("%3d. MULVAL %2d, DIVADDVAL %2d, FD %lf \n", index+1, param[index].MULVAL, param[index].DIVADDVAL, param[index].FD);
    }

    printf("Enter PCLK : ");
    scanf("%d", &pclk);
    printf("Enter Baud : ");
    scanf("%d", &baud);
/*
    pclk = 25000000;
    baud = 115200;
*/
    printf("PCLK=%dHZ, Baud=%d\n", pclk, baud);
    printf("==============================================================================\n");
    //printf("divider\t wantFD\t\t closeFD\t Baud\t\t offset\t \n");
    printf("DLM\t DLL\t DIVADDVAL\t MULVAL\t Baud\t offset\t error%% \n");

    // if we use ceil() the first divider want a fractional divider (FD) is probably 0.9XXX.
    // if we use floor() the first divider want a (FD) is probably 1.XXXX.
    // The available (FD) value is between 1 and 2. 
    // If (second divider = first divider +1), the needed FD is smaller than 1
    // If (second divider = first divider -1), the needed FD is larger than 1
    // It always compare needed FD with 1 as initial minOffset. 
    // The 1 mean no fractional divider.
    divider = (uint32_t)ceil( pclk / 16.0 / baud );

    while(divider>2) {
        double wantFD, minOffsetFD, offsetFD, error;
        uint32_t realBaud, offsetBaud;
        
        wantFD = pclk / 16.0 / divider / baud;
        if( wantFD>2.0 ) break;
 
        // The param[0].FD=1 which mean NO fractional divider.
        minOffsetFD = fabs( wantFD - param[0].FD );

        for(index=1; ; index++) {

            if( (param[index].DIVADDVAL == 0xFF) || (param[index].MULVAL == 0xFF) ) {
                index--;
                break;
            }

            offsetFD = fabs ( wantFD - param[index].FD );

            if( offsetFD < minOffsetFD ) {
                minOffsetFD = offsetFD;
            } else {
                offsetFD = minOffsetFD;
                index--;
                break; 
            }

        }

        realBaud = (uint32_t)(pclk / 16.0 / divider / param[index].FD);
        offsetBaud = abs(baud-realBaud);
        error = (((double) offsetBaud ) / baud ) *100;
        //printf("%d\t %lf\t %lf\t %d\t\t %d\t \n", divider, wantFD, param[index].FD, realBaud,  offsetBaud);
        printf("%d\t %d\t %d\t\t %d\t %d\t %d\t %lf \n",divider/256, divider%256,  param[index].DIVADDVAL, param[index].MULVAL, realBaud, offsetBaud, error);

        divider--;
    }

}