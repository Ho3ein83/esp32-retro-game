#pragma once

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CONSTRAIN
#define CONSTRAIN(val, low, high) ((val) < (low) ? (low) : ((val) > (high) ? (high) : (val)))
#endif