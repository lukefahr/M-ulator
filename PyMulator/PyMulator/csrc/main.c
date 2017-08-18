/** 
 * Library to wrap M-ulator into the 
 * M3 soft-DBG project
 *
 * Andrew Lukefahr
 * lukefahr@umich.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "interface.h"

#include "core/common.h"

// Stuff needed to make the existing code happy


/**
 * My main test-driver program
 */
int main(void)
{

    call_to_mulator("stepi"); 

    return 0;
}


void call_from_mulator( char * command, char ** result)
{
    const uint32_t MAX_CHARS = 255;
    DBG2("Callback: %s\n", command);

    char * cmd1= strtok(command, " \t");

    //read registers
    if (!strncmp(cmd1, "info", MAX_CHARS)){
        char * cmd2 = strtok(NULL, " \t");
        
        if (!strncmp(cmd2, "register", MAX_CHARS)){
            char * cmd3 = strtok(NULL, " \t");

            if( (!strncmp(cmd3, "r1", MAX_CHARS)) ||
                (!strncmp(cmd3, "r2", MAX_CHARS)) ||
                (!strncmp(cmd3, "r3", MAX_CHARS)) ||
                (!strncmp(cmd3, "r4", MAX_CHARS)) ||
                (!strncmp(cmd3, "r5", MAX_CHARS)) ){
                uint32_t reg = atoi(cmd3);
                asprintf( result, "r%d\t0x%x", reg, reg);

            } else if( (!strncmp(cmd3, "r15", MAX_CHARS)) ){ 
                asprintf( result, "r15\t0x%x",  0xec4);

            } else if( (!strncmp(cmd3, "xpsr", MAX_CHARS)) ){ 
                asprintf( result, "xpsr\t0x%x",  0x41000000);
            } else {
                assert(0);
            }
        } else {
            assert(0);
        }

    //write register
    } else if (!strncmp(cmd1, "p", MAX_CHARS)){
        char * reg = strtok(NULL, " \t");
        char * eq =  strtok(NULL, " \t");
        char * val = strtok(NULL, " \t");

        assert(reg != NULL);
        assert(val != NULL);

        if( (!strncmp(reg, "$r1", MAX_CHARS)) ||
            (!strncmp(reg, "$r2", MAX_CHARS)) ||
            (!strncmp(reg, "$r3", MAX_CHARS)) ||
            (!strncmp(reg, "$r4", MAX_CHARS)) ||
            (!strncmp(reg, "$r5", MAX_CHARS)) ||
            (!strncmp(reg, "$r6", MAX_CHARS)) ||
            (!strncmp(reg, "$r7", MAX_CHARS)) ||
            // skip
            (!strncmp(reg, "$r11", MAX_CHARS)) ||
            (!strncmp(reg, "$r12", MAX_CHARS)) ||
            (!strncmp(reg, "$r13", MAX_CHARS)) ||
            (!strncmp(reg, "$r14", MAX_CHARS)) ||
            (!strncmp(reg, "$r15", MAX_CHARS)) ){
            
                uint32_t _val = strtol(val, NULL, 16);
                
                DBG2("Reg Write: %s -> 0x%x\n", reg, _val);

                asprintf( result, "$0 = 0x%x", _val);

        } else if( (!strncmp(reg, "$xpsr", MAX_CHARS)) ){ 
                uint32_t _val = strtol(val, NULL, 16);
                
                DBG2("Reg Write: %s -> 0x%x\n", reg, _val);

                asprintf( result, "$0 = 0x%x", _val);

        } else {
            assert(false);
        }

    //memory eXamine
    } else if (!strncmp(cmd1, "x/1xh", MAX_CHARS)){
        
        char * cmd2 = strtok(NULL, " \t");
        assert(cmd2 != NULL);

        uint32_t addr = strtol(cmd2, NULL, 16);

        uint16_t value = 0x4011;
        asprintf( result, "0x%x: 0x%x", addr, value);
    
    } else if ( !strncmp(cmd1, "set", MAX_CHARS)){

        char * cmd2 = strtok(NULL, " \t");
        assert(!strncmp(cmd2, "{uint16_t}", MAX_CHARS));
        char * cmd3 = strtok(NULL, " \t");
        uint32_t addr = strtol(cmd3, NULL, 16);
        strtok(NULL, " \t"); // = sign
        char * cmd4 = strtok(NULL, " \t");
        uint32_t val = strtol(cmd4, NULL, 16);
    
        DBG2("Mem Write: 0x%x -> 0x%x\n", addr, val);

        asprintf( result, "0x%x: 0x%x", addr, val);
       
    } else {
        assert(0);
    }

}
