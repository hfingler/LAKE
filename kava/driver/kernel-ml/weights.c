#include "weights.h"

float w0_arr[15][5] = {
    {-0000011.669888, -0000000.249456, -0000000.038613, 00000005.642278, 00000007.012926},
    {-0000012.145359, 00000003.872515, 00000004.019009 ,00000012.550223, -0000002.621693},
    {00000008.960056, -0000000.016092, -0000000.562617, 00000015.599205, -0000000.361083},
    {00000014.456594, -0000013.322599, -0000013.313159, -0000019.182358, 00000001.948441},
    {-0000034.163129, 00000006.558228, 00000006.389304, -0000039.633775, 00000048.753821},
    {-0000013.934725, -0000007.519114, -0000007.329937, 00000011.234843, 00000004.198010},
    {00000009.273137, -0000027.453354, -0000027.124823, -0000005.129141, 00000026.243805},
    {00000009.617660, 00000013.975671, 00000014.221053, 00000029.918973, 00000018.547957},
    {-0000004.539204, -0000002.561855, -0000002.881559, -0000048.449138, -0000006.024052},
    {-0000006.996530, -0000003.499494, -0000004.001873, 00000025.095392, -0000000.873080},
    {-0000001.834595, -0000024.983553, -0000024.442274, -0000005.016328, 00000003.491228},
    {-0000015.516551, 00000001.166713, 00000001.190491, -0000023.617902, -0000002.510977},
    {00000002.785250, -0000003.971537, -0000004.621931, 00000019.081904, -0000002.339924},
    {-0000011.060982, -0000001.113953, -0000000.611457, -0000003.284909, -0000007.897714},
    {-0000023.032224, -0000007.876164, -0000008.074158, -0000001.024627, -0000002.299194}
};

float b0_arr[15][1] = {
    {00000002.992922},
    {-0000004.898516},
    {00000011.470737},
    {-0000011.640097},
    {00000004.372712},
    {-0000009.422000},
    {-0000058.417139},
    {-0000005.164213},
    {00000001.074321},
    {00000019.854877},
    {00000003.557259},
    {-0000005.584953},
    {00000002.676954},
    {-0000004.404445},
    {00000010.303386}
};

float w1_arr[5][15] = {
    {-0000010.416551, 00000010.207635, 00000014.135603, -0000004.558068, -0000008.942299, 00000007.590070, 00000002.929620, -0000008.077410, -0000028.280543, 00000017.591318, 00000009.423640, 00000001.113650, -0000003.239676, -0000010.321794, 00000008.653932},
    {00000012.242060, -0000014.918776, 00000009.806193, -0000021.781936, 00000008.023761, 00000014.767116, 00000007.154670, 00000017.055565, -0000013.487206, -0000010.394139, -0000009.279621, -0000021.993192, 00000024.587426, 00000001.663421, 00000000.846539},
    {00000006.164338, -0000014.798030, -0000023.657100, 00000001.923294, 00000001.072160, -0000007.740484, 00000007.343431, -0000002.514456, 00000010.546505, 00000000.892898, -0000001.336684, 00000009.573787, -0000003.529458, 00000004.246379, -0000018.515028},
    {00000000.025790, 00000001.776147, -0000008.665069, 00000020.336332, 00000005.798561, -0000004.780248, 00000034.501388, -0000010.909422, 00000014.693963, -0000002.547216, 00000005.864494, 00000027.920209, -0000040.353642, -0000002.663298, -0000009.888228},
    {00000010.370017, -0000005.095625, -0000009.117038, 00000018.484024, -0000018.706940, -0000001.956738, 00000014.861873, -0000011.606552, 00000028.985207, -0000029.506188, 00000002.412825, 00000020.593578, -0000022.539810, 00000010.869101, -0000013.389612}
};

float b1_arr[5][1] = {
    {00000002.395463},
    {00000006.801791},
    {00000005.419590},
    {-0000001.822087},
    {00000018.575540}
};

float w2_arr[4][5] = {
    {-0000013.901530, 00000047.290453, -0000018.861083, -0000037.144629, 00000048.703547},
    {00000032.041269, -0000016.726115, 00000008.311415, 00000024.001997, -0000057.524419},
    {00000004.571861, -0000020.739063, -0000020.472469, 00000015.574213, 00000005.334875},
    {-0000023.773152, -0000009.025779, 00000031.621847, -0000002.161713, 00000003.990320}
};

float b2_arr[4][1] = {
    {-0000004.582073},
    {00000013.337211},
    {-0000002.816034},
    {-0000006.808400}
};

float intial_stats[5] = {00050766.928910, 00000512.914170, 00000512.935878, 00000066.478758, 00000000.425078}; 
