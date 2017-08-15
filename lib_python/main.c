/** 
 * Library to wrap M-ulator into the 
 * M3 soft-DBG project
 *
 * Andrew Lukefahr
 * lukefahr@umich.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <Python.h>

#include "core/opcodes.h"

#include "step.h"

// Stuff needed to make the existing code happy


/**
 * My main test-driver program
 */
int main(void)
{

    step(0x4011);

    return 0;
}
