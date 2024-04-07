#include "edid/edid.h"
#include "util/util.h"
#include "bsp/i2cm.h"

#define EDID_I2C_ADDR  		0x50
#define EDID_I2C_SEGMENT_ADDR  	0x30

static supported_dtd_t _dtd[] = {
//refresh_rate mCode mLimitedToYcc420 mYcc420 mPixelRepetitionInput mPixelClock mInterlaced mHActive mHBlanking mHBorder mHImageSize mHSyncOffset mHSyncPulseWidth mHSyncPolarity mVActive mVBlanking mVBorder mVImageSize mVSyncOffset mVSyncPulseWidth mVSyncPolarity;
	{60000,  {  1,  0, 0, 0, 25175,  0,  640,  160,  0,  4,   16,  96, 0,  480, 45, 0, 3, 10,  2, 0}},
	{59940,  {  1,  0, 0, 0, 25175,  0,  640,  160,  0,  4,   16,  96, 0,  480, 45, 0, 3, 10,  2, 0}},
	{60000,  {  2,  0, 0, 0, 27027,  0,  720,  138,  0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0}},
	{59940,  {  2,  0, 0, 0, 27000,  0,  720,  138,  0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0}},
	{60000,  {  3,  0, 0, 0, 27027,  0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{59940,  {  3,  0, 0, 0, 27000,  0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{60000,  {  4,  0, 0, 0, 74250,  0, 1280,  370,  0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{59940,  {  4,  0, 0, 0, 74176,  0, 1280,  370,  0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{60000,  {  5,  0, 0, 0, 74250,  1, 1920,  280,  0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{59940,  {  5,  0, 0, 0, 74176,  1, 1920,  280,  0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{60000,  {  6,  0, 0, 1, 27027,  1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{59940,  {  6,  0, 0, 1, 27000,  1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{60000,  {  7,  0, 0, 1, 27027,  1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{59940,  {  7,  0, 0, 1, 27000,  1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{60000,  {  8,  0, 0, 1, 27027,  0, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{60054,  {  8,  0, 0, 1, 27000,  0, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{59826,  {  8,  0, 0, 1, 27000,  0, 1440,  276,  0,  4,   38, 124, 0,  240, 23, 0, 3,  5,  3, 0}},
	{60000,  {  9,  0, 0, 1, 27027,  0, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{60054,  {  9,  0, 0, 1, 27000,  0, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{59826,  {  9,  0, 0, 1, 27000,  0, 1440,  276,  0, 16,   38, 124, 0,  240, 23, 0, 9,  5,  3, 0}},
	{60000,  { 10,  0, 0, 0, 54054,  1, 2880,  552,  0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0}},
	{59940,  { 11,  0, 0, 0, 54000,  1, 2880,  552,  0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0}},
	{60000,  { 12,  0, 0, 0, 54054,  0, 2880,  552,  0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0}},
	{60054,  { 12,  0, 0, 0, 54000,  0, 2880,  552,  0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0}},
	{59826,  { 12,  0, 0, 0, 54000,  0, 2880,  552,  0,  4,   76, 248, 0,  240, 23, 0, 3,  5,  3, 0}},
	{60000,  { 13,  0, 0, 0, 54054,  0, 2880,  552,  0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0}},
	{60054,  { 13,  0, 0, 0, 54000,  0, 2880,  552,  0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0}},
	{59826,  { 13,  0, 0, 0, 54000,  0, 2880,  552,  0, 16,   76, 248, 0,  240, 23, 0, 9,  5,  3, 0}},
	{60000,  { 14,  0, 0, 0, 54054,  0, 1440,  276,  0,  4,   32, 124, 0,  480, 45, 0, 3,  9,  6, 0}},
	{59940,  { 14,  0, 0, 0, 54000,  0, 1440,  276,  0,  4,   32, 124, 0,  480, 45, 0, 3,  9,  6, 0}},
	{60000,  { 15,  0, 0, 0, 54054,  0, 1440,  276,  0, 16,   32, 124, 0,  480, 45, 0, 9,  9,  6, 0}},
	{59940,  { 15,  0, 0, 0, 54000,  0, 1440,  276,  0, 16,   32, 124, 0,  480, 45, 0, 9,  9,  6, 0}},
	{60000,  { 16,  0, 0, 0, 148500, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{59940,  { 16,  0, 0, 0, 148352, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{50000,  { 17,  0, 0, 0, 27000,  0,  720,  144,  0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0}},
	{50000,  { 18,  0, 0, 0, 27000,  0,  720,  144,  0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0}},
	{50000,  { 19,  0, 0, 0, 74250,  0, 1280,  700,  0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{50000,  { 20,  0, 0, 0, 74250,  1, 1920,  720,  0, 16,  528,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{50000,  { 21,  0, 0, 1, 27000,  1, 1440,  288,  0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0}},
	{50000,  { 22,  0, 0, 1, 27000,  1, 1440,  288,  0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0}},
	{50000,  { 23,  0, 0, 1, 27000,  0, 1440,  288,  0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0}},
	{50000,  { 23,  0, 0, 1, 27000,  0, 1440,  288,  0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0}},
	{49920,  { 23,  0, 0, 1, 27000,  0, 1440,  288,  0,  4,   24, 126, 0,  288, 25, 0, 3,  3,  3, 0}},
	{50000,  { 24,  0, 0, 1, 27000,  0, 1440,  288,  0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0}},
	{50000,  { 24,  0, 0, 1, 27000,  0, 1440,  288,  0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0}},
	{49920,  { 24,  0, 0, 1, 27000,  0, 1440,  288,  0, 16,   24, 126, 0,  288, 25, 0, 9,  3,  3, 0}},
	{50000,  { 25,  0, 0, 0, 54000,  1, 2880,  576,  0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0}},
	{50000,  { 26,  0, 0, 0, 54000,  1, 2880,  576,  0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0}},
	{50000,  { 27,  0, 0, 0, 54000,  0, 2880,  576,  0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0}},
	{49920,  { 27,  0, 0, 0, 54000,  0, 2880,  576,  0,  4,   48, 252, 0,  288, 25, 0, 3,  3,  3, 0}},
	{50000,  { 27,  0, 0, 0, 54000,  0, 2880,  576,  0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0}},
	{50000,  { 28,  0, 0, 0, 54000,  0, 2880,  576,  0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0}},
	{49920,  { 28,  0, 0, 0, 54000,  0, 2880,  576,  0, 16,   48, 252, 0,  288, 25, 0, 9,  3,  3, 0}},
	{50000,  { 28,  0, 0, 0, 54000,  0, 2880,  576,  0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0}},
	{50000,  { 29,  0, 0, 0, 54000,  0, 1440,  288,  0,  4,   24, 128, 0,  576, 49, 0, 3,  5,  5, 0}},
	{50000,  { 30,  0, 0, 0, 54000,  0, 1440,  288,  0, 16,   24, 128, 0,  576, 49, 0, 9,  5,  5, 0}},
	{50000,  { 31,  0, 0, 0, 148500, 0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{24000,  { 32,  0, 0, 0, 74250,  0, 1920,  830,  0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{23976,  { 32,  0, 0, 0, 74176,  0, 1920,  830,  0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{25000,  { 33,  0, 0, 0, 74250,  0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{30000,  { 34,  0, 0, 0, 74250,  0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{29970,  { 34,  0, 0, 0, 74176,  0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{60000,  { 35,  0, 0, 0, 108108, 0, 2880,  552,  0,  4,   64, 248, 0,  480, 45, 0, 3,  9,  6, 0}},
	{59940,  { 35,  0, 0, 0, 108000, 0, 2880,  552,  0,  4,   64, 248, 0,  480, 45, 0, 3,  9,  6, 0}},
	{60000,  { 36,  0, 0, 0, 108108, 0, 2880,  552,  0, 16,   64, 248, 0,  480, 45, 0, 9,  9,  6, 0}},
	{59940,  { 36,  0, 0, 0, 108100, 0, 2880,  552,  0, 16,   64, 248, 0,  480, 45, 0, 9,  9,  6, 0}},
	{50000,  { 37,  0, 0, 0, 108000, 0, 2880,  576,  0,  4,   48, 256, 0,  576, 49, 0, 3,  5,  5, 0}},
	{50000,  { 38,  0, 0, 0, 108000, 0, 2880,  576,  0, 16,   48, 256, 0,  576, 49, 0, 9,  5,  5, 0}},
	{50000,  { 39,  0, 0, 0, 72000,  1, 1920,  384,  0, 16,   32, 168, 1,  540, 85, 0, 9, 23,  5, 0}},
	{100000, { 40,  0, 0, 0, 148500, 1, 1920,  720,  0, 16,  528,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{100000, { 41,  0, 0, 0, 148500, 0, 1280,  700,  0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{100000, { 42,  0, 0, 0, 54000,  0,  720,  144,  0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0}},
	{100000, { 43,  0, 0, 0, 54000,  0,  720,  144,  0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0}},
	{100000, { 44,  0, 0, 1, 54000,  1, 1440,  288,  0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0}},
	{100000, { 45,  0, 0, 1, 54000,  1, 1440,  288,  0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0}},
	{120000, { 46,  0, 0, 0, 148500, 1, 1920,  280,  0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{119880, { 46,  0, 0, 0, 148352, 1, 1920,  280,  0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1}},
	{120000, { 47,  0, 0, 0, 148500, 0, 1280,  370,  0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{120000, { 48,  0, 0, 0, 54054,  0,  720,  138,  0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0}},
	{120000, { 49,  0, 0, 0, 54054,  0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{119880, { 49,  0, 0, 0, 54000,  0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{120000, { 50,  0, 0, 1, 54054,  1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{119880, { 50,  0, 0, 1, 54000,  1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{120000, { 51,  0, 0, 1, 54054,  1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{119880, { 51,  0, 0, 1, 54000,  1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{200000, { 52,  0, 0, 0, 108000, 0,  720,  144,  0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0}},
	{200000, { 53,  0, 0, 0, 108000, 0,  720,  144,  0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0}},
	{200000, { 54,  0, 0, 1, 108000, 1, 1440,  288,  0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0}},
	{200000, { 55,  0, 0, 1, 108000, 1, 1440,  288,  0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0}},
	{240000, { 56,  0, 0, 0, 108100, 0,  720,  138,  0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0}},
	{240000, { 57,  0, 0, 0, 108100, 0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{239760, { 57,  0, 0, 0, 108000, 0,  720,  138,  0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0}},
	{240000, { 58,  0, 0, 1, 108100, 1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{239760, { 58,  0, 0, 1, 108000, 1, 1440,  276,  0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0}},
	{240000, { 59,  0, 0, 1, 108100, 1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{239760, { 59,  0, 0, 1, 108000, 1, 1440,  276,  0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0}},
	{24000,  { 60,  0, 0, 0, 59400,  0, 1280, 2020,  0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{23970,  { 60,  0, 0, 0, 59341,  0, 1280, 2020,  0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{25000,  { 61,  0, 0, 0, 74250,  0, 1280, 2680,  0, 16, 2420,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{30000,  { 62,  0, 0, 0, 74250,  0, 1280, 2020,  0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{29970,  { 62,  0, 0, 0, 74176,  0, 1280, 2020,  0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{120000, { 63,  0, 0, 0, 297000, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{119880, { 63,  0, 0, 0, 296703, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{100000, { 64,  0, 0, 0, 297000, 0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{24000,  { 65,  0, 0, 0, 59341,  0, 1280, 2020,  0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{25000,  { 66,  0, 0, 0, 74250,  0, 1280, 2680,  0, 16, 2420,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{50000,  { 68,  0, 0, 0, 74250,  0, 1280,  700,  0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{60000,  { 69,  0, 0, 0, 74250,  0, 1280,  370,  0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{100000, { 70,  0, 0, 0, 148500, 0, 1280,  700,  0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{120000, { 71,  0, 0, 0, 148500, 0, 1280,  370,  0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1}},
	{24000,  { 72,  0, 0, 0, 74250,  0, 1920,  830,  0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{25000,  { 73,  0, 0, 0, 74250,  0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{30000,  { 74,  0, 0, 0, 74250,  0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{50000,  { 75,  0, 0, 0, 148500, 0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{60000,  { 76,  0, 0, 0, 148500, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{100000, { 77,  0, 0, 0, 297000, 0, 1920,  720,  0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{120000, { 78,  0, 0, 0, 297000, 0, 1920,  280,  0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1}},
	{24000,  { 79,  0, 0, 0, 59400,  0, 1680,  1620, 0, 7,  1360,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{25000,  { 80,  0, 0, 0, 59400,  0, 1680,  1488, 0, 7,  1228,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{30000,  { 81,  0, 0, 0, 59400,  0, 1680,  960,  0, 7,   700,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{50000,  { 82,  0, 0, 0, 82500,  0, 1680,  520,  0, 7,   260,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{60000,  { 83,  0, 0, 0, 99000,  0, 1680,  520,  0, 7,   260,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{100000, { 84,  0, 0, 0, 165000, 0, 1680,  320,  0,	7,    60,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{120000, { 85,  0, 0, 0, 198000, 0, 1680,  320,  0, 7,	  60,  40, 1,  720,	30,	0, 3,  5,  5, 1}},
	{24000,  { 86,  0, 0, 0, 99000,  0, 2560, 1190,  0, 0,   998,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{25000,  { 87,  0, 0, 0, 90000,  0, 2560,  640,  0, 0,   448,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{30000,  { 88,  0, 0, 0, 118800, 0, 2560,  960,  0, 0,   768,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{50000,  { 89,  0, 0, 0, 185625, 0, 2560,  740,  0, 0,   548,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{60000,  { 90,  0, 0, 0, 198000, 0, 2560,  440,  0, 0,   248,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{100000, { 91,  0, 0, 0, 371250, 0, 2560,  410,  0, 0,   218,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{120000, { 92,  0, 0, 0, 495000, 0, 2560,  740,  0, 0,   548,  44, 1, 1080, 20, 0, 0,  4,  5, 1}},
	{24000,  { 93,  0, 0, 0, 297000, 0, 3840, 1660,  0, 16, 1276,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{25000,  { 94,  0, 0, 0, 297000, 0, 3840, 1440,  0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{30000,  { 95,  0, 0, 0, 297000, 0, 3840,  560,  0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{50000,  { 96,  0, 0, 0, 594000, 0, 3840, 1440,  0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{60000,  { 97,  0, 0, 0, 594000, 0, 3840,  560,  0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{24000,  { 98,  0, 0, 0, 297000, 0, 4096, 1404,  0, 16, 1020,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{25000,  { 99,  0, 0, 0, 297000, 0, 4096, 1184,  0, 16,  968,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{30000,  { 100, 0, 0, 0, 297000, 0, 4096,  304,  0, 16,   88,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{50000,  { 101, 0, 0, 0, 594000, 0, 4096, 1184,  0, 16,  968,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{60000,  { 102, 0, 0, 0, 594000, 0, 4096,  304,  0, 16,   88,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{24000,  { 103, 0, 0, 0, 297000, 0, 3840, 1660,  0, 16, 1276,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{25000,  { 104, 0, 0, 0, 297000, 0, 3840, 1440,  0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{30000,  { 105, 0, 0, 0, 297000, 0, 3840,  560,  0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{50000,  { 106, 0, 0, 0, 297000, 0, 3840, 1440,  0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{60000,  { 107, 0, 0, 0, 594000, 0, 3840,  560,  0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1}},
	{59950,  { 108, 0, 0, 0, 241500, 0, 2560,  160,  0, 16,   32,  48, 1, 1440, 41, 0, 4,  3,  5, 0}},
	{0   ,   {   0, 0, 0, 0,      0,  0,    0,   0,  0,  0,    0,   0, 0,    0,  0, 0, 0,  0,  0, 0}},
};

int _edid_checksum(u8 * edid)
{
	int i, checksum = 0;

	for(i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];

	return checksum % 256; //CEA-861 Spec
}

int edid_read(hdmi_tx_dev_t *dev, struct edid * edid)
{
	int error = 0;
	const u8 header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	i2cddc_clk_config(dev, 2500, I2C_MIN_SS_SCL_LOW_TIME, I2C_MIN_SS_SCL_HIGH_TIME, I2C_MIN_FS_SCL_LOW_TIME, I2C_MIN_FS_SCL_HIGH_TIME);

	error = ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, 0, 0, 128, (u8 *)edid);
	if(error){
		pr_debug("EDID read failed\n");
		return error;
	}
	error = memcmp((u8 * ) edid, (u8 *) header, sizeof(header));
	if(error){
		pr_debug("EDID header check failed\n");
		return error;
	}

	error = _edid_checksum((u8 *) edid);
	if(error){
		pr_debug("EDID checksum failed\n");
		return error;
	}
	return 0;
}

int edid_extension_read(hdmi_tx_dev_t *dev, int block, u8 * edid_ext)
{
	int error = 0;
	/*to incorporate extensions we have to include the following - see VESA E-DDC spec. P 11 */
	u8 start_pointer = block / 2; // pointer to segments of 256 bytes
	u8 start_address = ((block % 2) * 0x80); //offset in segment; first block 0-127; second 128-255

	error = ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR, start_pointer, start_address, 128, edid_ext);
	if(error){
		pr_debug("EDID extension read failed\n");
		return error;
	}

	error = _edid_checksum(edid_ext);
	if(error){
		pr_debug("EDID extension checksum failed\n");
		return error;
	}
	return 0;
}

int dtd_parse(hdmi_tx_dev_t *dev, dtd_t * dtd, u8 data[18])
{

	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;
	dtd->mLimitedToYcc420 = 0;
	dtd->mYcc420 = 0;

	dtd->mPixelClock = byte_to_word(data[1], data[0]);	/*  [10000Hz] */
	if (dtd->mPixelClock < 0x01) {	/* 0x0000 is defined as reserved */
		return FALSE;
	}

	dtd->mHActive = concat_bits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking = concat_bits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset = concat_bits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = concat_bits(data[11], 4, 2, data[9], 0, 8);
	dtd->mHImageSize = concat_bits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder = data[15];

	dtd->mVActive = concat_bits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking = concat_bits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset = concat_bits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = concat_bits(data[11], 0, 2, data[10], 0, 4);
	dtd->mVImageSize = concat_bits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder = data[16];

	if (bit_field(data[17], 4, 1) != 1) {	/* if not DIGITAL SYNC SIGNAL DEF */
		pr_err("Invalid DTD Parameters - DIGITAL SYNC SIGNAL DEF\n");
		return FALSE;
	}
	if (bit_field(data[17], 3, 1) != 1) {	/* if not DIGITAL SEPATATE SYNC */
		pr_err("Invalid DTD Parameters - DIGITAL SEPATATE SYNC\n");
		return FALSE;
	}
	/* no stereo viewing support in HDMI */
	dtd->mInterlaced = bit_field(data[17], 7, 1) == 1;
	dtd->mVSyncPolarity = bit_field(data[17], 2, 1) == 1;
	dtd->mHSyncPolarity = bit_field(data[17], 1, 1) == 1;
	return TRUE;
}

/**
 * @short Get the DTD structure that contains the video parameters
 * @param[in] code VIC code to search for
 * @param[in] refreshRate
 * @return returns a pointer to the DTD structure or NULL if not supported.
 * If refreshRate=0 then the first (default) parameters are returned for the VIC code.
 */
dtd_t * get_dtd(u8 code, u32 refreshRate){
	int i = 0;

	for(i = 0; _dtd[i].dtd.mCode != 0; i++){
		if(_dtd[i].dtd.mCode == code){
			if(is_equal(refreshRate, 0)){
				return &_dtd[i].dtd;
			}
			if(is_equal(refreshRate, _dtd[i].refresh_rate)){
				return &_dtd[i].dtd;
			}
		}
	}
	return NULL;
}

int dtd_fill(hdmi_tx_dev_t *dev, dtd_t * dtd, u8 code, u32 refreshRate)
{
	dtd_t * p_dtd = NULL;

	p_dtd = get_dtd(code, refreshRate);
	if(p_dtd == NULL){
		pr_err("VIC code [%d] with refresh rate [%uHz] is not supported", code, refreshRate);
		return FALSE;
	}
	p_dtd->mLimitedToYcc420 = FALSE;
	p_dtd->mYcc420 = FALSE;

	memcpy(dtd, p_dtd, sizeof(dtd_t));

	return TRUE;
}

u32 dtd_get_refresh_rate(dtd_t *dtd){
	int i = 0;
	for(i = 0; _dtd[i].dtd.mCode != 0; i++){
		if(_dtd[i].dtd.mCode == dtd->mCode){
			if(is_equal(dtd->mPixelClock, 0) || is_equal(_dtd[i].dtd.mPixelClock, dtd->mPixelClock))
				return _dtd[i].refresh_rate;
		}
	}
	return 0;
}

dtd_t * find_dtd(u16 width, u16 height, u32 pclk){
	int i;
	pr_debug("width:%u, height:%u, pclk:%u\n", width, height, pclk);
	for(i = 0; _dtd[i].dtd.mCode != 0; i++){
		if((_dtd[i].dtd.mHActive == width) &&
		   (_dtd[i].dtd.mVActive == height) &&
		   (_dtd[i].dtd.mPixelClock == pclk)){
				return &_dtd[i].dtd;
			}
	}
	return NULL;
}