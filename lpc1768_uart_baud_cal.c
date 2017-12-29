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
    uint8_t DIVADDVAL, MULVAL, index;
    double tmpFractional;
    uint32_t pclk, baud, divider;
    
    //Due to mintty buffered for a pipe
    //We can use below setting or append each printf() with fflush(stdout)  
    setvbuf(stdout, 0, _IONBF, 0);

    index=0;
    for( DIVADDVAL=1; DIVADDVAL<=14; DIVADDVAL++) {
        for( MULVAL=2; MULVAL<=15; MULVAL++) {
            if(DIVADDVAL<MULVAL) {
                if( gcd(DIVADDVAL, MULVAL)==1 ) {
                    param[index].DIVADDVAL = DIVADDVAL;
                    param[index].MULVAL = MULVAL;
                    //param[index].FD = ((double)MULVAL)/((double)(MULVAL+DIVADDVAL));
                    param[index].FD = ((double)(MULVAL+DIVADDVAL))/((double)MULVAL);
                    index++;
                }
            }
        }
    }

    param[index].DIVADDVAL = 0xFF;
    param[index].MULVAL = 0xFF;

    for(int i=0; i<index; i++) {
        for(int j=i; j<index; j++) {
            if( param[i].FD > param[j].FD ) {
                DIVADDVAL = param[i].DIVADDVAL;
                MULVAL = param[i].MULVAL;
                tmpFractional = param[i].FD;
                param[i].DIVADDVAL = param[j].DIVADDVAL;
                param[i].MULVAL = param[j].MULVAL;
                param[i].FD = param[j].FD;
                param[j].DIVADDVAL = DIVADDVAL;
                param[j].MULVAL = MULVAL;
                param[j].FD = tmpFractional;
            }
        }
    }

    index=0;
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
    printf("========================================================\n");
    //printf("divider\t wantFD\t\t closeFD\t realBaud\t offset\t\n");
    printf("DLM\t DLL\t DIVADDVAL\t MULVAL\t Baud\t offset\t error% \n");

    // if we use ceil() the first divider need a fractional divider (FD) is probably 0.9XXX.
    // if we use floor() the first divider need a (FD) is probably 1.XXXX.
    // The available (FD) value is between 1 and 2. 
    // If (second divider = first divider +1), the needed FD is smaller than 1
    // If (second divider = first divider -1), the needed FD is larger than 1
    // It always compare needed FD with 1 as initial minOffset. 
    // The 1 mean no fractional divider.
    divider = (uint32_t)ceil( pclk / 16.0 / baud );

    while(divider>2) {
        double minOffset, offset, error;
        double wantFD, closeFD;
        uint32_t realBaud;
        
        wantFD = pclk / 16.0 / divider / baud;
        if( wantFD>2.0 ) break;
 
        // Before we compare the param table we should always compare the 1 which mean NO fractional divider.
        minOffset = fabs(wantFD - 1);

        for(index=0; ; index++) {

            if( (param[index].DIVADDVAL == 0xFF) || (param[index].MULVAL == 0xFF) ) {
                index--;
                break;
            }

            offset = fabs ( param[index].FD - wantFD );

            if( offset < minOffset ) {
                minOffset = offset;
            } else {
                offset = minOffset;
                index--;
                break; 
            }

        }

        if(index == 0xFF) { closeFD = 1.0; }
        else { closeFD = param[index].FD; }

        realBaud = (uint32_t)(pclk / 16.0 / divider / closeFD);
        error = ( abs(baud-realBaud) * 1.0 ) / baud;
        //printf("%d\t %lf\t %lf\t %d\t\t %d\t \n", divider, wantFD, closeFD, realBaud,  abs(baud-realBaud));
        printf("%d\t %d\t %d\t\t %d\t %d\t %d\t %lf \n",divider/256, divider%256,  (index==-1)?0:param[index].DIVADDVAL, (index==-1)?0:param[index].MULVAL, realBaud, abs(baud-realBaud), error);

        divider--;
    }

}